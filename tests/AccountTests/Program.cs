using System.Diagnostics;
using System.Net;
using System.Security.Cryptography;
using System.Text;
using Google.Protobuf;
using Microsoft.Data.Sqlite;
using Mimic.AccountTests;
using Mimic.PlatformServer.Accounts;
using Mimic.Protocol;

int failures = 0;
void Check(bool condition, string reason)
{
    if (condition) { Console.WriteLine("PASS " + reason); return; }
    failures++;
    Console.WriteLine("FAIL " + reason);
}

static string Rand() => Guid.NewGuid().ToString("N")[..10];

static async Task<HttpResponseMessage> PostProto(HttpClient client, string path, IMessage message, string? bearer = null)
{
    using var content = new ByteArrayContent(message.ToByteArray());
    content.Headers.ContentType = new("application/x-protobuf");
    using var request = new HttpRequestMessage(HttpMethod.Post, path) { Content = content };
    if (bearer is not null) request.Headers.Authorization = new("Bearer", bearer);
    return await client.SendAsync(request);
}

static async Task<HttpResponseMessage> GetProto(HttpClient client, string path, string? bearer = null)
{
    using var request = new HttpRequestMessage(HttpMethod.Get, path);
    if (bearer is not null) request.Headers.Authorization = new("Bearer", bearer);
    return await client.SendAsync(request);
}

static async Task<byte[]> Bytes(HttpResponseMessage response) => await response.Content.ReadAsByteArrayAsync();

// ---------------------------------------------------------------------------
// 1. Registration validation
// ---------------------------------------------------------------------------
await using (var host = await TestHost.StartAsync())
{
    var client = host.Client;

    async Task ExpectRegisterRejected(RegisterRequest request, HttpStatusCode status, string reason)
    {
        var response = await PostProto(client, "/auth/register", request);
        Check(response.StatusCode == status, $"{reason} -> status {response.StatusCode}");
        Check(response.Content.Headers.ContentType?.MediaType == "application/x-protobuf", $"{reason} -> protobuf content type");
        var error = ErrorReply.Parser.ParseFrom(await Bytes(response));
        Check(!string.IsNullOrEmpty(error.Code), $"{reason} -> error code present");
    }

    await ExpectRegisterRejected(new RegisterRequest { AccountName = "ab", Password = "longenough1", DisplayName = "Player" }, HttpStatusCode.BadRequest, "Account name too short");
    await ExpectRegisterRejected(new RegisterRequest { AccountName = new string('a', 25), Password = "longenough1", DisplayName = "Player" }, HttpStatusCode.BadRequest, "Account name too long");
    await ExpectRegisterRejected(new RegisterRequest { AccountName = "bad name!", Password = "longenough1", DisplayName = "Player" }, HttpStatusCode.BadRequest, "Account name with invalid characters");
    await ExpectRegisterRejected(new RegisterRequest { AccountName = "validname1", Password = "short1", DisplayName = "Player" }, HttpStatusCode.BadRequest, "Password too short");
    await ExpectRegisterRejected(new RegisterRequest { AccountName = "validname2", Password = new string('a', 129), DisplayName = "Player" }, HttpStatusCode.BadRequest, "Password too long");
    await ExpectRegisterRejected(new RegisterRequest { AccountName = "validname3", Password = "longenough1", DisplayName = "" }, HttpStatusCode.BadRequest, "Display name empty");
    await ExpectRegisterRejected(new RegisterRequest { AccountName = "validname4", Password = "longenough1", DisplayName = new string('P', 25) }, HttpStatusCode.BadRequest, "Display name too long");
    await ExpectRegisterRejected(new RegisterRequest { AccountName = "validname5", Password = "longenough1", DisplayName = "BadName" }, HttpStatusCode.BadRequest, "Display name with control character");
}

// ---------------------------------------------------------------------------
// 2. Registration success, auto-login shape, and profile round trip
// ---------------------------------------------------------------------------
await using (var host = await TestHost.StartAsync())
{
    var client = host.Client;
    var accountName = "Alice_" + Rand();
    var registerResponse = await PostProto(client, "/auth/register", new RegisterRequest
    {
        AccountName = accountName,
        Password = "correct horse battery",
        DisplayName = "Alice",
    });
    Check(registerResponse.StatusCode == HttpStatusCode.Created, "Register success -> 201");
    var login = LoginReply.Parser.ParseFrom(await Bytes(registerResponse));
    Check(login.SessionToken.Length == 64, "Register issues 64-char session token");
    Check(login.AccountName == accountName, "Register reply echoes account name");
    Check(login.DisplayName == "Alice", "Register reply echoes display name");
    Check(login.DemoChips == 1000, "New account starts with demo chips, no real wallet");
    Check(login.ExpiresUnix > DateTimeOffset.UtcNow.ToUnixTimeSeconds(), "Session expiry is in the future");
    Check(!string.IsNullOrEmpty(login.PlayerId), "Register reply includes player id");

    var profileResponse = await GetProto(client, "/account/profile", login.SessionToken);
    Check(profileResponse.StatusCode == HttpStatusCode.OK, "Profile with valid token -> 200");
    var profile = ProfileReply.Parser.ParseFrom(await Bytes(profileResponse));
    Check(profile.PlayerId == login.PlayerId, "Profile player id matches login");
    Check(profile.AccountName == accountName, "Profile account name matches");
    Check(profile.DisplayName == "Alice", "Profile display name matches");
    Check(profile.DemoChips == 1000, "Profile demo chips matches");
    Check(profile.CreatedUnix > 0, "Profile created_unix populated");

    var noAuthResponse = await GetProto(client, "/account/profile");
    Check(noAuthResponse.StatusCode == HttpStatusCode.Unauthorized, "Profile without bearer -> 401");
    var badAuthResponse = await GetProto(client, "/account/profile", "not-a-real-token");
    Check(badAuthResponse.StatusCode == HttpStatusCode.Unauthorized, "Profile with garbage token -> 401");

    // /sessions/validate must keep working for the C++ TableServer integration.
    var validateResponse = await PostProto(client, "/sessions/validate", new ValidateSessionRequest { SessionToken = login.SessionToken });
    Check(validateResponse.StatusCode == HttpStatusCode.OK, "sessions/validate with account session -> 200");
    var session = SessionReply.Parser.ParseFrom(await Bytes(validateResponse));
    Check(session.PlayerId == login.PlayerId, "sessions/validate returns matching player id");
    Check(session.DisplayName == "Alice", "sessions/validate returns matching display name");
}

// ---------------------------------------------------------------------------
// 3. Duplicate accounts are rejected case-insensitively
// ---------------------------------------------------------------------------
await using (var host = await TestHost.StartAsync())
{
    var client = host.Client;
    var baseName = "Bob_" + Rand();
    var first = await PostProto(client, "/auth/register", new RegisterRequest { AccountName = baseName, Password = "longenough1", DisplayName = "Bob" });
    Check(first.StatusCode == HttpStatusCode.Created, "First registration succeeds");

    foreach (var variant in new[] { baseName, baseName.ToUpperInvariant(), baseName.ToLowerInvariant() })
    {
        var duplicate = await PostProto(client, "/auth/register", new RegisterRequest { AccountName = variant, Password = "differentpass1", DisplayName = "Impostor" });
        Check(duplicate.StatusCode == HttpStatusCode.Conflict, $"Duplicate '{variant}' rejected with 409");
        var error = ErrorReply.Parser.ParseFrom(await Bytes(duplicate));
        Check(error.Code == "ACCOUNT_EXISTS", $"Duplicate '{variant}' reports ACCOUNT_EXISTS");
    }
}

// ---------------------------------------------------------------------------
// 4. Login: correct credentials, wrong password, unknown account
//    (credential checks must not leak which case failed)
// ---------------------------------------------------------------------------
await using (var host = await TestHost.StartAsync())
{
    var client = host.Client;
    var accountName = "Carol_" + Rand();
    await PostProto(client, "/auth/register", new RegisterRequest { AccountName = accountName, Password = "correcthorse1", DisplayName = "Carol" });

    var goodLogin = await PostProto(client, "/auth/login", new AccountLoginRequest { AccountName = accountName, Password = "correcthorse1" });
    Check(goodLogin.StatusCode == HttpStatusCode.OK, "Login with correct credentials -> 200");
    var loginReply = LoginReply.Parser.ParseFrom(await Bytes(goodLogin));
    Check(loginReply.SessionToken.Length == 64, "Login issues a fresh session token");

    var wrongPassword = await PostProto(client, "/auth/login", new AccountLoginRequest { AccountName = accountName, Password = "totallywrong1" });
    var unknownAccount = await PostProto(client, "/auth/login", new AccountLoginRequest { AccountName = "nosuchaccount" + Rand(), Password = "whatever12" });
    Check(wrongPassword.StatusCode == HttpStatusCode.Unauthorized, "Login with wrong password -> 401");
    Check(unknownAccount.StatusCode == HttpStatusCode.Unauthorized, "Login with unknown account -> 401");
    var wrongError = ErrorReply.Parser.ParseFrom(await Bytes(wrongPassword));
    var unknownError = ErrorReply.Parser.ParseFrom(await Bytes(unknownAccount));
    Check(wrongError.Code == unknownError.Code && wrongError.Message == unknownError.Message,
        "Wrong password and unknown account return identical generic error (no user enumeration)");
}

// ---------------------------------------------------------------------------
// 5. Logout invalidates the session for both profile and sessions/validate
// ---------------------------------------------------------------------------
await using (var host = await TestHost.StartAsync())
{
    var client = host.Client;
    var register = await PostProto(client, "/auth/register", new RegisterRequest { AccountName = "Dave_" + Rand(), Password = "longenough1", DisplayName = "Dave" });
    var login = LoginReply.Parser.ParseFrom(await Bytes(register));

    var logoutResponse = await PostProto(client, "/auth/logout", new Empty(), login.SessionToken);
    Check(logoutResponse.StatusCode == HttpStatusCode.NoContent, "Logout with valid bearer -> 204");

    var logoutNoAuth = await PostProto(client, "/auth/logout", new Empty());
    Check(logoutNoAuth.StatusCode == HttpStatusCode.Unauthorized, "Logout without bearer -> 401");

    var profileAfterLogout = await GetProto(client, "/account/profile", login.SessionToken);
    Check(profileAfterLogout.StatusCode == HttpStatusCode.Unauthorized, "Profile after logout -> 401");

    var validateAfterLogout = await PostProto(client, "/sessions/validate", new ValidateSessionRequest { SessionToken = login.SessionToken });
    Check(validateAfterLogout.StatusCode == HttpStatusCode.Unauthorized, "sessions/validate after logout -> 401");
}

// ---------------------------------------------------------------------------
// 6. Session expiry
// ---------------------------------------------------------------------------
await using (var host = await TestHost.StartAsync(sessionTtlSeconds: 1))
{
    var client = host.Client;
    var register = await PostProto(client, "/auth/register", new RegisterRequest { AccountName = "Erin_" + Rand(), Password = "longenough1", DisplayName = "Erin" });
    var login = LoginReply.Parser.ParseFrom(await Bytes(register));

    await Task.Delay(TimeSpan.FromSeconds(1.5));

    var expiredProfile = await GetProto(client, "/account/profile", login.SessionToken);
    Check(expiredProfile.StatusCode == HttpStatusCode.Unauthorized, "Expired session rejected by profile");
    var expiredValidate = await PostProto(client, "/sessions/validate", new ValidateSessionRequest { SessionToken = login.SessionToken });
    Check(expiredValidate.StatusCode == HttpStatusCode.Unauthorized, "Expired session rejected by sessions/validate");
}

// ---------------------------------------------------------------------------
// 7. No plaintext password storage
// ---------------------------------------------------------------------------
await using (var host = await TestHost.StartAsync())
{
    var client = host.Client;
    const string plaintext = "SuperSecretPassphrase1";
    var accountName = "Frank_" + Rand();
    await PostProto(client, "/auth/register", new RegisterRequest { AccountName = accountName, Password = plaintext, DisplayName = "Frank" });

    var database = new AccountDatabase(host.DataDirectory);
    var stored = database.FindByAccountName(accountName);
    Check(stored is not null, "Account row exists for password-storage check");
    Check(stored!.PasswordHash != plaintext, "Password hash column is not the plaintext password");
    Check(!stored.PasswordHash.Contains(plaintext, StringComparison.Ordinal), "Password hash does not embed the plaintext password");
    Check(stored.PasswordHash.StartsWith("PBKDF2-SHA256$", StringComparison.Ordinal), "Password hash uses the documented PBKDF2-SHA256 format");
}

// ---------------------------------------------------------------------------
// 8. Persistent reload: data survives a service restart
// ---------------------------------------------------------------------------
{
    var dataDir = Path.Combine(Path.GetTempPath(), "mimic-account-tests-" + Guid.NewGuid().ToString("N"));
    var accountName = "Grace_" + Rand();
    ProfileReply beforeRestart;
    await using (var host = await TestHost.StartAsync(dataDir))
    {
        var register = await PostProto(host.Client, "/auth/register", new RegisterRequest { AccountName = accountName, Password = "longenough1", DisplayName = "Grace" });
        var login = LoginReply.Parser.ParseFrom(await Bytes(register));
        var profileResponse = await GetProto(host.Client, "/account/profile", login.SessionToken);
        beforeRestart = ProfileReply.Parser.ParseFrom(await Bytes(profileResponse));
    }

    await using (var host = await TestHost.StartAsync(dataDir))
    {
        var loginAfterRestart = await PostProto(host.Client, "/auth/login", new AccountLoginRequest { AccountName = accountName, Password = "longenough1" });
        Check(loginAfterRestart.StatusCode == HttpStatusCode.OK, "Login succeeds after service restart");
        var login = LoginReply.Parser.ParseFrom(await Bytes(loginAfterRestart));
        var profileResponse = await GetProto(host.Client, "/account/profile", login.SessionToken);
        var profile = ProfileReply.Parser.ParseFrom(await Bytes(profileResponse));
        Check(profile.PlayerId == beforeRestart.PlayerId, "Player id survives restart");
        Check(profile.CreatedUnix == beforeRestart.CreatedUnix, "Created timestamp survives restart");
        Check(profile.DemoChips == beforeRestart.DemoChips, "Demo chips survive restart");
    }
}

// ---------------------------------------------------------------------------
// 9. Malicious input / SQL parameterization safety
// ---------------------------------------------------------------------------
await using (var host = await TestHost.StartAsync())
{
    var client = host.Client;

    // Account name charset rejects SQL metacharacters outright.
    var injectionAttempt = await PostProto(client, "/auth/register", new RegisterRequest
    {
        AccountName = "a'; DROP TABLE accounts;--",
        Password = "longenough1",
        DisplayName = "Attacker",
    });
    Check(injectionAttempt.StatusCode == HttpStatusCode.BadRequest, "SQL-metacharacter account name rejected by validation");

    // Display name allows broad unicode/punctuation; it must be stored safely via parameters.
    const string trickyDisplayName = "O'Brien\"; DROP--";
    var accountName = "Heidi_" + Rand();
    var register = await PostProto(client, "/auth/register", new RegisterRequest { AccountName = accountName, Password = "longenough1", DisplayName = trickyDisplayName });
    Check(register.StatusCode == HttpStatusCode.Created, "Registration with punctuation-heavy display name succeeds");
    var login = LoginReply.Parser.ParseFrom(await Bytes(register));
    Check(login.DisplayName == trickyDisplayName, "Punctuation-heavy display name is stored and returned unmodified");

    // The accounts table must still be intact and usable after the injection attempt.
    var followUpName = "Ivan_" + Rand();
    var followUp = await PostProto(client, "/auth/register", new RegisterRequest { AccountName = followUpName, Password = "longenough1", DisplayName = "Ivan" });
    Check(followUp.StatusCode == HttpStatusCode.Created, "Accounts table remains functional after injection attempt");
}

// ---------------------------------------------------------------------------
// 10. Concurrent duplicate registration: exactly one winner
// ---------------------------------------------------------------------------
await using (var host = await TestHost.StartAsync())
{
    var client = host.Client;
    var baseName = "Judy" + Rand();
    var casings = new[] { baseName, baseName.ToUpperInvariant(), baseName.ToLowerInvariant(), baseName, baseName.ToUpperInvariant(), baseName.ToLowerInvariant() };
    var tasks = casings.Select(name => PostProto(client, "/auth/register", new RegisterRequest { AccountName = name, Password = "longenough1", DisplayName = "Judy" }));
    var responses = await Task.WhenAll(tasks);

    int created = responses.Count(r => r.StatusCode == HttpStatusCode.Created);
    int conflicted = responses.Count(r => r.StatusCode == HttpStatusCode.Conflict);
    Check(created == 1, $"Exactly one concurrent registration wins (got {created})");
    Check(conflicted == casings.Length - 1, $"All other concurrent attempts see 409 (got {conflicted})");
}

// ---------------------------------------------------------------------------
// 11. Rate limiting on auth endpoints returns a protobuf ErrorReply, not an empty 429
// ---------------------------------------------------------------------------
await using (var host = await TestHost.StartAsync(authRateLimitPerMinute: 3))
{
    var client = host.Client;
    HttpResponseMessage? limited = null;
    for (int i = 0; i < 6 && limited is null; i++)
    {
        var response = await PostProto(client, "/auth/login", new AccountLoginRequest { AccountName = "nobody" + Rand(), Password = "whatever12" });
        if ((int)response.StatusCode == 429) limited = response;
    }
    Check(limited is not null, "Auth rate limit eventually returns 429 Too Many Requests");
    if (limited is not null)
    {
        Check(limited.Content.Headers.ContentType?.MediaType == "application/x-protobuf", "429 response is protobuf, not empty/HTML");
        var error = ErrorReply.Parser.ParseFrom(await Bytes(limited));
        Check(error.Code == "RATE_LIMITED", "429 response carries a RATE_LIMITED ErrorReply");
    }
}

// ---------------------------------------------------------------------------
// 12. Oversized request bodies produce a protobuf 413, not a generic 500
// ---------------------------------------------------------------------------
await using (var host = await TestHost.StartAsync())
{
    var client = host.Client;
    // Kestrel's MaxRequestBodySize is 4096 bytes; this raw (non-protobuf) body is well over that,
    // so the limit must trip before the request handler ever runs.
    using var oversized = new ByteArrayContent(new byte[8192]);
    oversized.Headers.ContentType = new("application/x-protobuf");
    using var request = new HttpRequestMessage(HttpMethod.Post, "/auth/register") { Content = oversized };
    var response = await client.SendAsync(request);
    Check((int)response.StatusCode == 413, $"Oversized body -> 413 (got {(int)response.StatusCode})");
    Check(response.Content.Headers.ContentType?.MediaType == "application/x-protobuf", "413 response is protobuf, not an HTML/empty error page");
    var error = ErrorReply.Parser.ParseFrom(await Bytes(response));
    Check(error.Code == "PAYLOAD_TOO_LARGE", "413 response carries a PAYLOAD_TOO_LARGE ErrorReply");
}

// ---------------------------------------------------------------------------
// 13. Health JSON and 204 logout remain explicit exceptions to the protobuf-body rule
// ---------------------------------------------------------------------------
await using (var host = await TestHost.StartAsync())
{
    var client = host.Client;
    var health = await client.GetAsync("/health");
    Check(health.StatusCode == HttpStatusCode.OK, "Health check succeeds");
    Check(health.Content.Headers.ContentType?.MediaType == "application/json", "Health response is JSON, the documented exception to protobuf bodies");

    var register = await PostProto(client, "/auth/register", new RegisterRequest { AccountName = "Kate_" + Rand(), Password = "longenough1", DisplayName = "Kate" });
    var login = LoginReply.Parser.ParseFrom(await Bytes(register));
    var logout = await PostProto(client, "/auth/logout", new Empty(), login.SessionToken);
    Check(logout.StatusCode == HttpStatusCode.NoContent, "Logout succeeds");
    Check((await Bytes(logout)).Length == 0, "204 logout has an empty body, the documented exception to protobuf bodies");
}

// ---------------------------------------------------------------------------
// 14. Guest logout invalidates the in-memory guest session too, not just the database
// ---------------------------------------------------------------------------
await using (var host = await TestHost.StartAsync(development: true))
{
    var client = host.Client;
    var guestResponse = await PostProto(client, "/auth/guest", new LoginRequest { DisplayName = "GuestPlayer" });
    Check(guestResponse.StatusCode == HttpStatusCode.OK, "Guest login succeeds (dev-only endpoint)");
    var guest = LoginReply.Parser.ParseFrom(await Bytes(guestResponse));

    var validateBefore = await PostProto(client, "/sessions/validate", new ValidateSessionRequest { SessionToken = guest.SessionToken });
    Check(validateBefore.StatusCode == HttpStatusCode.OK, "Guest session validates before logout");

    var guestLogout = await PostProto(client, "/auth/logout", new Empty(), guest.SessionToken);
    Check(guestLogout.StatusCode == HttpStatusCode.NoContent, "Guest logout -> 204");

    var validateAfter = await PostProto(client, "/sessions/validate", new ValidateSessionRequest { SessionToken = guest.SessionToken });
    Check(validateAfter.StatusCode == HttpStatusCode.Unauthorized, "Guest session is invalidated by logout, not just the (nonexistent) database row");
}

// ---------------------------------------------------------------------------
// 15. Login always runs PBKDF2, even for an unknown account (dummy-hash timing defense)
// ---------------------------------------------------------------------------
await using (var host = await TestHost.StartAsync())
{
    var client = host.Client;
    var accountName = "Leo_" + Rand();
    await PostProto(client, "/auth/register", new RegisterRequest { AccountName = accountName, Password = "correcthorse1", DisplayName = "Leo" });

    async Task<double> TimeLoginMs(string name, string password)
    {
        var stopwatch = Stopwatch.StartNew();
        await PostProto(client, "/auth/login", new AccountLoginRequest { AccountName = name, Password = password });
        stopwatch.Stop();
        return stopwatch.Elapsed.TotalMilliseconds;
    }

    // Warm up JIT/connection so the first real measurement isn't skewed by one-time setup cost.
    await TimeLoginMs(accountName, "correcthorse1");

    const int samples = 5;
    double knownAccountTotalMs = 0, unknownAccountTotalMs = 0;
    for (int i = 0; i < samples; i++)
    {
        knownAccountTotalMs += await TimeLoginMs(accountName, "wrongpassword" + i);
        unknownAccountTotalMs += await TimeLoginMs("nosuchaccount" + Rand(), "whatever" + i);
    }
    double knownAvgMs = knownAccountTotalMs / samples, unknownAvgMs = unknownAccountTotalMs / samples;

    // Best-effort regression check: an unknown account must still pay roughly the same PBKDF2
    // cost as a known one, not return near-instantly. Generously bounded to avoid CI flakiness —
    // the bug this guards against is a >10x gap (near-zero vs a real PBKDF2 delay), not noise.
    Check(unknownAvgMs >= knownAvgMs * 0.2,
        $"Unknown-account login pays comparable PBKDF2 cost to a known account (known={knownAvgMs:F1}ms, unknown={unknownAvgMs:F1}ms)");
}

// ---------------------------------------------------------------------------
// 16. PasswordHasher.Verify rejects malformed/out-of-bounds stored hashes
// ---------------------------------------------------------------------------
{
    Check(!PasswordHasher.Verify("anything", "not-a-hash-at-all"), "Verify rejects a completely malformed string");
    Check(!PasswordHasher.Verify("anything", "PBKDF2-SHA256$210000$onlythreeparts"), "Verify rejects a hash missing segments");
    Check(!PasswordHasher.Verify("anything", "BCRYPT$210000$c2FsdA==$a2V5"), "Verify rejects an unrecognized algorithm prefix");
    Check(!PasswordHasher.Verify("anything", "PBKDF2-SHA256$notanumber$c2FsdA==$a2V5"), "Verify rejects a non-numeric iteration count");
    Check(!PasswordHasher.Verify("anything", "PBKDF2-SHA256$0$c2FsdA==$a2V5"), "Verify rejects a zero iteration count");
    Check(!PasswordHasher.Verify("anything", "PBKDF2-SHA256$-5$c2FsdA==$a2V5"), "Verify rejects a negative iteration count");
    Check(!PasswordHasher.Verify("anything", "PBKDF2-SHA256$210000$not-base64!$a2V5"), "Verify rejects non-base64 salt");
    Check(!PasswordHasher.Verify("anything", "PBKDF2-SHA256$210000$c2FsdA==$not-base64!"), "Verify rejects non-base64 key");

    // The critical case: an empty salt AND an empty key would otherwise make Rfc2898DeriveBytes
    // produce a zero-length "hash" that trivially equals the zero-length stored value for *any*
    // password. This must never verify as a match.
    Check(!PasswordHasher.Verify("literally any password", "PBKDF2-SHA256$210000$$"),
        "Verify rejects a zero-length salt/key pair (no degenerate always-true match)");
    Check(!PasswordHasher.Verify("literally any password", $"PBKDF2-SHA256$210000${Convert.ToBase64String(new byte[16])}$"),
        "Verify rejects a zero-length key even with a plausible salt");

    // A wildly excessive iteration count must be rejected up front, not actually attempted.
    var stopwatch = Stopwatch.StartNew();
    var hugeIterations = $"PBKDF2-SHA256$2000000000${Convert.ToBase64String(new byte[16])}${Convert.ToBase64String(new byte[32])}";
    var result = PasswordHasher.Verify("anything", hugeIterations);
    stopwatch.Stop();
    Check(!result, "Verify rejects an excessive iteration count instead of attempting it");
    Check(stopwatch.Elapsed.TotalMilliseconds < 500, "Verify rejects the excessive iteration count quickly (bounds checked before PBKDF2 runs)");

    // Sanity: a hash this class actually produces still round-trips correctly.
    var real = PasswordHasher.Hash("correct password 1");
    Check(PasswordHasher.Verify("correct password 1", real), "Verify accepts a correctly-produced hash with the right password");
    Check(!PasswordHasher.Verify("wrong password 1", real), "Verify rejects a correctly-produced hash with the wrong password");
}

// ---------------------------------------------------------------------------
// 17. Expired sessions are purged from storage when a new session is issued
// ---------------------------------------------------------------------------
await using (var host = await TestHost.StartAsync(sessionTtlSeconds: 1))
{
    static string HashTokenForTest(string token) => Convert.ToHexString(SHA256.HashData(Encoding.UTF8.GetBytes(token)));
    async Task<long> CountSessionRows(string dataDir, string tokenHash)
    {
        await using var connection = new SqliteConnection(new SqliteConnectionStringBuilder
        {
            DataSource = Path.Combine(dataDir, "accounts.db"),
        }.ToString());
        await connection.OpenAsync();
        await using var command = connection.CreateCommand();
        command.CommandText = "SELECT COUNT(*) FROM sessions WHERE token_hash = $hash;";
        command.Parameters.AddWithValue("$hash", tokenHash);
        return (long)(await command.ExecuteScalarAsync())!;
    }

    var client = host.Client;
    var register = await PostProto(client, "/auth/register", new RegisterRequest { AccountName = "Mona_" + Rand(), Password = "longenough1", DisplayName = "Mona" });
    var login = LoginReply.Parser.ParseFrom(await Bytes(register));
    var tokenHash = HashTokenForTest(login.SessionToken);

    Check(await CountSessionRows(host.DataDirectory, tokenHash) == 1, "Session row exists right after issuance");

    await Task.Delay(TimeSpan.FromSeconds(1.5));
    Check(await CountSessionRows(host.DataDirectory, tokenHash) == 1,
        "Expired session row is not deleted merely by the passage of time");

    // Issuing a new session runs DeleteExpiredSessions as a side effect.
    await PostProto(client, "/auth/register", new RegisterRequest { AccountName = "Nina_" + Rand(), Password = "longenough1", DisplayName = "Nina" });
    Check(await CountSessionRows(host.DataDirectory, tokenHash) == 0,
        "Expired session row is purged the next time a session is issued");
}

// ---------------------------------------------------------------------------
// 18. Extreme/invalid TTL and rate-limit configuration fall back to safe defaults
// ---------------------------------------------------------------------------
await using (var host = await TestHost.StartAsync(sessionTtlSeconds: -100))
{
    var client = host.Client;
    var register = await PostProto(client, "/auth/register", new RegisterRequest { AccountName = "Oscar_" + Rand(), Password = "longenough1", DisplayName = "Oscar" });
    Check(register.StatusCode == HttpStatusCode.Created, "Registration succeeds despite a negative configured TTL");
    var login = LoginReply.Parser.ParseFrom(await Bytes(register));
    var now = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
    Check(login.ExpiresUnix > now + 3600,
        $"Negative TTL configuration falls back to a safe default instead of an already-expired session (expires_unix={login.ExpiresUnix}, now={now})");
}
await using (var host = await TestHost.StartAsync(authRateLimitPerMinute: 0))
{
    var client = host.Client;
    var response = await PostProto(client, "/auth/login", new AccountLoginRequest { AccountName = "nosuchaccount" + Rand(), Password = "whatever12" });
    Check((int)response.StatusCode != 429,
        $"A zero/invalid configured rate limit falls back to a safe default instead of rejecting every request (got {(int)response.StatusCode})");
}

if (failures > 0)
{
    Console.WriteLine($"{failures} check(s) failed.");
    return 1;
}
Console.WriteLine("All MIMIC account tests passed.");
return 0;

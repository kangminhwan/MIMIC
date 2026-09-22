using System.Collections.Concurrent;
using System.Security.Cryptography;
using System.Text;
using System.Threading.RateLimiting;
using Google.Protobuf;
using Microsoft.AspNetCore.Diagnostics;
using Microsoft.AspNetCore.Http;
using Mimic.PlatformServer.Accounts;
using Mimic.Protocol;

namespace Mimic.PlatformServer;

// Builds the PlatformServer WebApplication. Split out from Program.cs so tests can host the
// real app in-process (bind to an ephemeral port, exercise it over HTTP, then shut it down)
// without pulling in an external test/hosting framework.
public static class PlatformServerApp
{
    public static WebApplication Build(string[] args)
    {
        var builder = WebApplication.CreateBuilder(args);
        builder.WebHost.UseUrls(builder.Configuration["MIMIC_PLATFORM_URL"] ?? "http://127.0.0.1:5081");
        builder.WebHost.ConfigureKestrel(o => o.Limits.MaxRequestBodySize = 4096);

        // Bad or extreme configuration (negative, zero, or absurdly large) must fall back to a
        // safe default rather than destabilizing the service — e.g. PermitLimit<=0 would make
        // the rate limiter reject every request, and a negative/huge session TTL would issue
        // sessions that are either instantly expired or effectively permanent.
        var authPermitLimit = (int)ResolveBoundedLong(builder.Configuration["MIMIC_AUTH_RATE_LIMIT_PER_MINUTE"], 1, 100_000, 30);
        var sessionTtlSeconds = ResolveBoundedLong(builder.Configuration["MIMIC_SESSION_TTL_SECONDS"], 1, 2_592_000, 86_400);

        builder.Services.AddRateLimiter(o =>
        {
            o.RejectionStatusCode = StatusCodes.Status429TooManyRequests;
            o.OnRejected = async (context, cancellationToken) =>
            {
                context.HttpContext.Response.StatusCode = StatusCodes.Status429TooManyRequests;
                context.HttpContext.Response.ContentType = "application/x-protobuf";
                var bytes = new ErrorReply { Code = "RATE_LIMITED", Message = "Too many requests" }.ToByteArray();
                await context.HttpContext.Response.Body.WriteAsync(bytes, cancellationToken);
            };
            o.AddPolicy("sessions", context => RateLimitPartition.GetFixedWindowLimiter(
                context.Connection.RemoteIpAddress?.ToString() ?? "unknown",
                _ => new FixedWindowRateLimiterOptions { PermitLimit = 60, Window = TimeSpan.FromMinutes(1), QueueLimit = 0 }));
            o.AddPolicy("auth", context => RateLimitPartition.GetFixedWindowLimiter(
                context.Connection.RemoteIpAddress?.ToString() ?? "unknown",
                _ => new FixedWindowRateLimiterOptions { PermitLimit = authPermitLimit, Window = TimeSpan.FromMinutes(1), QueueLimit = 0 }));
        });

        var app = builder.Build();

        var dataDirectory = DataDirectory.Resolve(builder.Configuration);
        var database = new AccountDatabase(dataDirectory);

        // Guest sessions stay in-memory and development-only; they never touch persistent storage.
        var guestSessions = new ConcurrentDictionary<string, LoginReply>();

        // Guarantee every response — including unexpected failures — is protobuf, never an HTML
        // error page, and never leaks exception details or credentials. An oversized body (over
        // Kestrel's MaxRequestBodySize) surfaces as a BadHttpRequestException with its own
        // intended status (413); everything else is a generic, detail-free 500.
        app.UseExceptionHandler(handler => handler.Run(async context =>
        {
            var error = context.Features.Get<IExceptionHandlerFeature>()?.Error;
            var (status, code, message) = error switch
            {
                BadHttpRequestException { StatusCode: StatusCodes.Status413PayloadTooLarge } tooLarge
                    => (tooLarge.StatusCode, "PAYLOAD_TOO_LARGE", "Request body too large"),
                BadHttpRequestException badRequest
                    => (badRequest.StatusCode, "BAD_REQUEST", "Malformed request"),
                _ => (StatusCodes.Status500InternalServerError, "INTERNAL_ERROR", "Internal server error"),
            };
            context.Response.StatusCode = status;
            context.Response.ContentType = "application/x-protobuf";
            var bytes = new ErrorReply { Code = code, Message = message }.ToByteArray();
            await context.Response.Body.WriteAsync(bytes);
        }));

        app.UseRateLimiter();

        app.MapGet("/health", () => Results.Ok(new { service = "MIMIC.PlatformServer", status = "ok" }));

        app.MapPost("/auth/guest", async (HttpRequest request) =>
        {
            if (!app.Environment.IsDevelopment()) return Results.NotFound();
            var now = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
            foreach (var entry in guestSessions)
                if (entry.Value.ExpiresUnix <= now) guestSessions.TryRemove(entry.Key, out _);
            if (guestSessions.Count >= 1000) return ProtoError(503, "SERVER_BUSY", "Too many guest sessions");
            try
            {
                var input = LoginRequest.Parser.ParseFrom(await ReadBody(request));
                if (!AccountValidation.TryNormalizeDisplayName(input.DisplayName, out var name))
                    return ProtoError(400, "INVALID_DISPLAY_NAME", "Display name is invalid");
                var token = NewToken();
                var login = new LoginReply
                {
                    SessionToken = token,
                    PlayerId = Guid.NewGuid().ToString("N"),
                    DisplayName = name,
                    ExpiresUnix = now + 3600,
                };
                guestSessions[token] = login;
                return ProtoOk(200, login);
            }
            catch (InvalidProtocolBufferException) { return ProtoError(400, "INVALID_REQUEST", "Malformed request body"); }
        }).RequireRateLimiting("sessions");

        app.MapPost("/auth/register", async (HttpRequest request) =>
        {
            RegisterRequest input;
            try { input = RegisterRequest.Parser.ParseFrom(await ReadBody(request)); }
            catch (InvalidProtocolBufferException) { return ProtoError(400, "INVALID_REQUEST", "Malformed request body"); }

            if (!AccountValidation.IsValidAccountName(input.AccountName))
                return ProtoError(400, "INVALID_ACCOUNT_NAME", "Account name must be 3-24 ASCII letters, digits, or underscores");
            if (!AccountValidation.IsValidPassword(input.Password))
                return ProtoError(400, "INVALID_PASSWORD", "Password must be 8-128 characters");
            if (!AccountValidation.TryNormalizeDisplayName(input.DisplayName, out var displayName))
                return ProtoError(400, "INVALID_DISPLAY_NAME", "Display name must be 2-24 characters with no control characters");

            var now = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
            var account = new Account(
                PlayerId: Guid.NewGuid().ToString("N"),
                AccountName: input.AccountName,
                DisplayName: displayName,
                PasswordHash: PasswordHasher.Hash(input.Password),
                DemoChips: 1000,
                CreatedUnix: now);

            var outcome = database.TryCreateAccount(account);
            if (outcome == RegisterOutcome.Duplicate)
                return ProtoError(409, "ACCOUNT_EXISTS", "Account name is already taken");

            var login = IssueSession(database, account, sessionTtlSeconds);
            return ProtoOk(201, login);
        }).RequireRateLimiting("auth");

        app.MapPost("/auth/login", async (HttpRequest request) =>
        {
            AccountLoginRequest input;
            try { input = AccountLoginRequest.Parser.ParseFrom(await ReadBody(request)); }
            catch (InvalidProtocolBufferException) { return ProtoError(400, "INVALID_REQUEST", "Malformed request body"); }

            // Generic failure for any bad input: never reveal whether the account exists.
            if (!AccountValidation.IsValidAccountName(input.AccountName) || !AccountValidation.IsValidPassword(input.Password))
                return ProtoError(401, "INVALID_CREDENTIALS", "Invalid account name or password");

            var account = database.FindByAccountName(input.AccountName);
            // Always run PBKDF2, even for an unknown account name, against a process-wide dummy
            // hash — otherwise an unknown account returns instantly while a known one takes a
            // measurable PBKDF2 delay, letting response timing enumerate valid account names.
            var passwordMatches = PasswordHasher.Verify(input.Password, account?.PasswordHash ?? PasswordHasher.DummyHash);
            if (account is null || !passwordMatches)
                return ProtoError(401, "INVALID_CREDENTIALS", "Invalid account name or password");

            var login = IssueSession(database, account, sessionTtlSeconds);
            return ProtoOk(200, login);
        }).RequireRateLimiting("auth");

        app.MapPost("/auth/logout", (HttpRequest request) =>
        {
            if (!TryGetBearerToken(request, out var token))
                return ProtoError(401, "UNAUTHENTICATED", "Missing bearer token");
            database.DeleteSession(HashToken(token));
            // A logout call may also be presenting a dev-only guest token; drop that too so the
            // in-memory guest session cannot outlive an explicit logout.
            guestSessions.TryRemove(token, out _);
            return Results.StatusCode(204);
        }).RequireRateLimiting("auth");

        app.MapGet("/account/profile", (HttpRequest request) =>
        {
            if (!TryGetBearerToken(request, out var token))
                return ProtoError(401, "UNAUTHENTICATED", "Missing bearer token");
            var session = database.FindSession(HashToken(token));
            var now = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
            if (session is null || session.ExpiresUnix <= now)
                return ProtoError(401, "SESSION_EXPIRED", "Session is invalid or expired");
            var account = database.FindById(session.PlayerId);
            if (account is null)
                return ProtoError(401, "UNAUTHENTICATED", "Account no longer exists");
            return ProtoOk(200, new ProfileReply
            {
                PlayerId = account.PlayerId,
                AccountName = account.AccountName,
                DisplayName = account.DisplayName,
                DemoChips = account.DemoChips,
                CreatedUnix = account.CreatedUnix,
            });
        }).RequireRateLimiting("auth");

        app.MapPost("/sessions/validate", async (HttpRequest request) =>
        {
            ValidateSessionRequest input;
            try { input = ValidateSessionRequest.Parser.ParseFrom(await ReadBody(request)); }
            catch (InvalidProtocolBufferException) { return ProtoError(400, "INVALID_REQUEST", "Malformed request body"); }

            var now = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
            if (guestSessions.TryGetValue(input.SessionToken, out var guest))
            {
                if (guest.ExpiresUnix <= now) return ProtoError(401, "SESSION_EXPIRED", "Session is invalid or expired");
                return ProtoOk(200, new SessionReply { PlayerId = guest.PlayerId, DisplayName = guest.DisplayName });
            }

            var session = database.FindSession(HashToken(input.SessionToken));
            if (session is null || session.ExpiresUnix <= now)
                return ProtoError(401, "SESSION_EXPIRED", "Session is invalid or expired");
            var account = database.FindById(session.PlayerId);
            if (account is null)
                return ProtoError(401, "SESSION_EXPIRED", "Session is invalid or expired");
            return ProtoOk(200, new SessionReply { PlayerId = account.PlayerId, DisplayName = account.DisplayName });
        }).RequireRateLimiting("sessions");

        return app;
    }

    private static LoginReply IssueSession(AccountDatabase database, Account account, long ttlSeconds)
    {
        var token = NewToken();
        var now = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
        var expires = now + ttlSeconds;
        database.CreateSession(HashToken(token), account.PlayerId, expires, now);
        // Best-effort housekeeping piggy-backed on session issuance; a failure here must never
        // block the caller from actually logging in.
        try { database.DeleteExpiredSessions(now); } catch { /* housekeeping only */ }
        return new LoginReply
        {
            SessionToken = token,
            PlayerId = account.PlayerId,
            DisplayName = account.DisplayName,
            ExpiresUnix = expires,
            AccountName = account.AccountName,
            DemoChips = account.DemoChips,
        };
    }

    private static string NewToken() => Convert.ToHexString(RandomNumberGenerator.GetBytes(32));

    // Parses a config value into a bounded range, falling back to a known-safe default on a
    // missing/unparsable/out-of-range value instead of letting extreme configuration through.
    private static long ResolveBoundedLong(string? raw, long min, long max, long fallback)
        => long.TryParse(raw, out var value) && value >= min && value <= max ? value : fallback;

    // Session tokens are stored only as a hash so a stolen database backup cannot be replayed directly.
    private static string HashToken(string token) => Convert.ToHexString(SHA256.HashData(Encoding.UTF8.GetBytes(token)));

    private static bool TryGetBearerToken(HttpRequest request, out string token)
    {
        token = string.Empty;
        var header = request.Headers.Authorization.ToString();
        const string prefix = "Bearer ";
        if (!header.StartsWith(prefix, StringComparison.Ordinal)) return false;
        token = header[prefix.Length..].Trim();
        return token.Length > 0;
    }

    private static async Task<byte[]> ReadBody(HttpRequest request)
    {
        using var data = new MemoryStream();
        await request.Body.CopyToAsync(data, request.HttpContext.RequestAborted);
        return data.ToArray();
    }

    private static IResult ProtoOk(int status, IMessage message) => new ProtoResult(status, message.ToByteArray());

    private static IResult ProtoError(int status, string code, string message)
        => new ProtoResult(status, new ErrorReply { Code = code, Message = message }.ToByteArray());

    // Results.Bytes(...) has no statusCode overload, so a tiny custom IResult carries the status.
    private sealed class ProtoResult(int status, byte[] body) : IResult
    {
        public Task ExecuteAsync(HttpContext context)
        {
            context.Response.StatusCode = status;
            context.Response.ContentType = "application/x-protobuf";
            context.Response.ContentLength = body.Length;
            return context.Response.Body.WriteAsync(body).AsTask();
        }
    }
}

using System.Collections.Concurrent;
using System.Security.Cryptography;
using System.Threading.RateLimiting;
using Google.Protobuf;
using Mimic.Protocol;

var builder = WebApplication.CreateBuilder(args);
builder.WebHost.UseUrls(builder.Configuration["MIMIC_PLATFORM_URL"] ?? "http://127.0.0.1:5081");
builder.WebHost.ConfigureKestrel(o => o.Limits.MaxRequestBodySize = 4096);
builder.Services.AddRateLimiter(o => o.AddPolicy("sessions", context =>
    RateLimitPartition.GetFixedWindowLimiter(context.Connection.RemoteIpAddress?.ToString() ?? "unknown",
        _ => new FixedWindowRateLimiterOptions { PermitLimit = 60, Window = TimeSpan.FromMinutes(1), QueueLimit = 0 })));
var app = builder.Build();
var sessions = new ConcurrentDictionary<string, LoginReply>();
app.UseRateLimiter();
app.MapGet("/health", () => Results.Ok(new { service = "MIMIC.PlatformServer", status = "ok" }));
app.MapPost("/auth/guest", async (HttpRequest request) =>
{
    if (!app.Environment.IsDevelopment()) return Results.NotFound();
    var now = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
    foreach (var entry in sessions)
        if (entry.Value.ExpiresUnix <= now) sessions.TryRemove(entry.Key, out _);
    if (sessions.Count >= 1000) return Results.StatusCode(503);
    try
    {
        using var data = new MemoryStream();
        await request.Body.CopyToAsync(data, request.HttpContext.RequestAborted);
        var input = LoginRequest.Parser.ParseFrom(data.ToArray());
        var name = input.DisplayName.Trim();
        if (name.Length is < 1 or > 24 || name.Any(char.IsControl)) return Results.BadRequest();
        var token = Convert.ToHexString(RandomNumberGenerator.GetBytes(32));
        var login = new LoginReply { SessionToken = token, PlayerId = Guid.NewGuid().ToString("N"),
            DisplayName = name, ExpiresUnix = now + 3600 };
        sessions[token] = login;
        return Results.Bytes(login.ToByteArray(), "application/x-protobuf");
    }
    catch (InvalidProtocolBufferException) { return Results.BadRequest(); }
}).RequireRateLimiting("sessions");
app.MapPost("/sessions/validate", async (HttpRequest request) =>
{
    try
    {
        using var data = new MemoryStream();
        await request.Body.CopyToAsync(data, request.HttpContext.RequestAborted);
        var input = ValidateSessionRequest.Parser.ParseFrom(data.ToArray());
        if (!sessions.TryGetValue(input.SessionToken, out var login) || login.ExpiresUnix <= DateTimeOffset.UtcNow.ToUnixTimeSeconds())
            return Results.Unauthorized();
        return Results.Bytes(new SessionReply { PlayerId = login.PlayerId, DisplayName = login.DisplayName }.ToByteArray(), "application/x-protobuf");
    }
    catch (InvalidProtocolBufferException) { return Results.BadRequest(); }
}).RequireRateLimiting("sessions");
app.Run();

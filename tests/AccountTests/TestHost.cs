using Microsoft.AspNetCore.Builder;
using Microsoft.AspNetCore.Hosting.Server;
using Microsoft.AspNetCore.Hosting.Server.Features;
using Microsoft.Extensions.DependencyInjection;
using Mimic.PlatformServer;

namespace Mimic.AccountTests;

// Hosts the real PlatformServer app in-process on an OS-assigned loopback port so tests can
// exercise it over real HTTP without an external test framework or a persistent server process.
public sealed class TestHost : IAsyncDisposable
{
    public WebApplication App { get; }
    public HttpClient Client { get; }
    public string DataDirectory { get; }

    private TestHost(WebApplication app, HttpClient client, string dataDirectory)
    {
        App = app;
        Client = client;
        DataDirectory = dataDirectory;
    }

    public static async Task<TestHost> StartAsync(string? dataDirectory = null, long sessionTtlSeconds = 86_400, int authRateLimitPerMinute = 100_000, bool development = false)
    {
        dataDirectory ??= Path.Combine(Path.GetTempPath(), "mimic-account-tests-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(dataDirectory);

        Environment.SetEnvironmentVariable("MIMIC_DATA_DIR", dataDirectory);
        Environment.SetEnvironmentVariable("MIMIC_PLATFORM_URL", "http://127.0.0.1:0");
        Environment.SetEnvironmentVariable("MIMIC_SESSION_TTL_SECONDS", sessionTtlSeconds.ToString());
        Environment.SetEnvironmentVariable("MIMIC_AUTH_RATE_LIMIT_PER_MINUTE", authRateLimitPerMinute.ToString());
        // The dev-only /auth/guest endpoint requires the Development environment; most scenarios
        // run as Production to match how the service actually runs.
        Environment.SetEnvironmentVariable("ASPNETCORE_ENVIRONMENT", development ? "Development" : "Production");
        Environment.SetEnvironmentVariable("Logging__LogLevel__Default", "Warning");
        Environment.SetEnvironmentVariable("Logging__LogLevel__Microsoft.AspNetCore", "Warning");

        var app = PlatformServerApp.Build(Array.Empty<string>());
        await app.StartAsync();

        var addresses = app.Services.GetRequiredService<IServer>().Features.Get<IServerAddressesFeature>()
            ?? throw new InvalidOperationException("Server did not report a bound address");
        var baseAddress = addresses.Addresses.First();

        var client = new HttpClient { BaseAddress = new Uri(baseAddress), Timeout = TimeSpan.FromSeconds(10) };
        return new TestHost(app, client, dataDirectory);
    }

    public async ValueTask DisposeAsync()
    {
        Client.Dispose();
        await App.StopAsync();
        await App.DisposeAsync();
        try { Directory.Delete(DataDirectory, recursive: true); } catch { /* best effort cleanup */ }
    }
}

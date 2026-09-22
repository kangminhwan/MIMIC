namespace Mimic.PlatformServer.Accounts;

// Resolves where persistent SQLite data lives. Defaults to <repo-root>/artifacts/data so no
// real database is ever committed; MIMIC_DATA_DIR overrides this for deployments and tests.
public static class DataDirectory
{
    public static string Resolve(IConfiguration configuration)
    {
        var configured = configuration["MIMIC_DATA_DIR"];
        if (!string.IsNullOrWhiteSpace(configured)) return Path.GetFullPath(configured);

        var directory = new DirectoryInfo(AppContext.BaseDirectory);
        while (directory is not null && !Directory.Exists(Path.Combine(directory.FullName, ".git")))
            directory = directory.Parent;
        var root = directory?.FullName ?? Directory.GetCurrentDirectory();
        return Path.Combine(root, "artifacts", "data");
    }
}

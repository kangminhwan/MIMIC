using Microsoft.Data.Sqlite;

namespace Mimic.PlatformServer.Accounts;

public sealed record Account(
    string PlayerId,
    string AccountName,
    string DisplayName,
    string PasswordHash,
    long DemoChips,
    long CreatedUnix);

public sealed record SessionRow(string PlayerId, long ExpiresUnix);

public enum RegisterOutcome { Ok, Duplicate }

// Persistent account + session storage backed by a single SQLite file under MIMIC_DATA_DIR.
// One short-lived connection per operation; WAL + a busy timeout let concurrent readers/writers
// coexist without the caller needing to manage a connection pool explicitly.
public sealed class AccountDatabase
{
    private readonly string connectionString;

    public AccountDatabase(string dataDirectory)
    {
        Directory.CreateDirectory(dataDirectory);
        var dbPath = Path.Combine(dataDirectory, "accounts.db");
        connectionString = new SqliteConnectionStringBuilder
        {
            DataSource = dbPath,
            Cache = SqliteCacheMode.Shared,
        }.ToString();
        Initialize();
    }

    private SqliteConnection OpenConnection()
    {
        var connection = new SqliteConnection(connectionString);
        connection.Open();
        using var pragma = connection.CreateCommand();
        pragma.CommandText = "PRAGMA journal_mode=WAL; PRAGMA busy_timeout=5000; PRAGMA foreign_keys=ON;";
        pragma.ExecuteNonQuery();
        return connection;
    }

    private void Initialize()
    {
        using var connection = OpenConnection();
        using var command = connection.CreateCommand();
        command.CommandText = """
            CREATE TABLE IF NOT EXISTS accounts (
                player_id TEXT PRIMARY KEY,
                account_name TEXT NOT NULL,
                account_name_normalized TEXT NOT NULL UNIQUE,
                display_name TEXT NOT NULL,
                password_hash TEXT NOT NULL,
                demo_chips INTEGER NOT NULL,
                created_unix INTEGER NOT NULL
            );
            CREATE TABLE IF NOT EXISTS sessions (
                token_hash TEXT PRIMARY KEY,
                player_id TEXT NOT NULL,
                expires_unix INTEGER NOT NULL,
                created_unix INTEGER NOT NULL
            );
            CREATE INDEX IF NOT EXISTS idx_sessions_player ON sessions(player_id);
            """;
        command.ExecuteNonQuery();
    }

    public RegisterOutcome TryCreateAccount(Account account)
    {
        using var connection = OpenConnection();
        using var command = connection.CreateCommand();
        command.CommandText = """
            INSERT INTO accounts (player_id, account_name, account_name_normalized, display_name, password_hash, demo_chips, created_unix)
            VALUES ($playerId, $accountName, $normalized, $displayName, $passwordHash, $demoChips, $createdUnix);
            """;
        command.Parameters.AddWithValue("$playerId", account.PlayerId);
        command.Parameters.AddWithValue("$accountName", account.AccountName);
        command.Parameters.AddWithValue("$normalized", AccountValidation.NormalizeAccountName(account.AccountName));
        command.Parameters.AddWithValue("$displayName", account.DisplayName);
        command.Parameters.AddWithValue("$passwordHash", account.PasswordHash);
        command.Parameters.AddWithValue("$demoChips", account.DemoChips);
        command.Parameters.AddWithValue("$createdUnix", account.CreatedUnix);
        try
        {
            command.ExecuteNonQuery();
            return RegisterOutcome.Ok;
        }
        catch (SqliteException error) when (error.SqliteErrorCode == 19) // SQLITE_CONSTRAINT
        {
            return RegisterOutcome.Duplicate;
        }
    }

    public Account? FindByAccountName(string accountName)
    {
        using var connection = OpenConnection();
        using var command = connection.CreateCommand();
        command.CommandText = """
            SELECT player_id, account_name, display_name, password_hash, demo_chips, created_unix
            FROM accounts WHERE account_name_normalized = $normalized;
            """;
        command.Parameters.AddWithValue("$normalized", AccountValidation.NormalizeAccountName(accountName));
        using var reader = command.ExecuteReader();
        return reader.Read() ? ReadAccount(reader) : null;
    }

    public Account? FindById(string playerId)
    {
        using var connection = OpenConnection();
        using var command = connection.CreateCommand();
        command.CommandText = """
            SELECT player_id, account_name, display_name, password_hash, demo_chips, created_unix
            FROM accounts WHERE player_id = $playerId;
            """;
        command.Parameters.AddWithValue("$playerId", playerId);
        using var reader = command.ExecuteReader();
        return reader.Read() ? ReadAccount(reader) : null;
    }

    private static Account ReadAccount(SqliteDataReader reader) => new(
        PlayerId: reader.GetString(0),
        AccountName: reader.GetString(1),
        DisplayName: reader.GetString(2),
        PasswordHash: reader.GetString(3),
        DemoChips: reader.GetInt64(4),
        CreatedUnix: reader.GetInt64(5));

    public void CreateSession(string tokenHash, string playerId, long expiresUnix, long createdUnix)
    {
        using var connection = OpenConnection();
        using var command = connection.CreateCommand();
        command.CommandText = """
            INSERT INTO sessions (token_hash, player_id, expires_unix, created_unix)
            VALUES ($tokenHash, $playerId, $expiresUnix, $createdUnix);
            """;
        command.Parameters.AddWithValue("$tokenHash", tokenHash);
        command.Parameters.AddWithValue("$playerId", playerId);
        command.Parameters.AddWithValue("$expiresUnix", expiresUnix);
        command.Parameters.AddWithValue("$createdUnix", createdUnix);
        command.ExecuteNonQuery();
    }

    public SessionRow? FindSession(string tokenHash)
    {
        using var connection = OpenConnection();
        using var command = connection.CreateCommand();
        command.CommandText = "SELECT player_id, expires_unix FROM sessions WHERE token_hash = $tokenHash;";
        command.Parameters.AddWithValue("$tokenHash", tokenHash);
        using var reader = command.ExecuteReader();
        return reader.Read() ? new SessionRow(reader.GetString(0), reader.GetInt64(1)) : null;
    }

    public void DeleteSession(string tokenHash)
    {
        using var connection = OpenConnection();
        using var command = connection.CreateCommand();
        command.CommandText = "DELETE FROM sessions WHERE token_hash = $tokenHash;";
        command.Parameters.AddWithValue("$tokenHash", tokenHash);
        command.ExecuteNonQuery();
    }

    public void DeleteExpiredSessions(long nowUnix)
    {
        using var connection = OpenConnection();
        using var command = connection.CreateCommand();
        command.CommandText = "DELETE FROM sessions WHERE expires_unix <= $now;";
        command.Parameters.AddWithValue("$now", nowUnix);
        command.ExecuteNonQuery();
    }
}

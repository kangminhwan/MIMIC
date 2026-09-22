using System.Security.Cryptography;

namespace Mimic.PlatformServer.Accounts;

// Salted PBKDF2-SHA256 password storage. Never stores or logs plaintext passwords.
public static class PasswordHasher
{
    private const int Iterations = 210_000;
    private const int SaltSize = 16;
    private const int KeySize = 32;
    private const string Prefix = "PBKDF2-SHA256";

    // Bounds applied when parsing a stored hash back out of the database. Without these, a
    // corrupted or maliciously-crafted row could either (a) force an arbitrarily large PBKDF2
    // work factor (denial of service) or (b) encode a zero-length salt/key, which would make
    // Rfc2898DeriveBytes.Pbkdf2 return an empty array that trivially "matches" any password
    // against an equally empty stored key via FixedTimeEquals(zero-length, zero-length) == true.
    private const int MinIterations = 1;
    private const int MaxIterations = 2_000_000;
    private const int MinSaltSize = 8;
    private const int MaxSaltSize = 64;
    private const int MinKeySize = 16;
    private const int MaxKeySize = 64;

    public static string Hash(string password)
    {
        var salt = RandomNumberGenerator.GetBytes(SaltSize);
        var key = Rfc2898DeriveBytes.Pbkdf2(password, salt, Iterations, HashAlgorithmName.SHA256, KeySize);
        return $"{Prefix}${Iterations}${Convert.ToBase64String(salt)}${Convert.ToBase64String(key)}";
    }

    // A syntactically valid hash of a random, never-revealed password. Verifying against this
    // costs the same PBKDF2 work as a real account, so /auth/login cannot use response timing to
    // tell "unknown account" apart from "wrong password" for an account that does exist.
    public static readonly string DummyHash = Hash(Convert.ToHexString(RandomNumberGenerator.GetBytes(32)));

    // Constant-time comparison so failure timing does not leak how much of the hash matched.
    // Rejects any stored value outside the bounds this class itself would ever produce, so a
    // malformed database row can only ever fail verification, never force excessive work or a
    // degenerate always-true comparison.
    public static bool Verify(string password, string stored)
    {
        var parts = stored.Split('$');
        if (parts.Length != 4 || parts[0] != Prefix) return false;
        if (!int.TryParse(parts[1], out var iterations) || iterations < MinIterations || iterations > MaxIterations) return false;

        byte[] salt, expected;
        try
        {
            salt = Convert.FromBase64String(parts[2]);
            expected = Convert.FromBase64String(parts[3]);
        }
        catch (FormatException) { return false; }

        if (salt.Length < MinSaltSize || salt.Length > MaxSaltSize) return false;
        if (expected.Length < MinKeySize || expected.Length > MaxKeySize) return false;

        var actual = Rfc2898DeriveBytes.Pbkdf2(password, salt, iterations, HashAlgorithmName.SHA256, expected.Length);
        return CryptographicOperations.FixedTimeEquals(actual, expected);
    }
}

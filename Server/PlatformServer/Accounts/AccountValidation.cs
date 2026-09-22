using System.Text.RegularExpressions;

namespace Mimic.PlatformServer.Accounts;

// Input validation rules shared by /auth/register and /auth/login.
public static partial class AccountValidation
{
    public const int AccountNameMinLength = 3;
    public const int AccountNameMaxLength = 24;
    public const int PasswordMinLength = 8;
    public const int PasswordMaxLength = 128;
    public const int DisplayNameMinLength = 2;
    public const int DisplayNameMaxLength = 24;

    [GeneratedRegex("^[A-Za-z0-9_]{3,24}$")]
    private static partial Regex AccountNamePattern();

    public static bool IsValidAccountName(string? accountName)
        => accountName is not null && AccountNamePattern().IsMatch(accountName);

    public static bool IsValidPassword(string? password)
        => password is not null && password.Length is >= PasswordMinLength and <= PasswordMaxLength;

    // Trims surrounding whitespace first, matching the existing guest display-name handling.
    public static bool TryNormalizeDisplayName(string? displayName, out string normalized)
    {
        normalized = (displayName ?? string.Empty).Trim();
        return normalized.Length is >= DisplayNameMinLength and <= DisplayNameMaxLength
            && !normalized.Any(char.IsControl);
    }

    // Case-insensitive lookup key; account name charset is already ASCII so ToLowerInvariant is safe.
    public static string NormalizeAccountName(string accountName) => accountName.ToLowerInvariant();
}

using Mimic.Protocol;
namespace Mimic.Data
{
    public enum AppPhase { Title, Login, Lobby, Table }
    public sealed class SessionData
    {
        public AppPhase Phase { get; set; }
        public string PlayerId { get; set; } = "";
        public string DisplayName { get; set; } = "";
        public string AccountName { get; set; } = "";
        public long DemoChips { get; set; }
        public LobbyReply Lobby { get; set; }
        public TableSnapshot Table { get; set; }
        public void Clear() { Phase = AppPhase.Title; PlayerId = ""; DisplayName = ""; AccountName = ""; DemoChips = 0; Lobby = null; Table = null; }
    }
}

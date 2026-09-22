using Mimic.Protocol;
namespace Mimic.Data
{
    public enum AppPhase { Title, Lobby, Table }
    public sealed class SessionData
    {
        public AppPhase Phase { get; set; }
        public string PlayerId { get; set; } = "";
        public string DisplayName { get; set; } = "";
        public LobbyReply Lobby { get; set; }
        public TableSnapshot Table { get; set; }
        public void Clear() { Phase = AppPhase.Title; PlayerId = ""; DisplayName = ""; Lobby = null; Table = null; }
    }
}

using Mimic.Managers;
using UnityEngine;
namespace Mimic.Lobby
{
    public static class LobbyView
    {
        public static void Draw(GaManager game)
        {
            GUILayout.Label("HOLDEM LOBBY");
            GUILayout.Label("Development chips only / 6 seats / blinds 10-20");
            GUILayout.Space(16);
            if (game.Data.Lobby != null)
                foreach (var table in game.Data.Lobby.Tables)
                    if (GUILayout.Button(table.Name + "    " + table.Players + "/" + table.Capacity + "    Join", GUILayout.Height(48)))
                        game.Run(() => game.Join(table.TableId));
            if (GUILayout.Button("Refresh tables")) game.Run(game.RefreshLobby);
        }
    }
}

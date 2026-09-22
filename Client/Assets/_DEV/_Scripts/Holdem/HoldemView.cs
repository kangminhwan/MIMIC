using System.Linq;
using Mimic.Managers;
using Mimic.Protocol;
using UnityEngine;
namespace Mimic.Holdem
{
    public static class HoldemView
    {
        private static string raise = "40";
        public static void Draw(GaManager game)
        {
            var table = game.Data.Table;
            if (table == null) { GUILayout.Label("Waiting for table snapshot..."); return; }
            GUILayout.Label("TABLE 01  /  " + table.Street + "  /  HAND " + table.HandId);
            GUILayout.Space(12);
            GUILayout.Label("BOARD  " + string.Join("   ", table.Board.Select(CardText)));
            GUILayout.Label("POT  " + table.Pot + "       TO MATCH  " + table.CurrentBet + "       MIN RAISE  " + table.MinRaise);
            GUILayout.Space(16);
            foreach (var player in table.Players)
            {
                string turn = (int)player.Seat == table.ActingSeat ? "> " : "  ";
                string marker = player.PlayerId == game.Data.PlayerId ? " (YOU)" : "";
                string cards = player.HoleCards.Count > 0 ? string.Join(" ", player.HoleCards.Select(CardText)) : "[--] [--]";
                GUILayout.Label(turn + "SEAT " + (player.Seat + 1) + "  " + player.DisplayName + marker + "    " + player.Chips + " chips    bet " + player.StreetBet + "    " + cards + (player.Folded ? "  FOLDED" : player.Ready ? "  READY" : "") + (!player.Connected ? "  OFFLINE" : ""));
            }
            GUILayout.Space(16);
            var own = table.Players.FirstOrDefault(p => p.PlayerId == game.Data.PlayerId);
            bool between = table.Street == Street.Waiting || table.Street == Street.Complete;
            if (between)
            {
                GUILayout.Label(table.Result);
                GUILayout.BeginHorizontal();
                if (GUILayout.Button("Ready / Next hand")) game.Run(game.Ready);
                if (GUILayout.Button("Back to lobby")) game.Run(game.Leave);
                GUILayout.EndHorizontal();
            }
            else
            {
                GUI.enabled = !game.Busy && own != null && (int)own.Seat == table.ActingSeat;
                GUILayout.BeginHorizontal();
                if (GUILayout.Button("Fold")) game.Run(() => game.Act(ActionKind.Fold));
                if (own != null && own.StreetBet == table.CurrentBet)
                { if (GUILayout.Button("Check")) game.Run(() => game.Act(ActionKind.Check)); }
                else if (GUILayout.Button("Call")) game.Run(() => game.Act(ActionKind.Call));
                raise = GUILayout.TextField(raise, 10, GUILayout.Width(100));
                if (GUILayout.Button("Raise to") && long.TryParse(raise, out long amount)) game.Run(() => game.Act(ActionKind.Raise, amount));
                GUILayout.EndHorizontal();
                GUI.enabled = !game.Busy;
            }
        }
        private static string CardText(Card card)
        {
            string rank = card.Rank <= 10 ? card.Rank.ToString() : card.Rank == 11 ? "J" : card.Rank == 12 ? "Q" : card.Rank == 13 ? "K" : "A";
            string[] suits = { "C", "D", "H", "S" };
            return "[" + rank + suits[(int)card.Suit] + "]";
        }
    }
}

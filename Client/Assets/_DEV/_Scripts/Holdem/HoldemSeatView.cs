using Mimic.Protocol;
using UnityEngine;
using UnityEngine.UI;
namespace Mimic.Screens
{
    public sealed class HoldemSeatView : MonoBehaviour
    {
        public Text playerName, chips, state, cards;
        public Graphic outline;
        public void Bind(PlayerState player, bool acting, bool own)
        {
            if (player == null) { playerName.text = "빈 자리"; chips.text = ""; state.text = ""; cards.text = ""; outline.color = new Color(.12f,.2f,.23f); return; }
            playerName.text = player.DisplayName + (own ? "  ·  나" : ""); chips.text = player.Chips.ToString("N0");
            state.text = !player.Connected ? "연결 끊김" : player.Folded ? "폴드" : player.Ready ? "준비 완료" : acting ? "플레이 중" : player.StreetBet > 0 ? "베팅 " + player.StreetBet : "대기";
            cards.text = player.HoleCards.Count == 0 ? "▧  ▧" : HoldemScreen.CardLabel(player.HoleCards[0]) + "   " + (player.HoleCards.Count > 1 ? HoldemScreen.CardLabel(player.HoleCards[1]) : "?");
            outline.color = acting ? new Color(.8f,.65f,.36f) : own ? new Color(.14f,.38f,.4f) : new Color(.12f,.2f,.23f);
        }
    }
}

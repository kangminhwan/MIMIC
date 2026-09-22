using System.Linq;
using Mimic.Protocol;
using Mimic.UI;
using UnityEngine;
using UnityEngine.UI;
namespace Mimic.Screens
{
    public sealed class HoldemScreen : ScreenBase
    {
        public HoldemSeatView[] seats;
        public Text[] board;
        public Text pot, stage, instruction, result, callLabel;
        public Button ready, leave, fold, call, raise;
        public InputField raiseAmount;
        public GameObject actionBar;
        private ulong revision = ulong.MaxValue;
        private void Start()
        {
            if (string.IsNullOrEmpty(Game.Data.PlayerId)) { Game.Run(Game.ShowLogin); return; }
            ready.onClick.AddListener(() => Game.Run(Game.Ready)); leave.onClick.AddListener(() => Game.Run(Game.Leave));
            fold.onClick.AddListener(() => Game.Run(() => Game.Act(ActionKind.Fold)));
            call.onClick.AddListener(() =>
            {
                var state = Game.Data.Table; var own = state?.Players.FirstOrDefault(p => p.PlayerId == Game.Data.PlayerId);
                if (own != null) Game.Run(() => Game.Act(own.StreetBet == state.CurrentBet ? ActionKind.Check : ActionKind.Call));
            });
            raise.onClick.AddListener(() =>
            {
                if (!long.TryParse(raiseAmount.text, out long amount)) { Game.SetStatus("레이즈 금액을 숫자로 입력해 주세요.", true); return; }
                var state = Game.Data.Table; var own = state.Players.First(p => p.PlayerId == Game.Data.PlayerId);
                if (amount < state.CurrentBet + state.MinRaise || amount > own.Chips + own.StreetBet) { Game.SetStatus("최소 레이즈와 보유 칩 범위 안에서 입력해 주세요.", true); return; }
                Game.Run(() => Game.Act(ActionKind.Raise, amount));
            });
            Refresh();
        }
        public static string CardLabel(Card card)
        {
            string rank = card.Rank <= 10 ? card.Rank.ToString() : card.Rank == 11 ? "J" : card.Rank == 12 ? "Q" : card.Rank == 13 ? "K" : "A";
            return rank + new[] { "♣", "♦", "♥", "♠" }[(int)card.Suit];
        }
        protected override void Refresh()
        {
            base.Refresh(); if (Game == null || pot == null) return;
            var state = Game.Data.Table; if (state == null) return;
            var own = state.Players.FirstOrDefault(p => p.PlayerId == Game.Data.PlayerId);
            for (int i = 0; i < seats.Length; i++) { var player = state.Players.FirstOrDefault(p => p.Seat == i); seats[i].Bind(player, state.ActingSeat == i, player?.PlayerId == Game.Data.PlayerId); }
            for (int i = 0; i < board.Length; i++)
            {
                board[i].text = i < state.Board.Count ? CardLabel(state.Board[i]) : "M";
                board[i].color = i < state.Board.Count ? (state.Board[i].Suit == 1 || state.Board[i].Suit == 2 ? new Color(.8f,.28f,.25f) : new Color(.08f,.13f,.16f)) : new Color(.55f,.61f,.6f);
            }
            pot.text = state.Pot.ToString("N0"); stage.text = "HAND " + state.HandId.ToString("000") + "  /  " + state.Street.ToString().ToUpperInvariant();
            bool waiting = state.Street == Street.Waiting || state.Street == Street.Complete;
            bool turn = !waiting && own != null && own.Seat == state.ActingSeat;
            ready.gameObject.SetActive(waiting); ready.interactable = !Game.Busy && own != null && !own.Ready && own.Chips >= 20;
            leave.interactable = !Game.Busy && waiting; actionBar.SetActive(!waiting);
            fold.interactable = call.interactable = raise.interactable = raiseAmount.interactable = !Game.Busy && turn;
            callLabel.text = own != null && own.StreetBet == state.CurrentBet ? "체크" : "콜 " + (own == null ? 0 : System.Math.Min(own.Chips, state.CurrentBet - own.StreetBet));
            instruction.text = waiting ? "모든 플레이어가 준비하면 게임이 시작됩니다." : turn ? "당신의 차례입니다." : "다른 플레이어의 액션을 기다리는 중";
            result.text = state.Result;
            if (revision != state.Revision && !raiseAmount.isFocused) raiseAmount.text = (state.CurrentBet + state.MinRaise).ToString();
            revision = state.Revision;
        }
    }
}

using System.Collections.Generic;
using General;
using Mimic.Managers;
using Mimic.Protocol;
using Mimic.Screens;
using UnityEngine;
using UnityEngine.UI;

namespace Mimic.UI
{
    public sealed class NativeHoldemControls
    {
        private readonly HoldemScreen screen;
        private readonly GaManager game;
        private readonly List<Button> actions = new List<Button>();
        private readonly List<TableAction> kinds = new List<TableAction>();

        public NativeHoldemControls(HoldemScreen screen, GaManager game)
        {
            this.screen = screen; this.game = game;
            screen.fold.gameObject.SetActive(false); screen.call.gameObject.SetActive(false);
            screen.raise.gameObject.SetActive(false); screen.raiseAmount.gameObject.SetActive(false);
            var bar = (RectTransform)screen.actionBar.transform;
            bar.anchoredPosition = new Vector2(370, bar.anchoredPosition.y);
            for (int i = 0; i < 8; i++)
            {
                var button = Object.Instantiate(screen.raise, bar);
                button.name = "Native action " + i;
                var rect = (RectTransform)button.transform;
                rect.anchoredPosition = new Vector2(i * 148, 0); rect.sizeDelta = new Vector2(140, 76);
                int index = i; button.onClick.RemoveAllListeners();
                button.onClick.AddListener(() => { if (index < kinds.Count) game.Run(() => game.BetNative(kinds[index])); });
                actions.Add(button);
            }
            var seats = new List<HoldemSeatView>(screen.seats);
            while (seats.Count < 9) seats.Add(Object.Instantiate(screen.seats[0], screen.seats[0].transform.parent));
            screen.seats = seats.ToArray();
            Vector2[] positions = { new Vector2(835,767), new Vector2(440,735), new Vector2(110,510),
                new Vector2(220,220), new Vector2(580,120), new Vector2(1000,120),
                new Vector2(1400,220), new Vector2(1550,510), new Vector2(1250,735) };
            for (int i = 0; i < 9; i++) ((RectTransform)seats[i].transform).anchoredPosition = new Vector2(positions[i].x, -positions[i].y);
        }

        public void Refresh(TableSnapshot snapshot)
        {
            var roomTitle = screen.transform.Find("Room name")?.GetComponent<Text>();
            if (roomTitle != null) roomTitle.text = "TABLE " + snapshot.TableId + "  /  TEXAS HOLD'EM";
            var state = game.Network.TableState;
            kinds.Clear(); if (state != null) kinds.AddRange(state.Actions);
            for (int i = 0; i < actions.Count; i++)
            {
                actions[i].gameObject.SetActive(i < kinds.Count);
                if (i >= kinds.Count) continue;
                actions[i].GetComponentInChildren<Text>().text = Label(kinds[i]);
                actions[i].interactable = !game.Busy;
            }
            bool complete = snapshot.Street == Street.Complete;
            bool waiting = snapshot.Street == Street.Waiting;
            screen.actionBar.SetActive(!waiting && !complete);
            screen.ready.gameObject.SetActive(waiting || complete);
            screen.ready.GetComponentInChildren<Text>().text = complete ? "결과 확인" : "게임 시작";
            screen.ready.interactable = !game.Busy && (complete || state?.CanStart == true);
            screen.leave.interactable = !game.Busy && (waiting || complete);
            screen.instruction.text = complete ? "결과를 확인하면 다음 판을 진행합니다." : waiting ?
                "2명 이상 입장 후 시작 담당자가 시작할 수 있습니다." : kinds.Count > 0 ? "베팅을 선택해 주세요." : "다른 플레이어의 차례입니다.";
        }

        private static string Label(TableAction action)
        {
            switch (action)
            {
                case TableAction.GiveUp: return "폴드";
                case TableAction.Check: return "체크";
                case TableAction.Call: return "콜";
                case TableAction.QuarterPot: return "쿼터";
                case TableAction.HalfPot: return "하프";
                case TableAction.FullPot: return "풀";
                case TableAction.DoubleRaise: return "따당";
                case TableAction.Maximum: return "맥스";
                case TableAction.AllIn: return "올인";
                case TableAction.SeedOnly: return "삥";
                default: return action.ToString();
            }
        }
    }
}

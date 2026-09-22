using System.Collections.Generic;
using System.Linq;
using Mimic.Managers;
using Mimic.UI;
using UnityEngine;
using UnityEngine.UI;
namespace Mimic.Screens
{
    public sealed class LobbyScreen : ScreenBase
    {
        public Text greeting, accountName, balance, tableCount, connection;
        public Transform tablesRoot;
        public TableCardView tablePrefab;
        public Button quickStart, refresh, logout;
        public GameObject emptyState;
        private readonly List<TableCardView> cards = new List<TableCardView>();
        private void Start()
        {
            if (string.IsNullOrEmpty(Game.Data.PlayerId)) { Game.Run(Game.ShowLogin); return; }
            refresh.onClick.AddListener(() => Game.Run(Game.RefreshLobby));
            logout.onClick.AddListener(() => Game.Run(Game.LogoutAsync));
            quickStart.onClick.AddListener(() =>
            {
                var table = Game.Data.Lobby?.Tables.FirstOrDefault(t => t.Players < t.Capacity);
                if (table == null) Game.SetStatus("입장 가능한 테이블이 없습니다. 새로고침해 주세요.", true);
                else Game.Run(() => Game.Join(table.TableId));
            });
            Refresh();
        }
        protected override void Refresh()
        {
            base.Refresh(); if (Game == null || greeting == null) return;
            greeting.text = Game.Data.DisplayName + "님, 좋은 한 판 되세요.";
            accountName.text = Game.Data.AccountName;
            balance.text = Game.Data.DemoChips.ToString("N0");
            connection.text = Game.Network.Client?.IsConnected == true ? "● 연결됨" : "○ 연결 확인 중";
            int count = Game.Data.Lobby?.Tables.Count ?? 0;
            tableCount.text = "플레이 가능한 테이블  " + count;
            emptyState.SetActive(count == 0);
            while (cards.Count < count) cards.Add(Instantiate(tablePrefab, tablesRoot));
            for (int i = 0; i < cards.Count; i++)
            {
                cards[i].gameObject.SetActive(i < count);
                if (i < count) cards[i].Bind(Game.Data.Lobby.Tables[i], Game.Busy, id => Game.Run(() => Game.Join(id)));
            }
            quickStart.interactable = !Game.Busy && count > 0; refresh.interactable = logout.interactable = !Game.Busy;
        }
    }
}

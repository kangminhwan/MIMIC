using System;
using Mimic.Protocol;
using UnityEngine;
using UnityEngine.UI;
namespace Mimic.Screens
{
    public sealed class TableCardView : MonoBehaviour
    {
        public Text title, blinds, seats, description;
        public Button join;
        public void Bind(TableInfo table, bool busy, Action<uint> onJoin, bool native = false)
        {
            title.text = "홀덤 테이블 " + table.TableId.ToString("00");
            blinds.text = table.SmallBlind + " / " + table.BigBlind;
            seats.text = table.Players + " / " + table.Capacity + " 플레이어";
            description.text = "NO LIMIT HOLD'EM  ·  연습 칩";
            if (native) { title.text = table.Name; description.text = "TEXAS HOLD'EM"; }
            join.interactable = !busy && table.Players < table.Capacity;
            join.onClick.RemoveAllListeners(); join.onClick.AddListener(() => onJoin(table.TableId));
        }
    }
}

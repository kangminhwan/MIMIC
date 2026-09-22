using Mimic.Managers;
using UnityEngine;
using UnityEngine.UI;
namespace Mimic.UI
{
    public abstract class ScreenBase : MonoBehaviour
    {
        public Text status;
        public GameObject busyOverlay;
        protected GaManager Game => GaManager.Instance;
        protected virtual void OnEnable() { if (Game != null) { Game.Changed += Refresh; Refresh(); } }
        protected virtual void OnDisable() { if (Game != null) Game.Changed -= Refresh; }
        protected virtual void Refresh()
        {
            if (Game == null) return;
            if (status != null) { status.text = Game.Status; status.color = Game.StatusIsError ? new Color(1, .46f, .44f) : new Color(.53f, .7f, .7f); }
            if (busyOverlay != null) busyOverlay.SetActive(Game.Busy);
        }
    }
}

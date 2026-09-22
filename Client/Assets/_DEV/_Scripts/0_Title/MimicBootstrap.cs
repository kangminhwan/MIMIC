using Mimic.Managers;
using UnityEngine;
namespace Mimic
{
    public sealed class MimicBootstrap : MonoBehaviour
    {
        private void Start() { GaManager.Instance.Run(() => GaManager.Instance.Navigate(GaManager.TitleScene)); }
    }
}

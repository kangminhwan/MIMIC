using Mimic.UI;
using UnityEngine;
using UnityEngine.UI;
namespace Mimic.Screens
{
    public sealed class TitleScreen : ScreenBase
    {
        public Button startButton, guideButton, closeGuideButton, quitButton;
        public GameObject guide;
        private void Start()
        {
            startButton.onClick.AddListener(() => Game.Run(Game.ShowLogin));
            guideButton.onClick.AddListener(() => guide.SetActive(true));
            closeGuideButton.onClick.AddListener(() => guide.SetActive(false));
            quitButton.onClick.AddListener(() => Application.Quit());
        }
        protected override void Refresh() { base.Refresh(); if (startButton != null && Game != null) startButton.interactable = !Game.Busy; }
    }
}

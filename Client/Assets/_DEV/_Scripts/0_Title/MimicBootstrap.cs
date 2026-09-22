using Mimic.Data;
using Mimic.Holdem;
using Mimic.Lobby;
using Mimic.Managers;
using UnityEngine;
namespace Mimic
{
    public sealed class MimicBootstrap : MonoBehaviour
    {
        private GaManager game;
        private string playerName = "Player";
        private GUIStyle title;
        private void Awake()
        {
            Application.runInBackground = true;
            Application.targetFrameRate = 60;
            game = gameObject.AddComponent<GaManager>();
#if DEVELOPMENT_BUILD || UNITY_EDITOR
            gameObject.AddComponent<Mimic.Tests.RuntimeSmokeBot>();
#endif
            if (Camera.main == null)
            {
                var camera = new GameObject("Camera").AddComponent<Camera>();
                camera.clearFlags = CameraClearFlags.SolidColor;
                camera.backgroundColor = new Color(0.025f, 0.06f, 0.085f);
            }
        }
        private void OnGUI()
        {
            if (game == null) return;
            if (title == null) title = new GUIStyle(GUI.skin.label) { fontSize = 38, fontStyle = FontStyle.Bold };
            float scale = Mathf.Min(Screen.width / 1000f, Screen.height / 650f);
            GUI.matrix = Matrix4x4.TRS(Vector3.zero, Quaternion.identity, new Vector3(scale, scale, 1));
            GUILayout.BeginArea(new Rect(40, 28, 920, 594), GUI.skin.box);
            GUILayout.Label("MIMIC", title);
            GUILayout.Label("TEXAS HOLD'EM  /  FRAMEWORK PREVIEW");
            GUILayout.Space(20);
            GUI.enabled = !game.Busy;
            if (game.Data.Phase == AppPhase.Title)
            {
                GUILayout.Label("Display name");
                playerName = GUILayout.TextField(playerName, 24, GUILayout.Width(300));
                if (GUILayout.Button("Connect", GUILayout.Width(300), GUILayout.Height(42))) game.Run(() => game.Login(playerName));
            }
            else
            {
                if (game.Data.Phase == AppPhase.Lobby) LobbyView.Draw(game);
                else HoldemView.Draw(game);
                GUILayout.Space(18);
                if (GUILayout.Button("Disconnect", GUILayout.Width(150))) game.Logout();
            }
            GUI.enabled = true;
            GUILayout.FlexibleSpace();
            GUILayout.Label(game.Busy ? "Working..." : game.Status);
            GUILayout.EndArea();
        }
    }
}

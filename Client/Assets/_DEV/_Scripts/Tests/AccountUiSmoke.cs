#if DEVELOPMENT_BUILD || UNITY_EDITOR
using System;
using System.IO;
using System.Linq;
using System.Threading.Tasks;
using Mimic.Data;
using Mimic.Managers;
using Mimic.Protocol;
using Mimic.Screens;
using UnityEngine;
using UnityEngine.SceneManagement;
namespace Mimic.Tests
{
    public sealed class AccountUiSmoke : MonoBehaviour
    {
        private string prefix, captureDirectory;
        private GaManager game;
        private static async Task Wait(Func<bool> condition, string description)
        {
            var deadline = DateTime.UtcNow.AddSeconds(45);
            while (!condition())
            {
                if (DateTime.UtcNow > deadline) throw new TimeoutException(description);
                await Task.Delay(40);
            }
            await Task.Delay(100);
        }
        private async Task Capture(string name)
        {
            if (string.IsNullOrEmpty(captureDirectory)) return;
            Directory.CreateDirectory(captureDirectory);
            var path = Path.Combine(captureDirectory, prefix + "-" + name + ".png");
            await Task.Delay(350);
            var camera = Camera.main;
            var canvas = FindFirstObjectByType<Canvas>();
            var previousMode = canvas.renderMode; var previousCamera = canvas.worldCamera;
            var previousTarget = camera.targetTexture; var previousActive = RenderTexture.active;
            var target = new RenderTexture(1600, 900, 24);
            var pixels = new Texture2D(1600, 900, TextureFormat.RGB24, false);
            try
            {
                camera.targetTexture = target;
                canvas.renderMode = RenderMode.ScreenSpaceCamera; canvas.worldCamera = camera; canvas.planeDistance = 1;
                Canvas.ForceUpdateCanvases(); camera.Render(); RenderTexture.active = target;
                pixels.ReadPixels(new Rect(0, 0, 1600, 900), 0, 0); pixels.Apply();
                File.WriteAllBytes(path, pixels.EncodeToPNG());
            }
            finally
            {
                RenderTexture.active = previousActive; camera.targetTexture = previousTarget;
                canvas.renderMode = previousMode; canvas.worldCamera = previousCamera;
                target.Release(); Destroy(target); Destroy(pixels); Canvas.ForceUpdateCanvases();
            }
        }
        private async void Start()
        {
            string[] args = Environment.GetCommandLineArgs();
            int at = Array.IndexOf(args, "-mimic-ui-smoke");
            if (at < 0 || at + 1 >= args.Length) { enabled = false; return; }
            prefix = args[at + 1];
            int captureAt = Array.IndexOf(args, "-mimic-capture-dir");
            if (captureAt >= 0 && captureAt + 1 < args.Length) captureDirectory = args[captureAt + 1];
            game = GetComponent<GaManager>();
            try
            {
                await Wait(() => FindFirstObjectByType<TitleScreen>() != null && !game.Busy, "Title scene");
                await Capture("01-title");
                FindFirstObjectByType<TitleScreen>().startButton.onClick.Invoke();
                await Wait(() => FindFirstObjectByType<LoginScreen>() != null && !game.Busy, "Login scene");
                var login = FindFirstObjectByType<LoginScreen>();
                login.account.text = ""; login.password.text = ""; login.submit.onClick.Invoke();
                if (string.IsNullOrEmpty(login.validation.text)) throw new Exception("Empty login validation missing");
                await Capture("02-login");
                login.registerTab.onClick.Invoke();
                string account = "ui_" + prefix.ToLowerInvariant() + "_" + Guid.NewGuid().ToString("N").Substring(0, 8);
                string password = "T_" + Guid.NewGuid().ToString("N");
                login.account.text = account; login.nickname.text = prefix;
                login.password.text = password; login.confirmPassword.text = "not_matching"; login.rememberAccount.isOn = false;
                login.submit.onClick.Invoke();
                if (string.IsNullOrEmpty(login.validation.text) || game.Busy) throw new Exception("Password confirmation validation missing");
                login.confirmPassword.text = password;
                await Capture("03-register");
                login.submit.onClick.Invoke();
                await Wait(() => FindFirstObjectByType<LobbyScreen>() != null && !game.Busy, "Register to lobby: " + game.Status);
                string player = game.Data.PlayerId;
                if (game.Data.AccountName != account || game.Data.DisplayName != prefix || string.IsNullOrEmpty(player)) throw new Exception("Wrong account in lobby");
                await Capture("04-lobby");
                FindFirstObjectByType<LobbyScreen>().logout.onClick.Invoke();
                await Wait(() => FindFirstObjectByType<LoginScreen>() != null && !game.Busy, "Logout to login");
                login = FindFirstObjectByType<LoginScreen>(); login.account.text = account; login.password.text = "incorrect_password"; login.submit.onClick.Invoke();
                await Wait(() => !game.Busy && game.StatusIsError, "Invalid login is rejected");
                login.password.text = password; login.submit.onClick.Invoke();
                await Wait(() => FindFirstObjectByType<LobbyScreen>() != null && !game.Busy, "Relogin to lobby");
                if (game.Data.PlayerId != player) throw new Exception("Persistent identity changed after relogin");
                var lobby = FindFirstObjectByType<LobbyScreen>(); lobby.refresh.onClick.Invoke();
                await Wait(() => !game.Busy && game.Data.Lobby != null, "Refresh lobby");
                lobby.quickStart.onClick.Invoke();
                await Wait(() => FindFirstObjectByType<HoldemScreen>() != null && game.Data.Table != null && !game.Busy, "Join table");
                FindFirstObjectByType<HoldemScreen>().ready.onClick.Invoke();
                await Wait(() => game.Data.Table?.Street == Street.Preflop && !game.Busy, "Both players ready");
                await Capture("05-holdem");
                ulong revision = ulong.MaxValue;
                var deadline = DateTime.UtcNow.AddSeconds(45);
                while (DateTime.UtcNow < deadline)
                {
                    var table = game.Data.Table ?? throw new Exception("Missing table snapshot");
                    if (table.Street == Street.Complete)
                    {
                        if (table.Board.Count != 5 || table.Pot != 0 || table.Players.Count != 2 || table.Players.Sum(p => p.Chips) != 2000) throw new Exception("Settlement check failed");
                        await Capture("06-showdown");
                        Debug.Log("MIMIC_ACCOUNT_UI_PASS " + prefix + " register/logout/login/lobby/table/showdown");
                        await Task.Delay(1800); Application.Quit(0); return;
                    }
                    var own = table.Players.Single(p => p.PlayerId == player);
                    if (own.HoleCards.Count != 2 || table.Players.Any(p => p.PlayerId != player && p.HoleCards.Count != 0)) throw new Exception("Private card leak");
                    if (!game.Busy && own.Seat == table.ActingSeat && revision != table.Revision)
                    {
                        revision = table.Revision;
                        var screen = FindFirstObjectByType<HoldemScreen>();
                        if (!screen.call.interactable) throw new Exception("Current player action disabled");
                        screen.call.onClick.Invoke();
                    }
                    await Task.Delay(40);
                }
                throw new TimeoutException("Holdem UI flow timed out: " + game.Status);
            }
            catch (Exception error)
            {
                Debug.LogError("MIMIC_ACCOUNT_UI_FAIL " + prefix + " " + error + " status=" + game.Status + " scene=" + SceneManager.GetActiveScene().name);
                Application.Quit(1);
            }
        }
    }
}
#endif

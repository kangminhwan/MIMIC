using System;
using System.Threading.Tasks;
using Mimic.Data;
using Mimic.Network;
using Mimic.Protocol;
using Mimic.Systems;
using UnityEngine;
using UnityEngine.SceneManagement;
namespace Mimic.Managers
{
    public sealed class GaManager : MonoBehaviour
    {
        public static GaManager Instance { get; private set; }
        public const string TitleScene = "0_Title", LoginScene = "0_Login", LobbyScene = "1_Lobby", TableScene = "2_Holdem";
        public SessionData Data { get; } = new SessionData();
        public NtManager Network { get; } = new NtManager();
        public bool Busy { get; private set; }
        public string Status { get; private set; } = "";
        public bool StatusIsError { get; private set; }
        public event Action Changed;
        private ClientConfig config;
        private bool subscribedToTable, changingScene;
        private string desiredScene;
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.BeforeSceneLoad)]
        private static void Initialize()
        {
            if (Instance == null) new GameObject("MIMIC • Application").AddComponent<GaManager>();
        }
        private void Awake()
        {
            if (Instance != null && Instance != this) { Destroy(gameObject); return; }
            Instance = this; DontDestroyOnLoad(gameObject); config = ClientConfig.Load();
            Application.runInBackground = true; Application.targetFrameRate = 60;
#if DEVELOPMENT_BUILD || UNITY_EDITOR
            gameObject.AddComponent<Mimic.Tests.RuntimeSmokeBot>();
            gameObject.AddComponent<Mimic.Tests.AccountUiSmoke>();
#endif
        }
        private void Update() { Network.Pump(); }
        private void OnDestroy() { if (Instance == this) { Network.Dispose(); Instance = null; } }
        public void SetStatus(string message, bool error = false) { Status = message; StatusIsError = error; Changed?.Invoke(); }
        public async void Run(Func<Task> operation)
        {
            if (Busy) return;
            Busy = true; SetStatus("");
            try { await operation(); }
            catch (Exception error) { SetStatus(FriendlyError(error), true); }
            finally { Busy = false; Changed?.Invoke(); }
        }
        private static string FriendlyError(Exception error)
        {
            if (error is ApiException api)
            {
                if (api.StatusCode == 409) return "이미 사용 중인 아이디입니다. 다른 아이디를 입력해 주세요.";
                if (api.StatusCode == 401) return "아이디 또는 비밀번호를 확인해 주세요.";
                if (api.StatusCode == 429) return "요청이 많습니다. 잠시 후 다시 시도해 주세요.";
                if (api.StatusCode >= 500 || api.StatusCode == 0) return "서버에 연결할 수 없습니다. 잠시 후 다시 시도해 주세요.";
            }
            return error.Message;
        }
        public async Task Navigate(string scene)
        {
            desiredScene = scene;
            if (changingScene) return;
            changingScene = true;
            try
            {
                while (SceneManager.GetActiveScene().name != desiredScene)
                {
                    var load = SceneManager.LoadSceneAsync(desiredScene);
                    while (!load.isDone) await Task.Yield();
                }
            }
            finally { changingScene = false; }
        }
        public async Task ShowLogin() { Data.Phase = AppPhase.Login; SetStatus(""); await Navigate(LoginScene); }
        public Task ShowTitle() { Data.Phase = AppPhase.Title; SetStatus(""); return Navigate(TitleScene); }
        public async Task Login(string name) { await CompleteLogin(() => Network.LoginAsync(config, name)); }
        public async Task LoginAccount(string account, string password) { await CompleteLogin(() => Network.LoginAccountAsync(config, account, password)); }
        public async Task Register(string account, string password, string nickname) { await CompleteLogin(() => Network.RegisterAsync(config, account, password, nickname)); }
        private async Task CompleteLogin(Func<Task<AuthenticateReply>> login)
        {
            subscribedToTable = false; Data.Clear();
            try
            {
                var session = await login();
                Data.PlayerId = session.PlayerId; Data.DisplayName = session.DisplayName;
                Data.AccountName = Network.Session.AccountName; Data.DemoChips = Network.Session.DemoChips;
                Network.Client.Received += Receive;
                Network.Client.Disconnected += Disconnected;
                await RefreshLobby(); Data.Phase = AppPhase.Lobby;
                await Navigate(LobbyScene); SetStatus("로비에 연결되었습니다.");
            }
            catch { Network.Dispose(); Data.Clear(); Data.Phase = AppPhase.Login; throw; }
        }
        private void Receive(Envelope message)
        {
            if (!subscribedToTable || message.Snapshot == null) return;
            Data.Table = message.Snapshot; Data.Phase = AppPhase.Table; Changed?.Invoke();
            if (SceneManager.GetActiveScene().name != TableScene) _ = NavigateSafely(TableScene);
        }
        private async Task NavigateSafely(string scene)
        { try { await Navigate(scene); } catch (Exception error) { SetStatus(error.Message, true); } }
        private void Disconnected(Exception error)
        {
            subscribedToTable = false; Data.Clear(); Data.Phase = AppPhase.Login;
            SetStatus("연결이 끊어졌습니다. 다시 로그인해 주세요.", true);
            _ = NavigateSafely(LoginScene);
        }
        public async Task RefreshLobby()
        {
            Data.Lobby = (await Network.RequestAsync(new Envelope { ListTables = new Empty() })).Lobby;
            Changed?.Invoke();
        }
        public async Task Join(uint tableId)
        {
            subscribedToTable = true;
            try { await Network.RequestAsync(new Envelope { JoinTable = new JoinTableRequest { TableId = tableId } }); SetStatus("모든 플레이어가 준비하면 시작합니다."); }
            catch { subscribedToTable = false; throw; }
        }
        public async Task Ready() { await Network.RequestAsync(new Envelope { Ready = new ReadyRequest { Ready = true } }); SetStatus("준비 완료! 다른 플레이어를 기다리고 있습니다."); }
        public async Task Leave()
        {
            await Network.RequestAsync(new Envelope { LeaveTable = new Empty() });
            subscribedToTable = false; Data.Table = null; Data.Phase = AppPhase.Lobby;
            await RefreshLobby(); await Navigate(LobbyScene); SetStatus("");
        }
        public Task Act(ActionKind kind, long raiseTo = 0)
        {
            var table = Data.Table ?? throw new InvalidOperationException("테이블에 입장해 주세요.");
            return Network.RequestAsync(new Envelope { Action = new ActionRequest { HandId = table.HandId, Revision = table.Revision, Kind = kind, RaiseTo = raiseTo } });
        }
        public async Task LogoutAsync()
        {
            subscribedToTable = false;
            try { await Network.LogoutAsync(); }
            finally { Data.Clear(); Data.Phase = AppPhase.Login; await Navigate(LoginScene); }
        }
        public void Logout() { Run(LogoutAsync); }
    }
}

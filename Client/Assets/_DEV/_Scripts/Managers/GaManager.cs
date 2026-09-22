using System;
using System.Threading.Tasks;
using Mimic.Data;
using Mimic.Network;
using Mimic.Protocol;
using Mimic.Systems;
using UnityEngine;
namespace Mimic.Managers
{
    public sealed class GaManager : MonoBehaviour
    {
        public SessionData Data { get; } = new SessionData();
        public NtManager Network { get; } = new NtManager();
        public bool Busy { get; private set; }
        public string Status { get; private set; } = "Start the local servers, then connect two clients.";
        private ClientConfig config;
        private bool subscribedToTable;
        private void Awake() { config = ClientConfig.Load(); }
        private void Update() { Network.Pump(); }
        private void OnDestroy() { Network.Dispose(); }
        public async void Run(Func<Task> operation)
        {
            if (Busy) return;
            Busy = true;
            try { await operation(); }
            catch (Exception error) { Status = error.Message; Debug.LogWarning(error.Message); }
            finally { Busy = false; }
        }
        public async Task Login(string name)
        {
            subscribedToTable = false; Data.Clear();
            var session = await Network.LoginAsync(config, name);
            Data.PlayerId = session.PlayerId; Data.DisplayName = session.DisplayName;
            Network.Client.Received += message => { if (subscribedToTable && message.Snapshot != null) { Data.Table = message.Snapshot; Data.Phase = AppPhase.Table; } };
            Network.Client.Disconnected += error => { subscribedToTable = false; Data.Clear(); Status = "Disconnected: " + error.Message; };
            Data.Phase = AppPhase.Lobby;
            await RefreshLobby(); Status = "Connected as " + session.DisplayName;
        }
        public async Task RefreshLobby() { Data.Lobby = (await Network.RequestAsync(new Envelope { ListTables = new Empty() })).Lobby; }
        public async Task Join(uint tableId)
        {
            subscribedToTable = true;
            try { await Network.RequestAsync(new Envelope { JoinTable = new JoinTableRequest { TableId = tableId } }); Status = "Waiting for players to ready up."; }
            catch { subscribedToTable = false; throw; }
        }
        public async Task Ready() { await Network.RequestAsync(new Envelope { Ready = new ReadyRequest { Ready = true } }); Status = "Ready."; }
        public async Task Leave()
        {
            await Network.RequestAsync(new Envelope { LeaveTable = new Empty() });
            subscribedToTable = false; Data.Table = null; Data.Phase = AppPhase.Lobby; await RefreshLobby();
        }
        public Task Act(ActionKind kind, long raiseTo = 0)
        {
            var table = Data.Table ?? throw new InvalidOperationException("No table");
            return Network.RequestAsync(new Envelope { Action = new ActionRequest { HandId = table.HandId, Revision = table.Revision, Kind = kind, RaiseTo = raiseTo } });
        }
        public void Logout() { subscribedToTable = false; Network.Dispose(); Data.Clear(); Status = "Disconnected."; }
    }
}

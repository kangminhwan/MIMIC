using System;
using System.Threading;
using System.Threading.Tasks;
using Mimic.Protocol;
using Mimic.Systems;
namespace Mimic.Network
{
    public sealed class NtManager : IDisposable
    {
        public NetClient Client { get; private set; }
        public TableServerClient Native { get; private set; }
        public HoldemTableState TableState { get; private set; }
        public bool IsConnected => Native?.Transport.IsConnected ?? Client?.IsConnected ?? false;
        public event Action<Envelope> Received;
        public event Action<Exception> Disconnected;
        public event Action TableLeft;
        private readonly System.Collections.Generic.Dictionary<uint, General.RoomListEntry> nativeRooms = new System.Collections.Generic.Dictionary<uint, General.RoomListEntry>();
        private string nativeChannel;
        private General.BetPolicy nativeBetPolicy;
        public PortalReply Portal { get; private set; }
        public LoginReply Session { get; private set; }
        private CancellationTokenSource operation = new CancellationTokenSource();
        public async Task<PortalReply> DiscoverAsync(ClientConfig config)
        {
            var portal = await ProtoHttp.SendAsync(config.frontUrl.TrimEnd('/') + "/portal", PortalReply.Parser, cancellation: operation.Token);
            if (portal.ProtocolVersion != 1) throw new InvalidOperationException("클라이언트 업데이트가 필요합니다.");
            Portal = portal; return portal;
        }
        public async Task<AuthenticateReply> LoginAsync(ClientConfig config, string displayName)
        {
            if (config.nativeTableServer) throw new InvalidOperationException("Use an existing TableServer account.");
            Reset();
            await DiscoverAsync(config);
            var login = await ProtoHttp.SendAsync(Portal.PlatformUrl.TrimEnd('/') + "/auth/guest", LoginReply.Parser,
                new LoginRequest { DisplayName = displayName }, cancellation: operation.Token);
            return await ConnectSession(login);
        }
        public async Task<AuthenticateReply> LoginAccountAsync(ClientConfig config, string account, string password)
        {
            if (config.nativeTableServer) return await LoginNative(config, account, password);
            Reset(); await DiscoverAsync(config);
            var login = await ProtoHttp.SendAsync(Portal.PlatformUrl.TrimEnd('/') + "/auth/login", LoginReply.Parser,
                new AccountLoginRequest { AccountName = account, Password = password }, cancellation: operation.Token);
            return await ConnectSession(login);
        }
        public async Task<AuthenticateReply> RegisterAsync(ClientConfig config, string account, string password, string displayName)
        {
            if (config.nativeTableServer) throw new InvalidOperationException("Account registration requires the original account service.");
            Reset(); await DiscoverAsync(config);
            var login = await ProtoHttp.SendAsync(Portal.PlatformUrl.TrimEnd('/') + "/auth/register", LoginReply.Parser,
                new RegisterRequest { AccountName = account, Password = password, DisplayName = displayName }, cancellation: operation.Token);
            return await ConnectSession(login);
        }
        private async Task<AuthenticateReply> LoginNative(ClientConfig config, string account, string password)
        {
            Reset();
            nativeChannel = config.holdemChannel;
            nativeBetPolicy = (General.BetPolicy)config.holdemBetPolicy;
            Native = new TableServerClient();
            TableState = new HoldemTableState();
            Native.Received += TableState.Receive;
            Native.Disconnected += reason => Disconnected?.Invoke(new InvalidOperationException(reason.ToString()));
            TableState.Changed += snapshot =>
            {
                var own = System.Linq.Enumerable.FirstOrDefault(snapshot.Players, p => p.PlayerId == Session?.PlayerId);
                if (own != null && TableState.Asset == General.AssetKind.Chip)
                {
                    Session.DemoChips = own.Chips;
                    Native.Session.MemberInfo.WalletChips = checked((ulong)own.Chips);
                }
                Received?.Invoke(new Envelope { Snapshot = snapshot });
            };
            TableState.Left += () => TableLeft?.Invoke();
            try
            {
                var result = await Native.LoginAsync(config.tableHost, config.tablePort,
                    new PmNet.SigninRQ { NativeId = account, NativeKey = password, AppVer = config.appVersion,
                        StoreKind = (General.StoreChannel)config.storeChannel, ClientLocale = "ko-KR" }, operation.Token);
                Session = new LoginReply { PlayerId = result.MemberInfo.MemberId.ToString(), DisplayName = result.MemberInfo.DisplayName,
                    AccountName = account, DemoChips = checked((long)result.MemberInfo.WalletChips) };
                return new AuthenticateReply { PlayerId = Session.PlayerId, DisplayName = Session.DisplayName };
            }
            catch { Reset(); throw; }
        }

        private async Task<Envelope> RequestNative(Envelope message)
        {
            switch (message.PayloadCase)
            {
                case Envelope.PayloadOneofCase.ListTables:
                    var list = await Native.ListAsync(new[] { nativeChannel }, ct: operation.Token);
                    nativeRooms.Clear(); var lobby = new LobbyReply(); uint key = 0;
                    foreach (var room in list.ChamberList)
                    {
                        nativeRooms.Add(++key, room);
                        lobby.Tables.Add(new TableInfo { TableId = key, Name = string.IsNullOrEmpty(room.RoomName) ? room.ChannelCode + " / " + room.RoomNo : room.RoomName,
                            Players = (uint)room.SeatedCount, Capacity = (uint)room.SeatLimit,
                            SmallBlind = checked((long)room.SeedAmount), BigBlind = checked((long)room.SeedAmount * 2) });
                    }
                    return new Envelope { Lobby = lobby };
                case Envelope.PayloadOneofCase.JoinTable:
                    if (!nativeRooms.TryGetValue(message.JoinTable.TableId, out var target)) throw new InvalidOperationException("Refresh the room list first.");
                    await Native.JoinAsync(target, operation.Token); break;
                case Envelope.PayloadOneofCase.LeaveTable:
                    await Native.LeaveAsync(checked((int)(TableState.Snapshot?.TableId ?? 0)), operation.Token); break;
                case Envelope.PayloadOneofCase.Ready:
                    if (TableState.Snapshot?.Street == Street.Complete)
                    {
                        if (!Native.CompleteOutcome()) throw new System.IO.IOException("TableServer is disconnected.");
                        break;
                    }
                    if (!TableState.CanStart) throw new InvalidOperationException("The lead player can start with at least two players.");
                    await Native.StartAsync(operation.Token); break;
                case Envelope.PayloadOneofCase.Action:
                    var action = message.Action.Kind == ActionKind.Fold ? General.TableAction.GiveUp :
                        message.Action.Kind == ActionKind.Check ? General.TableAction.Check :
                        message.Action.Kind == ActionKind.Call ? General.TableAction.Call : General.TableAction.None;
                    await BetNative(action); break;
                default: throw new NotSupportedException("Unsupported TableServer request.");
            }
            return new Envelope { Acknowledged = new Empty() };
        }

        public async Task CreateTableAsync()
        {
            if (Native == null) throw new InvalidOperationException("TableServer is not connected.");
            await Native.CreateAsync(nativeChannel, nativeBetPolicy, ct: operation.Token);
        }

        public async Task BetNative(General.TableAction action)
        {
            if (Native == null || !System.Linq.Enumerable.Contains(TableState.Actions, action))
                throw new InvalidOperationException("This action is not available on the current turn.");
            await Native.BetAsync(action, operation.Token);
        }

        private async Task<AuthenticateReply> ConnectSession(LoginReply session)
        {
            Session = session;
            Client = new NetClient();
            Client.Received += message => Received?.Invoke(message);
            Client.Disconnected += error => Disconnected?.Invoke(error);
            try
            {
                await Client.ConnectAsync(Portal.TableHost, (int)Portal.TablePort);
                var result = await Client.RequestAsync(new Envelope { Authenticate = new AuthenticateRequest { SessionToken = session.SessionToken } });
                return result.Authenticated ?? throw new InvalidOperationException("인증 응답이 올바르지 않습니다.");
            }
            catch { Client.Dispose(); Client = null; throw; }
        }
        public Task<ProfileReply> ProfileAsync() => Native != null ? Task.FromResult(new ProfileReply {
            PlayerId = Session.PlayerId, AccountName = Session.AccountName, DisplayName = Session.DisplayName,
            DemoChips = checked((long)Native.Session.MemberInfo.WalletChips)
        }) : ProtoHttp.SendAsync(Portal.PlatformUrl.TrimEnd('/') + "/account/profile", ProfileReply.Parser,
            bearerToken: Session.SessionToken, cancellation: operation.Token);
        public async Task LogoutAsync()
        {
            if (Native != null) { await Native.DisposeAsync(); Native = null; Reset(); return; }
            var portal = Portal; var token = Session?.SessionToken;
            Client?.Dispose(); Client = null;
            try
            {
                if (portal != null && !string.IsNullOrEmpty(token))
                    await ProtoHttp.SendAsync(portal.PlatformUrl.TrimEnd('/') + "/auth/logout", Empty.Parser, new Empty(), token, "POST", operation.Token);
            }
            finally { Reset(); }
        }
        public Task<Envelope> RequestAsync(Envelope message) => Native != null ? RequestNative(message) : Client != null ? Client.RequestAsync(message) : throw new InvalidOperationException("먼저 로그인해 주세요.");
        public void Pump() { Native?.Pump(); Client?.Pump(); }
        private void Reset()
        {
            operation.Cancel(); operation.Dispose(); operation = new CancellationTokenSource();
            if (Native != null) { _ = Native.DisposeAsync(); Native = null; }
            TableState = null; nativeRooms.Clear(); Portal = null;
            Client?.Dispose(); Client = null; Session = null;
        }
        public void Dispose() { Reset(); }
    }
}

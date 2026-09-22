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
            Reset();
            await DiscoverAsync(config);
            var login = await ProtoHttp.SendAsync(Portal.PlatformUrl.TrimEnd('/') + "/auth/guest", LoginReply.Parser,
                new LoginRequest { DisplayName = displayName }, cancellation: operation.Token);
            return await ConnectSession(login);
        }
        public async Task<AuthenticateReply> LoginAccountAsync(ClientConfig config, string account, string password)
        {
            Reset(); await DiscoverAsync(config);
            var login = await ProtoHttp.SendAsync(Portal.PlatformUrl.TrimEnd('/') + "/auth/login", LoginReply.Parser,
                new AccountLoginRequest { AccountName = account, Password = password }, cancellation: operation.Token);
            return await ConnectSession(login);
        }
        public async Task<AuthenticateReply> RegisterAsync(ClientConfig config, string account, string password, string displayName)
        {
            Reset(); await DiscoverAsync(config);
            var login = await ProtoHttp.SendAsync(Portal.PlatformUrl.TrimEnd('/') + "/auth/register", LoginReply.Parser,
                new RegisterRequest { AccountName = account, Password = password, DisplayName = displayName }, cancellation: operation.Token);
            return await ConnectSession(login);
        }
        private async Task<AuthenticateReply> ConnectSession(LoginReply session)
        {
            Session = session;
            Client = new NetClient();
            try
            {
                await Client.ConnectAsync(Portal.TableHost, (int)Portal.TablePort);
                var result = await Client.RequestAsync(new Envelope { Authenticate = new AuthenticateRequest { SessionToken = session.SessionToken } });
                return result.Authenticated ?? throw new InvalidOperationException("인증 응답이 올바르지 않습니다.");
            }
            catch { Client.Dispose(); Client = null; throw; }
        }
        public Task<ProfileReply> ProfileAsync() => ProtoHttp.SendAsync(Portal.PlatformUrl.TrimEnd('/') + "/account/profile", ProfileReply.Parser,
            bearerToken: Session.SessionToken, cancellation: operation.Token);
        public async Task LogoutAsync()
        {
            var portal = Portal; var token = Session?.SessionToken;
            Client?.Dispose(); Client = null;
            try
            {
                if (portal != null && !string.IsNullOrEmpty(token))
                    await ProtoHttp.SendAsync(portal.PlatformUrl.TrimEnd('/') + "/auth/logout", Empty.Parser, new Empty(), token, "POST", operation.Token);
            }
            finally { Reset(); }
        }
        public Task<Envelope> RequestAsync(Envelope message) => Client != null ? Client.RequestAsync(message) : throw new InvalidOperationException("먼저 로그인해 주세요.");
        public void Pump() => Client?.Pump();
        private void Reset()
        {
            operation.Cancel(); operation.Dispose(); operation = new CancellationTokenSource();
            Client?.Dispose(); Client = null; Session = null;
        }
        public void Dispose() { operation.Cancel(); Client?.Dispose(); Client = null; Session = null; }
    }
}

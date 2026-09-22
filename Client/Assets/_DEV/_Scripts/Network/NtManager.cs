using System;
using System.Threading.Tasks;
using Mimic.Protocol;
using Mimic.Systems;
namespace Mimic.Network
{
    public sealed class NtManager : IDisposable
    {
        public NetClient Client { get; private set; }
        public async Task<AuthenticateReply> LoginAsync(ClientConfig config, string displayName)
        {
            Dispose();
            var portal = await ProtoHttp.SendAsync(config.frontUrl.TrimEnd('/') + "/portal", PortalReply.Parser);
            if (portal.ProtocolVersion != 1) throw new InvalidOperationException("Unsupported portal protocol");
            var login = await ProtoHttp.SendAsync(portal.PlatformUrl.TrimEnd('/') + "/auth/guest", LoginReply.Parser,
                new LoginRequest { DisplayName = displayName });
            Client = new NetClient();
            try
            {
                await Client.ConnectAsync(portal.TableHost, (int)portal.TablePort);
                var result = await Client.RequestAsync(new Envelope { Authenticate = new AuthenticateRequest { SessionToken = login.SessionToken } });
                return result.Authenticated ?? throw new InvalidOperationException("Missing authentication response");
            }
            catch { Dispose(); throw; }
        }
        public Task<Envelope> RequestAsync(Envelope message) => Client.RequestAsync(message);
        public void Pump() => Client?.Pump();
        public void Dispose() { Client?.Dispose(); Client = null; }
    }
}

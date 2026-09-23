using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using Ayve.Net.Core;
using General;
using Google.Protobuf;
using PmNet;

namespace Mimic.Network
{
    // Uses the original casino transport with raw protobuf requests and PktBase replies.
    public sealed class TableServerClient : IAsyncDisposable
    {
        public Ayve.Net.Core.NetClient Transport { get; }
        public SigninRS Session { get; private set; }
        public event Action<PacketID, PktBase> Received;
        public event Action<FatalReason> Disconnected;
        private readonly SemaphoreSlim requests = new SemaphoreSlim(1, 1);
        private bool disposed;
        private SigninRQ sessionLogin;
        private static readonly PacketID[] alternatives = {
            PacketID.PacketLobbyRedirect, PacketID.PacketServiceNotice, PacketID.PacketClientRevisionNotice
        };

        public TableServerClient(NetClientOptions options = null)
        {
            Transport = new Ayve.Net.Core.NetClient(options);
            Transport.PacketReceived += OnReceived;
            Transport.Fatal += OnFatal;
        }

        public void Pump() => Transport.Pump();
        private void OnReceived(PacketID id, PktBase packet) => Received?.Invoke(id, packet);
        private void OnFatal(FatalReason reason) => Disconnected?.Invoke(reason);

        public async Task<SigninRS> LoginAsync(string host, int port, SigninRQ request, CancellationToken ct = default)
        {
            if (!await Transport.ConnectAsync(host, port, ct)) throw new IOException("TableServer connection failed.");
            sessionLogin = request.Clone();
            sessionLogin.NativeId = ""; sessionLogin.NativeKey = ""; sessionLogin.AuxKey = "";
            for (int attempt = 0; attempt < 3; attempt++)
            {
                Session = await RequestAsync(PacketID.PacketAccessOpen, request, SigninRS.Parser, ct);
                if (Session.ShiftAtriumRes == null)
                {
                    ValidateSession();
                    sessionLogin.ProfileUid = Session.ProfileUid; sessionLogin.OutletUid = Session.OutletUid;
                    return Session;
                }
                await ConnectRedirect(Session.ShiftAtriumRes, ct);
                request = sessionLogin.Clone();
            }
            throw new IOException("TableServer login redirect limit exceeded.");
        }

        public Task<ChamberIndexRS> ListAsync(IEnumerable<string> channels, int start = 0, int count = 50, CancellationToken ct = default)
        {
            var request = new ChamberIndexRQ { MatchKind = PlayCategory.TexasHoldem, StartPos = start, PageSz = count };
            request.ChTokens.Add(channels);
            return RequestAsync(PacketID.PacketSpaceList, request, ChamberIndexRS.Parser, ct);
        }

        public Task<ChamberEnterRS> JoinAsync(RoomListEntry room, CancellationToken ct = default) =>
            RequestAsync(PacketID.PacketSpaceEnter, new ChamberEnterRQ {
                NodeId = room.ServerNodeId, ChamberNo = room.RoomNo, ChToken = room.ChannelCode,
                WagerRule = room.BetPolicy, RevealDelayPerMember = 400, RevealSharedDelay = 500
            }, ChamberEnterRS.Parser, ct);

        public Task<ChamberBuildRS> CreateAsync(string channel, BetPolicy policy = BetPolicy.HoldemStandard, int seats = 9, CancellationToken ct = default) =>
            RequestAsync(PacketID.PacketSpaceCreate, new ChamberBuildRQ {
                ChToken = channel, SeatCap = seats, WagerRule = policy, MatchRule = RuleProfile.Standard,
                RevealDelayPerMember = 400, RevealSharedDelay = 500
            }, ChamberBuildRS.Parser, ct);

        public Task<ChamberLeaveRS> LeaveAsync(int room, CancellationToken ct = default) =>
            RequestAsync(PacketID.PacketSpaceLeave, new ChamberLeaveRQ { ChamberNo = room }, ChamberLeaveRS.Parser, ct);

        public Task<MatchKickoffRS> StartAsync(CancellationToken ct = default) =>
            RequestAsync(PacketID.PacketRoundOpen, new MatchKickoffRQ(), MatchKickoffRS.Parser, ct);

        public Task<MatchWagerRS> BetAsync(TableAction action, CancellationToken ct = default) =>
            RequestAsync(PacketID.PacketStakeSubmit, new MatchWagerRQ { WagerKind = action }, MatchWagerRS.Parser, ct);

        public bool CompleteOutcome() => Transport.Post(PacketID.PacketResultViewDone, new OutcomeCompleteRQ());

        public async Task<T> RequestAsync<T>(PacketID id, IMessage request, MessageParser<T> parser, CancellationToken ct = default)
            where T : IMessage<T>
        {
            if (disposed) throw new ObjectDisposedException(nameof(TableServerClient));
            // The native server does not echo a nonce. Keep requests ordered on this connection.
            await requests.WaitAsync(ct);
            try
            {
                if (disposed) throw new ObjectDisposedException(nameof(TableServerClient));
                for (int attempt = 0; attempt < 3; attempt++)
                {
                    var reply = await Exchange(id, request, ct);
                    if ((PacketID)reply.Key != PacketID.PacketLobbyRedirect)
                        return parser.ParseFrom(reply.Response.Payload);
                    if (id == PacketID.PacketAccessOpen || sessionLogin == null)
                        throw new IOException("Unexpected TableServer login redirect.");
                    await ConnectRedirect(ShiftAtriumRS.Parser.ParseFrom(reply.Response.Payload), ct);
                    var login = await Exchange(PacketID.PacketAccessOpen, sessionLogin, ct);
                    if ((PacketID)login.Key != PacketID.PacketAccessOpen) throw new IOException("Repeated TableServer redirect.");
                    Session = SigninRS.Parser.ParseFrom(login.Response.Payload);
                    ValidateSession();
                }
                throw new IOException("TableServer request redirect limit exceeded.");
            }
            catch (TimeoutException)
            {
                // A late reply cannot safely be correlated with a subsequent request of the same kind.
                await Transport.CloseAsync();
                if (!disposed) Transport.RaiseFatalExternal(FatalReason.NetworkError);
                throw;
            }
            catch (OperationCanceledException)
            {
                await Transport.CloseAsync();
                throw;
            }
            finally { requests.Release(); }
        }

        private async Task<PendingRequestTable<PktBase>.Completion> Exchange(PacketID id, IMessage request, CancellationToken ct)
        {
            var reply = await Transport.RequestWithIdAsync(id, request.ToByteArray(), alternatives, ct);
            var packet = reply.Response;
            if (packet == null) throw new IOException("TableServer disconnected before replying.");
            if (packet.ErrKind != ResultCode.ResultSuccess) throw new TableServerException(packet.ErrKind);
            if ((PacketID)reply.Key == PacketID.PacketServiceNotice || (PacketID)reply.Key == PacketID.PacketClientRevisionNotice)
                throw new IOException("TableServer is unavailable or requires a client update.");
            return reply;
        }

        private async Task ConnectRedirect(ShiftAtriumRS redirect, CancellationToken ct)
        {
            if (string.IsNullOrWhiteSpace(redirect.AtriumNodeIp) || redirect.Endpoint < 1 || redirect.Endpoint > 65535)
                throw new InvalidDataException("Invalid TableServer redirect endpoint.");
            if (!string.IsNullOrEmpty(redirect.ProfileUid)) sessionLogin.ProfileUid = redirect.ProfileUid;
            if (!string.IsNullOrEmpty(redirect.OutletUid)) sessionLogin.OutletUid = redirect.OutletUid;
            if (string.IsNullOrEmpty(sessionLogin.ProfileUid) || string.IsNullOrEmpty(sessionLogin.OutletUid))
                throw new InvalidDataException("TableServer redirect is missing session credentials.");
            if (!await Transport.ConnectAsync(redirect.AtriumNodeIp, redirect.Endpoint, ct))
                throw new IOException("Could not connect to the target TableServer.");
        }

        private void ValidateSession()
        {
            if (Session.MemberInfo == null || Session.MemberInfo.MemberId == 0 || Session.ShiftAtriumRes != null)
                throw new InvalidDataException("TableServer did not return a player profile.");
        }

        public async ValueTask DisposeAsync()
        {
            if (disposed) return;
            disposed = true;
            Transport.PacketReceived -= OnReceived;
            Transport.Fatal -= OnFatal;
            await Transport.DisposeAsync();
        }
    }

    public sealed class TableServerException : Exception
    {
        public ResultCode Code { get; }
        public TableServerException(ResultCode code) : base("TableServer: " + code) { Code = code; }
    }
}

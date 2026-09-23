using System.Net;
using System.Net.Sockets;
using Ayve.Net.Core;
using Ayve.Net.Wire;
using General;
using Google.Protobuf;
using Mimic.Network;
using PmNet;

static class Tests
{
    static void Check(bool condition, string message) { if (!condition) throw new Exception(message); }
    static byte[] Frame(PacketID command, IMessage payload, ResultCode code = ResultCode.ResultSuccess, uint? size = null)
    {
        var body = payload.ToByteString();
        var wrapped = new PktBase { Payload = body, PayloadSize = size ?? (uint)body.Length, ErrKind = code }.ToByteArray();
        var frame = new byte[PacketHeader.Size + wrapped.Length];
        new PacketHeader((uint)command, (uint)wrapped.Length).Write(frame);
        wrapped.CopyTo(frame, PacketHeader.Size);
        new XorCipher().Encrypt(frame.AsSpan(PacketHeader.Size));
        return frame;
    }
    static async Task<(PacketID, byte[])> Read(NetworkStream stream)
    {
        var header = new byte[24]; await stream.ReadExactlyAsync(header);
        Check(PacketHeader.TryRead(header, 4 * 1024 * 1024, out var h, out _), "Invalid outbound header");
        var body = new byte[h.PayloadSize]; await stream.ReadExactlyAsync(body);
        new XorCipher().Decrypt(body);
        return ((PacketID)h.Command, body);
    }
    static async Task Pump(TableServerClient client, Task operation)
    {
        var deadline = DateTime.UtcNow.AddSeconds(10);
        while (!operation.IsCompleted && DateTime.UtcNow < deadline) { client.Pump(); await Task.Delay(2); }
        Check(operation.IsCompleted, "Operation hung"); await operation;
    }
    static TcpListener Listen() { var listener = new TcpListener(IPAddress.Loopback, 0); listener.Start(); return listener; }
    static TableServerClient Client(int timeout = 1500) => new(new NetClientOptions { RequestTimeoutMs = timeout, DeadLinkMs = 20000 });
    static int Port(TcpListener listener) => ((IPEndPoint)listener.LocalEndpoint).Port;
    static ParticipantProfile Player(ulong id) => new() { MemberId = id, DisplayName = "Player " + id, WalletChips = 1000 };

    public static async Task Main()
    {
        using (var listener = Listen())
        await using (var client = Client())
        {
            var state = new HoldemTableState(); client.Received += state.Receive;
            int snapshots = 0; state.Changed += _ => snapshots++;
            var server = Task.Run(async () => {
                using var socket = await listener.AcceptTcpClientAsync(); var stream = socket.GetStream();
                var (id, bytes) = await Read(stream);
                var login = SigninRQ.Parser.ParseFrom(bytes);
                Check(id == PacketID.PacketAccessOpen && login.NativeId == "test1" && login.NativeKey == "pass1234", "Login must be raw protobuf");
                var frame = Frame(id, new SigninRS { MemberInfo = Player(7) });
                foreach (var b in frame) await stream.WriteAsync(new[] { b });
                (id, bytes) = await Read(stream);
                var list = ChamberIndexRQ.Parser.ParseFrom(bytes);
                Check(id == PacketID.PacketSpaceList && list.MatchKind == PlayCategory.TexasHoldem && list.ChTokens[0] == "Holdem_Chip_10K", "Wrong lobby request");
                await stream.WriteAsync(Frame(id, new ChamberIndexRS()));
                (id, bytes) = await Read(stream);
                Check(ChamberBuildRQ.Parser.ParseFrom(bytes).SeatCap == 9, "Expected nine seats");
                var created = new ChamberBuildRS { ChamberInfo = new RoomSnapshot { RoomNo = 12, PlayCategory = PlayCategory.TexasHoldem, AssetKind = AssetKind.Chip }, CaptainIdx = 8, LeadIdx = 7 };
                created.Members.Add(Player(7)); created.Members.Add(new ParticipantProfile()); created.Members.Add(Player(8));
                await stream.WriteAsync(Frame(id, created));
                (id, bytes) = await Read(stream);
                Check(id == PacketID.PacketRoundOpen && bytes.Length == 0, "Empty kickoff must remain valid protobuf");
                await stream.WriteAsync(Frame(id, new MatchKickoffRS()));
                await stream.WriteAsync(Frame(PacketID.PacketRoundStateNotice, new MatchStateSwapRS { MatchPhase = 2 }));
                var deal = new SeedDealoutRS();
                var cards = new CardSet(); cards.PlayingCards.Add(new PlayingCard { RankCode = CardRank.Ace, SuitCode = CardSuit.Heart });
                deal.MemberCards.Add(7, cards); await stream.WriteAsync(Frame(PacketID.PacketInitialDeal, deal));
                var turn = new PhaseTurnRS { MemberIdx = 7, MatchPhase = 3, MaxWager = 10 };
                turn.WagerOptions.Add(TableAction.Call); turn.WagerOptions.Add(TableAction.GiveUp);
                await stream.WriteAsync(Frame(PacketID.PacketTurnNotice, turn));
                (id, bytes) = await Read(stream);
                Check(id == PacketID.PacketStakeSubmit && MatchWagerRQ.Parser.ParseFrom(bytes).WagerKind == TableAction.Call, "Wrong wager codec");
                await stream.WriteAsync(Frame(id, new MatchWagerRS { WagerMemberIdx = 7, MemberWager = TableAction.Call, FundBefore = 1000, FundAfter = 990 }));
                (id, bytes) = await Read(stream);
                Check(id == PacketID.PacketSpaceLeave && ChamberLeaveRQ.Parser.ParseFrom(bytes).ChamberNo == 12, "Wrong leave request");
                await stream.WriteAsync(Frame(id, new ChamberLeaveRS { MemberIdx = 7, ChamberNo = 12 }));
                await Task.Delay(50);
            });
            var scenario = Scenario();
            async Task Scenario()
            {
                await client.LoginAsync("127.0.0.1", Port(listener), new SigninRQ { NativeId = "test1", NativeKey = "pass1234" });
                Check(state.MemberId == 7, "State must update before login continuation");
                await client.ListAsync(new[] { "Holdem_Chip_10K" });
                await client.CreateAsync("Holdem_Chip_10K");
                Check(state.Snapshot.Players[1].Seat == 2 && state.CanStart, "Seat gaps or captain were lost");
                await client.StartAsync();
                while (state.Actions.Count == 0) await Task.Delay(2);
                Check(state.Snapshot.Players[0].HoleCards[0].Rank == 14 && state.Snapshot.Players[0].HoleCards[0].Suit == 2, "Card mapping");
                await client.BetAsync(TableAction.Call);
                Check(state.Snapshot.Pot == 10 && state.Snapshot.Players[0].Chips == 990 && state.Actions.Count == 0, "Wager reducer or response ordering");
                await client.LeaveAsync(12); Check(state.Snapshot == null, "Own leave must clear room");
            }
            await Pump(client, Task.WhenAll(server, scenario));
            Check(snapshots >= 5, "Missing state notifications");
        }
        Console.WriteLine("PASS fragmented frames, raw protobuf requests, empty replies, ordered Holdem state and nine-seat mapping");

        using (var listener = Listen())
        await using (var client = Client())
        {
            var fatal = new TaskCompletionSource<FatalReason>(); client.Disconnected += reason => fatal.TrySetResult(reason);
            var server = Task.Run(async () => {
                using var socket = await listener.AcceptTcpClientAsync(); var stream = socket.GetStream();
                await stream.WriteAsync(Frame(PacketID.PacketSpaceList, new ChamberIndexRS(), size: 99));
                await Task.Delay(50);
            });
            Check(await client.Transport.ConnectAsync("127.0.0.1", Port(listener)), "Connect");
            await Pump(client, Task.WhenAll(server, fatal.Task)); Check(fatal.Task.Result == FatalReason.ProtocolError, "Malformed wrapper must be fatal");
        }
        Console.WriteLine("PASS malformed protobuf wrapper closes the session");

        using (var listener = Listen())
        await using (var client = Client(100))
        {
            var accepted = listener.AcceptTcpClientAsync();
            Check(await client.Transport.ConnectAsync("127.0.0.1", Port(listener)), "Connect");
            using var socket = await accepted;
            var pending = client.ListAsync(new[] { "Holdem_Chip_10K" });
            try { await Pump(client, pending); throw new Exception("Timeout was not reported"); }
            catch (TimeoutException) { }
            Check(!client.Transport.IsConnected, "Timeout must retire uncorrelated connection");
        }
        Console.WriteLine("PASS timed-out request retires connection");

        using (var listener = Listen())
        {
            var client = Client(); var accepted = listener.AcceptTcpClientAsync();
            Check(await client.Transport.ConnectAsync("127.0.0.1", Port(listener)), "Connect");
            using var socket = await accepted;
            var pending = client.ListAsync(new[] { "Holdem_Chip_10K" });
            await client.DisposeAsync();
            try { await pending.WaitAsync(TimeSpan.FromSeconds(2)); throw new Exception("Pending request survived disposal"); }
            catch (IOException) { }
        }
        Console.WriteLine("PASS disposal releases pending requests");

        using (var source = Listen())
        using (var destination = Listen())
        await using (var client = Client())
        {
            var from = Task.Run(async () => {
                using var socket = await source.AcceptTcpClientAsync(); var stream = socket.GetStream();
                await Read(stream);
                await stream.WriteAsync(Frame(PacketID.PacketAccessOpen, new SigninRS { MemberInfo = Player(7), ProfileUid = "profile", OutletUid = "outlet" }));
                await Read(stream);
                await stream.WriteAsync(Frame(PacketID.PacketLobbyRedirect, new ShiftAtriumRS { AtriumNodeIp = "127.0.0.1", Endpoint = Port(destination) }));
                await Task.Delay(100);
            });
            var to = Task.Run(async () => {
                using var socket = await destination.AcceptTcpClientAsync(); var stream = socket.GetStream();
                var (id, bytes) = await Read(stream); var login = SigninRQ.Parser.ParseFrom(bytes);
                Check(id == PacketID.PacketAccessOpen && login.NativeKey == "" && login.NativeId == "" && login.ProfileUid == "profile" && login.OutletUid == "outlet", "Redirect must use session identity without forwarding the password");
                await stream.WriteAsync(Frame(id, new SigninRS { MemberInfo = Player(7), ProfileUid = "profile", OutletUid = "outlet" }));
                (id, bytes) = await Read(stream);
                Check(id == PacketID.PacketSpaceCreate && ChamberBuildRQ.Parser.ParseFrom(bytes).ChToken == "Holdem_Chip_10K", "Redirect did not retry the original room request");
                await stream.WriteAsync(Frame(id, new ChamberBuildRS { ChamberNo = 42 }));
                await Task.Delay(50);
            });
            var scenario = Run();
            async Task Run() {
                await client.LoginAsync("127.0.0.1", Port(source), new SigninRQ { NativeId = "test1", NativeKey = "pass1234" });
                Check((await client.CreateAsync("Holdem_Chip_10K")).ChamberNo == 42, "Redirect reply");
            }
            await Pump(client, Task.WhenAll(from, to, scenario));
        }
        Console.WriteLine("PASS room redirect, session reauthentication and request retry");

        using (var listener = Listen())
        await using (var client = Client())
        {
            var fatal = new TaskCompletionSource<FatalReason>(); client.Disconnected += reason => fatal.TrySetResult(reason);
            var peer = Task.Run(async () => {
                using var socket = await listener.AcceptTcpClientAsync(); var stream = socket.GetStream();
                await stream.WriteAsync(Frame(PacketID.PacketLinkProbe, new HeartbeatRQ { HbVal = 777 }));
                var (id, bytes) = await Read(stream);
                Check(id == PacketID.PacketLinkProbe && HeartbeatRS.Parser.ParseFrom(bytes).HbVal == 777, "Heartbeat codec");
                await stream.WriteAsync(Frame(PacketID.PacketAccessDuplicateClose, new DupTunnelRS()));
                await Task.Delay(50);
            });
            Check(await client.Transport.ConnectAsync("127.0.0.1", Port(listener)), "Connect");
            await Pump(client, Task.WhenAll(peer, fatal.Task));
            Check(fatal.Task.Result == FatalReason.DuplicateSession, "Duplicate session reason was lost");
        }
        Console.WriteLine("PASS protobuf heartbeat and duplicate-session shutdown");

        {
            var state = new HoldemTableState();
            void Apply(PacketID id, IMessage message) {
                var payload = message.ToByteString(); state.Receive(id, new PktBase { Payload = payload, PayloadSize = (uint)payload.Length });
            }
            Apply(PacketID.PacketAccessOpen, new SigninRS { MemberInfo = Player(7) });
            var room = new ChamberEnterRS { ChamberInfo = new RoomSnapshot { RoomNo = 1, PlayCategory = PlayCategory.TexasHoldem, AssetKind = AssetKind.Chip }, CaptainIdx = 7, LeadIdx = 8 };
            for (ulong i = 1; i <= 9; i++) room.Members.Add(Player(i));
            Apply(PacketID.PacketSpaceEnter, room);
            Check(state.Snapshot.Players.Count == 9 && state.Snapshot.Players[8].Seat == 8, "Nine seats");
            Check(!state.CanStart, "The lead player, not the room captain, owns the start action");
            Apply(PacketID.PacketRoundStateNotice, new MatchStateSwapRS { MatchPhase = 1 });
            var deal = new SeedDealoutRS { SeedFunds = 5 }; var cards = new CardSet();
            cards.PlayingCards.Add(new PlayingCard { RankCode = CardRank.Ace, SuitCode = CardSuit.Spade });
            deal.MemberCards.Add(7, cards);
            deal.BlindWagers.Add(new MatchWagerRS { WagerMemberIdx = 7, FundBefore = 995, FundAfter = 985, MemberWager = TableAction.SmallBlind });
            Apply(PacketID.PacketInitialDeal, deal);
            Apply(PacketID.PacketRoundStateNotice, new MatchStateSwapRS { MatchPhase = 2 });
            Check(state.Snapshot.Players[6].HoleCards.Count == 1 && state.Snapshot.Pot == 15 && state.Snapshot.Players[6].StreetBet == 10 && state.Snapshot.Players[6].Chips == 985, "Native deal-before-phase order lost cards or blinds");
            var reveal = new RevealRS(); reveal.SharedCards.Add(new PlayingCard { RankCode = CardRank.Ace, SuitCode = CardSuit.Heart });
            Apply(PacketID.PacketFinalReveal, reveal);
            Check(state.Snapshot.Board.Count == 1, "Final reveal was ignored");
        }
        Console.WriteLine("PASS native deal-before-phase ordering and final showdown");

    }
}

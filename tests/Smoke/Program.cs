using System.Net;
using System.Net.Sockets;
using Google.Protobuf;
using Mimic.Network;
using Mimic.Protocol;

var http = new HttpClient { Timeout = TimeSpan.FromSeconds(5) };
var front = args.Length > 0 ? args[0] : "http://127.0.0.1:5080";
var portal = PortalReply.Parser.ParseFrom(await http.GetByteArrayAsync(front + "/portal"));
Check(portal.Product == "MIMIC" && portal.ProtocolVersion == 1, "Front discovery");
async Task<LoginReply> Login(string name)
{
    using var body = new ByteArrayContent(new LoginRequest { DisplayName = name }.ToByteArray());
    body.Headers.ContentType = new("application/x-protobuf");
    using var response = await http.PostAsync(portal.PlatformUrl + "/auth/guest", body);
    response.EnsureSuccessStatusCode();return LoginReply.Parser.ParseFrom(await response.Content.ReadAsByteArrayAsync());
}
async Task ExpectError(NetClient client, Envelope message)
{
    try { await client.RequestAsync(message); throw new Exception("Expected rejected request"); }
    catch (InvalidOperationException) { }
}
static void Check(bool condition, string reason) { if (!condition) throw new Exception(reason); Console.WriteLine("PASS " + reason); }
var a = await Login("Alice");var b = await Login("Bob");
using var first = new NetClient();using var second = new NetClient();using var stranger = new NetClient();
await first.ConnectAsync(portal.TableHost, (int)portal.TablePort);
await second.ConnectAsync(portal.TableHost, (int)portal.TablePort);
await stranger.ConnectAsync(portal.TableHost, (int)portal.TablePort);
await ExpectError(stranger, new Envelope { ListTables = new Empty() });
await ExpectError(stranger, new Envelope { Authenticate = new AuthenticateRequest { SessionToken = new string('0', 64) } });
Check(true, "Unauthorized requests rejected");
await first.RequestAsync(new Envelope { Authenticate = new AuthenticateRequest { SessionToken = a.SessionToken } });
await second.RequestAsync(new Envelope { Authenticate = new AuthenticateRequest { SessionToken = b.SessionToken } });
await ExpectError(stranger, new Envelope { Authenticate = new AuthenticateRequest { SessionToken = a.SessionToken } });
Check(true, "Duplicate session rejected");
var lobby = await first.RequestAsync(new Envelope { ListTables = new Empty() });
Check(lobby.Lobby.Tables.Count == 1 && lobby.Lobby.Tables[0].Capacity == 6, "Holdem-only lobby");
await ExpectError(first, new Envelope { JoinTable = new JoinTableRequest { TableId = 999 } });
TableSnapshot left = null, right = null;
first.Received += e => { if (e.Snapshot != null) left = e.Snapshot; };
second.Received += e => { if (e.Snapshot != null) right = e.Snapshot; };
async Task WaitFor(Func<bool> predicate)
{
    for (int i = 0; i < 300; ++i) { first.Pump();second.Pump();if(predicate()) return;await Task.Delay(10); }
    throw new TimeoutException("Expected snapshot not received");
}
await first.RequestAsync(new Envelope { JoinTable = new JoinTableRequest { TableId = 1 } });
await second.RequestAsync(new Envelope { JoinTable = new JoinTableRequest { TableId = 1 } });
await first.RequestAsync(new Envelope { Ready = new ReadyRequest { Ready = true } });
await second.RequestAsync(new Envelope { Ready = new ReadyRequest { Ready = true } });
await WaitFor(() => left?.Street == Street.Preflop && right?.Street == Street.Preflop);
Check(left.Players.Single(p => p.PlayerId == a.PlayerId).HoleCards.Count == 2 && left.Players.Single(p => p.PlayerId == b.PlayerId).HoleCards.Count == 0, "Per-client hole card redaction");
Check(left.Pot == 30, "Blinds posted");
var hand = left.HandId;
await ExpectError(first, new Envelope { Action = new ActionRequest { HandId = hand, Revision = 0, Kind = ActionKind.Call } });
Check(true, "Stale state rejected");
int actions = 0;
while (left.Street != Street.Complete)
{
    Check(++actions < 30, "Hand progress " + actions);
    var state = left;var actor = state.Players.Single(p => (int)p.Seat == state.ActingSeat);
    var client = actor.PlayerId == a.PlayerId ? first : second;
    var kind = actor.StreetBet == state.CurrentBet ? ActionKind.Check : ActionKind.Call;
    await client.RequestAsync(new Envelope { Action = new ActionRequest { HandId = state.HandId, Revision = state.Revision, Kind = kind } });
    await WaitFor(() => left.Revision > state.Revision && right.Revision == left.Revision);
}
Check(left.Board.Count == 5 && left.Pot == 0 && left.Players.Sum(p => p.Chips) == 2000, "Showdown and chip conservation");
Check(left.Players.All(p => p.HoleCards.Count == 2), "Showdown reveals live hands");
await first.RequestAsync(new Envelope { LeaveTable = new Empty() });
await second.RequestAsync(new Envelope { LeaveTable = new Empty() });
var empty = await first.RequestAsync(new Envelope { ListTables = new Empty() });
Check(empty.Lobby.Tables[0].Players == 0, "Leave clears seats");
var pings = await Task.WhenAll(Enumerable.Range(0, 12).Select(_ => first.RequestAsync(new Envelope { Ping = new Empty() })));
Check(pings.All(p => p.Pong != null), "Concurrent requests correlate correctly");
using (var raw = new TcpClient())
{
    await raw.ConnectAsync(portal.TableHost, (int)portal.TablePort);
    var stream = raw.GetStream();
    var frame = new Envelope { ProtocolVersion = 99, RequestId = 1, Ping = new Empty() }.ToByteArray();
    var prefix = BitConverter.GetBytes(IPAddress.HostToNetworkOrder(frame.Length));
    foreach (var octet in prefix.Concat(frame)) await stream.WriteAsync(new[] { octet });
    var header = new byte[4];using var timeout = new CancellationTokenSource(5000);
    await stream.ReadExactlyAsync(header, timeout.Token);
    int length = IPAddress.NetworkToHostOrder(BitConverter.ToInt32(header));var bytes = new byte[length];await stream.ReadExactlyAsync(bytes, timeout.Token);
    Check(Envelope.Parser.ParseFrom(bytes).Error != null, "Fragmented framing and version rejection");
    await stream.WriteAsync(new byte[] { 0x7f, 0xff, 0xff, 0xff });
    try { Check(await stream.ReadAsync(header, timeout.Token) == 0, "Oversized frames close connection"); }
    catch (IOException) { Check(true, "Oversized frames reset connection"); }
}
Console.WriteLine("MIMIC integration smoke passed.");

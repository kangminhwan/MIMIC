using System;
using System.Collections.Concurrent;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using Ayve.Net.Transport;
using Google.Protobuf;
using Mimic.Protocol;

namespace Mimic.Network
{
    public sealed class NetClient : IDisposable
    {
        private readonly TcpLink link = new TcpLink(new TcpLinkOptions { MaxPayloadBytes = 65536, ConnectTimeoutMs = 5000 });
        private readonly ConcurrentDictionary<uint, TaskCompletionSource<Envelope>> pending = new ConcurrentDictionary<uint, TaskCompletionSource<Envelope>>();
        private readonly ConcurrentQueue<Envelope> inbox = new ConcurrentQueue<Envelope>();
        private readonly CancellationTokenSource lifetime = new CancellationTokenSource();
        private readonly object sendGate = new object();
        private uint sequence;
        private int queued, disposed;
        private Exception disconnectError;
        public event Action<Envelope> Received;
        public event Action<Exception> Disconnected;
        public bool IsConnected => link.IsConnected && !lifetime.IsCancellationRequested;

        public NetClient()
        {
            link.PacketReceived += Receive;
            link.Faulted += fault => Fail(new IOException(fault.Message, fault.Exception));
        }
        public async Task ConnectAsync(string host, int port)
        {
            if (lifetime.IsCancellationRequested) throw new ObjectDisposedException(nameof(NetClient));
            if (!await link.ConnectAsync(host, port, lifetime.Token)) throw new IOException("테이블 서버에 연결할 수 없습니다.");
            _ = HeartbeatLoop();
        }
        public async Task<Envelope> RequestAsync(Envelope message)
        {
            uint id;
            var completion = new TaskCompletionSource<Envelope>(TaskCreationOptions.RunContinuationsAsynchronously);
            lock (sendGate)
            {
                if (!IsConnected) throw new IOException("서버 연결이 끊어졌습니다. 다시 로그인해 주세요.");
                if (pending.Count >= 64) throw new IOException("요청이 너무 많습니다. 잠시 후 다시 시도해 주세요.");
                if (sequence == uint.MaxValue) { Dispose(); throw new IOException("연결을 갱신해 주세요."); }
                id = ++sequence;
                message.ProtocolVersion = 1; message.RequestId = id;
                var body = message.ToByteArray();
                if (body.Length == 0 || body.Length > 65536) throw new IOException("Invalid packet size");
                pending[id] = completion;
                if (!link.TrySend((uint)message.PayloadCase, body, id))
                { pending.TryRemove(id, out _); throw new IOException("요청을 전송하지 못했습니다."); }
            }
            try
            {
                if (await Task.WhenAny(completion.Task, Task.Delay(5000, lifetime.Token)) != completion.Task)
                {
                    var error = new TimeoutException("서버 응답이 늦어지고 있습니다. 다시 연결해 주세요.");
                    Fail(error); throw error;
                }
                var response = await completion.Task;
                if (response.Error != null) throw new InvalidOperationException(response.Error.Message);
                return response;
            }
            finally { pending.TryRemove(id, out _); }
        }
        private void Receive(InboundPacket packet)
        {
            try
            {
                var message = Envelope.Parser.ParseFrom(packet.Payload);
                if (message.ProtocolVersion != 1 || message.RequestId != packet.Nonce || (uint)message.PayloadCase != packet.Command)
                    throw new IOException("프로토콜이 일치하지 않습니다.");
                if (message.RequestId != 0)
                {
                    if (pending.TryRemove(packet.Nonce, out var waiter)) waiter.TrySetResult(message);
                }
                else
                {
                    if (Interlocked.Increment(ref queued) > 256) throw new IOException("Too many server events");
                    inbox.Enqueue(message);
                }
            }
            catch (Exception error) { Fail(error); }
        }
        private async Task HeartbeatLoop()
        {
            try
            {
                while (!lifetime.IsCancellationRequested)
                {
                    await Task.Delay(20000, lifetime.Token).ConfigureAwait(false);
                    await RequestAsync(new Envelope { Ping = new Empty() }).ConfigureAwait(false);
                }
            }
            catch (Exception error) { if (!lifetime.IsCancellationRequested) Fail(error); }
        }
        private void Fail(Exception error)
        {
            if (Volatile.Read(ref disposed) != 0) return;
            Interlocked.CompareExchange(ref disconnectError, error, null);
            Dispose();
        }
        public void Pump()
        {
            for (int i = 0; i < 64 && inbox.TryDequeue(out var message); ++i)
            { Interlocked.Decrement(ref queued); Received?.Invoke(message); }
            var error = Interlocked.Exchange(ref disconnectError, null);
            if (error != null) Disconnected?.Invoke(error);
        }
        public void Dispose()
        {
            if (Interlocked.Exchange(ref disposed, 1) != 0) return;
            lifetime.Cancel();
            foreach (var entry in pending)
                if (pending.TryRemove(entry.Key, out var waiter)) waiter.TrySetException(new IOException("Connection closed"));
            _ = CloseLink();
        }
        private async Task CloseLink()
        {
            try { await link.DisposeAsync(); }
            catch (Exception) { /* Shutdown must not escape a Unity lifecycle callback. */ }
        }
    }
}

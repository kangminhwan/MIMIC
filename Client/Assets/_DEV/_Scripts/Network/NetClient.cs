using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;
using Google.Protobuf;
using Mimic.Protocol;

namespace Mimic.Network
{
    public sealed class NetClient : IDisposable
    {
        private const int MaxFrame = 65536;
        private readonly TcpClient socket = new TcpClient();
        private readonly SemaphoreSlim sendGate = new SemaphoreSlim(1, 1);
        private readonly ConcurrentDictionary<ulong, TaskCompletionSource<Envelope>> pending = new ConcurrentDictionary<ulong, TaskCompletionSource<Envelope>>();
        private readonly ConcurrentQueue<Envelope> inbox = new ConcurrentQueue<Envelope>();
        private readonly CancellationTokenSource lifetime = new CancellationTokenSource();
        private NetworkStream stream;
        private long sequence;
        private int queued;
        private int disposed;
        public event Action<Envelope> Received;
        public event Action<Exception> Disconnected;
        private Exception disconnectError;
        public bool IsConnected => stream != null && !lifetime.IsCancellationRequested;

        public async Task ConnectAsync(string host, int port)
        {
            if (stream != null || lifetime.IsCancellationRequested) throw new InvalidOperationException("Create a new client to reconnect");
            var connect = socket.ConnectAsync(host, port);
            if (await Task.WhenAny(connect, Task.Delay(5000)) != connect) { Dispose(); throw new TimeoutException("Table connection timed out"); }
            await connect;
            socket.NoDelay = true;
            stream = socket.GetStream();
            _ = ReceiveLoop();
            _ = HeartbeatLoop();
        }

        public async Task<Envelope> RequestAsync(Envelope request)
        {
            if (!IsConnected) throw new IOException("Not connected");
            var completion = new TaskCompletionSource<Envelope>(TaskCreationOptions.RunContinuationsAsynchronously);
            ulong id = 0;
            await sendGate.WaitAsync(lifetime.Token);
            try
            {
                // Allocate IDs inside the send gate so wire order is strictly increasing.
                id = (ulong)Interlocked.Increment(ref sequence);
                request.ProtocolVersion = 1; request.RequestId = id;
                if (pending.Count >= 64) throw new IOException("Too many pending requests");
                var body = request.ToByteArray();
                if (body.Length == 0 || body.Length > MaxFrame) throw new IOException("Invalid frame length");
                pending[id] = completion;
                var frame = new byte[4 + body.Length];
                frame[0] = (byte)(body.Length >> 24); frame[1] = (byte)(body.Length >> 16);
                frame[2] = (byte)(body.Length >> 8); frame[3] = (byte)body.Length;
                Buffer.BlockCopy(body, 0, frame, 4, body.Length);
                using (var timeout = CancellationTokenSource.CreateLinkedTokenSource(lifetime.Token))
                {
                    timeout.CancelAfter(5000);
                    await stream.WriteAsync(frame, 0, frame.Length, timeout.Token);
                }
            }
            catch (Exception error) { pending.TryRemove(id, out _); disconnectError = error; Dispose(); throw; }
            finally { sendGate.Release(); }
            try
            {
                if (await Task.WhenAny(completion.Task, Task.Delay(5000, lifetime.Token)) != completion.Task)
                    throw new TimeoutException("Request timed out; reconnect to resynchronize");
                var response = await completion.Task;
                if (response.Error != null) throw new InvalidOperationException(response.Error.Message);
                return response;
            }
            catch (TimeoutException error) { disconnectError = error; Dispose(); throw; }
            finally { pending.TryRemove(id, out _); }
        }

        private async Task ReadExactly(byte[] bytes)
        {
            int offset = 0;
            while (offset < bytes.Length)
            {
                int count = await stream.ReadAsync(bytes, offset, bytes.Length - offset, lifetime.Token).ConfigureAwait(false);
                if (count == 0) throw new EndOfStreamException("Server disconnected");
                offset += count;
            }
        }
        private async Task ReceiveLoop()
        {
            try
            {
                var header = new byte[4];
                while (!lifetime.IsCancellationRequested)
                {
                    await ReadExactly(header).ConfigureAwait(false);
                    uint size = ((uint)header[0] << 24) | ((uint)header[1] << 16) | ((uint)header[2] << 8) | header[3];
                    if (size == 0 || size > MaxFrame) throw new IOException("Invalid server frame length");
                    var body = new byte[(int)size]; await ReadExactly(body).ConfigureAwait(false);
                    var message = Envelope.Parser.ParseFrom(body);
                    if (message.ProtocolVersion != 1) throw new IOException("Unsupported protocol version");
                    if (message.RequestId != 0 && pending.TryRemove(message.RequestId, out var waiter)) waiter.TrySetResult(message);
                    else if (message.RequestId == 0)
                    {
                        if (Interlocked.Increment(ref queued) > 256) throw new IOException("Server event queue overflow");
                        inbox.Enqueue(message);
                    }
                }
            }
            catch (Exception error) { if (!lifetime.IsCancellationRequested) disconnectError = error; }
            finally { Dispose(); }
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
            catch (Exception error) { if (!lifetime.IsCancellationRequested) { disconnectError = error; Dispose(); } }
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
            lifetime.Cancel(); socket.Close();
            foreach (KeyValuePair<ulong, TaskCompletionSource<Envelope>> entry in pending)
                if (pending.TryRemove(entry.Key, out var waiter)) waiter.TrySetException(new IOException("Connection closed"));
        }
    }
}

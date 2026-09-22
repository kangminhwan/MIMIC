// =============================================================================
// TcpLink — TcpSession(839줄) 전면 교체.
//
// 기존 TcpSession 에서 잡은 버그 / 구조 문제
//  1. _isShuttingDown 이 volatile 이 아님 → 재접속 시 구 writer 스레드가 살아남아
//     두 스레드가 같은 송신 큐를 소비 (스레드 누수 + 패킷 순서 붕괴).
//  2. _writerThread 에 IsBackground=true 없음 → 포그라운드 스레드가 앱 종료를 막음.
//  3. writer 스레드를 join 하지 않음. WaitForThreadsExit 는 IOCP 경로에서만 세팅되는
//     _pendingWriteResult 만 기다림 → _socket.Close() 가 Send() 중에 실행되는 레이스.
//  4. _writerProc.BeginInvoke — 델리게이트 APM. IL2CPP/.NET Core 미지원.
//  5. while (thread.ThreadState != ThreadState.Running) — 플래그 조합이라 무한 스핀 가능.
//  6. connectionTask.Wait() — 전용 스레드 위 sync-over-async.
//  7. 헤더 magic/길이 무검증 → 손상된 length 로 임의 크기 할당, 64KB 초과 패킷은 무한 정지.
//  8. IOCP 경로 while(offset > Header.SIZE) vs 이벤트 경로 while(offset >= Header.SIZE)
//     불일치 → payload 0 패킷 처리 갈림.
//  9. 전송 계층이 NtManager / PopupManager / LoadingPopup 을 직접 참조 (테스트 불가).
// 10. CancellationToken 부재.
//
// 새 구현
//  - Thread 0개. 수신/송신 모두 async 루프(Task).
//  - 상태는 int 하나(Interlocked). 불리언 플래그 없음.
//  - ArrayPool 기반 수신 누적 버퍼 + 자동 확장 + 상한 초과 시 명시적 오류.
//  - 송신 프레임은 풀에서 빌려 헤더까지 한 번에 쓰고, 암호화는 복사본에만 적용.
//  - Unity / UI 참조 0개. 외부와는 이벤트 3개로만 소통한다.
//
// 요구 사항: Unity Player Settings → Api Compatibility Level = .NET Standard 2.1
//            (Socket 의 Memory<byte> 기반 async 오버로드가 여기서부터 제공된다)
// =============================================================================

using System;
using System.Buffers;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;
using Ayve.Net.Wire;

namespace Ayve.Net.Transport
{
    public sealed class TcpLink : IAsyncDisposable
    {
        private readonly TcpLinkOptions _opt;
        private readonly IPayloadCipher _cipher;

        private readonly ConcurrentQueue<OutFrame> _sendQueue = new ConcurrentQueue<OutFrame>();
        private readonly SemaphoreSlim _sendSignal = new SemaphoreSlim(0);
        private readonly object _gate = new object();

        private Socket _socket;
        private CancellationTokenSource _cts;
        private Task _receiveLoop;
        private Task _sendLoop;

        private int _state = (int)LinkState.Disconnected;
        private int _faultRaised;

        /// <summary>
        /// ConnectAsync/CloseAsync 가 겹쳐 도는 걸 구분하기 위한 세대 번호.
        /// CloseAsync 는 옛 소켓의 송수신 루프가 끝날 때까지 기다리느라 시간이 걸리는데, 그 사이
        /// (동시에 실행된) ConnectAsync 가 새 소켓으로 이미 연결에 성공했을 수 있다. 그 상태에서
        /// 늦게 끝난 CloseAsync 가 무조건 _state = Disconnected 를 쓰면 방금 붙은 새 연결의 상태가
        /// 조용히 깨진다 (소켓은 멀쩡한데 IsConnected 만 false — 재접속 루프가 원인 불명으로 돈다).
        /// </summary>
        private long _generation;

        /// <summary>상태 변화. IO 스레드에서 올라올 수 있다 — UI 를 만지지 말 것.</summary>
        public event Action<LinkState> StateChanged;

        /// <summary>패킷 1개 수신. IO 스레드에서 호출된다 — 큐에 넣고 메인 스레드에서 처리할 것.</summary>
        public event Action<InboundPacket> PacketReceived;

        /// <summary>연결이 죽었다. 링크당 정확히 한 번만 발생한다.</summary>
        public event Action<LinkFault> Faulted;

        public TcpLink(TcpLinkOptions options)
        {
            _opt = options ?? new TcpLinkOptions();
            _cipher = _opt.Cipher ?? NullCipher.Instance;
        }

        public LinkState State => (LinkState)Volatile.Read(ref _state);
        public bool IsConnected => State == LinkState.Connected;

        // ---------------------------------------------------------------------
        // Connect
        // ---------------------------------------------------------------------

        /// <summary>
        /// 접속. 이미 연결/연결중이면 false. 성공 시 수신/송신 루프가 돌기 시작한다.
        /// 여러 주소(IPv6 → IPv4)를 순서대로 시도하고, 하나라도 붙으면 성공.
        /// </summary>
        public async Task<bool> ConnectAsync(string host, int port, CancellationToken ct = default)
        {
            if (string.IsNullOrWhiteSpace(host) || port <= 0)
            {
                RaiseFault(new LinkFault(LinkFaultKind.ResolveFailed, $"invalid endpoint {host}:{port}"));
                return false;
            }

            if (Interlocked.CompareExchange(ref _state, (int)LinkState.Connecting, (int)LinkState.Disconnected)
                != (int)LinkState.Disconnected)
            {
                Log($"ConnectAsync ignored, state={State}");
                return false;
            }

            // 이 시점부터는 새 연결 세대다 — 이전에 시작됐지만 아직 안 끝난 CloseAsync 가 있다면
            // 그건 이제 스테일이니 나중에 끝나도 이 연결의 상태를 건드리면 안 된다.
            Interlocked.Increment(ref _generation);

            Volatile.Write(ref _faultRaised, 0);
            OnStateChanged(LinkState.Connecting);

            IPAddress[] candidates;
            try
            {
                candidates = Order(await Dns.GetHostAddressesAsync(host).ConfigureAwait(false));
            }
            catch (Exception ex)
            {
                ResetToDisconnected();
                RaiseFault(new LinkFault(LinkFaultKind.ResolveFailed, ex.Message, ex));
                return false;
            }

            if (candidates.Length == 0)
            {
                ResetToDisconnected();
                RaiseFault(new LinkFault(LinkFaultKind.ResolveFailed, $"no address for {host}"));
                return false;
            }

            Exception last = null;
            for (int i = 0; i < candidates.Length; i++)
            {
                if (ct.IsCancellationRequested) break;

                Socket sock = null;
                try
                {
                    sock = new Socket(candidates[i].AddressFamily, SocketType.Stream, ProtocolType.Tcp)
                    {
                        NoDelay = true,
                        SendBufferSize = _opt.SocketSendBufferBytes,
                        ReceiveBufferSize = _opt.SocketReceiveBufferBytes,
                    };

                    Task connect = sock.ConnectAsync(new IPEndPoint(candidates[i], port));
                    Task done = await Task.WhenAny(connect, Task.Delay(_opt.ConnectTimeoutMs, ct)).ConfigureAwait(false);

                    if (done != connect)
                    {
                        SafeDispose(sock);
                        last = new TimeoutException($"connect timeout {candidates[i]}:{port}");
                        continue;
                    }

                    await connect.ConfigureAwait(false); // 예외 관찰

                    if (!sock.Connected)
                    {
                        SafeDispose(sock);
                        last = new SocketException((int)SocketError.NotConnected);
                        continue;
                    }

                    StartLoops(sock);
                    Log($"connected to {candidates[i]}:{port}");
                    return true;
                }
                catch (OperationCanceledException)
                {
                    SafeDispose(sock);
                    break;
                }
                catch (Exception ex)
                {
                    SafeDispose(sock);
                    last = ex;
                }
            }

            ResetToDisconnected();

            LinkFaultKind kind = last is TimeoutException
                ? LinkFaultKind.ConnectTimeout
                : LinkFaultKind.ConnectFailed;
            RaiseFault(new LinkFault(kind, last?.Message ?? "connect failed", last));
            return false;
        }

        private IPAddress[] Order(IPAddress[] addrs)
        {
            var v6 = new List<IPAddress>();
            var v4 = new List<IPAddress>();
            for (int i = 0; i < addrs.Length; i++)
            {
                if (addrs[i].AddressFamily == AddressFamily.InterNetworkV6) v6.Add(addrs[i]);
                else if (addrs[i].AddressFamily == AddressFamily.InterNetwork) v4.Add(addrs[i]);
            }

            var result = new List<IPAddress>(addrs.Length);
            if (_opt.PreferIPv6) { result.AddRange(v6); result.AddRange(v4); }
            else { result.AddRange(v4); result.AddRange(v6); }
            return result.ToArray();
        }

        private void StartLoops(Socket sock)
        {
            lock (_gate)
            {
                _socket = sock;
                _cts = new CancellationTokenSource();
                Volatile.Write(ref _state, (int)LinkState.Connected);
                _receiveLoop = Task.Run(() => ReceiveLoopAsync(sock, _cts.Token));
                _sendLoop = Task.Run(() => SendLoopAsync(sock, _cts.Token));
            }
            OnStateChanged(LinkState.Connected);
        }

        // ---------------------------------------------------------------------
        // Send
        // ---------------------------------------------------------------------

        /// <summary>
        /// 송신 큐에 넣는다. 즉시 반환. 연결이 없으면 false.
        /// payload 버퍼는 이 메서드 안에서 복사되므로 호출자가 바로 재사용해도 안전하다.
        /// (기존 EncryptPayload 는 호출자 배열을 in-place 로 뒤집었다 — 이중 XOR 평문 노출 버그)
        /// </summary>
        public bool TrySend(uint command, ReadOnlySpan<byte> payload, uint nonce = 0, uint entity = 0)
        {
            if (State != LinkState.Connected) return false;
            if ((uint)payload.Length > _opt.MaxPayloadBytes) return false;

            int total = PacketHeader.Size + payload.Length;
            byte[] buffer = ArrayPool<byte>.Shared.Rent(total);

            var header = new PacketHeader(command, (uint)payload.Length, nonce, entity);
            header.Write(new Span<byte>(buffer, 0, PacketHeader.Size));

            if (payload.Length > 0)
            {
                var dst = new Span<byte>(buffer, PacketHeader.Size, payload.Length);
                payload.CopyTo(dst);
                _cipher.Encrypt(dst); // 복사본에만 적용
            }

            _sendQueue.Enqueue(new OutFrame(buffer, total, command));
            try { _sendSignal.Release(); }
            catch (ObjectDisposedException) { /* 종료 중 */ }
            return true;
        }

        private async Task SendLoopAsync(Socket sock, CancellationToken ct)
        {
            try
            {
                while (!ct.IsCancellationRequested)
                {
                    await _sendSignal.WaitAsync(ct).ConfigureAwait(false);

                    while (_sendQueue.TryDequeue(out OutFrame frame))
                    {
                        try
                        {
                            int sent = 0;
                            while (sent < frame.Length)
                            {
                                int n = await sock.SendAsync(
                                    new ReadOnlyMemory<byte>(frame.Buffer, sent, frame.Length - sent),
                                    SocketFlags.None, ct).ConfigureAwait(false);

                                if (n <= 0) throw new SocketException((int)SocketError.ConnectionReset);
                                sent += n;
                            }
                        }
                        finally
                        {
                            ArrayPool<byte>.Shared.Return(frame.Buffer);
                        }
                    }
                }
            }
            catch (OperationCanceledException) { }
            catch (ObjectDisposedException) { }
            catch (Exception ex)
            {
                RaiseFault(new LinkFault(LinkFaultKind.SendFailed, ex.Message, ex));
            }
        }

        // ---------------------------------------------------------------------
        // Receive + framing
        // ---------------------------------------------------------------------

        private async Task ReceiveLoopAsync(Socket sock, CancellationToken ct)
        {
            byte[] buffer = ArrayPool<byte>.Shared.Rent(_opt.InitialReceiveBufferBytes);
            int filled = 0;

            try
            {
                while (!ct.IsCancellationRequested)
                {
                    if (filled == buffer.Length && !TryGrow(ref buffer, filled))
                    {
                        RaiseFault(new LinkFault(LinkFaultKind.FrameTooLarge,
                            $"packet exceeds MaxPayloadBytes ({_opt.MaxPayloadBytes})"));
                        return;
                    }

                    int read = await sock.ReceiveAsync(
                        new Memory<byte>(buffer, filled, buffer.Length - filled),
                        SocketFlags.None, CancellationToken.None).ConfigureAwait(false);

                    if (read == 0)
                    {
                        if (!ct.IsCancellationRequested)
                            RaiseFault(new LinkFault(LinkFaultKind.RemoteClosed, "remote closed the connection"));
                        return;
                    }

                    filled += read;

                    if (!DrainFrames(buffer, ref filled)) return; // 프로토콜 파손
                }
            }
            catch (OperationCanceledException) { }
            catch (ObjectDisposedException) { }
            catch (SocketException ex) when (ct.IsCancellationRequested || ex.SocketErrorCode == SocketError.OperationAborted || ex.SocketErrorCode == SocketError.Shutdown)
            {
                // 정상 종료 경로
            }
            catch (Exception ex)
            {
                RaiseFault(new LinkFault(LinkFaultKind.SocketError, ex.Message, ex));
            }
            finally
            {
                ArrayPool<byte>.Shared.Return(buffer);
            }
        }

        /// <summary>
        /// 누적 버퍼에서 완성된 패킷을 모두 뽑아낸다.
        /// 기존 구현은 패킷 하나 뽑을 때마다 BlockCopy 로 앞당겨서 O(n²) 였다.
        /// 여기서는 consumed 오프셋만 밀고 마지막에 한 번만 압축한다.
        /// </summary>
        private bool DrainFrames(byte[] buffer, ref int filled)
        {
            int consumed = 0;

            while (filled - consumed >= PacketHeader.Size)
            {
                var view = new ReadOnlySpan<byte>(buffer, consumed, filled - consumed);

                if (!PacketHeader.TryRead(view, _opt.MaxPayloadBytes, out PacketHeader header, out HeaderError err))
                {
                    RaiseFault(new LinkFault(LinkFaultKind.ProtocolError,
                        err == HeaderError.BadMagic
                            ? "header magic mismatch — stream desynchronised"
                            : $"payload length exceeds limit ({_opt.MaxPayloadBytes})"));
                    return false;
                }

                if (view.Length < header.TotalSize) break; // 아직 덜 왔다

                byte[] payload;
                if (header.PayloadSize == 0)
                {
                    payload = Array.Empty<byte>();
                }
                else
                {
                    payload = new byte[header.PayloadSize];
                    view.Slice(PacketHeader.Size, (int)header.PayloadSize).CopyTo(payload);
                    _cipher.Decrypt(payload);
                }

                consumed += header.TotalSize;

                try
                {
                    PacketReceived?.Invoke(new InboundPacket(header.Command, header.Nonce, header.Entity, payload));
                }
                catch (Exception ex)
                {
                    // 구독자 예외가 수신 루프를 죽이면 안 된다.
                    Log($"PacketReceived handler threw: {ex}");
                }
            }

            if (consumed > 0)
            {
                int rest = filled - consumed;
                if (rest > 0) Buffer.BlockCopy(buffer, consumed, buffer, 0, rest);
                filled = rest;
            }

            return true;
        }

        private bool TryGrow(ref byte[] buffer, int filled)
        {
            int max = (int)Math.Min(int.MaxValue - PacketHeader.Size, _opt.MaxPayloadBytes) + PacketHeader.Size;
            if (buffer.Length >= max) return false;

            int next = Math.Min(max, buffer.Length * 2);
            byte[] bigger = ArrayPool<byte>.Shared.Rent(next);
            Buffer.BlockCopy(buffer, 0, bigger, 0, filled);
            ArrayPool<byte>.Shared.Return(buffer);
            buffer = bigger;
            Log($"receive buffer grown to {buffer.Length}");
            return true;
        }

        // ---------------------------------------------------------------------
        // Shutdown
        // ---------------------------------------------------------------------

        /// <summary>
        /// 링크를 닫고 두 루프가 끝날 때까지 기다린다.
        /// 기존 CloseSocket() 은 writer 스레드를 기다리지 않고 소켓을 Dispose 해서
        /// Send() 도중 ObjectDisposedException 이 나는 레이스를 만들었다.
        /// </summary>
        public async Task CloseAsync()
        {
            Socket sock;
            CancellationTokenSource cts;
            Task rx, tx;
            long myGeneration;

            lock (_gate)
            {
                if (State == LinkState.Disconnected && _socket == null) return;
                myGeneration = Interlocked.Increment(ref _generation);
                Volatile.Write(ref _state, (int)LinkState.Closing);
                sock = _socket; cts = _cts; rx = _receiveLoop; tx = _sendLoop;
                _socket = null; _cts = null; _receiveLoop = null; _sendLoop = null;
            }

            OnStateChanged(LinkState.Closing);

            try { cts?.Cancel(); } catch { }

            try { sock?.Shutdown(SocketShutdown.Both); } catch { }

            // 대기 중인 SendAsync/ReceiveAsync 를 확실히 깨우기 위해 Dispose.
            SafeDispose(sock);

            Task all = Task.WhenAll(
                rx ?? Task.CompletedTask,
                tx ?? Task.CompletedTask);

            await Task.WhenAny(all, Task.Delay(_opt.ShutdownGraceMs)).ConfigureAwait(false);

            DrainSendQueue();
            try { cts?.Dispose(); } catch { }

            // 옛 루프를 기다리는 동안 다른 ConnectAsync 가 이미 새 연결을 맺었다면(세대가 바뀌었으면)
            // 그 연결의 상태를 여기서 Disconnected 로 덮어쓰면 안 된다 — 소켓은 멀쩡한데 상태값만
            // 깨져서 IsConnected 가 false 가 되고 원인 불명의 재접속 루프가 도는 버그였다.
            lock (_gate)
            {
                if (Interlocked.Read(ref _generation) != myGeneration)
                {
                    Log("link closed (stale close, superseded by a newer connect)");
                    return;
                }
            }

            Volatile.Write(ref _state, (int)LinkState.Disconnected);
            OnStateChanged(LinkState.Disconnected);
            Log("link closed");
        }

        public async ValueTask DisposeAsync()
        {
            await CloseAsync().ConfigureAwait(false);
            _sendSignal.Dispose();
        }

        private void DrainSendQueue()
        {
            while (_sendQueue.TryDequeue(out OutFrame frame))
                ArrayPool<byte>.Shared.Return(frame.Buffer);
        }

        private void ResetToDisconnected()
        {
            Volatile.Write(ref _state, (int)LinkState.Disconnected);
            OnStateChanged(LinkState.Disconnected);
        }

        private void RaiseFault(LinkFault fault)
        {
            if (Interlocked.Exchange(ref _faultRaised, 1) != 0) return;

            if (State == LinkState.Connected || State == LinkState.Connecting)
            {
                Volatile.Write(ref _state, (int)LinkState.Faulted);
                OnStateChanged(LinkState.Faulted);
            }

            Log($"fault: {fault}");
            try { Faulted?.Invoke(fault); } catch (Exception ex) { Log($"Faulted handler threw: {ex}"); }
        }

        private void OnStateChanged(LinkState s)
        {
            try { StateChanged?.Invoke(s); } catch (Exception ex) { Log($"StateChanged handler threw: {ex}"); }
        }

        private static void SafeDispose(Socket s)
        {
            if (s == null) return;
            try { s.Dispose(); } catch { }
        }

        private void Log(string msg) => _opt.Log?.Invoke($"[TcpLink] {msg}");

        private readonly struct OutFrame
        {
            public readonly byte[] Buffer;
            public readonly int Length;
            public readonly uint Command;

            public OutFrame(byte[] buffer, int length, uint command)
            {
                Buffer = buffer; Length = length; Command = command;
            }
        }
    }
}

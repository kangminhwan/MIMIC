// Adapted from C:/NewClient/casino for the protobuf-only TableServer.
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;
using Ayve.Net.Transport;
using General;
using Google.Protobuf;
using System.IO;
using PmNet;

namespace Ayve.Net.Core
{
    public enum SessionPhase
    {
        Offline = 0,
        Connecting,
        Online,
        Reconnecting,
        /// <summary>복구 불가. 재로그인/종료 유도가 필요한 상태.</summary>
        Fatal,
    }

    public enum FatalReason
    {
        None = 0,
        DuplicateSession,
        SessionFault,
        ReconnectExhausted,
        NetworkError,
        ProtocolError,
    }

    public sealed class NetClientOptions
    {
        public int RequestTimeoutMs = 5000;
        /// <summary>서버 트래픽이 이 시간 이상 없으면 링크가 죽은 것으로 본다.</summary>
        public int DeadLinkMs = 15000;
        /// <summary>false이면 링크 장애 시 재접속하지 않고 Fatal(NetworkError)를 올린다.</summary>
        public bool ReconnectEnabled = false;
        public int ReconnectAttempts = 3;
        public int ReconnectDelayMs = 1500;
        /// <summary>한 프레임에 처리할 최대 수신 패킷 수 (버스트 보호).</summary>
        public int MaxPacketsPerFrame = 64;
#if UNITY_EDITOR
        public bool VerboseLog = true;
#else
        public bool VerboseLog = false;
#endif
        /// <summary>서버가 요청 Nonce 를 응답에 에코해 주면 true (권장). 미지원이면 false.</summary>
        public bool CorrelateByNonce = false;
        public TcpLinkOptions Link = new TcpLinkOptions();
    }

    public sealed class NetClient : IAsyncDisposable
    {
        private readonly NetClientOptions _opt;
        private readonly TcpLink _link;
        // 와이어 타입은 서버 C++ 구조체와 필드 타입이 정확히 일치하는 PktBase.

        private readonly PendingRequestTable<PktBase> _pending = new PendingRequestTable<PktBase>();
        private readonly ReceiveBatchGate _receiveBatchGate = new ReceiveBatchGate();
        private readonly ConcurrentQueue<InboundPacket> _inbox = new ConcurrentQueue<InboundPacket>();
        private readonly ConcurrentQueue<Action> _mainThreadActions = new ConcurrentQueue<Action>();
        private readonly object _reconnectGate = new object();

        private readonly HeartbeatRS _heartbeatReply = new HeartbeatRS();

        private Task<bool> _reconnectInFlight;
        private long _lastServerTickMs;
        private int _nonceSeed;
        private SessionPhase _phase = SessionPhase.Offline;

        public string Host { get; private set; }
        public int Port { get; private set; }

        /// <summary>재접속 성공 직후 재인증(재로그인)을 수행. 게임 쪽에서 주입한다.</summary>
        public Func<CancellationToken, Task<bool>> ReauthHandler { get; set; }

        /// <summary>대기자가 없는 서버 푸시. 메인 스레드에서 호출된다.</summary>
        public event Action<PacketID, PktBase> PacketPushed;

        // State reducers run before request completion to preserve wire order.
        public event Action<PacketID, PktBase> PacketReceived;

        /// <summary>세션 상태 변화. 메인 스레드에서 호출된다.</summary>
        public event Action<SessionPhase> PhaseChanged;

        /// <summary>복구 불가 상황. 메인 스레드에서 호출된다.</summary>
        public event Action<FatalReason> Fatal;

        /// <summary>진단 로그.</summary>
        public event Action<string> Log;

        public NetClient(NetClientOptions options = null)
        {
            _opt = options ?? new NetClientOptions();
            _opt.Link.Log = msg => { if (_opt.VerboseLog) Log?.Invoke(msg); };

            _link = new TcpLink(_opt.Link);
            _link.PacketReceived += OnPacketReceived;   // IO 스레드
            _link.Faulted += OnLinkFaulted;             // IO 스레드
        }

        public SessionPhase Phase => _phase;
        public bool IsConnected => _link.IsConnected;
        public bool IsOnline => _phase == SessionPhase.Online && _link.IsConnected;
        public bool ReconnectEnabled
        {
            get => _opt.ReconnectEnabled;
            set => _opt.ReconnectEnabled = value;
        }

        public bool IsServerAliveRecently(int withinMs)
            => NowMs - Interlocked.Read(ref _lastServerTickMs) <= withinMs;

        // ---------------------------------------------------------------------
        // 연결
        // ---------------------------------------------------------------------

        public async Task<bool> ConnectAsync(string host, int port, CancellationToken ct = default)
        {
            Host = host;
            Port = port;

            await _link.CloseAsync().ConfigureAwait(false);
            _pending.CompleteAllWithNull();
            DrainInbox();

            SetPhase(SessionPhase.Connecting);
            bool ok = await _link.ConnectAsync(host, port, ct).ConfigureAwait(false);

            if (ok)
            {
                MarkServerAlive();
                SetPhase(SessionPhase.Online);
            }
            else
            {
                SetPhase(SessionPhase.Offline);
            }
            return ok;
        }

        /// <summary>
        /// 온라인 보장. 이미 온라인이면 즉시 true.
        /// 동시에 여러 번 불려도 재접속은 단 한 번만 수행되고 모두 같은 Task 를 기다린다.
        /// (기존 while(IsSessionReconnecting) await Task.Yield() 스핀을 대체)
        /// </summary>
        public Task<bool> EnsureOnlineAsync(CancellationToken ct = default)
        {
            if (IsOnline) return TrueTask;
            if (_phase == SessionPhase.Fatal) return FalseTask;
            if (!_opt.ReconnectEnabled)
            {
                RaiseNetworkFatal();
                return FalseTask;
            }

            lock (_reconnectGate)
            {
                if (IsOnline) return TrueTask;
                if (_reconnectInFlight != null) return _reconnectInFlight;
                _reconnectInFlight = ReconnectCoreAsync(ct);
                return _reconnectInFlight;
            }
        }

        private async Task<bool> ReconnectCoreAsync(CancellationToken ct)
        {
            try
            {
                SetPhase(SessionPhase.Reconnecting);
                _pending.CompleteAllWithNull();
                DrainInbox();

                for (int attempt = 0; attempt <= _opt.ReconnectAttempts; attempt++)
                {
                    if (ct.IsCancellationRequested) return false;

                    if (attempt > 0)
                    {
                        Emit($"reconnect attempt {attempt}/{_opt.ReconnectAttempts}");
                        await Task.Delay(_opt.ReconnectDelayMs, ct).ConfigureAwait(false);
                    }

                    await _link.CloseAsync().ConfigureAwait(false);

                    if (!await _link.ConnectAsync(Host, Port, ct).ConfigureAwait(false))
                        continue;

                    MarkServerAlive();
                    SetPhase(SessionPhase.Online);

                    Func<CancellationToken, Task<bool>> reauth = ReauthHandler;
                    if (reauth == null) return true;

                    bool authed;
                    try { authed = await InvokeOnMainThreadAsync(reauth, ct).ConfigureAwait(false); }
                    catch (Exception ex) { Emit($"reauth threw: {ex.Message}"); authed = false; }

                    if (authed) return true;

                    if (_phase == SessionPhase.Fatal) return false;

                    Emit("reauth failed");
                    await _link.CloseAsync().ConfigureAwait(false);
                }

                RaiseFatal(FatalReason.ReconnectExhausted);
                return false;
            }
            catch (OperationCanceledException)
            {
                SetPhase(SessionPhase.Offline);
                return false;
            }
            finally
            {
                lock (_reconnectGate) { _reconnectInFlight = null; }
            }
        }

        /// <summary>
        /// ReauthHandler(NtManager.ReauthAsync)는 시작하자마자 PM_BundleManager.IsTitleScene() 같은
        /// Unity API를 동기적으로 건드린다. 여기 호출부(ReconnectCoreAsync)는 ConfigureAwait(false)
        /// 체인이라 스레드풀에서 돌고 있을 수 있으므로, 핸들러의 시작만큼은 Pump()가 도는 메인 스레드로
        /// 넘겨서 호출한다 (그 이후 await는 핸들러 쪽 SynchronizationContext 캡처에 맡긴다).
        /// </summary>
        private Task<bool> InvokeOnMainThreadAsync(Func<CancellationToken, Task<bool>> handler, CancellationToken ct)
        {
            var tcs = new TaskCompletionSource<bool>(TaskCreationOptions.RunContinuationsAsynchronously);
            _mainThreadActions.Enqueue(() => RunOnMainThread(handler, ct, tcs));
            return tcs.Task;
        }

        private static async void RunOnMainThread(Func<CancellationToken, Task<bool>> handler, CancellationToken ct, TaskCompletionSource<bool> tcs)
        {
            try { tcs.TrySetResult(await handler(ct)); }
            catch (OperationCanceledException) { tcs.TrySetCanceled(); }
            catch (Exception ex) { tcs.TrySetException(ex); }
        }

        public Task CloseAsync()
        {
            _pending.CompleteAllWithNull();
            DrainInbox();
            SetPhase(SessionPhase.Offline);
            return _link.CloseAsync();
        }

        public async ValueTask DisposeAsync()
        {
            _link.PacketReceived -= OnPacketReceived;
            _link.Faulted -= OnLinkFaulted;
            await CloseAsync().ConfigureAwait(false);
            await _link.DisposeAsync().ConfigureAwait(false);
        }

        // ---------------------------------------------------------------------
        // 송신
        // ---------------------------------------------------------------------

        /// <summary>응답을 기다리지 않는 송신.</summary>
        public bool Post(PacketID id, object message)
            => PostRaw(id, Serialize(message), 0);

        public bool PostRaw(PacketID id, byte[] payload, uint nonce = 0)
        {
            bool ok = _link.TrySend((uint)id, payload ?? Array.Empty<byte>(), nonce);
            if (!ok) Emit($"send dropped, link not ready: {id}");
            else if (_opt.VerboseLog && !IsHeartbeat(id)) Emit($"[SND] {id}");
            return ok;
        }

        /// <summary>요청 후 응답 대기. 응답 ID 가 요청 ID 와 다를 수 있으면 extraKeys 로 지정.</summary>
        public Task<PktBase> RequestAsync(PacketID id, object message,
                                          IReadOnlyList<PacketID> extraKeys = null,
                                          CancellationToken ct = default,
                                          int receiveBatchPauseToken = 0)
            => RequestRawAsync(id, Serialize(message), extraKeys, ct, receiveBatchPauseToken);

        public async Task<PktBase> RequestRawAsync(PacketID id, byte[] payload,
                                                   IReadOnlyList<PacketID> extraKeys = null,
                                                   CancellationToken ct = default,
                                                   int receiveBatchPauseToken = 0)
        {
            PendingRequestTable<PktBase>.Completion c =
                await RequestWithIdAsync(
                    id, payload, extraKeys, ct, receiveBatchPauseToken).ConfigureAwait(false);
            return c.Response;
        }

        /// <summary>어떤 ID 로 응답이 왔는지도 필요할 때 (기존 IDBase 반환 오버로드 대응).</summary>
        public async Task<PendingRequestTable<PktBase>.Completion> RequestWithIdAsync(
            PacketID id, byte[] payload, IReadOnlyList<PacketID> extraKeys, CancellationToken ct,
            int receiveBatchPauseToken = 0)
        {
            uint nonce = _opt.CorrelateByNonce ? NextNonce() : 0u;
            uint[] keys = BuildKeys(id, extraKeys, nonce);

            using (var requestCts = CancellationTokenSource.CreateLinkedTokenSource(ct))
            {
                Task<PendingRequestTable<PktBase>.Completion> wait =
                    _pending.WaitAsync(
                        keys,
                        TimeSpan.FromMilliseconds(_opt.RequestTimeoutMs),
                        requestCts.Token,
                        receiveBatchPauseToken);

                if (!PostRaw(id, payload, nonce))
                {
                    requestCts.Cancel();
                    try { await wait.ConfigureAwait(false); }
                    catch (OperationCanceledException) { }
                    return default; // 링크 미준비 — 호출부는 Response == null 로 처리
                }

                return await wait.ConfigureAwait(false);
            }
        }

        public int ArmReceiveBatchPause() => _receiveBatchGate.Arm();

        public bool ResumeReceiveBatch(int token) => _receiveBatchGate.Resume(token);

        private uint[] BuildKeys(PacketID id, IReadOnlyList<PacketID> extra, uint nonce)
        {
            if (_opt.CorrelateByNonce) return new[] { nonce };

            int n = 1 + (extra?.Count ?? 0);
            var keys = new uint[n];
            keys[0] = (uint)id;
            for (int i = 0; i < n - 1; i++) keys[i + 1] = (uint)extra[i];
            return keys;
        }

        private uint NextNonce()
        {
            // 0 은 "상관 ID 없음" 예약값이므로 건너뛴다.
            uint v;
            do { v = unchecked((uint)Interlocked.Increment(ref _nonceSeed)); } while (v == 0);
            return v;
        }

        private static byte[] Serialize(object message)
        {
            if (message == null) return Array.Empty<byte>();
            if (message is byte[] raw) return raw;
            if (message is IMessage protobuf) return protobuf.ToByteArray();
            throw new ArgumentException("A protobuf message is required.", nameof(message));
        }

        // ---------------------------------------------------------------------
        // 수신 (IO 스레드)
        // ---------------------------------------------------------------------

        private void OnPacketReceived(InboundPacket packet)
        {
            MarkServerAlive();

            // 하트비트는 Unity 를 건드리지 않으므로 IO 스레드에서 즉시 응답한다.
            // (기존에는 폴링 스레드를 한 번 거쳐 최대 10ms 가 더 붙었다)
            PacketID id = (PacketID)packet.Command;
            if (id == PacketID.PacketLinkProbe)
            {
                HandleHeartbeat(packet);
                return;
            }

            // Echo는 생존 확인용 단방향 패킷이다. 다시 응답하면 ping-pong 루프가 생길 수 있다.
            if (id == PacketID.PacketLinkEcho)
                return;

            _inbox.Enqueue(packet);
        }

        private void HandleHeartbeat(InboundPacket packet)
        {
            if (packet.Payload.Length == 0) return; // 빈 ping 에 응답하면 서버가 backping 루프에 빠진다

            long hb = 0;
            if (!TryDecodeHeartbeat(packet.Payload, ref hb) || hb == 0) return;

            _heartbeatReply.HbVal = hb;
            PostRaw(PacketID.PacketLinkProbe, _heartbeatReply.ToByteArray());
        }

        private static bool TryDecodeHeartbeat(byte[] body, ref long hbVal)
        {
            // 서버가 PktBase 로 한 번 감싸 보내는 경우와 raw 로 보내는 경우 둘 다 지원.
            try
            {
                PktBase wrapped = PktBase.Parser.ParseFrom(body);
                if (wrapped?.Payload != null && wrapped.Payload.Length > 0)
                {
                    HeartbeatRQ inner = HeartbeatRQ.Parser.ParseFrom(wrapped.Payload);
                    if (inner != null) { hbVal = inner.HbVal; return true; }
                }
            }
            catch { }

            try
            {
                HeartbeatRQ direct = HeartbeatRQ.Parser.ParseFrom(body);
                if (direct != null) { hbVal = direct.HbVal; return true; }
            }
            catch { }

            return false;
        }

        private void OnLinkFaulted(LinkFault fault)
        {
            Emit($"link fault: {fault}");

            // IO 스레드이므로 메인 스레드로 넘긴다.
            _mainThreadActions.Enqueue(() =>
            {
                _pending.CompleteAllWithNull();

                if (!fault.IsRecoverable)
                {
                    RaiseFatal(FatalReason.ProtocolError);
                    return;
                }

                if (_phase == SessionPhase.Online || _phase == SessionPhase.Connecting)
                {
                    if (_opt.ReconnectEnabled)
                        _ = EnsureOnlineAsync();
                    else
                        RaiseNetworkFatal();
                }
            });
        }

        // ---------------------------------------------------------------------
        // 메인 스레드 펌프
        // ---------------------------------------------------------------------

        /// <summary>MonoBehaviour.Update 에서 매 프레임 호출.</summary>
        public void Pump()
        {
            // 인박스(수신 패킷)를 mainThreadActions보다 먼저 처리한다.
            // 서버는 PacketAccessDuplicateClose 등을 보낸 뒤 소켓을 닫으므로, 그 패킷은
            // 항상 링크 fault보다 먼저 도착한다. 순서를 반대로 하면 아직 Dispatch 되지 않은
            // PacketAccessDuplicateClose 로 Fatal 전환되기 전에, 이미 큐잉된 링크 fault의
            // 재접속 시도(EnsureOnlineAsync → 재로그인)가 먼저 실행되어 중복 로그인으로
            // 끊긴 세션을 그대로 재로그인해버리는 레이스가 생긴다.
            if (!_receiveBatchGate.IsPaused)
            {
                int processed = 0;
                while (processed < _opt.MaxPacketsPerFrame &&
                       _inbox.TryDequeue(out InboundPacket packet))
                {
                    bool shouldYield = Dispatch(packet);
                    processed++;

                    if (shouldYield)
                        break;
                }
            }

            while (_mainThreadActions.TryDequeue(out Action action))
            {
                try { action(); } catch (Exception ex) { Emit($"main-thread action threw: {ex}"); }
            }

            PumpDeadLinkWatchdog();
        }

        private void PumpDeadLinkWatchdog()
        {
            // 워치독: 서버 트래픽이 끊기면 재접속.
            if (_phase == SessionPhase.Online && !IsServerAliveRecently(_opt.DeadLinkMs))
            {
                Emit($"dead link detected (no traffic for {_opt.DeadLinkMs}ms)");
                MarkServerAlive(); // 중복 트리거 방지
                if (_opt.ReconnectEnabled)
                    _ = EnsureOnlineAsync();
                else
                    RaiseNetworkFatal();
            }
        }

        private bool Dispatch(InboundPacket packet)
        {
            var id = (PacketID)packet.Command;

            PktBase pkt = null;
            if (packet.Payload != null)
            {
                try
                {
                    pkt = PktBase.Parser.ParseFrom(packet.Payload);
                    if (pkt.PayloadSize != pkt.Payload.Length)
                        throw new InvalidDataException("Protobuf payload length mismatch.");
                }
                catch (Exception ex)
                {
                    Emit($"decode failed id={id}({packet.Command}) len={packet.Payload.Length}: {ex.GetType().Name}");
                    RaiseFatal(FatalReason.ProtocolError);
                    return false;
                }
            }

            if (pkt == null) return false;

            if (_opt.VerboseLog && !IsHeartbeat(id)) Emit($"[RCV] {id}");

            // 치명 오류 응답은 대기자에게 먼저 전달하고 나서 세션을 정리한다.
            if (IsFatalResponse(id, pkt))
            {
                uint key = _opt.CorrelateByNonce ? packet.Nonce : (uint)id;
                _pending.TryComplete(key, pkt, out int receiveBatchPauseToken);
                bool shouldYield = _receiveBatchGate.Pause(receiveBatchPauseToken);
                RaiseFatal(id == PacketID.PacketAccessDuplicateClose ? FatalReason.DuplicateSession : FatalReason.SessionFault);
                return shouldYield;
            }

            uint correlationKey = _opt.CorrelateByNonce ? packet.Nonce : (uint)id;
            try { PacketReceived?.Invoke(id, pkt); }
            catch (Exception ex)
            {
                Emit($"packet handler failed ({id}): {ex.GetType().Name}");
                RaiseFatal(FatalReason.ProtocolError);
                return true;
            }
            if (correlationKey != 0 && _pending.TryComplete(correlationKey, pkt, out int receiveBatchPauseTokenOnResponse))
                return _receiveBatchGate.Pause(receiveBatchPauseTokenOnResponse);

            if (id == PacketID.PacketLobbyRedirect)
            {
                // 리다이렉트가 "푸시"로 왔다는 건 = 이 연결 컨텍스트를 곧 버린다는 뜻이고, 이 연결에
                // 다른 응답을 기다리며 걸려있던 대기자(예: GameJoin 의 PacketSpaceEnter 대기)는 이제
                // 다시는 응답을 못 받는다. RequestTimeoutMs(5초) 뒤늦게 깨어나서 스테일 콜백(예: 방금
                // RES_PacketLobbyRedirect 가 새로 로드한 씬을 GameJoin 의 catch 가 다시 언로드)이 도는 걸 막기
                // 위해, 여기서 즉시 null 로 깨운다 (호출부는 이미 null 응답을 정상 처리하도록 되어있다).
                _pending.CompleteAllWithNull();
            }

            // 대기자 없음 → 서버 푸시
            try { PacketPushed?.Invoke(id, pkt); }
            catch (Exception ex) { Emit($"push handler threw ({id}): {ex}"); }
            return false;
        }

        // PktBase.ErrKind 는 서버 구조체와 동일하게 int — 여기서만 ResultCode 로 캐스팅해서 비교.
        private static bool IsFatalResponse(PacketID id, PktBase pkt)
        {
            if (id == PacketID.PacketCoreFault || id == PacketID.PacketAccessDuplicateClose) return true;
            if (pkt == null) return false;
            var errKind = (ResultCode)pkt.ErrKind;
            return errKind == ResultCode.ResultUnexpectedCondition
                || errKind == ResultCode.ResultSessionFault
                || errKind == ResultCode.ResultSessionExpired;
        }

        private static bool IsHeartbeat(PacketID id)
            => id == PacketID.PacketLinkProbe || id == PacketID.PacketLinkEcho || id == PacketID.PacketNodeHeartbeat;

        // ---------------------------------------------------------------------
        // 상태
        // ---------------------------------------------------------------------

        /// <summary>
        /// ConnectAsync/ReconnectCoreAsync 내부는 ConfigureAwait(false) 라서 이 함수가 스레드풀
        /// 스레드에서 불릴 수 있다. PhaseChanged 구독자(NtManager.OnPhaseChanged)가 PopupManager 같은
        /// Unity API를 바로 건드리기 때문에, 이벤트 호출 자체는 Pump()가 도는 메인 스레드로 미룬다.
        /// (재현: get_gameObject can only be called from the main thread — 로딩 팝업이 안 닫혀서 무한로딩)
        /// </summary>
        private void SetPhase(SessionPhase next)
        {
            if (_phase == next) return;
            if (_phase == SessionPhase.Fatal && next != SessionPhase.Offline) return; // Fatal 은 명시적 리셋으로만 벗어난다

            SessionPhase prev = _phase;
            _phase = next;
            Emit($"phase {prev} -> {next}");
            _mainThreadActions.Enqueue(() =>
            {
                try { PhaseChanged?.Invoke(next); } catch (Exception ex) { Emit($"PhaseChanged threw: {ex}"); }
            });
        }

        public void ResetFatal()
        {
            if (_phase != SessionPhase.Fatal) return;
            _phase = SessionPhase.Offline;
            SessionPhase next = _phase;
            _mainThreadActions.Enqueue(() =>
            {
                try { PhaseChanged?.Invoke(next); } catch (Exception ex) { Emit($"PhaseChanged threw: {ex}"); }
            });
        }

        private void RaiseFatal(FatalReason reason)
        {
            if (_phase == SessionPhase.Fatal) return;
            _ = _link.CloseAsync();
            _pending.CompleteAllWithNull();
            DrainInbox();
            SetPhase(SessionPhase.Fatal);
            _mainThreadActions.Enqueue(() =>
            {
                try { Fatal?.Invoke(reason); } catch (Exception ex) { Emit($"Fatal handler threw: {ex}"); }
            });
        }

        private void RaiseNetworkFatal()
        {
            if (_phase == SessionPhase.Fatal)
                return;

            RaiseFatal(FatalReason.NetworkError);
            _ = _link.CloseAsync();
        }

        /// <summary>중복 로그인 등 게임 로직에서 판단한 치명 상황을 알릴 때.</summary>
        public void RaiseFatalExternal(FatalReason reason) => RaiseFatal(reason);

        private void DrainInbox()
        {
            _receiveBatchGate.Reset();
            while (_inbox.TryDequeue(out _)) { }
        }

        private sealed class ReceiveBatchGate
        {
            private readonly object _gate = new object();
            private int _nextToken;
            private int _pausedToken;
            private int _resumedToken;

            public bool IsPaused
            {
                get
                {
                    lock (_gate)
                        return _pausedToken != 0;
                }
            }

            public int Arm()
            {
                int token = Interlocked.Increment(ref _nextToken);
                if (token == 0)
                    token = Interlocked.Increment(ref _nextToken);
                return token;
            }

            public bool Pause(int token)
            {
                if (token == 0)
                    return false;

                lock (_gate)
                {
                    if (_resumedToken == token)
                    {
                        _resumedToken = 0;
                        return false;
                    }

                    if (_pausedToken != 0)
                        return false;

                    _pausedToken = token;
                    return true;
                }
            }

            public bool Resume(int token)
            {
                if (token == 0)
                    return false;

                lock (_gate)
                {
                    if (_pausedToken == token)
                    {
                        _pausedToken = 0;
                        return true;
                    }

                    // Task continuation이 Pause보다 먼저 실행된 경우를 기억한다.
                    _resumedToken = token;
                    return false;
                }
            }

            public void Reset()
            {
                lock (_gate)
                {
                    _pausedToken = 0;
                    _resumedToken = 0;
                }
            }
        }

        private void MarkServerAlive() => Interlocked.Exchange(ref _lastServerTickMs, NowMs);

        private static long NowMs => DateTimeOffset.UtcNow.ToUnixTimeMilliseconds();

        private void Emit(string msg) => Log?.Invoke($"[NetClient] {msg}");

        private static readonly Task<bool> TrueTask = Task.FromResult(true);
        private static readonly Task<bool> FalseTask = Task.FromResult(false);
    }
}

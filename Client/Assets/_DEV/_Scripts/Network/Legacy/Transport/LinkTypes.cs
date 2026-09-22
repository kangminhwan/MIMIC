// =============================================================================
// TcpLink 이 사용하는 값 타입 / 옵션 / 오류 모델.
//
// 기존 Session.ChunkType(30여 개, 값이 118/47/6/132... 로 임의 배정) 을 대체한다.
// 그 열거형은 로그·연결·수신·오류가 한 축에 섞여 있어서 어떤 값이 "정상 흐름"이고
// 어떤 값이 "치명적 오류"인지 타입으로 구분할 수 없었다.
// 여기서는 상태(LinkState) / 데이터(InboundPacket) / 오류(LinkFault) 를 분리한다.
// =============================================================================

using System;
using Ayve.Net.Wire;

namespace Ayve.Net.Transport
{
    public enum LinkState
    {
        Disconnected = 0,
        Connecting,
        Connected,
        Closing,
        Faulted,
    }

    public enum LinkFaultKind
    {
        None = 0,
        /// <summary>DNS 실패 / 후보 주소 없음</summary>
        ResolveFailed,
        /// <summary>connect 타임아웃</summary>
        ConnectTimeout,
        /// <summary>connect 거부/실패</summary>
        ConnectFailed,
        /// <summary>서버가 정상 종료(0바이트 수신)</summary>
        RemoteClosed,
        /// <summary>소켓 오류 (reset, 네트워크 끊김 등)</summary>
        SocketError,
        /// <summary>magic 불일치 / 길이 초과 — 스트림 동기 깨짐</summary>
        ProtocolError,
        /// <summary>MaxPayloadBytes 를 넘는 패킷 도착</summary>
        FrameTooLarge,
        /// <summary>송신 실패</summary>
        SendFailed,
    }

    public readonly struct LinkFault
    {
        public readonly LinkFaultKind Kind;
        public readonly string Message;
        public readonly Exception Exception;

        public LinkFault(LinkFaultKind kind, string message, Exception ex = null)
        {
            Kind = kind;
            Message = message ?? string.Empty;
            Exception = ex;
        }

        /// <summary>재접속을 시도해볼 만한 오류인지 (프로토콜 파손은 재시도해도 무의미).</summary>
        public bool IsRecoverable =>
            Kind != LinkFaultKind.ProtocolError && Kind != LinkFaultKind.FrameTooLarge;

        public override string ToString() =>
            Exception == null ? $"{Kind}: {Message}" : $"{Kind}: {Message} ({Exception.GetType().Name})";
    }

    /// <summary>
    /// 수신 패킷 1개. Payload 는 이 패킷 전용으로 할당된 배열이며 호출자가 자유롭게 보관해도 된다
    /// (풀에서 빌려준 것이 아님). 길이 0 이면 Array.Empty 를 돌려주므로 null 검사는 불필요.
    /// </summary>
    public readonly struct InboundPacket
    {
        public readonly uint Command;
        public readonly uint Nonce;
        public readonly uint Entity;
        public readonly byte[] Payload;

        public InboundPacket(uint command, uint nonce, uint entity, byte[] payload)
        {
            Command = command;
            Nonce = nonce;
            Entity = entity;
            Payload = payload ?? Array.Empty<byte>();
        }
    }

    public sealed class TcpLinkOptions
    {
        /// <summary>connect 1회 시도 제한 (ms). 기존 TCP_CONNECT_TIME_OUT_MILLISECOND 와 동일 기본값.</summary>
        public int ConnectTimeoutMs = 8000;

        /// <summary>수신 누적 버퍼 초기 크기. 큰 패킷이 오면 MaxPayloadBytes 까지 자동 확장.</summary>
        public int InitialReceiveBufferBytes = 64 * 1024;

        /// <summary>
        /// 허용 최대 페이로드. 기존 구현은 64KB 고정 버퍼라 이보다 큰 패킷이 오면
        /// 영원히 완성되지 않고 조용히 멈췄다. 여기서는 초과 시 명시적 FrameTooLarge 오류.
        /// </summary>
        public uint MaxPayloadBytes = 4 * 1024 * 1024;

        public int SocketSendBufferBytes = 64 * 1024;
        public int SocketReceiveBufferBytes = 64 * 1024;

        /// <summary>close 시 송/수신 루프 종료를 기다리는 상한 (ms).</summary>
        public int ShutdownGraceMs = 2000;

        /// <summary>IPv6 주소를 먼저 시도할지 (iOS IPv6-only 망 대응). 실패하면 IPv4 로 넘어간다.</summary>
        public bool PreferIPv6 = true;

        public IPayloadCipher Cipher = new XorCipher(0xA7);

        /// <summary>진단 로그 싱크. Unity 의존을 없애기 위해 델리게이트로 받는다.</summary>
        public Action<string> Log;
    }
}

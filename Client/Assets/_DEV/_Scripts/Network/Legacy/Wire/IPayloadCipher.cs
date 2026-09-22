// =============================================================================
// IPayloadCipher — 페이로드 난독화/암호화 교체 지점.
//
// 기존 NetPacket 의 문제
//   - 상수 1바이트 XOR(0xA7) 을 "암호화"로 취급.
//   - EncryptPayload() 가 호출자가 넘긴 byte[] 를 in-place 로 뒤집었다.
//     같은 배열을 두 번 보내면 XOR 이 상쇄되어 평문이 그대로 나간다.
//
// 새 구조에서는 TcpLink 가 항상 자기 소유의 송신 버퍼에 payload 를 복사한 뒤
// 그 복사본에만 Encrypt 를 적용한다. 호출자 버퍼는 절대 건드리지 않는다.
//
// 실서비스라면 XorCipher 를 그대로 두지 말고 TLS(SslStream) 또는 세션키 기반
// 스트림 암호로 교체할 것. 이 인터페이스가 그 교체를 1파일 작업으로 만든다.
// =============================================================================

using System;

namespace Ayve.Net.Wire
{
    public interface IPayloadCipher
    {
        void Encrypt(Span<byte> payload);
        void Decrypt(Span<byte> payload);
    }

    /// <summary>서버와 현재 합의된 1바이트 XOR. 호환용이며 보안 기능이 아니다.</summary>
    public sealed class XorCipher : IPayloadCipher
    {
        private readonly byte _mask;

        public XorCipher(byte mask = 0xA7) => _mask = mask;

        public void Encrypt(Span<byte> payload) => Apply(payload);
        public void Decrypt(Span<byte> payload) => Apply(payload);

        private void Apply(Span<byte> b)
        {
            for (int i = 0; i < b.Length; i++)
                b[i] ^= _mask;
        }
    }

    public sealed class NullCipher : IPayloadCipher
    {
        public static readonly NullCipher Instance = new NullCipher();
        public void Encrypt(Span<byte> payload) { }
        public void Decrypt(Span<byte> payload) { }
    }
}

// =============================================================================
// PacketHeader — 24 byte 고정 헤더.
//
// 기존 NetPacket.Header 대비 변경점
//   1. BitConverter.GetBytes(4byte 배열 6개 할당) 제거 → BinaryPrimitives + Span
//   2. Identity(magic) 검증 추가        (기존: 검증 없음)
//   3. PayloadSize 상한 검증 추가       (기존: 손상된 길이로 임의 크기 할당 가능)
//   4. readonly struct                  (기존: 가변 struct, 부분 초기화 생성자 2종)
//
// 와이어 포맷은 서버와 동일하게 유지한다 (필드 순서/오프셋 불변).
//   [0]  Identity   uint32  = 0x6B2E
//   [4]  Entity     uint32
//   [8]  Nonce      uint32  ← 요청/응답 상관 ID 로 사용 (기존: 항상 0)
//   [12] Command    uint32  = PacketID
//   [16] PayloadSize uint32
//   [20] Sequence   uint32  (기존 PacketNum)
// =============================================================================

using System;
using System.Buffers.Binary;

namespace Ayve.Net.Wire
{
    public enum HeaderError
    {
        None = 0,
        BadMagic,
        PayloadTooLarge,
    }

    public readonly struct PacketHeader
    {
        public const int Size = 24;
        public const uint Magic = 0x6B2E;

        public readonly uint Identity;
        public readonly uint Entity;
        public readonly uint Nonce;
        public readonly uint Command;
        public readonly uint PayloadSize;
        public readonly uint Sequence;

        public PacketHeader(uint command, uint payloadSize, uint nonce = 0, uint entity = 0, uint sequence = 0)
        {
            Identity = Magic;
            Entity = entity;
            Nonce = nonce;
            Command = command;
            PayloadSize = payloadSize;
            Sequence = sequence;
        }

        private PacketHeader(uint identity, uint entity, uint nonce, uint command, uint payloadSize, uint sequence)
        {
            Identity = identity;
            Entity = entity;
            Nonce = nonce;
            Command = command;
            PayloadSize = payloadSize;
            Sequence = sequence;
        }

        /// <summary>헤더 + 페이로드 전체 바이트 수.</summary>
        public int TotalSize => Size + (int)PayloadSize;

        public void Write(Span<byte> dst)
        {
            if (dst.Length < Size)
                throw new ArgumentException($"header buffer too small ({dst.Length} < {Size})", nameof(dst));

            BinaryPrimitives.WriteUInt32LittleEndian(dst.Slice(0, 4), Identity);
            BinaryPrimitives.WriteUInt32LittleEndian(dst.Slice(4, 4), Entity);
            BinaryPrimitives.WriteUInt32LittleEndian(dst.Slice(8, 4), Nonce);
            BinaryPrimitives.WriteUInt32LittleEndian(dst.Slice(12, 4), Command);
            BinaryPrimitives.WriteUInt32LittleEndian(dst.Slice(16, 4), PayloadSize);
            BinaryPrimitives.WriteUInt32LittleEndian(dst.Slice(20, 4), Sequence);
        }

        /// <summary>
        /// src 는 최소 Size 바이트를 담고 있어야 한다 (호출부가 보장).
        /// magic / 길이 검증에 실패하면 false 를 돌려준다 — 이 경우 스트림이 어긋난 것이므로
        /// 호출부는 연결을 끊어야 한다. 기존 구현은 이 검증이 없어서 손상된 길이 필드 하나로
        /// 무한 대기하거나 임의 크기를 할당했다.
        /// </summary>
        public static bool TryRead(ReadOnlySpan<byte> src, uint maxPayload,
                                   out PacketHeader header, out HeaderError error)
        {
            header = default;

            uint identity = BinaryPrimitives.ReadUInt32LittleEndian(src.Slice(0, 4));
            if (identity != Magic)
            {
                error = HeaderError.BadMagic;
                return false;
            }

            uint payload = BinaryPrimitives.ReadUInt32LittleEndian(src.Slice(16, 4));
            if (payload > maxPayload)
            {
                error = HeaderError.PayloadTooLarge;
                return false;
            }

            header = new PacketHeader(
                identity,
                BinaryPrimitives.ReadUInt32LittleEndian(src.Slice(4, 4)),
                BinaryPrimitives.ReadUInt32LittleEndian(src.Slice(8, 4)),
                BinaryPrimitives.ReadUInt32LittleEndian(src.Slice(12, 4)),
                payload,
                BinaryPrimitives.ReadUInt32LittleEndian(src.Slice(20, 4)));

            error = HeaderError.None;
            return true;
        }

        public override string ToString()
            => $"cmd={Command} nonce={Nonce} size={PayloadSize} seq={Sequence}";
    }
}

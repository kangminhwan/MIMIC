// =============================================================================
// PendingRequestTable — 요청/응답 상관관계 관리.
//
// 기존 문제
//   Dictionary<PacketID, Action<PktBase>> 하나로 관리해서 같은 ID 를 동시에 두 번
//   보내면 앞의 awaiter 가 사라졌다. 그걸 막으려고 "이전 awaiter 무력화" 로직이
//   3개 오버로드에 각각 복붙되어 있었고, 그 안에서 ReferenceEquals 비교로
//   자기 콜백인지 확인하는 코드까지 있었다. 락도 없었다.
//
// 새 구조
//   - 키(PacketID)마다 waiter 를 FIFO 큐로 보관 → 동시 다중 요청 정상 동작.
//   - 하나의 waiter 를 여러 키에 동시 등록 가능 (기존 callbackId / List<PacketID> 대응).
//     먼저 도착한 응답이 이기고, 나머지 키에서는 자동으로 빠진다.
//   - 모든 접근을 단일 락으로 보호.
//   - 타임아웃은 CancellationTokenSource.CancelAfter 로 처리하고 반드시 Dispose.
//     (기존: 요청마다 Task.Delay(5000) 을 취소 없이 생성 → 타이머 누수)
//
// Nonce 모드
//   서버가 요청 헤더의 Nonce 를 응답에 그대로 에코해 주면 CorrelateByNonce = true 로
//   두는 것이 정확하다 (같은 ID 동시 요청이 100% 안전해진다).
//   서버 수정 전까지는 false 로 두고 PacketID FIFO 로 동작한다.
// =============================================================================

using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;

namespace Ayve.Net.Core
{
    public sealed class PendingRequestTable<TResponse> where TResponse : class
    {
        public readonly struct Completion
        {
            public readonly uint Key;
            public readonly TResponse Response;
            public Completion(uint key, TResponse response) { Key = key; Response = response; }
        }

        private sealed class Waiter
        {
            public readonly TaskCompletionSource<Completion> Tcs =
                new TaskCompletionSource<Completion>(TaskCreationOptions.RunContinuationsAsynchronously);
            public uint[] Keys;
            public int ReceiveBatchPauseToken;
            public int Settled;
        }

        private readonly object _gate = new object();
        private readonly Dictionary<uint, LinkedList<Waiter>> _byKey = new Dictionary<uint, LinkedList<Waiter>>();

        public int Count { get { lock (_gate) { return _byKey.Count; } } }

        /// <summary>
        /// keys 중 아무 응답이나 먼저 오는 것을 기다린다.
        /// 타임아웃 시 TimeoutException, 외부 취소 시 OperationCanceledException.
        /// </summary>
        public async Task<Completion> WaitAsync(uint[] keys, TimeSpan timeout, CancellationToken ct, int receiveBatchPauseToken = 0)
        {
            if (keys == null || keys.Length == 0) throw new ArgumentException("keys required", nameof(keys));

            var waiter = new Waiter
            {
                Keys = keys,
                ReceiveBatchPauseToken = receiveBatchPauseToken,
            };

            lock (_gate)
            {
                for (int i = 0; i < keys.Length; i++)
                {
                    if (!_byKey.TryGetValue(keys[i], out LinkedList<Waiter> list))
                    {
                        list = new LinkedList<Waiter>();
                        _byKey[keys[i]] = list;
                    }
                    list.AddLast(waiter);
                }
            }

            using (var timeoutCts = CancellationTokenSource.CreateLinkedTokenSource(ct))
            {
                timeoutCts.CancelAfter(timeout);
                using (timeoutCts.Token.Register(static state => Cancel((Waiter)state), waiter, false))
                {
                    try
                    {
                        return await waiter.Tcs.Task.ConfigureAwait(false);
                    }
                    catch (OperationCanceledException) when (!ct.IsCancellationRequested)
                    {
                        throw new TimeoutException($"request timed out after {timeout.TotalMilliseconds:F0}ms (keys={string.Join(",", keys)})");
                    }
                    finally
                    {
                        Remove(waiter);
                    }
                }
            }
        }

        /// <summary>응답 전달. 대기자가 없으면 false (= 서버 푸시).</summary>
        public bool TryComplete(uint key, TResponse response)
            => TryComplete(key, response, out _);

        /// <summary>
        /// 응답 전달. receiveBatchPauseToken이 0이 아니면 호출부가 준비될 때까지 수신 Pump를 멈춘다.
        /// </summary>
        public bool TryComplete(uint key, TResponse response, out int receiveBatchPauseToken)
        {
            receiveBatchPauseToken = 0;
            Waiter target = null;

            lock (_gate)
            {
                if (_byKey.TryGetValue(key, out LinkedList<Waiter> list))
                {
                    LinkedListNode<Waiter> node = list.First;
                    while (node != null)
                    {
                        LinkedListNode<Waiter> next = node.Next;
                        if (Volatile.Read(ref node.Value.Settled) == 0) { target = node.Value; break; }
                        list.Remove(node);
                        node = next;
                    }
                }

                if (target == null) return false;
                if (Interlocked.Exchange(ref target.Settled, 1) != 0) return false;
                RemoveLocked(target);
            }

            target.Tcs.TrySetResult(new Completion(key, response));
            receiveBatchPauseToken = target.ReceiveBatchPauseToken;
            return true;
        }

        /// <summary>연결이 끊겼을 때 대기자를 모두 null 응답으로 깨운다 (기존 ClearNetwork 동작 유지).</summary>
        public void CompleteAllWithNull()
        {
            List<Waiter> all = TakeAll();
            for (int i = 0; i < all.Count; i++)
            {
                if (Interlocked.Exchange(ref all[i].Settled, 1) != 0) continue;
                all[i].Tcs.TrySetResult(new Completion(0, null));
            }
        }

        public void FailAll(Exception ex)
        {
            List<Waiter> all = TakeAll();
            for (int i = 0; i < all.Count; i++)
            {
                if (Interlocked.Exchange(ref all[i].Settled, 1) != 0) continue;
                all[i].Tcs.TrySetException(ex);
            }
        }

        private List<Waiter> TakeAll()
        {
            var result = new List<Waiter>();
            lock (_gate)
            {
                foreach (KeyValuePair<uint, LinkedList<Waiter>> kv in _byKey)
                {
                    foreach (Waiter w in kv.Value)
                        if (!result.Contains(w)) result.Add(w);
                }
                _byKey.Clear();
            }
            return result;
        }

        private static void Cancel(Waiter w)
        {
            if (Interlocked.Exchange(ref w.Settled, 1) != 0) return;
            w.Tcs.TrySetCanceled();
        }

        private void Remove(Waiter w)
        {
            lock (_gate) { RemoveLocked(w); }
        }

        private void RemoveLocked(Waiter w)
        {
            uint[] keys = w.Keys;
            for (int i = 0; i < keys.Length; i++)
            {
                if (!_byKey.TryGetValue(keys[i], out LinkedList<Waiter> list)) continue;
                list.Remove(w);
                if (list.Count == 0) _byKey.Remove(keys[i]);
            }
        }
    }
}

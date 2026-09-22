using System;
using System.Threading;
using System.Threading.Tasks;
using Google.Protobuf;
using Mimic.Protocol;
using UnityEngine.Networking;
namespace Mimic.Network
{
    public sealed class ApiException : Exception
    {
        public long StatusCode { get; }
        public string Code { get; }
        public ApiException(long status, string code, string message) : base(message) { StatusCode = status; Code = code; }
    }
    public static class ProtoHttp
    {
        public static async Task<T> SendAsync<T>(string url, MessageParser<T> parser, IMessage body = null,
            string bearerToken = null, string method = null, CancellationToken cancellation = default) where T : IMessage<T>
        {
            using (var request = new UnityWebRequest(url, method ?? (body == null ? "GET" : "POST")))
            {
                request.timeout = 8;
                request.downloadHandler = new DownloadHandlerBuffer();
                request.SetRequestHeader("Accept", "application/x-protobuf");
                if (!string.IsNullOrEmpty(bearerToken)) request.SetRequestHeader("Authorization", "Bearer " + bearerToken);
                if (body != null)
                {
                    request.uploadHandler = new UploadHandlerRaw(body.ToByteArray());
                    request.SetRequestHeader("Content-Type", "application/x-protobuf");
                }
                var operation = request.SendWebRequest();
                while (!operation.isDone)
                {
                    if (cancellation.IsCancellationRequested) { request.Abort(); cancellation.ThrowIfCancellationRequested(); }
                    await Task.Yield();
                }
                cancellation.ThrowIfCancellationRequested();
                var bytes = request.downloadHandler.data;
                if (bytes.Length > 65536) throw new InvalidOperationException("서버 응답 크기가 너무 큽니다.");
                if (request.result != UnityWebRequest.Result.Success)
                {
                    string code = "CONNECTION_FAILED", message = "서버에 연결할 수 없습니다. 서버 실행 상태를 확인해 주세요.";
                    try { var error = ErrorReply.Parser.ParseFrom(bytes); if (!string.IsNullOrWhiteSpace(error.Message)) { code = error.Code; message = error.Message; } }
                    catch (InvalidProtocolBufferException) { }
                    throw new ApiException(request.responseCode, code, message);
                }
                return parser.ParseFrom(bytes);
            }
        }
    }
}

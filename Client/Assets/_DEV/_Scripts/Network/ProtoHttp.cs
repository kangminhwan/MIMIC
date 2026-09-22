using System;
using System.Threading.Tasks;
using Google.Protobuf;
using UnityEngine.Networking;
namespace Mimic.Network
{
    public static class ProtoHttp
    {
        public static async Task<T> SendAsync<T>(string url, MessageParser<T> parser, IMessage body = null) where T : IMessage<T>
        {
            using (var request = new UnityWebRequest(url, body == null ? "GET" : "POST"))
            {
                request.timeout = 5;
                request.downloadHandler = new DownloadHandlerBuffer();
                request.SetRequestHeader("Accept", "application/x-protobuf");
                if (body != null)
                {
                    request.uploadHandler = new UploadHandlerRaw(body.ToByteArray());
                    request.SetRequestHeader("Content-Type", "application/x-protobuf");
                }
                var operation = request.SendWebRequest();
                while (!operation.isDone) await Task.Yield();
                if (request.result != UnityWebRequest.Result.Success)
                    throw new InvalidOperationException("HTTP " + request.responseCode + ": " + request.error);
                if (request.downloadHandler.data.Length > 65536) throw new InvalidOperationException("HTTP response too large");
                return parser.ParseFrom(request.downloadHandler.data);
            }
        }
    }
}

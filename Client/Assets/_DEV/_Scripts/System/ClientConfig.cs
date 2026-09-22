using System;
using UnityEngine;
namespace Mimic.Systems
{
    [Serializable]
    public sealed class ClientConfig
    {
        public string frontUrl = "http://127.0.0.1:5080";
        public static ClientConfig Load()
        {
            var asset = Resources.Load<TextAsset>("Config/client");
            return asset == null ? new ClientConfig() : JsonUtility.FromJson<ClientConfig>(asset.text);
        }
    }
}

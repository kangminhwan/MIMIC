using System;
using UnityEngine;
namespace Mimic.Systems
{
    [Serializable]
    public sealed class ClientConfig
    {
        public bool nativeTableServer = true;
        public string tableHost = "127.0.0.1";
        public int tablePort = 22001;
        public string holdemChannel = "Holdem_Chip_10K";
        public int holdemBetPolicy = 4;
        public string appVersion = "1.0.0";
        public int storeChannel = 4;
        public string frontUrl = "http://127.0.0.1:5080";
        public static ClientConfig Load()
        {
            var asset = Resources.Load<TextAsset>("Config/client");
            return asset == null ? new ClientConfig() : JsonUtility.FromJson<ClientConfig>(asset.text);
        }
    }
}

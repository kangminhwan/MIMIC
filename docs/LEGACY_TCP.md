# Legacy TCP integration

The user requested keeping the previous TCP connection layer. The Unity transport source is copied from `C:/NewClient/casino/Assets/_DEV/_Scripts/Network/Net/` into `Client/Assets/_DEV/_Scripts/Network/Legacy/`:

- `Transport/TcpLink.cs`: asynchronous sockets, pooled frame buffers, receive accumulation, partial-send handling, shutdown.
- `Transport/LinkTypes.cs`: options, connection states, events.
- `Wire/PacketHeader.cs`: original six-word header and magic.
- `Wire/IPayloadCipher.cs`: original XOR payload transform.

The original files and Korean comments are preserved; the references are not edited. Their `Ayve.Net` namespace is retained so the imported transport can be compared directly with the source project. `Mimic.Network.NetClient` adds protobuf message dispatch, bounded pending requests, heartbeat and Unity main-thread event pumping on top.

The wire header is **24 bytes, little-endian**: magic `0x6B2E`, entity (reserved zero), nonce, command, payload length, sequence. Only the body is XOR-transformed with `0xA7`, exactly as in the reference. This is compatibility obfuscation, not encryption. Payload is the shared `Envelope` Protobuf message; command is the protobuf oneof field number and nonce equals its request ID. The maximum body size is 64 KiB. Zero nonce means server push. The legacy client leaves sequence zero; the server does not rely on incoming sequence.

The C++ TableServer implements this header/body contract in its Netlib adapter. It does **not** copy the previous entire IOCP, MySQL, Redis and multi-game server dependency tree. Its current bounded Winsock workers remain behind the Netlib boundary. Original MessagePack application packets are not compatible with MIMIC protobuf packets even though the TCP framing is compatible. See `SERVER_AUTH.md` for the C++ reuse assessment and remaining work before production.

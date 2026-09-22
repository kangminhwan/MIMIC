# Server Authentication & Transport

This document covers two independent pieces of server-side work:

1. Persistent account signup/login in `Mimic.PlatformServer` (SQLite-backed).
2. The TCP wire framing used by `Mimic.TableServer`, including an honest assessment of what
   was reused from the legacy C++ `cHeader` / `Netlib` stack and what was not.

## 1. Account storage and HTTP routes

### Storage

- Backend: SQLite via `Microsoft.Data.Sqlite` 8.0.10 (`Server/PlatformServer/Accounts/AccountDatabase.cs`).
- Location: `MIMIC_DATA_DIR/accounts.db`. `MIMIC_DATA_DIR` is read from configuration
  (environment variable or `--MIMIC_DATA_DIR=...` argument). If unset, it defaults to
  `<repo-root>/artifacts/data`, where `<repo-root>` is located by walking up from the running
  assembly's directory until a `.git` folder is found (see `Accounts/DataDirectory.cs`). No
  database file is committed to the repository; `artifacts/data` is created on first run.
- Connection mode: WAL journaling with a 5s busy timeout, one short-lived connection per
  operation. This lets concurrent readers/writers coexist without a custom pool, and lets SQLite's
  own locking serialize concurrent writes (see the concurrency test below).
- Schema:
  ```sql
  CREATE TABLE accounts (
      player_id TEXT PRIMARY KEY,
      account_name TEXT NOT NULL,               -- original casing, as registered
      account_name_normalized TEXT NOT NULL UNIQUE, -- lower-invariant, enforces case-insensitive uniqueness
      display_name TEXT NOT NULL,
      password_hash TEXT NOT NULL,               -- PBKDF2-SHA256$<iterations>$<salt b64>$<hash b64>
      demo_chips INTEGER NOT NULL,
      created_unix INTEGER NOT NULL
  );
  CREATE TABLE sessions (
      token_hash TEXT PRIMARY KEY,                -- SHA-256(session token), never the raw token
      player_id TEXT NOT NULL,
      expires_unix INTEGER NOT NULL,
      created_unix INTEGER NOT NULL
  );
  ```
  Both accounts and sessions survive a process restart because they live in the SQLite file, not
  in memory (verified by an explicit "persistent reload" test).

### Password hashing

- PBKDF2-HMAC-SHA256, 210,000 iterations, 16-byte random salt, 32-byte derived key
  (`Server/PlatformServer/Accounts/PasswordHasher.cs`).
- Stored as `PBKDF2-SHA256$<iterations>$<base64 salt>$<base64 hash>` so the iteration count can
  be raised later without invalidating already-stored hashes.
- Verification re-derives the key with the stored salt/iteration count and compares with
  `CryptographicOperations.FixedTimeEquals`, so failure timing does not leak a partial match.
- Plaintext passwords are never written to storage or logs.
- `Verify` bounds everything it parses out of a stored hash string before using it: iteration
  count in `[1, 2_000_000]`, salt length in `[8, 64]` bytes, key length in `[16, 64]` bytes.
  This guards two distinct failure modes if a database row is ever corrupted or tampered with:
  1. **Zero-length match**: a stored hash with an empty salt *and* an empty key
     (`PBKDF2-SHA256$210000$$`) would otherwise make `Rfc2898DeriveBytes.Pbkdf2` derive a
     zero-length "key", which trivially equals an equally zero-length stored value via
     `FixedTimeEquals` — for *any* password. The `MinKeySize = 16` bound makes this impossible.
  2. **Excessive work / DoS**: an attacker-influenced or corrupted iteration count (e.g.
     `2000000000`) would otherwise make every login attempt against that row spend seconds of CPU
     in PBKDF2. The `MaxIterations = 2_000_000` bound (already ~10x the production value of
     210,000) rejects it immediately instead.
  Any stored value outside these bounds simply fails verification, exactly like any other
  malformed hash — it never gets special-cased into a pass.

### Validation rules (`Accounts/AccountValidation.cs`)

| Field | Rule |
|---|---|
| Account name | `^[A-Za-z0-9_]{3,24}$`, ASCII letters/digits/underscore only |
| Password | length 8-128 (any characters) |
| Display name | 2-24 Unicode characters after trimming, no control characters |

Account name uniqueness is case-insensitive: `Alice`, `ALICE`, and `alice` are the same account.
Lookups and the `UNIQUE` constraint both use `account_name_normalized` (lower-invariant); the
originally-registered casing is preserved in `account_name` for display purposes.

### Sessions

- Opaque bearer tokens: 32 random bytes, hex-encoded (64 characters), generated with
  `RandomNumberGenerator`. The same shape as the pre-existing guest tokens, so the C++
  `mimic::net::validate()` 64-character length check keeps working unchanged.
- Only `SHA-256(token)` is stored server-side (`token_hash`); the raw token is never persisted,
  so a stolen database backup cannot be replayed as a session.
- Expiry defaults to 86,400 seconds (24h) for real accounts, configurable via
  `MIMIC_SESSION_TTL_SECONDS` (used by tests to force near-immediate expiry deterministically).
- `POST /auth/logout` deletes the session row **and** removes a matching in-memory guest token
  (`guestSessions.TryRemove`), so presenting either kind of token to `/auth/logout` fully
  invalidates it. Logged-out tokens fail both `/account/profile` and `/sessions/validate`
  immediately.
- Expired session rows are not purged simply by the passage of time — nothing runs on a timer.
  `IssueSession` calls `AccountDatabase.DeleteExpiredSessions(now)` as a best-effort side effect
  every time a new session is created (register or login), wrapped in a try/catch so a
  housekeeping failure can never block an otherwise-successful login. This bounds how long stale
  rows can accumulate without needing a background job.
- Guest sessions (`POST /auth/guest`, development-only, `404` outside `Development`) are kept as
  a separate in-memory `ConcurrentDictionary`, exactly as before — they intentionally do not
  become persistent accounts and do not survive a restart. `/sessions/validate` checks the guest
  table first, then falls back to the persistent session store, so it keeps working for both the
  old guest-only smoke flow and real accounts (needed by the C++ TableServer's
  `mimic::net::validate()`).

### Login timing (`/auth/login`)

An unknown account name used to short-circuit before ever calling `PasswordHasher.Verify`,
meaning a response for "no such account" returned near-instantly while "wrong password for a real
account" always paid a real PBKDF2 delay — an attacker could use response timing alone to
enumerate valid account names without ever seeing a different status code or error message.

Fixed by always calling `PasswordHasher.Verify` for any syntactically valid login attempt (i.e.
one that already passed `IsValidAccountName`/`IsValidPassword`), against `account.PasswordHash`
when the account exists or `PasswordHasher.DummyHash` — a hash of a random, never-revealed
password computed once per process — when it doesn't. Both paths now do the same PBKDF2 work
before the identical generic `401 INVALID_CREDENTIALS` is returned.

### Routes

| Route | Method | Auth | Success | Notes |
|---|---|---|---|---|
| `/health` | GET | none | 200 JSON | unchanged |
| `/auth/guest` | POST | none | 200 `LoginReply` | dev-only, in-memory, unchanged behavior |
| `/auth/register` | POST | none | 201 `LoginReply` | validates, creates account, auto-logs-in |
| `/auth/login` | POST | none | 200 `LoginReply` | generic 401 on any bad input/credentials |
| `/auth/logout` | POST | `Authorization: Bearer <token>` | 204 | deletes the session row |
| `/account/profile` | GET | `Authorization: Bearer <token>` | 200 `ProfileReply` | |
| `/sessions/validate` | POST | none (token in body) | 200 `SessionReply` | used by the C++ TableServer |

Status codes: register invalid input → 400, duplicate account → 409, bad login/auth → 401. Every
response is `application/x-protobuf` **except two explicit, intentional exceptions**: `GET
/health` (plain JSON, matching its pre-existing `Results.Ok(new {...})` shape) and a successful
`POST /auth/logout` (`204 No Content`, no body — there is nothing to say on success). Every other
response, success or error, carries a protobuf body — including every error path below.

Errors never render as HTML or an empty framework default page:
- **Unhandled exceptions** → a global `UseExceptionHandler` always emits a protobuf `ErrorReply`.
  It distinguishes an oversized body from a genuine server bug: a request over Kestrel's
  `MaxRequestBodySize` surfaces as `BadHttpRequestException` with `StatusCode == 413`, which is
  passed through as `413` + `PAYLOAD_TOO_LARGE`; any other `BadHttpRequestException` becomes `400`
  + `BAD_REQUEST`; anything else becomes a generic `500` + `INTERNAL_ERROR` with no exception
  details or credentials ever included.
- **Rate limiting** → `AddRateLimiter`'s `OnRejected` callback writes a `429` + `RATE_LIMITED`
  `ErrorReply` directly onto the response; without it, the framework's default rejection is an
  empty body, which would have been the one place a client saw a non-protobuf (empty) error.

### Rate limiting

Two fixed-window-per-IP policies (`Server/PlatformServer/PlatformServerApp.cs`):
- `sessions`: 60/min — `/auth/guest`, `/sessions/validate` (unchanged from before).
- `auth`: configurable via `MIMIC_AUTH_RATE_LIMIT_PER_MINUTE` (default 30/min, bounded to
  `[1, 100000]` — see "Configuration validation" below) — `/auth/register`, `/auth/login`,
  `/auth/logout`, `/account/profile`.

Both policies reject with `429 Too Many Requests` and a protobuf `RATE_LIMITED` `ErrorReply`
(never the framework's default empty `503`).

### Configuration validation

`MIMIC_SESSION_TTL_SECONDS` and `MIMIC_AUTH_RATE_LIMIT_PER_MINUTE` are parsed through a bounded
helper (`ResolveBoundedLong` in `PlatformServerApp.cs`) instead of being trusted as-is:
- `MIMIC_SESSION_TTL_SECONDS`: must parse as an integer in `[1, 2_592_000]` (30 days), else falls
  back to `86_400`. Without this, a negative or zero value would issue sessions that are already
  expired the instant they're created (`expires_unix <= now`), effectively breaking login for
  everyone; an absurdly large value would issue effectively-permanent sessions.
- `MIMIC_AUTH_RATE_LIMIT_PER_MINUTE`: must parse as an integer in `[1, 100_000]`, else falls back
  to `30`. Without this, a configured `0` or negative value would make the fixed-window limiter
  reject every single request to `/auth/*` and `/account/profile`, taking the service down.

Missing, unparsable, or out-of-range values are treated identically: use the safe default. This
is deliberately silent (no startup failure) so a bad environment variable degrades to "default
behavior" rather than "service does not start."

## 2. TCP wire framing (`Mimic.TableServer`)

### What was inspected

Per the task, the legacy C++ server tree under `C:\New\server` was inspected read-only:

- `Server/Include/Netlib/Common/cHeader.h` — the 24-byte TCP header: 6 `UINT` (uint32) fields
  `Identity` (magic `0x6B2E`), `Entity`, `Nonce`, `Command`, `Payload` (size), `PacketNum`
  (sequence).
- `Server/Netlib/Network/cPacketStack.cpp` — shows the header is followed immediately by the
  payload, and that `Encrypt`/`Decrypt` XOR every payload byte (never the header) with a single
  constant `_packetMask = 0xA7`, gated behind a `_CRYPT` build flag. The comment in the task and
  in the reference Unity client both agree this is "obfuscation, not encryption."
- `Server/TableServer/XorEncryption.h` — a *different*, unrelated multi-byte XOR
  (`"TopPlayerPoker"`) used elsewhere in that codebase for a different message type. This was
  **not** reused; the 0xA7 single-byte mask from `cHeader`/`cPacketStack` is what the task and the
  reference client (`C:\NewClient\casino`'s `XorCipher`) actually agree on.
- The `Netlib`/`IOCP` directory tree (`cIocp*`, `cSessionManager`, `cDispatcher`, thread pools,
  Redis/WinInet/REST helpers, etc.) is a large, tightly-coupled server framework (session
  management, worker-thread pools, UDP, logging, alerting...) built against a specific legacy
  project layout. Pulling any of it in would mean importing a large, largely unrelated dependency
  tree with no guarantee it builds standalone outside its original solution, for a single 24-byte
  header definition. That tree was **not** imported.
- The Unity-side reference (`C:\NewClient\casino\Assets\_DEV\_Scripts\Network\Net\Wire\PacketHeader.cs`,
  `...\Transport\TcpLink.cs`, `...\Wire\IPayloadCipher.cs`) confirms the exact byte layout,
  offsets, and the `XorCipher(0xA7)` wiring on the client side that the new MIMIC client (owned by
  the coordinator, not this change) is expected to converge on.

### What was actually reused vs. reimplemented

**Reused (exact values/layout, matching the legacy header 1:1):**
- Header size (24 bytes) and field order: `Identity, Entity, Nonce, Command, PayloadSize, Sequence`.
- Magic constant `0x6B2E`.
- The single-byte XOR mask `0xA7`, applied only to the payload.
- The "payload immediately follows the header, size comes from the header" framing shape.

**Reimplemented from scratch (new code, not copied):** the actual read/write logic in
`Server/Netlib/Transport.cpp`. The legacy `cPacketStack`/`cHeader` classes work in terms of a
fixed internal buffer, in/out `BYTE*`, and a build-time `_CRYPT` flag; MIMIC's transport instead
writes/reads the 6 header fields directly into a 24-byte stack buffer with `memcpy` (host is
always little-endian Windows x86/x64, so no byte-swapping is needed — unlike the *previous*
MIMIC framing, which used `htonl`/`ntohl` for its 4-byte big-endian length prefix). No IOCP,
session manager, or thread pool code was pulled in; MIMIC's `TableServer` keeps its existing
one-`std::async`-task-per-connection model (`Server/TableServer/main.cpp`, unchanged).

### New framing (replaces the previous 4-byte length prefix)

24-byte little-endian header, 6×`uint32`, immediately followed by an XOR-masked `Envelope`
protobuf body (max 65536 bytes):

```
offset  field         value
0       identity      0x6B2E  (magic)
4       entity        0       (always 0 — MIMIC does not use this field)
8       nonce         request_id of the Envelope this frame carries, truncated to uint32;
                      0 for server-initiated pushes (e.g. table snapshots)
12      command       (uint32)Envelope.payload_case()  — the oneof field number
16      payload_size  body length in bytes, after XOR
20      sequence      monotonically increasing counter, per connection, incremented on every
                      frame *this side* sends (starts at 1; not required to be gap-free on receive)
```

Body: `Envelope` protobuf bytes, each byte XORed with the constant `0xA7`
(`Server/Netlib/Transport.cpp`, `PayloadXorMask`). This is the same "obfuscation, not encryption"
contract as the legacy header and the reference client's `XorCipher(0xA7)`.

`mimic::net::receive()` rejects (closes the connection) on:
- short read of the 24-byte header or the declared payload,
- `identity != 0x6B2E`,
- `entity != 0` — this contract reserves the field at `0`; MIMIC has no use for it, so any nonzero
  value is treated as a protocol violation rather than silently ignored,
- `payload_size == 0` or `payload_size > 65536`,
- a `request_id` that does not fit in `uint32` (nonce cannot represent it),
- header `nonce != envelope.request_id()`,
- header `command != (uint32)envelope.payload_case()`.

`sequence` is read but never validated on receive (see "Limitations" below — the reference client
sends `0`, and that must keep working).

`Connection::send()` allocates its outgoing `sequence` value (`++sendSequence`) **after**
acquiring `writeMutex`, immediately before the header is written and the frame goes out on the
socket. Two concurrent calls to `send()` on the same connection (e.g. a request's response racing
a broadcast push) now have their sequence numbers assigned in the same order their bytes actually
reach the wire. Allocating the counter before the lock (the original implementation) could let a
thread that grabs the lock second still write the wire's *first* frame, so the sequence numbers
observed by a peer would not match send order.

`Envelope.protocol_version` and "must authenticate first" checks remain at the application layer
in `Server/TableServer/main.cpp` (unchanged) — those produce a structured `ErrorReply` response
instead of a hard disconnect, which is friendlier for legitimate clients that are simply talking
an old protocol version.

### Scope note: no IOCP migration in this or the prior follow-up

Per the original assessment, `Server/Netlib/Transport.cpp` remains a small, from-scratch
implementation of just the wire framing (header read/write, XOR, validation) on top of MIMIC's
existing one-`std::async`-task-per-connection model. This follow-up's changes (write-lock-ordered
sequence allocation, entity rejection) are confined to that same file; nothing from the legacy
`Netlib`/`IOCP` tree (session manager, worker-thread pools, dispatcher, etc.) was pulled in, and
no such migration was in scope for this task.

### Client integration status

As of this follow-up, the coordinator reports the Unity client now uses the actual, unchanged
legacy `TcpLink`/`PacketHeader`/`XorCipher` source and wraps `Envelope` protobuf correctly — i.e.
the client side of the 24-byte header + XOR(0xA7) contract described above is in place and UI
scenes compile. `tests/Smoke/Program.cs` is outside this change's ownership and was not inspected
or modified in this follow-up; whether it has been updated to the new framing was not verified
from this session — treat its status as unknown until confirmed.

### Authorization boundary (read before assuming this is secure end-to-end)

TCP-level authorization is checked **once, at connection time**: `mimic::net::validate()` runs
only when a connection sends its `Authenticate` request (`Server/TableServer/main.cpp`,
`E::kAuthenticate` branch). On success, the connection's `playerId`/`displayName` are cached for
the lifetime of that TCP connection; no subsequent request on that same connection re-checks the
session against `Mimic.PlatformServer`.

Consequences:
- A well-behaved client that calls `/auth/logout` and then closes its TCP connection is fully
  logged out — both the HTTP session and the socket that used it are gone.
- **A connection that stays open is not affected by a later logout or session expiry.** If a
  connection authenticated while its token was valid, and that token is subsequently invalidated
  via `/auth/logout`, `/auth/logout`'s guest-token cleanup, or natural expiry, `TableServer` has no
  mechanism to notice and will keep serving that already-open connection as the same authenticated
  player until the socket itself closes (client disconnect, `stop()`, or the process exiting).
  Real-time revocation of an already-open, potentially malicious connection is **not implemented**.
- This is a known, documented gap, not an oversight to be assumed away: **this system is not
  production-complete security.** Closing it would require either a periodic re-validation of open
  connections against `/sessions/validate`, or a push-based revocation channel from
  `Mimic.PlatformServer` to `Mimic.TableServer` — neither exists today.

### Other limitations

- `sequence` is written monotonically by the sender (now allocated under `writeMutex`, see above)
  but **not** enforced as strictly increasing on receive. The reference client (`TcpLink.TrySend`)
  currently always sends `sequence = 0`; a hard monotonic check on the receiving side would make
  every legitimate frame from that client rejected. Ordering is still guaranteed at the
  application layer via `Envelope.request_id` (`Server/TableServer/main.cpp` already rejects
  non-increasing `request_id`).
- No transport-layer encryption. XOR(0xA7) is explicitly obfuscation, not security, exactly as
  documented in both the legacy header and the reference client's `IPayloadCipher` comments. If
  real confidentiality is needed later, it belongs at the socket layer (TLS) or via a real
  session-keyed cipher — swapping it in is a one-function change (`applyXor` in
  `Server/Netlib/Transport.cpp`), by design.
- Not MessagePack-compatible. MIMIC's application payload is exclusively the `Envelope` protobuf
  message; no claim is made about compatibility with the legacy MessagePack-based game protocol.
- **Build verification for the C++ side could not be completed from this session** (this follow-up
  included): invoking `cmake`/MSVC build tools required elevated tool permission that was not
  granted. The coordinator should run:
  ```
  cmake --build build --target Mimic.TableServer --config Release
  cmake --build build --target Mimic.Holdem.Tests --config Release
  ctest --test-dir build -C Release
  ```
  to confirm `Server/Netlib/Transport.cpp`/`.hpp` (including this follow-up's entity-rejection and
  lock-ordered-sequence changes) compile, and that the Hold'em regression tests still pass. The C#
  side (`dotnet build`, `dotnet run` for `tests/AccountTests`) was built and run successfully from
  this session; see below.

  **Resolved 2026-09-23** (see `VERIFICATION.md` "Follow-up verification"): both `cmake --build`
  targets and `ctest` were run from a session with build-tool access. Clean build, tests pass.

## Changed files

Initial implementation:
- `Server/PlatformServer/Program.cs` — now a thin entry point calling `PlatformServerApp.Build`.
- `Server/PlatformServer/PlatformServerApp.cs` — all route wiring, rate limiting, exception
  handling.
- `Server/PlatformServer/Accounts/AccountValidation.cs` — input validation rules.
- `Server/PlatformServer/Accounts/PasswordHasher.cs` — PBKDF2-SHA256 hashing/verification.
- `Server/PlatformServer/Accounts/AccountDatabase.cs` — SQLite accounts/sessions storage.
- `Server/PlatformServer/Accounts/DataDirectory.cs` — `MIMIC_DATA_DIR` resolution.
- `Server/PlatformServer/Mimic.PlatformServer.csproj` — added `Microsoft.Data.Sqlite` 8.0.10.
- `Server/Netlib/Transport.hpp` / `Server/Netlib/Transport.cpp` — replaced the 4-byte length
  prefix with the 24-byte header + XOR(0xA7) framing. `validate()` (WinHTTP call to
  `/sessions/validate`) is unchanged.
- `Server/Mimic.sln` — added `tests/AccountTests` project (mirrors how `tests/Smoke` is already
  registered).
- `tests/AccountTests/` — `Mimic.AccountTests.csproj`, `TestHost.cs`, `Program.cs`.

This follow-up (bounded to items requested in review):
- `Server/PlatformServer/PlatformServerApp.cs` — rate limiter `OnRejected` now emits a protobuf
  `RATE_LIMITED` `ErrorReply`; the global exception handler now distinguishes `413` (oversized
  body) from generic `500`; `/auth/logout` also removes a matching in-memory guest token;
  `/auth/login` always calls `PasswordHasher.Verify` (against `DummyHash` for unknown accounts);
  `IssueSession` now calls `DeleteExpiredSessions` as a best-effort side effect;
  `MIMIC_SESSION_TTL_SECONDS`/`MIMIC_AUTH_RATE_LIMIT_PER_MINUTE` are now parsed through a bounded
  helper instead of trusted as-is.
- `Server/PlatformServer/Accounts/PasswordHasher.cs` — added the process-wide `DummyHash`;
  `Verify` now bounds iteration count/salt length/key length before using them (closes a
  zero-length-match and an unbounded-work-factor issue).
- `Server/Netlib/Transport.hpp` / `Server/Netlib/Transport.cpp` — `sendSequence` is now a plain
  counter allocated under `writeMutex` (previously an atomic incremented before the lock);
  `receive()` now rejects a nonzero `entity` field.
- `tests/AccountTests/TestHost.cs` — added an opt-in `development` flag so a scenario can start
  the host with the dev-only `/auth/guest` endpoint enabled.
- `tests/AccountTests/Program.cs` — new checks for all of the above (see "Tests and results").
- `docs/SERVER_AUTH.md` — this document, updated.

`Server/TableServer/main.cpp`, `Server/TableServer/Holdem/*`, and `Server/FrontServer/*` remain
unchanged across both rounds; the Hold'em game logic and portal discovery are unaffected.

## Tests and results

`tests/AccountTests` is a plain executable (no xunit/nunit/test-framework dependency), following
the same style as `tests/Smoke`. It hosts the real `Mimic.PlatformServer` app in-process via
`PlatformServerApp.Build(...)` on an OS-assigned loopback port (`TestHost.cs`), drives it over
real HTTP with `HttpClient`, and shuts it down at the end of each scenario — no persistent server
process is left running.

Run:
```
cd tests/AccountTests
dotnet run -c Release
```

Coverage (109 checks, all passing as of this follow-up):
- Registration validation (account name length/charset, password length, display name
  length/control characters) → 400 with a protobuf `ErrorReply`.
- Successful registration → 201 `LoginReply` with a 64-char session token, correct echoed fields,
  1000 starting demo chips, future expiry.
- Duplicate account rejection, case-insensitively (`Bob`, `BOB`, `bob`) → 409 `ACCOUNT_EXISTS`.
- Login: correct credentials → 200; wrong password and unknown account both → 401 with an
  **identical** error code/message (no account-existence oracle).
- Logout invalidates the session for both `/account/profile` and `/sessions/validate`.
- Session expiry (via `MIMIC_SESSION_TTL_SECONDS=1`) rejects both endpoints after expiry.
- No plaintext password storage: reads the SQLite row directly and asserts the stored hash is
  neither equal to nor contains the plaintext password, and uses the documented format.
- Persistent reload: register against one app instance, stop it, start a fresh instance pointed
  at the same `MIMIC_DATA_DIR`, and confirm login/profile return the same player id, creation
  time, and chip balance.
- Malicious input / parameterization: a SQL-metacharacter account name is rejected by validation
  before it ever reaches SQL; a punctuation-and-quote-heavy display name is safely stored and
  returned unmodified via parameterized queries; the accounts table remains fully functional
  afterward.
- Concurrency: 6 simultaneous registration attempts for case-variant spellings of the same account
  name resolve to exactly one `201` and five `409`s.
- Rate limiting: with a lowered `MIMIC_AUTH_RATE_LIMIT_PER_MINUTE`, repeated `/auth/login` calls
  eventually receive `429` **with a protobuf `RATE_LIMITED` `ErrorReply` body**.
- **(new)** Oversized request body → `413` with a protobuf `PAYLOAD_TOO_LARGE` `ErrorReply`, not
  an empty body or a generic `500`.
- **(new)** `GET /health` is JSON and a successful `POST /auth/logout` has an empty `204` body —
  both asserted as the documented, intentional exceptions to "every response is protobuf."
- **(new)** Guest logout: a guest token validates via `/sessions/validate` before logout and is
  rejected (`401`) after — proving the in-memory guest dictionary, not just the database, is
  cleared.
- **(new)** Login timing: a best-effort, generously-bounded check that logging in with an unknown
  account pays comparable PBKDF2 cost to a known account (not a >5x-faster near-instant reject),
  regression-testing the dummy-hash timing defense.
- **(new)** `PasswordHasher.Verify` against 13 malformed/out-of-bounds stored hashes (missing
  segments, wrong prefix, non-numeric/zero/negative/excessive iteration count, non-base64
  salt/key, and — the critical case — a zero-length salt/key pair, which must never verify as a
  match for any password) plus a round-trip sanity check against a hash the class actually
  produces.
- **(new)** Expired session rows are not deleted by the passage of time alone, but are purged the
  next time a new session is issued (`DeleteExpiredSessions` wired into `IssueSession`), verified
  by querying the `sessions` table directly.
- **(new)** A negative `MIMIC_SESSION_TTL_SECONDS` falls back to the safe default instead of
  issuing an already-expired session; a `0`/invalid `MIMIC_AUTH_RATE_LIMIT_PER_MINUTE` falls back
  to the safe default instead of rejecting every request.

Also verified from this session:
- `dotnet build Server/Mimic.sln` (Debug and Release) — builds `Mimic.Protocol`,
  `Mimic.FrontServer`, `Mimic.PlatformServer`, `Mimic.Smoke`, and `Mimic.AccountTests` cleanly,
  0 warnings / 0 errors.

Not verified from this session (see "Build verification" above): compiling
`Server/Netlib/Transport.cpp` and `Server/TableServer/main.cpp` with MSVC/CMake, and running the
existing `Mimic.Holdem.Tests` C++ test target.

## Integration notes for the coordinator

1. The Unity client has reportedly been updated to the legacy `TcpLink`/`PacketHeader`/`XorCipher`
   contract (see "Client integration status" above) — but this was not independently re-verified
   from this session, and `tests/Smoke/Program.cs` (outside this change's ownership) still
   contains raw framing assertions written against the *old* 4-byte length prefix as of the last
   time it was read in this workspace. Confirm `tests/Smoke` has also been updated (or accept that
   it will fail) before treating end-to-end wire compatibility as confirmed.

   **Resolved 2026-09-23**: `tests/Smoke/Program.cs` already asserts the 24-byte header (magic
   `0x6B2E` at offset 0, little-endian) — confirmed by direct source read and by running
   `Scripts/Test-Smoke.ps1`, which passes all 24 checks including the legacy-framing ones. End-to-
   end wire compatibility is confirmed, not just reported. `Network/Legacy/*` was also diffed
   directly against the `C:/NewClient/casino` source and matches (see `VERIFICATION.md`).
2. Please run the C++ build/test commands listed above; this session's tool permissions did not
   allow invoking `cmake`/MSVC directly, in either this follow-up or the original implementation.

   **Resolved 2026-09-23**: run from a session with build-tool access; both targets build clean
   and `ctest` passes. See `VERIFICATION.md`.
3. No real accounts, passwords, or database files are committed. `artifacts/data` is created at
   runtime and should stay untracked (already covered by the repository's existing ignore rules
   for build artifacts; verify before committing if in doubt).
4. `MIMIC_DATA_DIR`, `MIMIC_SESSION_TTL_SECONDS`, and `MIMIC_AUTH_RATE_LIMIT_PER_MINUTE` are all
   optional environment/config overrides with sane production defaults — no deployment change is
   required to pick up this work.
5. See "Authorization boundary" above: logout and expiry do not revoke an already-open TCP
   connection. Do not represent this system as production-complete security without closing that
   gap first.

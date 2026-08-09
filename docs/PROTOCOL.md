# PROTOCOL.md — Client ↔ server contract (Stretch)

Tiny characterisation of the **existing** wire protocol and ownership split.
**Docs only** — does not invent opcodes, change gameplay, or alter `src/**` / `server.py`.

Pair with `docs/ADR-001-modular-craft.md` (decision) and `docs/GAME_MAP.md` (source map).
Authoritative implementations today: `src/client.c` (send), `parse_buffer` in `src/main.c` (recv), `server.py` (`Handler` / `Model`).

---

## 1) What runs where

| Side | Runtime | Primary code | Role today |
|------|---------|--------------|------------|
| **Client** | Native C + OpenGL (GLFW) | `src/*.c` — especially `main.c`, `client.c`, `db.c`, `world.c`, render/input | Windowed game; offline world authority; online TCP peer with optimistic local edits |
| **Server** | Python 2-style TCP server | `server.py` (+ `world.py` / shared world-gen lib) | Authoritative multiplayer world, SQLite persist, fan-out to peers |
| **Wire** | TCP, default **`:4080`** | Line-based ASCII | Shared v1 protocol; knowledge is duplicated in C senders, Python handlers, and `main.c` parsers |

Offline sessions never open the socket: place/break/save stay on the C client + local `db.c`.
Online sessions still apply edits locally first, then sync; the server validates, persists, and broadcasts.

---

## 2) Framing (as implemented)

- Transport: TCP stream.
- Framing: **one command per line**, terminated by `\n` (CRLF normalised to LF on the server).
- Shape: `CMD,arg1,arg2,...\n` — comma-separated ASCII.
- Version: client sends `V,1` immediately after connect; server accepts only version `1` (`on_version`), otherwise closes.

Selected opcodes (full tables in ADR-001; constants in `server.py`):

| Direction | Codes used for join / world / chat |
|-----------|-------------------------------------|
| Client → server | `V`, `A`, `P`, `C`, `B`, `L`, `S`, `T` |
| Server → client | `U`, `E`, `B`, `L`, `S`, `K`, `R`, `C`, `P`, `D`, `N`, `T` |

---

## 3) Join (connect → identity → spawn)

**Client (`main.c` online session init + `client.c`):**

1. `client_enable` → `client_connect(host, port)` → `client_start` (recv thread).
2. `client_version(1)` → wire `V,1`.
3. `login()` → HTTPS identity via `auth.c` when a selected token exists in `auth.db`; then `client_login(username, access_token)` → `A,username,token` (empty strings if anonymous / failed).
4. Main loop: `client_recv` → `parse_buffer`; throttled `client_position` → `P,x,y,z,rx,ry`.

**Server (`Model.on_connect` / `on_version` / `on_authenticate`):**

1. On TCP accept: assign `client_id`, default nick `guestN`, append to `clients`.
2. Immediately send: `U,pid,x,y,z,rx,ry` (YOU + `SPAWN_POINT`), `E,elapsed,day_length` (TIME), welcome `T` lines, then peer `P` / `N` snapshots.
3. `V,1` recorded; other versions → disconnect.
4. `A,...` may POST to the legacy access API; sets `user_id` / nick or leaves guest; broadcasts join `T` and nick `N`.

**Client parse of join-relevant lines:** `U` sets local player id + pose (and `force_chunks`; if `y==0`, client lifts to `highest_block+2`); `E` sets day clock; `P`/`N`/`D`/`T` update peers and HUD chat.

---

## 4) Block place / break (and related edits)

### Offline

- Input → `set_block` / lights / signs in `main.c`.
- Authority: C client map + **`db_insert_*`** on the local SQLite worker (`db.c`).
- No `client_block` traffic (`client_enabled` is off).

### Online

| Step | Who | What |
|------|-----|------|
| Propose | Client | After local `_set_block` / DB cache insert, `client_block(x,y,z,w)` → `B,x,y,z,w` (`w≠0` place, `w=0` break). Same pattern for `L` (light) and `S` (sign). |
| Validate + persist | Server `on_block` | Checks auth (when `AUTH_REQUIRED`), bounds, `ALLOWED_ITEMS`, empty/occupied, indestructible; writes server `craft.db`; fans out `B,p,q,x,y,z,w` (+ edge replicas with negative `w`) and `R,p,q`. |
| Reject | Server | Echoes prior `B,p,q,x,y,z,previous` + `R` + explanatory `T`; client `parse_buffer` applies via `_set_block` (no extra client send). |
| Chunk sync | Client `client_chunk` / server `on_chunk` | `C,p,q,key` requests deltas with `rowid > key`; server streams stored `B`/`L`/`S`, optional `K,p,q,key`, `R`, then `C,p,q` complete. |

**Truth online:** server `B` / `L` / `S` (+ `R`) win. Local edits are optimistic UX only.

---

## 5) Save / load

| Mode | Load | Save / persist |
|------|------|----------------|
| **Offline** | `db_init` → `db_load_state`; chunk path overlays `db_load_blocks` / lights / signs on `create_world` | Each edit enqueues `db_insert_*`; periodic `db_commit` (`COMMIT_INTERVAL`); session end `db_save_state` + `db_close`. Default file: `craft.db`. |
| **Online** | World truth from server chunk/`B` stream; optional client cache DB if `USE_CACHE` (`cache.<host>.<port>.db`) — **not** authority | **Server** SQLite (`DB_PATH`, default `craft.db`) on accept/apply; client may cache keys (`K` → `db_set_key`) and blocks for faster reload, but rejected builds follow server echoes |

Player pose offline is `db_save_state` / `db_load_state`. Online pose is live `P` traffic; server does not own the offline single-player save file.

---

## 6) Local-only (not protocol; do not migrate to `server.py`)

These stay on the C client unless a future task explicitly says otherwise:

- GLFW window, input bindings, HUD, crosshair, hotbar UX
- GL programs, textures, chunk mesh build/upload, sky / picture-in-picture
- Local movement, collision, remote-player interpolation
- Offline DB paths, `db_save_state`, auth token storage in `auth.db`
- Pure helpers under characterisation (`map`, `matrix`, `item`, …) except when a task bridges them to net

Chat slash-commands: some are **client-local** (e.g. mode switches in `main.c`); server interprets `/nick`, `/goto`, `/spawn`, `/pq`, `/help`, `/list` via `T` text starting with `/`.

---

## 7) Ownership notes for later slices

Keep allowlists on one side of the C ↔ Python boundary per PR (see ADR-001 consequences).

| Concern | Own in | Do not casually touch |
|---------|--------|------------------------|
| Send framing / socket I/O | `src/client.c`, `src/client.h` (+ `tests/test_client.c` when tasked) | `server.py` in the same PR unless dual-allowlisted |
| Recv parse / apply | `parse_buffer` and call sites in `src/main.c` (only when the task allowlists them) | Server handlers |
| Server validate / persist / fan-out | `server.py` (and `world.py` only if tasked) | `src/client.c` |
| Offline persistence | `src/db.c` / `src/db.h` | Server DB schema in the same slice |
| Protocol **docs** | `docs/PROTOCOL.md`, ADR-001 | `src/**`, `server.py` |
| Wire shape change (`B`, `C`/`K`, `P`, …) | **One sequenced PR** (or explicit dual allowlist) | Parallel client + server PRs that redefine the same opcode |

Serialize when both sides must change the same message. Prefer characterisation tests before structural moves. No new opcodes or binary v2 in Stretch unless a later task asks.

---

## References

- `docs/ADR-001-modular-craft.md` — Stretch decision and opcode summary tables
- `docs/GAME_MAP.md` — `src/*` roles; place/break/save call flow
- `src/client.c`, `src/client.h` — client send helpers
- `src/main.c` — `login`, online init, `parse_buffer`, `set_block` → `client_block`
- `server.py` — `Handler`, `Model.on_*`, SQLite, broadcast helpers
- `docs/BASELINE.md` / `AGENTS.md` — proof and lab rules

*Lab Step 7 Stretch — protocol note. Characterises current v1 ASCII behaviour only.*

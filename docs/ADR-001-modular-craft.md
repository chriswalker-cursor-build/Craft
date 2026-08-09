# ADR-001 — Modular Craft: client / server split (Stretch)

- **Status:** Accepted
- **Date:** 2026-08-09
- **Lab step:** Step 6 (architecture ADR)
- **Choice:** Stretch — **not** a `libworld` / `librender` / `libpersist` library split

---

## Context

This is the Craft Cloud Agents lab fork (not upstream `fogleman/Craft`). Characterisation tests and a `test_runner` harness already exist (`tests/**`, Step 5). `docs/GAME_MAP.md` maps `src/*` into non-overlapping strangler slices (S0–S12).

The human chose **Stretch**: clarify and gradually harden the existing **C client vs Python server** boundary, rather than inventing a new in-process module layout (`libworld` / `librender` / `libpersist`).

Today:

| Side | Primary code | Role |
|------|----------------|------|
| **Client** | `src/*.c` (esp. `main.c`, `client.c`, `db.c`, render/input) | OpenGL game; offline SQLite world; online TCP peer |
| **Server** | `server.py` (+ `world.py` / shared world gen) | Authoritative multiplayer world, SQLite persist, fan-out |
| **Wire** | Line-based ASCII over TCP `:4080` | Shared protocol; duplicated knowledge in C senders + Python handlers + `main.c` parsers |

Offline mode keeps place/break/save entirely on the C client + local `db.c`. Online mode still applies optimistic local edits, then syncs via the protocol; the server validates and persists.

---

## Decision

**Split client vs server responsibilities more cleanly along the existing C / Python boundary**, and document a **tiny protocol contract** so Step 7+ allowlists and slices do not cross that boundary by accident.

### Responsibility split

| Concern | Client (C) | Server (Python) |
|---------|------------|-----------------|
| Window, GL, shaders, mesh upload | Owns | — |
| Input, movement, collision, hit-test UX | Owns (local feel) | — |
| Offline place / break / light / sign | Authoritative + `db.c` | — |
| Online place / break / light / sign | Proposes (`B`/`L`/`S`); applies server echoes | **Authoritative** validate + SQLite + broadcast |
| Chunk terrain defaults | Local `world.c` / cache | `World` + DB overlays |
| Player pose (self) | Local sim; sends `P,x,y,z,rx,ry` | Tracks; broadcasts `P,pid,...` |
| Chat / commands | Local `/offline` etc.; sends `T,...` | Server `/nick`, `/goto`, …; broadcasts `T` |
| Auth identity HTTPS | `auth.c` → token | Validates via `A,...` + access API |
| Protocol framing I/O | `client.c` | `Handler` / `Model` in `server.py` |

### Protocol note (messages)

Transport: TCP, default port **4080**. Framing: **one command per line**, `CMD,arg1,arg2,...\n` (comma-separated ASCII). Version **1** (`V,1`).

**Client → server (selected):**

| Code | Shape | Meaning |
|------|--------|---------|
| `V` | `V,version` | Protocol version |
| `A` | `A,username,token` | Authenticate |
| `P` | `P,x,y,z,rx,ry` | Local player pose |
| `C` | `C,p,q,key` | Request chunk delta since `key` |
| `B` | `B,x,y,z,w` | Place (`w≠0`) or break (`w=0`) |
| `L` | `L,x,y,z,w` | Light value |
| `S` | `S,x,y,z,face,text` | Sign |
| `T` | `T,text` | Talk / slash-command text |

**Server → client (selected):**

| Code | Shape | Meaning |
|------|--------|---------|
| `U` | `U,pid,x,y,z,rx,ry` | “You are” id + spawn/pose |
| `E` | `E,elapsed,day_length` | Time of day |
| `B` | `B,p,q,x,y,z,w` | Authoritative block (incl. edge replicas) |
| `L` | `L,p,q,x,y,z,w` | Light |
| `S` | `S,p,q,x,y,z,face,text` | Sign |
| `K` | `K,p,q,key` | Chunk cache key |
| `R` | `R,p,q` | Redraw chunk |
| `C` | `C,p,q` | Chunk request complete |
| `P` | `P,pid,x,y,z,rx,ry` | Other (or self) player pose |
| `D` | `D,pid` | Player disconnect |
| `N` | `N,pid,name` | Nick |
| `T` | `T,text` | Chat / server message |

(Full enumerations live in `server.py` constants and `src/client.c` / `parse_buffer` in `main.c`. This ADR is the contract summary, not a second wire spec.)

### Authority: place / break / save

| Mode | Place / break authority | Save / persistence |
|------|-------------------------|---------------------|
| **Offline** | C client (`set_block` → map + `db_insert_*`) | Local SQLite via `db.c` (`craft.db` or named offline file); `db_save_state` on session end |
| **Online** | **Server** (`on_block` / `on_light` / `on_sign`); may reject and echo prior state | **Server** SQLite (`craft.db` on server process); client may keep optional cache DB (`USE_CACHE`) — cache is not authority |

Clients must treat server `B`/`L`/`S` (+ `R`) as truth when online. Rejected builds are corrected by server echo + talk message.

### Local-only (stays on the C client)

These are **not** protocol concerns and must not migrate into `server.py` in Stretch slices:

- GLFW window, input bindings, HUD, crosshair, inventory hotbar UX
- GL programs, textures, chunk mesh generation/upload, sky/PiP rendering
- Local movement / collision / interpolation of remote players
- Offline-only DB paths and `db_save_state` player pose
- Pure helpers already under characterisation (`map`, `matrix`, `item`, …) except where a task explicitly bridges to net

---

## Non-goals

- **No gameplay feel changes** (walk, place/break timing, save/reload behaviour).
- **No full rewrite** of `main.c` or `server.py`.
- **No new features** (new block types, new opcodes, auth redesign) unless a later task explicitly asks.
- **No** `libworld` / `librender` / `libpersist` extraction as the architecture north star (rejected option for this ADR).
- **Small strangler slices only** — one allowlisted PR at a time; prefer tests before structural moves.

---

## Consequences

1. **Step 7 allowlists must separate:**
   - **Client (C / net):** e.g. `src/client.c`, `src/client.h`, related `main.c` parse/send call sites only when tasked, `tests/test_client.c`
   - **Server (Python):** e.g. `server.py`, `world.py` (only if the task says so), server-side tests if added later
   - **Protocol docs:** this ADR and any future `docs/*protocol*` notes — docs-only PRs must not edit `src/**` or server code

2. **Serialize when both sides change the same message.** If a PR alters a wire shape or meaning (`B`, `C`/`K`, `P`, …), do **not** land parallel client and server PRs that touch that opcode; one sequenced change (or a single explicitly dual-allowlisted PR) owns the handshake.

3. **`docs/GAME_MAP.md` slices realign to this ADR.** Treat multiplayer/`client.c` work as the client half of the C↔Python boundary; add or amend server-side slice IDs so Python paths do not overlap C client slices. Pure offline modules (map/item/db/…) remain as today unless a task bridges them to net.

4. **Proof culture unchanged:** `cmake . && make` and `./test_runner` stay green; no intentional gameplay changes; burden of proof in every PR per `docs/LAB_AUDIT.md`.

---

## Alternatives considered (rejected for Stretch)

| Option | Why not now |
|--------|-------------|
| **`libworld` / `librender` / `libpersist` split** | Human chose Stretch; would redraw in-process boundaries already partially covered by GAME_MAP S1–S8/S12 without clarifying multiplayer authority. |
| **Rewrite protocol (binary / version 2)** | Out of scope; no new features; document and strangler around v1 ASCII. |
| **Merge server into C** | Huge rewrite; rejected. |

---

## References

- `docs/GAME_MAP.md` — source map and parallel slices
- `docs/BASELINE.md` — proof commands
- `docs/LAB_AUDIT.md` — PR / audit template
- `AGENTS.md` — lab hard rules
- `src/client.c`, `server.py`, README “Implementation” notes — existing wire behaviour

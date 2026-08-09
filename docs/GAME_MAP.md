# GAME_MAP.md — Craft source map for multi-agent slices

Scout document for strangler-style refactors. **Docs only** — this file does not change gameplay or production code.

Pair with `AGENTS.md`, `docs/BASELINE.md`, and `docs/LAB_AUDIT.md`. Future agents must honour **non-overlapping path allowlists** when claiming a slice.

---

## 1) Purpose of each major `src/*.c` file

| File | Role |
|------|------|
| `src/main.c` | God-module: GLFW window, GL setup, shaders/textures, chunk workers, input, movement/collision, hit-test, place/break/light/sign, render loop, chat commands, online/offline mode switch. Most call-graph edges start or end here. |
| `src/db.c` | SQLite persistence: schema, prepared statements, async write worker (`tinycthread` + `Ring`), load/save player state, blocks, lights, signs, chunk keys, auth tokens (`auth.db` attach). |
| `src/world.c` | Procedural terrain for one chunk `(p,q)` via simplex noise; invokes a `world_func` callback per voxel (used when generating/loading chunks). Shared with `server.py` via the `world` shared lib. |
| `src/map.c` | Sparse voxel hash map (`Map` / `MapEntry`): alloc, grow, set, get. In-memory block/light storage per chunk. |
| `src/item.c` | Block/plant IDs, buildable `items[]`, face texture indices (`blocks[][]`), and classifiers: `is_plant`, `is_obstacle`, `is_transparent`, `is_destructable`. |
| `src/cube.c` | CPU-side mesh builders: cube faces (AO/light), plants, player body, wireframe, 2D/3D characters, sky sphere. Writes float vertex arrays — no GL calls. |
| `src/matrix.c` | Matrix/vector math: set/identity, matmul, apply, planes, perspective/ortho, frustum planes, 2D/3D helpers used by rendering and visibility. |
| `src/util.c` | File load, shader/program compile-link, PNG texture upload (`lodepng`), GL buffer helpers, FPS timer, day/time helpers. **GL-heavy.** |
| `src/client.c` | Multiplayer TCP client: connect, send/recv ring, version/login/position/chunk/block/light/sign/talk protocol. |
| `src/auth.c` | HTTPS identity exchange (`libcurl`) against the legacy Craft identity API; returns access token for `client_login`. |
| `src/sign.c` | Dynamic list of world signs (`SignList`): alloc, grow, add, remove, remove-all. |
| `src/ring.c` | Growable ring buffer of typed DB-worker jobs (`RingEntry`): block/light/sign/commit/key/exit. Used only by `db.c` today. |

Headers (`*.h`) and `src/config.h` define tunables (`DB_PATH`, chunk radii, `COMMIT_INTERVAL`, key bindings). Prefer editing config via an allowlisted config/test slice — not ad hoc in `main.c`.

---

## 2) Call flow: startup → main loop → place/break → save

```
main()
 ├─ curl_global_init / srand
 ├─ glfwInit → create_window → glewInit → GL state
 ├─ load_png_texture (texture/font/sky/sign)
 ├─ load_program (block/line/text/sky shaders)
 ├─ parse argv → MODE_ONLINE or MODE_OFFLINE, set g->db_path
 ├─ start WORKERS chunk threads (worker_run)
 └─ OUTER LOOP (mode sessions)
      ├─ db_enable + db_init(g->db_path)   [offline, or online if USE_CACHE]
      ├─ client_enable/connect/start/login [online only]
      ├─ reset_model; db_load_state → force_chunks
      ├─ INNER MAIN LOOP
      │    ├─ handle_mouse_input / handle_movement
      │    ├─ client_recv → parse_buffer
      │    ├─ periodic db_commit (COMMIT_INTERVAL seconds)
      │    ├─ client_position (throttle)
      │    ├─ delete_chunks / ensure_chunks (via render)
      │    ├─ render_sky / render_chunks / signs / players / HUD
      │    └─ glfwSwapBuffers + glfwPollEvents
      │         └─ mouse/key callbacks may call place/break (below)
      └─ SHUTDOWN session
           db_save_state → db_close → db_disable
           client_stop/disable; free chunks/players
```

### Place / break block (input → memory → disk → network)

1. `on_mouse_button` (or key path) → `on_left_click` (break) / `on_right_click` (place) / `on_light`.
2. `hit_test` / `hit_test_face` → world coords + face.
3. Break: `set_block(x,y,z,0)` (+ plant above cleared); Place: `set_block(..., items[g->item_index])` if `is_obstacle` and no player intersection.
4. `set_block` → `_set_block` for primary chunk and edge replicas:
   - `map_set` on chunk `Map` (if chunk resident)
   - `dirty_chunk` → mesh rebuild on worker
   - **`db_insert_block(p,q,x,y,z,w)`** (queued to DB worker)
   - clearing a block also `unset_sign` / `set_light(...,0)`
5. `client_block(x,y,z,w)` when multiplayer client enabled.
6. `record_block` updates builder selection markers (`block0`/`block1`).

### Save / persistence flush

| When | What |
|------|------|
| Each place/break/light/sign | `db_insert_*` / `db_delete_*` enqueued on DB worker ring |
| Every `COMMIT_INTERVAL` (5s) in main loop | `db_commit()` → worker runs `commit; begin;` |
| Session end (quit or mode change) | `db_save_state(x,y,z,rx,ry)` then `db_close()` (final commit) |
| Next startup | `db_init` → `db_load_state`; chunk load path `load_chunk` → `db_load_blocks` / `db_load_lights` / `db_load_signs` overlaying `create_world` |

Default offline DB path: `craft.db` (`DB_PATH` in `config.h`). Online cache: `cache.<host>.<port>.db`. Auth tokens live in attached `auth.db`.

---

## 3) Where SQLite persistence lives

| Location | Responsibility |
|----------|----------------|
| **`src/db.c` / `src/db.h`** | **Sole SQLite owner.** `sqlite3_open`, schema DDL, prepared stmts, worker thread, all CRUD. |
| **`src/ring.c` / `src/ring.h`** | Job queue backing the DB worker (not SQL itself). |
| **`src/main.c`** | Call sites only: `db_enable/init/load_state/commit/save_state/close`, `db_insert_block` via `_set_block`, lights/signs, auth select/set, path selection into `g->db_path`. |
| **`src/config.h`** | `DB_PATH`, `USE_CACHE`, `COMMIT_INTERVAL`. |
| **Runtime files** | `craft.db` (blocks/lights/signs/state/keys), `auth.db` (identity tokens), optional `cache.*.db`. |

Do **not** open SQLite from new modules; extend `db_*` APIs instead.

Deps: bundled `deps/sqlite/` (linked via `CMakeLists.txt`) — out of allowlist for production slices unless a task explicitly includes it.

---

## 4) Proposed parallel slices (non-overlapping path allowlists)

Use one slice per agent/PR. Allowlists must not share production paths. Shared wiring (`CMakeLists.txt`, new `tests/*`, `test_runner`) only when the task says so — typically owned by the **harness** slice first.

| Slice ID | Goal (later steps) | Exclusive allowlist |
|----------|--------------------|---------------------|
| **S0 — harness** | Step 5: add `test_runner` + CMake test target | `CMakeLists.txt`, `tests/**` (new), optional thin `src/*` only if task demands a test hook |
| **S1 — map** | Characterise / harden sparse map | `src/map.c`, `src/map.h`, `tests/test_map.c` |
| **S2 — items** | Lock block classifiers & tables | `src/item.c`, `src/item.h`, `tests/test_item.c` |
| **S3 — worldgen** | Deterministic terrain callback tests | `src/world.c`, `src/world.h`, `tests/test_world.c` |
| **S4 — matrix** | Pure math unit tests | `src/matrix.c`, `src/matrix.h`, `tests/test_matrix.c` |
| **S5 — ring** | DB job queue correctness | `src/ring.c`, `src/ring.h`, `tests/test_ring.c` |
| **S6 — signs** | SignList CRUD | `src/sign.c`, `src/sign.h`, `tests/test_sign.c` |
| **S7 — cube-mesh** | Vertex builder golden tests (no GL) | `src/cube.c`, `src/cube.h`, `tests/test_cube.c` |
| **S8 — db** | Persistence worker + SQL behaviour (temp DB files) | `src/db.c`, `src/db.h`, `tests/test_db.c` |
| **S9 — client** | Protocol framing / enable flags (mockable I/O later) | `src/client.c`, `src/client.h`, `tests/test_client.c` |
| **S10 — auth** | Token POST helper (mock HTTP later) | `src/auth.c`, `src/auth.h`, `tests/test_auth.c` |
| **S11 — util-non-gl** | Extract/test non-GL helpers only when tasked | paths named in that task — **never** overlap S7/S12 |
| **S12 — main extract** | Step 7 stranglers out of `main.c` (physics, hit-test, set_block) | `src/main.c` + **new** `src/<extracted>.{c,h}` named in the task; no edits to S1–S10 files |

**Collision rules**

- Only **S0** touches `CMakeLists.txt` until harness exists; afterward, a slice may add its `tests/test_*.c` line if the task allowlists `CMakeLists.txt` for that PR alone.
- **S8** may use `ring` APIs but must not modify `src/ring.*` (that is S5).
- **S12** extracts *from* `main.c` into new files; do not “borrow” map/item/db edits — open a follow-up if signatures must change across slices.
- Docs/ADR tasks: `docs/**` only (this file’s rule).

---

## 5) What to unit-test first (pure logic vs GL-heavy)

### Test first (pure / headless-friendly)

Highest ROI for Step 5 characterisation tests:

1. **`map_set` / `map_get` / grow** — sparse storage invariants; no GL, no GLFW.
2. **`item` classifiers** — `is_plant`, `is_obstacle`, `is_transparent`, `is_destructable` + selected `blocks[]` faces.
3. **`create_world`** — fixed `(p,q)` → collect voxels via callback; assert height/material stability (noise is deterministic for given coords).
4. **`matrix_*`** — identity, multiply, perspective basics.
5. **`ring_*`** — enqueue order, grow, commit/exit entries.
6. **`sign_list_*`** — add/remove/face uniqueness behaviour.
7. **`cube` makers** — write into a float buffer; snapshot a few vertices (still pure CPU).
8. **`db_*` with temp path** — insert block → commit → load into `Map` (needs sqlite link; still no window).

### Defer (GL / window / network heavy)

| Area | Why wait |
|------|----------|
| `util.c` shader/texture/`gen_buffer` | Needs GL context |
| `main.c` render_* / GLFW callbacks | Window, GPU, global `g` model |
| Chunk mesh upload / `draw_*` | GL buffer lifecycle |
| `client.c` real sockets / `auth.c` live HTTPS | Flaky, external; mock before testing |
| Full place/break E2E | Manual or integration after pure `set_block` logic is extracted (S12) |

### Practical order for the lab

`S0 harness` → `S1 map` + `S2 items` (parallel) → `S3 worldgen` + `S4 matrix` + `S5 ring` (parallel) → `S8 db` (after ring tests) → only then GL or `main.c` extracts.

---

## Quick reference: place/break symbols

| Symbol | File | Notes |
|--------|------|-------|
| `on_left_click` / `on_right_click` | `main.c` | Break / place entry |
| `hit_test` | `main.c` | Ray vs voxels |
| `set_block` / `_set_block` | `main.c` | Map + DB + client |
| `map_set` | `map.c` | In-chunk memory |
| `db_insert_block` | `db.c` | Persistent write queue |
| `db_commit` / `db_save_state` | `db.c` | Flush / player pose |

---

*Last updated for lab Step 4 (GAME_MAP). Amend when slices land or `main.c` is split.*

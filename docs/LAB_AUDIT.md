# LAB_AUDIT.md — Craft Cloud Agents lab

Append-only log. One row (or short block) **per agent run / PR merge**. This is how you see the write loop closing.

## How to use

1. After each Cloud Agent PR merges (or is abandoned), add a block below under **Log**.
2. Copy command output summaries — do not invent green.
3. At the end of the lab, write `docs/LAB_RETRO.md` and link themes back to rows here.

## PR body template (paste into every agent PR)

```markdown
## Goal
<one sentence>

## Allowlist
- <paths>

## Proof
- [ ] `cmake . && make` — pass / fail
- [ ] `./test_runner` — pass / fail / N/A (harness not yet)
- Commands actually run:
  ```
  <paste>
  ```

## Tests
- Added/updated: <list or none>
- Functions locked: <list>

## Manual playtest
- N/A / walked, place, break, save-reload: <notes>

## Inner loop
- Times red before green: <n>
- What failed first: <build|tests|other>
- What fixed it: <one line>

## Audit
- LAB_AUDIT row id: <YYYYMMDD-N>
```

## Baseline (fill in Step 2)

| Item | Value |
|------|--------|
| Fork URL | https://github.com/chriswalker-cursor-build/Craft |
| Baseline branch | master |
| Proof commands | `cmake . && make` then `./test_runner` then `python3 -m unittest tests.test_server_rejects -v` |
| Manual E2E check | place dirt → quit → reopen → block still there |
| Required CI checks | none configured |

## Log

<!-- Newest entry at the top -->

### 20260809-8 — Step 8: LAB_AUDIT refresh + human playtest
- **Agent/branch:** docs/lab-audit-step8
- **PR:** (this PR)
- **Allowlist:** `docs/LAB_AUDIT.md` ONLY
- **Proof:**
  - `cmake . && make` — pass
  - C `./test_runner` — pass (117 assertions, 0 failed; suites: matrix 37, util 28, db 27, client 25)
  - Python `python3 -m unittest tests.test_server_rejects -v` — pass (19 tests, OK)
- **Manual playtest:** PASTE HERE after you play
- **Inner loop:** 0 red (docs-only append from merged PR bodies + local proof re-run)
- **Collision:** none (allowlist single file)
- **Notes:** Appended missing merge log for PRs #1–#10; filled Baseline proof commands to match `docs/BASELINE.md`. Human playtest placeholder left as provided — replace when played.

### 20260809-7d — server reject characterisation (merged into #9 then master)
- **Agent/branch:** test/server-reject-helpers
- **PR:** https://github.com/chriswalker-cursor-build/Craft/pull/10
- **Allowlist:** `tests/**` (`tests/test_server_rejects.py`); `docs/BASELINE.md` (one Python proof line); `server.py` not modified
- **Proof:** `cmake . && make` pass; `./test_runner` pass (92 assertions at PR time); `python3 -m unittest tests.test_server_rejects -v` pass (19/19)
- **Inner loop:** 2 red — `ast.parse` of Py2 `print`; then class-attr binding of helpers as methods → line-based source extraction + instance/`ns` refs
- **Collision:** based on `refactor/server-boundary` before #9 hit master; merged into #9 then landed on master via #9
- **Notes:** Characterises `block_reject_message` / `light_reject_message` without importing Queue/SocketServer or starting TCP.

### 20260809-7c — Python server boundary vs client
- **Agent/branch:** refactor/server-boundary
- **PR:** https://github.com/chriswalker-cursor-build/Craft/pull/9
- **Allowlist:** `server.py` ONLY
- **Proof:** `cmake . && make` pass; `./test_runner` pass (92 assertions, 0 failed at PR time)
- **Inner loop:** 0 red
- **Collision:** none on allowlisted path; carried #10 merge commit before landing on master
- **Notes:** Extracted pure reject helpers; module docs for Handler vs Model; no wire-shape or gameplay feel changes.

### 20260809-7b — C client boundary vs server
- **Agent/branch:** refactor/client-boundary
- **PR:** https://github.com/chriswalker-cursor-build/Craft/pull/8
- **Allowlist:** `src/client.c`, `src/client.h`, `tests/test_client.c`; wiring `CMakeLists.txt`, `tests/test_runner.c`
- **Proof:** `cmake . && make` pass; `./test_runner` pass (117 assertions, 0 failed; client suite 25 OK)
- **Inner loop:** 0 red
- **Collision:** none noted
- **Notes:** Pure `client_fmt_*` framing helpers; ownership docs in `client.h`; call sites in `main.c` unchanged.

### 20260809-7a — PROTOCOL.md (client-server split)
- **Agent/branch:** docs/protocol
- **PR:** https://github.com/chriswalker-cursor-build/Craft/pull/7
- **Allowlist:** `docs/PROTOCOL.md` ONLY
- **Proof:** `cmake . && make` pass; `./test_runner` pass (92 assertions, 0 failed)
- **Inner loop:** 0 red
- **Collision:** none
- **Notes:** Docs-only characterisation of existing wire contract; no new opcodes.

### 20260809-6 — ADR-001 modular craft (client/server split)
- **Agent/branch:** docs/adr-001
- **PR:** https://github.com/chriswalker-cursor-build/Craft/pull/6
- **Allowlist:** `docs/ADR-001-modular-craft.md` ONLY
- **Proof:** `cmake . && make` pass; `./test_runner` pass (92 assertions, 0 failed)
- **Inner loop:** 0 red
- **Collision:** none
- **Notes:** Accepted Stretch ADR; no `src/**` or server edits.

### 20260809-5b — db/sqlite round-trip characterisation
- **Agent/branch:** test/db-roundtrip
- **PR:** https://github.com/chriswalker-cursor-build/Craft/pull/5
- **Allowlist:** `tests/**` (`tests/test_db.c`, `tests/test_runner.c`); `CMakeLists.txt` (test wiring); no production `src/db*` edits
- **Proof:** `cmake . && make` pass; `./test_runner` pass (92 assertions: matrix 37, util 28, db 27)
- **Inner loop:** 1 red — merge conflict after #4 landed (`CMakeLists.txt`, `tests/test_runner.c` add/add)
- **Collision:** yes — concurrent harness PR #4; resolved by merging master and keeping db suite + sqlite linkage
- **Notes:** Temp-DB insert/read/delete (w=0) via existing `db_*` APIs.

### 20260809-5a — test harness + pure helper unit tests
- **Agent/branch:** test/pure-helpers
- **PR:** https://github.com/chriswalker-cursor-build/Craft/pull/4
- **Allowlist:** `tests/**`; `CMakeLists.txt` (test target wiring); production `src/**` none
- **Proof:** `cmake . && make` pass; `./test_runner` pass (65 assertions at PR time: matrix 37, util 28)
- **Inner loop:** 1 red — wrong `mat_rotate` expectation; harness counters were `static` per TU so failures did not fail the process
- **Collision:** raced with #5 (db-roundtrip) on harness files
- **Notes:** First `./test_runner` on master; shared `extern` assertion counters.

### 20260809-4 — GAME_MAP for multi-agent slices
- **Agent/branch:** docs/game-map
- **PR:** https://github.com/chriswalker-cursor-build/Craft/pull/3
- **Allowlist:** `docs/GAME_MAP.md`
- **Proof:** N/A (docs-only; harness not yet required for this bounce)
- **Inner loop:** 0 red
- **Collision:** none
- **Notes:** Maps `src/*.c` roles, call flow, SQLite ownership, non-overlapping slice allowlists.

### 20260809-3 — cloud smoke (build verified)
- **Agent/branch:** docs/cloud-smoke
- **PR:** https://github.com/chriswalker-cursor-build/Craft/pull/2
- **Allowlist:** `docs/cloud-smoke.md`
- **Proof:** `cmake . && make` — pass; `./test_runner` — N/A (pre-harness)
- **Inner loop:** 0 red (build green on first run)
- **Collision:** none
- **Notes:** Recorded env (Ubuntu 24.04-based, CMake 3.28.3, gcc 13.3.0); `craft` target built.

### 20260809-2 — baseline, AGENTS.md, LAB_AUDIT, cursor rule
- **Agent/branch:** docs/baseline-and-rules
- **PR:** https://github.com/chriswalker-cursor-build/Craft/pull/1
- **Allowlist:** `AGENTS.md`, `docs/BASELINE.md`, `docs/LAB_AUDIT.md`, `.cursor/rules/craft-lab.mdc`
- **Proof:** N/A (docs-only; no `src/` / build changes); harness not yet
- **Inner loop:** 0 red
- **Collision:** none
- **Notes:** Lab policy + fork Baseline table seeded; Log left empty for later appends.

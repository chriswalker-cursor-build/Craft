# LAB_RETRO.md — Craft Cloud Agents lab

End-of-lab retrospective. Themes link to `docs/LAB_AUDIT.md` log rows (`20260809-2` … `20260809-8`).
Fork: https://github.com/chriswalker-cursor-build/Craft — not upstream `fogleman/Craft`.

---

## 1) What we did (bouncing ball)

| Bounce | Audit / PR | Outcome |
|--------|------------|---------|
| Baseline + rules | `20260809-2` / #1 | `AGENTS.md`, `docs/BASELINE.md`, `docs/LAB_AUDIT.md`, `.cursor/rules/craft-lab.mdc` — allowlists, proof culture, PR template |
| Cloud smoke | `20260809-3` / #2 | `cmake . && make` green in the cloud env (`docs/cloud-smoke.md`) |
| Source map | `20260809-4` / #3 | `docs/GAME_MAP.md` — `src/*` roles, place/break/save flow, non-overlapping slice allowlists |
| Characterisation tests | `20260809-5a`–`5b` / #4–#5 | C `test_runner` harness + matrix/util helpers; SQLite db round-trip |
| Stretch ADR | `20260809-6` / #6 | `docs/ADR-001-modular-craft.md` — **client/server** boundary, not `libworld`/`librender`/`libpersist` |
| Protocol note | `20260809-7a` / #7 | `docs/PROTOCOL.md` — existing v1 ASCII wire + ownership |
| Client / server slices | `20260809-7b`–`7c` / #8–#9 | C `client_fmt_*` + ownership docs; Python reject helpers + Handler/Model docs |
| Python reject tests | `20260809-7d` / #10 | `tests/test_server_rejects.py` (19 cases) without importing Py2 server runtime |
| Playtest / audit | `20260809-8` / #11 | Full merge log + re-run proof; human playtest placeholder left for paste |

Proof stack today (per Baseline): `cmake . && make` → `./test_runner` (117 assertions) → `python3 -m unittest tests.test_server_rejects -v` (19). Required GitHub CI checks: **none**.

---

## 2) Parallelism: what cleaned up vs what hurt

**Parallelised cleanly**

- Docs-only allowlists with one file each (smoke, GAME_MAP, ADR, PROTOCOL, audit) — no merge collisions.
- Client (#8) vs server (#9) after ADR/PROTOCOL: opposite sides of the C↔Python boundary; ADR said serialize only when the **same opcode** changes — these did not.

**Where it hurt**

- **Shared harness wiring:** #4 (pure-helpers) and #5 (db-roundtrip) both touched `CMakeLists.txt` + `tests/test_runner.c` → add/add conflict; db PR rebased/merged master (`20260809-5b`).
- **Stacked PRs:** reject tests (#10) based on `refactor/server-boundary` before #9 landed; had to merge into #9 then onto master (`20260809-7d`).
- **Allowlist rigidity is a feature:** server slice stayed `server.py` only; GAME_MAP lacked a server slice ID, so the agent followed ADR consequences and did not invent paths. Tight allowlists stop drive-bys; they also force stop-and-report when docs and code disagree.

Lesson: fan out docs and opposite-boundary slices; serialize anything that shares CMake/test_runner or stacks on an unmerged parent.

---

## 3) Gates that caught mistakes (and Bugbot)

| Gate | Caught |
|------|--------|
| **Build** (`cmake . && make`) | Env/dep reality early (smoke); kept every code PR honest |
| **C `./test_runner`** | Wrong `mat_rotate` expectation; harness counters that did not fail the process (`20260809-5a`) |
| **Python `unittest`** | Py2 `print` / import issues; method-binding of extracted helpers (`20260809-7d`) |
| **Playtest** | Step 8 left a human paste placeholder — agents must not invent “walked / place / save-reload” (`AGENTS.md`) |
| **Human review** | Merge order, stacked-base discipline, Stretch ADR choice vs library split |

**Bugbot:** enabled late in the lab. Craft PR reviews were not completing on this org/app setup. Treat as an **install lesson**, not a substitute for local proof: Bugbot sits **beside** `cmake` / `test_runner` / unittest / playtest, never instead of them. Finish the GitHub App install before trusting it as a gate.

---

## 4) Inner-loop red→green worth remembering

1. **Harness counters (`20260809-5a`):** `static` assert counts per translation unit → suite looked green while process exit stayed success. Fix: shared `extern` counters in `test_runner.c`, then lock real `mat_rotate` behaviour.
2. **CMake stack race (`20260809-5b`):** parallel test PRs fighting harness ownership. Fix: merge master; keep db suite + sqlite linkage on top of harness — do not “win” by deleting the other suite.
3. **Py2 under Py3 tests (`20260809-7d`):** `ast.parse` choked on `print` statements; helpers bound as unbound methods. Fix: line-based source extraction; store function refs on instance/`ns` only — still no TCP / SocketServer import.

Pattern: characterisation first, extract only enough purity to test, close the write loop before PR.

---

## 5) Customer pitch (≈2 minutes)

- **Rules vs prompt vs CI:** Project rules (`AGENTS.md` / `.cursor/rules`) encode non-negotiables; the task prompt supplies the allowlist and slice goal; CI (when present) re-runs the same proof so agents cannot skip it. Lab Baseline still has **no** required checks — that gap is real.
- **Tests before slices:** Lock helpers (matrix/util/db/client framing/server rejects) before structural boundary work — ADR Stretch did not invent a new module layout.
- **Path allowlists:** One coherent slice per PR; stop if you need a file outside the list. Opposite sides of a documented boundary can run in parallel; shared CMake/harness and same-opcode wire changes cannot.
- **Env before fan-out:** Smoke the cloud image (`cmake . && make`) before launching many agents — missing GL/deps or CMake cliffs waste parallel runs.
- **Bugbot beside proof, not instead:** Review bots help after install works; burden of proof in the PR body (commands + pass/fail + red→green notes) remains the contract.

---

## 6) Optional next hardening

1. **GitHub Actions** on this fork: `cmake . && make`, `./test_runner`, `python3 -m unittest tests.test_server_rejects -v` as required checks (Baseline table today: none).
2. **Finish Bugbot GitHub App** install/permissions on this org so Craft PR reviews actually complete — then keep it as an advisory/review gate next to CI, not a replacement for agent-run proof.
3. (Small follow-ups if continuing the strangler:) add a GAME_MAP server slice ID; paste Step 8 human playtest into `LAB_AUDIT`; prefer basing stacked test PRs on merged master.

---

## References

- `docs/LAB_AUDIT.md` — append-only log and PR template
- `docs/BASELINE.md` — proof commands
- `docs/ADR-001-modular-craft.md` — Stretch decision
- `docs/PROTOCOL.md` — wire + ownership characterisation
- `docs/GAME_MAP.md` — slice allowlists
- `AGENTS.md` — hard rules

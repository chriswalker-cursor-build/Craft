# AGENTS.md — Craft Cloud Agents lab

> Pair with `.cursor/rules/craft-lab.mdc` (same policy, shorter) if you use project rules.

## Mission

You are helping refactor a small Minecraft-like voxel game (C + OpenGL) using **strangler slices**, **path allowlists**, and **burden of proof** on every change. Work only on this fork — never open drive-by PRs against upstream `fogleman/Craft`.

## Hard rules (non-negotiable)

1. **Close the inner write loop before you stop.** After edits, run the project proof until green. Do not open or update a PR from a red loop.
2. **Exact proof commands** (update if `docs/BASELINE.md` differs):
   ```bash
   cmake . && make
   ./test_runner
   ```
   If `test_runner` does not exist yet, create/extend it only when the task is about the test harness; otherwise treat missing tests as a blocker and say so.
3. **Path allowlist is law.** Only edit paths listed in your task prompt (and files you must touch to wire a test target, e.g. `CMakeLists.txt`). If you need a file outside the allowlist, **stop** and report — do not expand scope.
4. **No drive-by rewrites.** No “while I’m here” cleanups, renames, or refactors outside the assigned slice. Prefer the smallest diff that meets the goal.
5. **No intentional gameplay changes** unless the task explicitly asks for them. Preserve feel: walk, place/break, save/reload.
6. **Docs-only tasks stay docs-only.** Scout/map/ADR/audit tasks must not change production `src/**` unless the prompt says otherwise.
7. **Burden of proof in the PR.** Every PR description must include:
   - Commands run + result (pass/fail)
   - Paths touched vs allowlist
   - Tests added/updated (or N/A with reason)
   - Manual playtest notes if behaviour could change (or N/A)
   - Any red→green fixes you made in-loop

## Prefer (policy)

- Characterisation / unit tests **before** structural refactors
- Extract pure functions when needed to make logic testable — minimal production edits for testability only
- One coherent slice per PR; stop when build + tests are green
- Update `docs/LAB_AUDIT.md` with one append-only row when the human asks (or when the prompt requires it)

## Do not

- Invent a new architecture north star — follow `docs/ADR-001-*.md` and `docs/GAME_MAP.md`
- Disable or skip checks to “go faster”
- Touch secrets, credentials, or upstream remotes other than this fork
- Claim playtest passed without actually launching `./craft` when the prompt requires it

## Definition of done

`cmake . && make` green **and** `./test_runner` green (once the harness exists) **and** PR body filled per the template in `docs/LAB_AUDIT.md` / the task prompt.

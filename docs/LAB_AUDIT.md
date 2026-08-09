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
| Proof commands | `cmake . && make` then `./test_runner` |
| Manual E2E check | place dirt → quit → reopen → block still there |
| Required CI checks | none configured |

## Log

<!-- Newest entry at the top -->

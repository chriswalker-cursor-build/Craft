# BASELINE.md — Craft Cloud Agents lab

Proof of a healthy baseline for this fork of [fogleman/Craft](https://github.com/fogleman/Craft).

## Proof commands

```bash
cmake . && make
./test_runner   # after harness exists (Step 5)
```

## Manual E2E check

1. Launch `./craft`
2. Place a dirt block
3. Quit
4. Reopen
5. Confirm the block is still there

## Notes

- Until Step 5 lands the harness, `./test_runner` is expected to be missing; treat that as N/A for docs-only and pre-harness steps.
- Do not claim playtest passed without actually launching `./craft` when a task requires it.

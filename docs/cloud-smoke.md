# Cloud smoke — build verified

Cloud Agent Step 3 smoke test: **build works** in this environment.

## Commands run

```bash
cmake . && make
```

**Result:** pass (exit 0). Targets built include `craft`, `glfw`, and GLFW examples/tests.

`./test_runner` was not present (expected pre-harness; N/A per `docs/BASELINE.md`).

## Environment notes

| Item | Value |
|------|--------|
| OS | Linux (Ubuntu 24.04-based), `x86_64`, kernel 6.12.94+ |
| CMake | 3.28.3 |
| Make | GNU Make 4.3 |
| Compiler | gcc 13.3.0 |
| Window/GL | X11 + GLX; OpenGL via legacy `libGL` (CMP0072 / GLVND preference unset — CMake dev warning only) |
| Key deps present | `build-essential`, `libcurl4-openssl-dev`, `libgl1-mesa-dev`, `libx11-dev`, `libglew-dev` |
| Doxygen | not installed (`Could NOT find Doxygen` — non-blocking) |

No `CMakeLists.txt` changes were required (minimum is 3.5; host CMake is 3.28.x, not 4.x).

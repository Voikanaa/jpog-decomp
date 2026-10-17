# Roadmap

A living document: update it whenever a phase advances or a decision changes. Findings go in
[modules.md](modules.md) and `docs/formats/`; function-level progress goes in
[progress.md](progress.md).

## Goal
A native Windows x86-64 build of *Jurassic Park: Operation Genesis* that runs the original
`Data/`, built from a clean-room reimplementation of the game and its Toshi engine.

## Decisions
| Topic | Decision |
|---|---|
| Strategy | Incremental reimplementation. Our x86 `jpog.exe` runs on the original engine DLLs (linked through generated import libraries), and modules are replaced one by one; every function is verified by differential tests against its original export. An x86 hook DLL for in-game A/B testing is deferred until a runnable original exists. At 100% coverage the same source builds as a standalone x64 exe. |
| Sources | Clean room. Only our own analysis of the shipped binaries and data; no community decomp code. |
| Names | Taken from the exported mangled symbols (`symbols/`, 923 classes); class declarations follow the recovered vtable order (`symbols/vtables.csv`). |
| Renderer | Direct3D 11 replaces D3D8 and the five shader DLLs. |
| Input | Raw Input / Win32 messages replace DirectInput 8. |
| Audio | miniaudio replaces FMOD 3. |
| Video | FFmpeg libavcodec (LGPL, dynamically linked) replaces Bink 1. |
| Toolchain | MSVC (VS 2022+ via vswhere), CMake presets + Ninja, Python 3 tools, Ghidra 12 via REA. |

Each third-party library is added only after the user approves it.

## Code rules
- Code must be portable to x64: no pointer↔int casts, no `sizeof(void*) == 4` assumptions, and
  explicit serialisation for every file format (never cast file bytes onto structs that contain
  pointers).
- In the x86 build, `static_assert` class layouts against the original
  (`src/common/layout.h`).
- **Float precision (gameplay):** the renderer creates its D3D8 device without
  `D3DCREATE_FPU_PRESERVE`, so during gameplay the x87 FPU runs at 24-bit precision and every
  `+ - * /` and `sqrt` rounds to float. Use plain `float` arithmetic, in the same operation order.
  The exceptions are the x87 transcendentals (fsin/fcos/fptan/fpatan) and CRT calls
  (pow/log/exp/...): compute those in `double` and round once. Kernel functions such as
  `TMath::ACos` return their double result on the x87 stack, so a caller that uses the value
  directly sees a double, not a float. Differential tests run the original inside
  `GamePrecisionScope` and must match bit for bit.
- Every reimplemented function gets a differential test against the original export (x86).
- Write clean, readable C++, not Ghidra transliteration. Cite the original symbol or address only
  where it helps re-verification.

## Phases and milestones
Each milestone has a matching GitHub milestone, with an issue for every work item.

| # | Phase | Exit criterion | Status |
|---|---|---|---|
| 0 | Foundations: symbols, build skeleton, repo/CI | Symbols dumped, skeleton builds in CI | **Done** |
| 0b | A runnable original for in-game testing | The retail exe runs with the physical disc | Blocked: no physical disc available; not required for engine work |
| 1 | Hook framework (x86) | **M1:** the game boots with the hook DLL and zero hooks | Deferred: needs a legitimately runnable original (0b) |
| L1 | Launch: our exe on the original engine DLLs | `jpog.exe` opens the game window | **Done** (crash on exit open) |
| L2 | Launch: front end | Intro and main menu render from the original data | Not started |
| 2 | TKernelInterface (memory → strings → math → RTTI → containers → files → events → scheduler → timers → lexer) | **M2:** the game boots with the kernel fully replaced | In progress: math classes complete (TMath, TVector2/3/4, TQuaternion, TMatrix44, TPlane), verified bit-exact |
| 3 | File formats (tmd/tml/tkl, fsm/inx/nwt, ttx/tto, gui, strpool, saves) | Parsers load every file of each type in `Data/` | Not started |
| 4 | Remaining engine modules (render, property parser, GUI, input, terrain, PGUI, SimJPTerrain, Units) | **M3:** a mission loads through our render and resource code | Not started |
| 5 | Game code (SimJP.exe), subsystem by subsystem | **M4:** the full game is playable on our code | Not started |
| 6 | Platform swap and x64 build | **M5:** the x64 exe reaches the menu. **M6:** the x64 exe plays a full mission and loads/saves original saves | Not started |

## Verification strategy
- **Per function:** a differential test calls the original export and our implementation with
  the same inputs from the x86 test harness (`tests/`) and requires a bit-exact match. When the
  decompiler's operand order is ambiguous, the disassembly decides.
- **Per module:** `jpog.exe` keeps running on the mix of original and reimplemented modules. Once
  a runnable original exists, the hook DLL adds per-function A/B toggles in the real game.
- **Per format:** the parser must handle every matching file in `Data/` (a tool script runs over
  the whole folder).
- **Continuous:** CI builds every preset and runs the repository checks on each push.

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
| Strategy | Hook-based incremental reimplementation. An x86 hook DLL redirects original functions to our C++; each can be toggled for A/B testing. At 100% coverage the same source builds as a standalone x64 exe. |
| Sources | Clean room. Only our own analysis of the shipped binaries and data; no community decomp code. |
| Names | Taken from the exported mangled symbols (`symbols/`, 923 classes). |
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
- In the x86 hook build, `static_assert` class layouts against the original
  (`src/common/layout.h`).
- **Float precision:** the original's x87 code runs at 53-bit precision and rounds to float only
  on store or return. Compute float expressions with `double` intermediates and round once; this
  reproduces the original bit for bit, as the differential tests confirm.
- Every reimplemented function gets a differential test against the original export (x86).
- Write clean, readable C++, not Ghidra transliteration. Cite the original symbol or address only
  where it helps re-verification.

## Phases and milestones
Each milestone has a matching GitHub milestone, with an issue for every work item.

| # | Phase | Exit criterion | Status |
|---|---|---|---|
| 0 | Foundations: symbols, build skeleton, repo/CI | Symbols dumped, skeleton builds in CI | **Done** |
| 0b | A runnable original for in-game testing | The owner's private runnable copy reaches the main menu | In progress: in-game test pending (copy kept outside the repo; see modules.md) |
| 1 | Hook framework (x86) | **M1:** the game boots with the hook DLL and zero hooks | Not started |
| 2 | TKernelInterface (memory → strings → math → RTTI → containers → files → events → scheduler → timers → lexer) | **M2:** the game boots with the kernel fully replaced | In progress: TMath scalar functions done, verified bit-exact |
| 3 | File formats (tmd/tml/tkl, fsm/inx/nwt, ttx/tto, gui, strpool, saves) | Parsers load every file of each type in `Data/` | Not started |
| 4 | Remaining engine modules (render, property parser, GUI, input, terrain, PGUI, SimJPTerrain, Units) | **M3:** a mission loads through our render and resource code | Not started |
| 5 | Game code (SimJP.exe), subsystem by subsystem | **M4:** the full game is playable on hooks | Not started |
| 6 | Platform swap and x64 build | **M5:** the x64 exe reaches the menu. **M6:** the x64 exe plays a full mission and loads/saves original saves | Not started |

## Verification strategy
- **Per function:** toggle its hook and compare behaviour in `game/`. Where practical, run
  differential tests that call the original export and our implementation with the same inputs
  from an x86 test harness.
- **Per format:** the parser must handle every matching file in `Data/` (a tool script runs over
  the whole folder).
- **Continuous:** CI builds every preset and runs the repository checks on each push.

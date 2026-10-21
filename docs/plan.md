# Roadmap

A living document: update it whenever a phase advances or a decision changes. Findings go in
[modules.md](modules.md) and `docs/formats/`; function-level progress goes in
[progress.md](progress.md).

## Goal
A modern, native, cross-platform build of *Jurassic Park: Operation Genesis* (Windows x64,
Linux x64, macOS arm64 and x64) that runs the original `Data/`, built from a reimplementation of
the game and its Toshi engine.

## Decisions
| Topic | Decision |
|---|---|
| Strategy | Incremental reimplementation. Our x86 `jpog.exe` runs on the engine DLLs (linked through generated import libraries), and modules are replaced one by one by drop-in DLLs built from our code (the kernel already is); every function is verified by differential tests against its original export. An x86 hook DLL for in-game A/B testing is deferred until a runnable original exists. The same source builds the portable game (see Build families). |
| Names | Taken from the exported mangled symbols (`symbols/`, 923 classes); class declarations follow the recovered vtable order (`symbols/vtables.csv`). |
| Memory | `TMemory` keeps its public API and observable behaviour (minimum size 8, alignment, zeroing, failure cases, `Realloc` returning null as in the original) on the C runtime heap instead of the original 32-bit packed heap, which cannot work on x64 and whose layout no other module uses. `TFreeList` is exact. In the hybrid x86 launch our kernel is not linked into `jpog.exe`, so the two heaps never mix; the switch happens when the whole kernel DLL is replaced. |
| Platform | SDL3: window, event loop, keyboard/mouse/gamepad input (replaces DirectInput 8), user paths (`SDL_GetPrefPath` for saves and options). |
| Renderer | bgfx (Direct3D 11/12 on Windows, Vulkan/OpenGL on Linux, Metal on macOS) replaces D3D8 and the five `Shaders/D3D` DLLs. Shaders are rewritten as bgfx `.sc` files and compiled by bgfx `shaderc` at build time. |
| Audio | miniaudio replaces FMOD 3. |
| Video | FFmpeg (libavformat/libavcodec, which decode Bink 1) replaces Bink. Loaded at run time: headers are vendored, binaries come from the system (Linux packages, Homebrew) or a pinned LGPL shared build on Windows. Without FFmpeg, videos are skipped. |
| Dev menu | Dear ImGui (SDL3 platform backend plus a small bgfx renderer backend of our own), in development builds. |
| Profiling | Tracy, behind `JPOG_TRACY` (default OFF), wired into `TProfiler` and frame markers. |
| Image output | stb_image_write for screenshots and texture dumps (dev menu, format tools). |
| Dependencies | Vendored in `third_party/`: pinned, unmodified (a documented subset of files is allowed), with version, license, upstream and hash in its README. Each library is added only after the user approves it. |
| Toolchains | Windows: MSVC (VS 2022+ via vswhere) for the x86 verification build and x64. Linux: GCC and Clang. macOS: AppleClang. CMake presets + Ninja, Python 3 tools, Ghidra 12 via REA. |
| CI | GitHub Actions on Windows, Linux and macOS; an ASan/UBSan job (Linux Clang), clang-tidy (portability/bugprone checks) and sccache for faster rebuilds. |

## Build families
- **x86 verification build** (Windows, MSVC): the hybrid `jpog.exe` on the original engine DLLs,
  our drop-in DLLs, and the differential tests against the original exports. It stays for as
  long as there is original code left to verify against.
- **Portable build** (Windows x64, Linux x64, macOS arm64/x64): our code as static libraries
  linked into a standalone `jpog`, with the platform layer above. Kernel, format and game code
  are shared with the x86 build; only the platform layer differs.

The kernel stays free of SDL: its OS calls (`TThread`, `TMutex`, `TNativeFile`, `THPTimer`,
`TDLL`, `TLocale`, `TProfiler`, `TKernelInterface`) go through a thin `src/platform/` layer
(Win32 and POSIX backends, standard C++ where the semantics allow), so the x86 drop-in DLL gains no
dependencies.

## Code rules
- Code must be portable: no pointer↔int casts, no `sizeof(void*) == 4` assumptions, and explicit
  serialisation for every file format (never cast file bytes onto structs that contain pointers).
- Fixed-width integer types for anything with size semantics: `long` is 64 bits on Linux and
  macOS (LP64) but 32 bits on Windows.
- Game wide strings are UTF-16 (`unsigned short`). `wchar_t` is 32 bits outside Windows, so the
  CRT `wcs*` calls in `TWString` and `TSystem` need our own UTF-16 routines there.
- Paths in the game data use `\` and are case-insensitive: the file layer resolves them against
  the real names on case-sensitive file systems (cached directory index).
- Compiler-specific code goes behind macros in `src/common/` (`TSTDCALL`, `TIMPORT_DATA`, …);
  compiler flags are set per compiler in CMake.
- In the x86 build, `static_assert` class layouts against the original
  (`src/common/layout.h`).
- **Float precision (gameplay):** the renderer creates its D3D8 device without
  `D3DCREATE_FPU_PRESERVE`, so during gameplay the x87 FPU runs at 24-bit precision and every
  `+ - * /` and `sqrt` rounds to float. Use plain `float` arithmetic, in the same operation order.
  The exceptions are the x87 transcendentals (fsin/fcos/fptan/fpatan) and CRT calls
  (pow/log/exp/...): compute those in `double` and round once. Kernel functions such as
  `TMath::ACos` return their double result on the x87 stack, so a caller that uses the value
  directly sees a double, not a float. Differential tests run the original inside
  `GamePrecisionScope` and must match bit for bit. Never build with fast-math, and disable FMA
  contraction (`-ffp-contract=off` on GCC/Clang; Clang contracts by default on arm64).
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
| L1 | Launch: our exe on the engine DLLs | `jpog.exe` opens the game window | **Done**, also on our kernel, with a clean exit |
| L2 | Launch: front end | Intro and main menu render from the original data | Not started |
| P | Portable foundation (parallel to phases 2–5) | **P1:** kernel, PPropertyParser and the unit tests build and pass on Windows x64, Linux and macOS in CI (`src/platform/`, multi-compiler CMake, presets). **P2:** the portable shell `jpog` opens an SDL3 window with bgfx, the ImGui dev menu, miniaudio and an FFmpeg probe, and passes a headless smoke run in CI | Not started |
| 2 | TKernelInterface (memory → strings → math → RTTI → containers → files → events → scheduler → timers → lexer) | **M2:** the game boots with the kernel fully replaced | In progress: every export reimplemented and verified (heap internals superseded); our `TKernelInterface.dll` runs L1; done when L2 boots on it |
| 3 | File formats (tmd/tml/tkl, fsm/inx/nwt, ttx/tto, gui, strpool, saves) | Parsers load every file of each type in `Data/` | Not started |
| 4 | Remaining engine modules (render, property parser, GUI, input, terrain, PGUI, SimJPTerrain, Units) | **M3:** a mission loads through our render and resource code | Not started. `TRender*` is reimplemented on bgfx; an x86 drop-in `TRenderD3DInterface.dll` on bgfx lets the hybrid launch exercise it early |
| 5 | Game code (SimJP.exe), subsystem by subsystem | **M4:** the full game is playable on our code | Not started |
| 6 | Platform swap and portable builds | **M5:** the portable build reaches the menu on Windows, Linux and macOS. **M6:** it plays a full mission and loads/saves original saves on all three | Not started |

## Verification strategy
- **Per function:** a differential test calls the original export and our implementation with
  the same inputs from the x86 test harness (`tests/`) and requires a bit-exact match. When the
  decompiler's operand order is ambiguous, the disassembly decides.
- **Per module:** `jpog.exe` keeps running on the mix of original and reimplemented modules. Once
  a runnable original exists, the hook DLL adds per-function A/B toggles in the real game.
- **Per format:** the parser must handle every matching file in `Data/` (a tool script runs over
  the whole folder).
- **Portable:** the unit tests (everything but the differential tests) run on all three OSes;
  sanitizers on Linux; a headless smoke run of `jpog` (bgfx Noop renderer).
- **Continuous:** CI builds every preset and runs the repository checks on each push.

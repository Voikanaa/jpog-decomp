# JPOG → native x64: decompilation & reimplementation plan

## Context
Goal: reverse engineer *Jurassic Park: Operation Genesis* (2003, PC, 32-bit) and produce a native
Windows x86-64 build that runs the original game data. Nothing has been analysed yet; REA + Ghidra
12.1.4 is verified working.

Decisions made with the user:
- **Strategy:** hook-based incremental reimplementation (re3/OpenJPOG style). Portable C++ replaces
  original functions inside the running 32-bit game, so each piece is checked against the original.
  When coverage is complete, the same source is built as a standalone x64 exe.
- **Clean room (revised 2026-10-17):** no community decomp sources are cloned or read. Everything
  comes from our own analysis of the shipped binaries and data.
- **Renderer:** Direct3D 11 replaces D3D8.
- **Audio/video:** miniaudio replaces FMOD 3; FFmpeg (libavcodec, LGPL, linked as a DLL) replaces
  Bink 1. Each library is downloaded only after asking the user.

## Recon findings (from this session)
- **Engine:** Blue Tongue "Toshi" (namespace `Toshi::`), built with MSVC 6 (linker 6.0). Math comes
  from Intel `libmmd.dll`.
- **Modules** (all x86; dependency order):
  `TKernelInterface` (1745 exports: memory, strings, files, math, TClass RTTI, scheduler/tasks,
  events) → `TApplication`, `TInputInterface`, `TRenderInterface` (1115) →
  `TInputDXInterface` (DInput8), `TRenderD3DInterface` (D3D8), `TTerrainInterface`,
  `PPropertyParser`, `TGuiInterface` → `PGUICore`, `PGUIRenderer`, `SimJPTerrain`, `Units` →
  shader plugins `Shaders/D3D/T{Line,Skin,Sprite,Sys,Terrain}ShaderD3D.dll` → `SimJP.exe` (game).
- **`SimJP.exe` exports 15,439 mangled C++ symbols**, and every DLL import is by mangled name. That
  gives us the class, method and signature names for almost the whole codebase.
- **Third-party libraries:** FMOD 3 (`fmod.dll`, UPX-packed, 41 imports), Bink (`binkw32.dll`,
  13 imports, 3 `.bik` videos), DirectSound, WinMM timers.
- **Data** (455 MB): wav 898, tmd 360, tml 195, ini 188, dds 173, fsm 129, inx 94, tga 66, ppm 55,
  ogg 30, nwt 28, mis 21, saves (s00/s01/s02/sav), tkl 12, ttx/tto 4, gui 3, bik 3, plus `strpool.dat`.
  The proprietary formats are tmd/tml/tkl/fsm/inx/nwt/ttx/tto.
- **Toolchain on the machine:** VS Build Tools 2026 (MSVC 14.51, x86 and x64 targets, bundled
  CMake + Ninja), Windows SDK 10.0.26100, git, Python 3.15, JDK 25. No new installs are needed to
  build.

## Repository layout (to create)
```
jpog-decomp/
  JPOG/                original install (read-only, untouched)
  game/                working copy of JPOG/ minus the .iso (where the hook DLL is tested; gitignored)
  references/          cloned community projects (gitignored, read-only)
  docs/                findings: modules.md, formats/*.md, classes/*.md, progress.md
  symbols/             demangled export tables per module (generated; source of truth for names/addresses)
  tools/               python scripts (export dumper, demangler, coverage reporter, format dumpers)
  src/
    toshi/             engine reimplementation, one dir per module (kernel/, render/, render_d3d11/, input/, gui/, terrain/, ...)
    game/              SimJP game code (+ units/, simjpterrain/, pgui/)
    platform/          Win32 window, timers, file I/O, audio (miniaudio), video (ffmpeg)
    hook/              x86-only: injector + hook DLL, address/export map, per-function enable list
  CMakeLists.txt, CMakePresets.json   presets: x86-hook-debug, x64-debug, x64-release
```
Initialise the project root as a git repo, with `JPOG/`, `game/`, `references/` and `.rea-tmp/`
gitignored.

## Phases

### Phase 0 — Foundations (first session)
2. Write `tools/dump_exports.py`, which reads PE exports/imports for all 19 binaries and demangles
   them with MSVC `undname.exe` from Build Tools. Its output goes to `symbols/<module>.csv`
   (name, demangled, RVA, kind). From that, derive the class list, method counts per class, and
   vtable symbols (`??_7`).
3. ~~Clone references~~ (dropped: clean-room policy).
4. Set up the CMake skeleton with the presets above, and update CLAUDE.md and memory.

### Phase 1 — Hook framework (x86)
- **Hook targets:** resolve them by mangled export name (`GetProcAddress` on the module), and use
  raw addresses only for non-exported statics. Install a 5-byte JMP patch on each, with a per-function
  on/off table read from `hooks.ini` so each piece can be A/B tested against the original.
- **Layout checks:** `static_assert` the size and offsets of every reimplemented class when built
  for x86, so the layout matches the original exactly. Under x64 the layout is natural.
- **Coverage report:** `tools/coverage.py` reports reimplemented vs. total functions per module,
  and writes the result to `docs/progress.md`.

### Phase 2 — TKernelInterface (bottom-up)
Order: memory (`tmalloc`/`tfree`, TFreeList) → TCString/TWString/TPCString/TLString/TLocale →
math (TVector2/3/4, TMatrix44, TQuaternion, TMath) → TClass/TObject RTTI → containers (TNodeList,
TQList, TGenericDList, TNodeTree) → TFile/TFileManager/TFileSystem → events
(TGenericEmitter/Listener) → TScheduler/TTask → THPTimer/TSystem → TFileLexer.
For each class: decompile with REA (`analyze_function` / `procedure_pseudo_code`) write clean C++, hook it, and confirm the game still boots and plays.

### Phase 3 — Formats and data
Document and write standalone parsers (with Python dumpers in `tools/`) for tmd/tml/tkl (models,
skeletons, animation), fsm/inx/nwt, ttx/tto, the GUI format, `strpool.dat` and saves. Never load
files by casting raw bytes onto 32-bit pointer layouts: parse explicitly so the same code works in
x64.

### Phase 4 — Remaining engine modules
TRenderInterface (resources, materials, meshes, skeletons/animation) → PPropertyParser → TGui →
TInput → TTerrain → PGUICore/PGUIRenderer → SimJPTerrain → Units. Keep the D3D8 path running under
hooks for now.

### Phase 5 — Game code (SimJP.exe, the bulk)
Work subsystem by subsystem, using the export names to group them (missions, dinosaur AI/FSM,
park management, visitors, economy, weather, UI screens). This is the longest phase. Track progress
in `docs/progress.md`.

### Phase 6 — Platform replacements and the x64 switch
- **Renderer:** write `render_d3d11/` against the TRenderInterface abstraction, and re-implement the
  5 shader DLLs as built-in D3D11 shaders (HLSL, emulating the fixed-function pipeline).
- **Input:** Raw Input / Win32 messages replace DirectInput 8.
- **Audio:** miniaudio replaces FMOD 3 (3D sound, sample/stream API surface).
- **Video:** FFmpeg Bink decoder for the 3 `.bik` files.
- **Math:** libmmd → CRT; drop the WinMM timers.
- Once 100% of the code is reimplemented, build the `x64-release` preset as one `JPOG64.exe` that
  links everything statically (no Toshi DLLs) and reads the original `Data/`.

## Working conventions
- Names come from the exported symbols. Notes and unknowns go in `docs/` and `symbols/`, because
  REA cannot rename items in the Ghidra database on Windows.
- Each function becomes clean, readable C++ (no Ghidra artefacts), and keeps a comment with its
  original address/symbol only where useful for re-verification.
- No `int`↔pointer casts and no `sizeof(void*)==4` assumptions; use `uintptr_t`/explicit
  serialisation.
- After every structural change, update CLAUDE.md (project rule).

## Verification
- **Per function:** toggle the hook in `hooks.ini` and compare original vs. reimplemented behaviour
  in `game/`. Where useful, run differential tests: call the original export and ours with the same
  inputs from a small x86 test harness (math, strings and parsers suit this well).
- **Per format:** the parser round-trips or dumps every file of that type in `Data/` without errors
  (`tools/` script over the whole folder).
- **Milestones:**
  - (M1) the game boots with the hook DLL and zero hooks;
  - (M2) the game boots with all of TKernelInterface replaced;
  - (M3) a mission loads using our render and resource code;
  - (M4) the full game is playable on hooks;
  - (M5) the x64 exe boots to the main menu;
  - (M6) the x64 build plays a full mission and loads/saves original save files.
- **Coverage:** `tools/coverage.py` output in `docs/progress.md`.

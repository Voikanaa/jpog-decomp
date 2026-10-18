# JPOG Decompilation Project

Reverse engineering of *Jurassic Park: Operation Genesis* (2003, Blue Tongue / Vivendi).
**Goal:** a native Windows x86-64 build of the game that runs the original `Data/`. Full plan:
`docs/plan.md`.

## Strategy (agreed with the user)
- Incremental reimplementation (re3/OpenJPOG style): portable C++ replaces original functions one
  at a time; at 100% coverage the same source builds as a standalone x64 exe.
- Order: TKernelInterface → file formats → other engine modules → SimJP game code → platform swap.
- x64 platform targets: Direct3D 11, Raw Input, miniaudio (replaces FMOD 3), FFmpeg DLL (replaces
  Bink). Ask before downloading any of these.
- Code rules: no pointer↔int casts, no 32-bit-pointer assumptions, parse file formats explicitly
  (never cast file bytes onto structs with pointers). x86 build: `static_assert` layouts
  (`src/common/layout.h`).
- Names come from the exported mangled symbols (`symbols/`). Notes go in `docs/` (REA cannot
  rename items in the Ghidra database on Windows).

## Ground rules (from the user — always follow)
1. **Never modify the system.** Only touch files inside this repo. No global installs, registry,
   env-var/PATH changes or writes to user/system dirs. Exceptions: session temp files and files
   tools write on their own (Ghidra settings in `%APPDATA%`, npm caches, temp).
2. **Ask before adding any external tool or dependency.** Approved tools live in `toolchain/`.
3. **Keep this CLAUDE.md current** in the same step as any structure/tool/finding/workflow change,
   and **keep it under 150 lines**: condense or move detail to `docs/` instead of growing it.
4. **Token budget:** at 90% of the context/token limit, stop and save: start no new work, commit
   and push verified work, note unfinished steps in the relevant issue, then end the task.

## License
GPL-3.0-or-later (`LICENSE`), covering our code/tools/docs only: not the game, its assets or
trademarks, or vendored third-party code. New deps must be GPLv3-compatible (MIT, BSD, zlib,
LGPL linked dynamically).

## Version control and issues
- Remote `git@github.com:Voikanaa/jpog-decomp.git` (private), branch `main`.
- **Never commit** game files (`JPOG/`, `game/`, ISO, any `.exe`/`.dll`/assets), `toolchain/`,
  `build/`, `analysis/`, `.rea-tmp/`, `rea-ghidra-*/`, `.claude/settings.local.json`, `.mcp.json`.
  Check with `git add -A --dry-run` before larger commits.
- **Commit frequently** in small logical units once verified, then push. Update README.md (status,
  tools, layout, setup) and this file in the same commit when they change.
- **Issues:** `toolchain/gh/bin/gh.exe -R Voikanaa/jpog-decomp`. Every work item has an issue;
  reference it (`Refs #n` / `Fixes #n`), tick checklists, open issues for new work, close finished
  ones with the hash. Milestones M0b–M6 mirror `docs/plan.md`.
- LF line endings except `.bat` (`.gitattributes`). Pre-commit hook (`.githooks/`, enabled) runs
  `check_repo.py --staged` and `format.py --check`. CI (`.github/workflows/ci.yml`) builds
  `x86-debug` + `x64-debug` and runs the same checks.

## Layout
- `JPOG/` original install (read-only reference): `SimJP.exe`, engine DLLs (`T*Interface.dll`,
  `TApplication.dll`), game DLLs (`Units`, `SimJPTerrain`, `PGUI*`, `PPropertyParser`), third-party
  (`binkw32`, `fmod`, `libmmd`), `Data/`, `Shaders/D3D/`, ISO. `game/` = working copy (gitignored).
- `symbols/` per-module exports/imports CSV, `classes.csv`, `vtables.csv` (generated).
- `docs/` `plan.md` (roadmap), `modules.md` (binary map), `progress.md` (coverage + log).
- `src/` `common/` (`abi.h` TSTDCALL, `TIMPORT_DATA` for data jpog.exe imports; `layout.h`);
  `toshi/kernel/` (lib `toshi_kernel`: every TKernelInterface export except the superseded heap
  internals); global `operator new`/`delete` route to `tmalloc`/`tfree` (as the original, also in
  jpog.exe); `ppropertyparser/` (lib `ppropertyparser`, in progress, #25); declarations only:
  `toshi/application/`, `toshi/render/`, `toshi/renderd3d/`, `toshi/gui/`; `game/` (**jpog.exe**: WinMain, `AApplication` with the
  original task tree, `ARootTask`/`ARenderer` skeletons, `CrashHandler`); `hook/` (deferred).
- `tests/` doctest suite `jpog_tests` (run by ctest). `support/OriginalModule` loads the original
  DLLs from `JPOG_ORIGINAL_DIR` (default `<root>/JPOG`) for differential tests (x86 only, skipped
  without the game). `support/Differential.h`: `Same`, `Original<Fn>`/`OriginalPP<Fn>`, `KernelMissing`.
  `support/FloatCompare.h`: ULP compare + `InputGenerator`. Call `__thiscall` originals via
  `Method<R, Args...>`; by-value class returns pass the hidden result pointer first.
- `third_party/` vendored pinned deps (versions/licenses/hashes in its README).
- `CMakePresets.json`: `x86-debug`, `x64-debug`, `x64-release` → `build/<preset>/`.
- **Hybrid launch (x86):** `jpog.exe` links the engine DLLs via import libs generated from
  `symbols/` (`tools/make_import_lib.py`, `cmake/OriginalModules.cmake`,
  `jpog_original_module(Name)`). Declarations in `src/toshi/` must match original signatures and
  vtable order. Post-build copies `jpog.exe` to `game/`; it must run there. Crashes write
  `game/jpog_crash.log`.

## Tools
- `tools/build.bat <preset> ["-DOPT=VAL"...]`: finds VS via vswhere, CMake + Ninja + ctest.
- `tools/format.py [--check]`: pinned clang-format over `src/`, `tests/`. **Run before committing.**
- `tools/coverage.py [--check]`: original exports ∩ our x86 symbols; regenerates the
  `docs/progress.md` table and README badge; `--check` (CI) also fails on unmatched `Toshi::`
  symbols (signature drift). Map new libs in its `MODULES` list; exports our design replaces go
  in its `SUPERSEDED` list (excluded from the percentage).
- `tools/ghidra_export.py <dll>|--all`: headless decompilation to `analysis/decomp/<Module>.c`/`.tsv`.
  **Prefer grepping these over REA for bulk work**; `tools/decomp_show.py <Module> <regex>` prints
  functions. `tools/resolve_imports.py` writes `SimJP.resolved.c` (IAT calls named; use it for
  game code; `--slots ADDR...` names IAT slots seen in disassembly). `tools/dump_vtables.py` → `symbols/vtables.csv` (**class declarations must follow
  it**). Also `dump_exports.py`, `check_repo.py`, `msvc.py`, `setup_mcp.py`.
- `tools/x87_expr.py <Module> <func|addr> [--args this,a] [--matrix] [--follow ADDR]`: symbolic
  x87 execution of an original function; prints each store as a fully parenthesised expression
  (spills become `tN`). **Use it for all float code**; Ghidra's sums are not reliable.
- Installed system-wide: VS Build Tools 2026 (MSVC 14.51, SDK 10.0.26100, CMake/Ninja), git,
  Python 3.15, JDK 25. Project-local `toolchain/`: Ghidra 12.1.4, clang-format 20.1.8, gh 2.102.0
  (user logs in once with `gh auth login`).
- **REA MCP** (`rea-agents@6.5.0`, skill `reverse-engineer-anything`) for targeted queries
  (xrefs, bytes, single functions). `.mcp.json` from `.mcp.json.example` via
  `python tools/setup_mcp.py`, then fully restart Claude Code. It points `GHIDRA_INSTALL_DIR` at
  `toolchain\ghidra` and TEMP/npm cache at `.rea-tmp\`. Usage: `open_binary` (absolute path) →
  `binary_overview`/`search_procedures`/`procedure_pseudo_code` → `close_binary`.

## Key findings
- **FPU precision:** D3D8 is created without `D3DCREATE_FPU_PRESERVE`, so gameplay x87 runs at
  24-bit precision. Plain float ops reproduce it. Differential tests wrap originals in
  `GamePrecisionScope` and require 0 ULP.
- fsin/fcos/fpatan are not affected by precision control: emulate them in double and round
  only the following arithmetic. `TMath::ACos` etc. return their double result in ST0, so callers
  that use the value directly (e.g. `TQuaternion::Slerp`) see a double.
- Ghidra prints commutative operands in arbitrary order and reassociates x87 sums: take float
  evaluation order from `tools/x87_expr.py`.
- Launch 1 reached, also on **our kernel**: x86 builds `TKernelInterface.dll` (exports from
  `tools/make_module_def.py`) and installs it into `game/` (`JPOG_REPLACE_KERNEL`, default ON;
  `"-DJPOG_REPLACE_KERNEL=OFF"` restores the original). The engine cannot delete its renderer
  (teardown frees the display texture before `DestroyDisplay` uses it, #20): `AApplication` keeps
  it alive until process exit; fix this in our own `TRenderD3DInterface`.
- Static constants in the DLLs are BSS, initialised at runtime: verify them through the loaded
  module, not the file.

# JPOG Decompilation Project

Reverse engineering of *Jurassic Park: Operation Genesis* (2003, Blue Tongue / Vivendi).
**Goal:** a modern native cross-platform build (Windows x64, Linux x64, macOS arm64/x64) that
runs the original `Data/`. Full plan: `docs/plan.md`.

## Strategy (agreed with the user)
- Incremental reimplementation (re3/OpenJPOG style): portable C++ replaces original functions one
  at a time, verified in the x86 Windows build; the same source builds the portable `jpog`.
- Order: TKernelInterface → file formats → other engine modules → SimJP game code → platform swap;
  the portable foundation (phase P: multi-OS CI, `src/platform/`, shell) runs in parallel.
- **Current state:** kernel and PPropertyParser 100% (drop-in DLLs, 9.1%); M2 needs the game to
  boot to the front end on our kernel (L2, #21). Phase P: kernel, tests and the portable shell
  `jpog` (#37) pass on Windows, Linux (GCC, Clang, ASan/UBSan) and macOS in CI.
- Portable stack: SDL3 (window/input/paths), bgfx (+ `shaderc`), miniaudio (for FMOD 3), FFmpeg
  loaded at run time (Bink), Dear ImGui (dev menu), Tracy (`JPOG_TRACY`), stb_image_write,
  doctest. All vendored in `third_party/` (pinned, unmodified); ask before downloading each. The
  kernel stays SDL-free: its OS calls go through `src/platform/` (Win32/POSIX).
- Code rules: no pointer↔int casts, no 32-bit-pointer assumptions, parse file formats explicitly
  (never cast file bytes onto structs with pointers); fixed-width ints (`long` is 64-bit on
  Linux/macOS), UTF-16 game strings (`wchar_t` is 32-bit off Windows), case-insensitive `\` data
  paths, no fast-math or FMA contraction. x86: `static_assert` layouts (`src/common/layout.h`).
- Names come from the exported mangled symbols (`symbols/`). Notes go in `docs/` (REA cannot
  rename items in the Ghidra database on Windows).
- Other JPOG/Toshi projects (OpenJPOG etc.): **facts only, never copy/port code** (licenses are
  non-commercial or absent). Credit every source in `CREDITS.md`.

## Ground rules (from the user — always follow)
1. **Never modify the system.** Only touch files inside this repo. No global installs, registry,
   env-var/PATH changes or writes to user/system dirs. Exceptions: session temp files and files
   tools write on their own (Ghidra settings in `%APPDATA%`, npm caches, temp).
2. **Ask before adding any external tool or dependency.** Approved tools are listed under Tools.
3. **Keep this CLAUDE.md current** in the same step as any structure/tool/finding/workflow change,
   and **keep it under 150 lines**: condense or move detail to `docs/` instead of growing it.
4. **Token budget:** at 90% of the context/token limit, stop and save: start no new work, commit
   and push verified work, note unfinished steps in the relevant issue, then end the task.
5. **Copy protection:** retail SimJP.exe is SecuROM-protected. Never add, document or distribute
   circumvention (tools, internals, patched/no-CD exes), and don't help run a cracked exe. There is
   no runnable original: verify with differential tests against the unprotected engine DLLs and
   static analysis.

## License
GPL-3.0-or-later (`LICENSE`), covering our code/tools/docs only: not the game, its assets or
trademarks, or vendored third-party code. New deps must be GPLv3-compatible (MIT, BSD, zlib,
Apache-2.0, LGPL linked dynamically).

## Version control and issues
- Remote `git@github.com:Voikanaa/jpog-decomp.git` (public), branch `main`.
- **Never commit** game files (`JPOG/`, `game/`, ISO, any `.exe`/`.dll`/assets), `.venv/`,
  `.cache/`, `build/`, `analysis/`, `rea-ghidra-*/`, `.claude/settings.local.json`, `.mcp.json`.
  Check with `git add -A --dry-run` before larger commits.
- **Commit frequently** in small logical units once verified, then push. Update README.md (status,
  tools, layout, setup) and this file in the same commit when they change. The README status
  table (per-module counts, milestones) is hand-written; only its badge is generated.
  No studio logos, game artwork or film stills/GIFs in the repo or README (legal; the user's call).
- **PRs are always merged with squash and merge** (`gh pr merge --squash`), never merge/rebase.
- **Issues:** `gh -R Voikanaa/jpog-decomp`. Every work item has an issue;
  reference it (`Refs #n` / `Fixes #n`), tick checklists, open issues for new work, close finished
  ones with the hash. Milestones M0b–M6 mirror `docs/plan.md`.
- LF line endings except `.bat` (`.gitattributes`). Pre-commit hook (`.githooks/`, enabled) runs
  `check_repo.py --staged`, `format.py --check` and `ruff check tools`. CI: `ci.yml` builds every
  preset with `JPOG_WERROR=ON` (keep code warning-free), skipping docs-only changes; `checks.yml`
  runs the hook's checks on every change. Don't trigger extra runs: the PR run is enough. Actions
  are pinned to commit hashes (`# vX` comment; Dependabot bumps them monthly). `SECURITY.md`.

## Layout
- `JPOG/` original install (read-only reference): `SimJP.exe`, engine DLLs (`T*Interface.dll`,
  `TApplication.dll`), game DLLs (`Units`, `SimJPTerrain`, `PGUI*`, `PPropertyParser`), third-party
  (`binkw32`, `fmod`, `libmmd`), `Data/`, `Shaders/D3D/`, ISO. `game/` = working copy (gitignored).
- `symbols/` per-module exports/imports CSV, `classes.csv`, `vtables.csv` (generated).
- `docs/` `plan.md` (roadmap), `modules.md` (binary map), `progress.md` (coverage + log),
  `formats/README.md` (catalogue of every `Data/` format: magic, status, sources, issue).
- `src/` `common/` (`abi.h` TSTDCALL, `TIMPORT_DATA` for data jpog.exe imports; `layout.h`,
  `JPOG_NO_SANITIZE_VPTR` for original layout tricks); `platform/` (every kernel OS call: `Thread.h`,
  `System.h`, `CRuntime.h` UTF-16/MSVC-compatible CRT; `win32/`, `posix/` backends);
  `toshi/kernel/` (lib `toshi_kernel`: every TKernelInterface export except the superseded heap
  internals; core classes at its root, the rest in `math/ containers/ strings/ file/ memory/
  thread/ time/`, mirrored by `tests/kernel/`); global `operator new`/`delete` route to `tmalloc`/`tfree` (as the original, also in
  jpog.exe); `ppropertyparser/` (lib `ppropertyparser` + drop-in DLL, complete); declarations only:
  `toshi/application/`, `toshi/render/`, `toshi/renderd3d/`, `toshi/gui/`; `game/` (**jpog.exe**: WinMain, `AApplication` with the
  original task tree, `ARootTask`/`ARenderer` skeletons, `AOptions`/`AOptionLogic` (`Options.ini`,
  display mode), `ALocale`, `CrashHandler`). No hook DLL yet (deferred, #2). `shell/` (portable
  **jpog**, not x86: SDL3, bgfx (D3D12/Vulkan/Metal; shaderc, DXC fetched to `.cache/` for DXIL),
  ImGui F1 menu, miniaudio, FFmpeg probe; `--smoke`/`--renderer`/`--screenshot`).
- `tests/` doctest suite `jpog_tests` (ctest; `kernel/ ppropertyparser/ game/ support/`; `lsan.supp`: leaks the original has). `support/OriginalModule` loads the original
  DLLs from `JPOG_ORIGINAL_DIR` (default `<root>/JPOG`) for differential tests (x86 only, skipped
  without the game). `support/Differential.h`: `Same`, `Original<Fn>`/`OriginalPP<Fn>`, `KernelMissing`.
  `support/FloatCompare.h`: ULP compare + `InputGenerator`. Call `__thiscall` originals via
  `Method<R, Args...>`; by-value class returns pass the hidden result pointer first.
- `third_party/` vendored pinned deps (README: versions/licenses/hashes). Nested upstream `.gitignore`s can hide files: check `git status --ignored third_party`, `git add -f`.
- `CMakePresets.json`: `x86-debug`, `x64-debug`, `x64-release`, `linux-debug`/`-release`/`-asan`,
  `macos-debug`/`-release` → `build/<preset>/`. Vendored code drops our warnings and sanitizers.
- **Hybrid launch (x86):** `jpog.exe` links the engine DLLs via import libs generated from
  `symbols/` (`tools/make_import_lib.py`, `cmake/OriginalModules.cmake`,
  `jpog_original_module(Name)`). Declarations in `src/toshi/` must match original signatures and
  vtable order. Post-build copies `jpog.exe` to `game/`; it must run there. Crashes write
  `game/jpog_crash.log`.

## Tools
- `tools/build.py <preset> [--keep-going] [-DOPT=VAL...]` (any OS; `build.bat` wraps it): VS env
  via vswhere on Windows, CMake + Ninja + ctest.
- `tools/format.py [--check]`: pinned clang-format over `src/`, `tests/`. **Run before committing.**
- `tools/coverage.py [--check]`: original exports ∩ our x86 symbols; regenerates the
  `docs/progress.md` table, README badge and the README progress cards
  `docs/progress-{light,dark}.svg` (drawn by `tools/progress_card.py`); `--check` (CI) also fails on unmatched `Toshi::`
  symbols (signature drift). Map new libs in its `MODULES` list; exports our design replaces go
  in its `SUPERSEDED` list (excluded from the percentage).
- `tools/ghidra_export.py <dll>|--all` (script `tools/ghidra/ExportDecompilation.java`): headless decompilation to `analysis/decomp/<Module>.c`/`.tsv`.
  **Prefer grepping these over REA for bulk work**; `tools/decomp_show.py <Module> <regex>` prints
  functions. `tools/resolve_imports.py` writes `SimJP.resolved.c` (IAT calls named; use it for
  game code; `--slots ADDR...` names IAT slots seen in disassembly). `tools/dump_vtables.py` → `symbols/vtables.csv` (**class declarations must follow
  it**). Also `dump_exports.py`, `check_repo.py`, `msvc.py`, `setup_mcp.py`.
- `tools/x87_expr.py <Module> <func|addr> [--args this,a] [--matrix] [--follow ADDR]`: symbolic
  x87 execution of an original function; prints each store as a fully parenthesised expression
  (spills become `tN`). **Use it for all float code**; Ghidra's sums are not reliable.
- Installed system-wide: VS Build Tools 2026 (MSVC 14.51, SDK 10.0.26100, CMake/Ninja), git,
  Python 3.15, JDK 25, gh 2.102.0 (global; user logs in once with `gh auth login`). Ghidra 12.1.4
  outside the repo (path set once via `setup_mcp.py --ghidra`). uv 0.12
  (user-installed). Project `.venv/` from `uv sync` (`pyproject.toml`/`uv.lock`; this machine needs
  `--system-certs`): clang-format 20.1.8, ruff 0.17.1 (config in `pyproject.toml`). After dep
  changes: `uv lock` and regenerate the pip fallback with
  `uv export --only-group dev --no-hashes --no-header -o requirements-dev.txt`.
- **REA MCP** (`rea-agents@6.5.0`, skill `reverse-engineer-anything`) for targeted queries
  (xrefs, bytes, single functions). `.mcp.json` from `.mcp.json.example` via
  `python tools/setup_mcp.py --ghidra <dir>` (remembered; `ghidra_export.py` reads it too), then
  fully restart Claude Code. It sets `GHIDRA_INSTALL_DIR` and TEMP/npm cache at `.cache\rea\`.
  Usage: `open_binary` (absolute path) → `binary_overview`/`search_procedures`/`procedure_pseudo_code` → `close_binary`.

## Key findings
- **FPU precision:** D3D8 is created without `D3DCREATE_FPU_PRESERVE`, so gameplay x87 runs at
  24-bit precision. Plain float ops reproduce it. Differential tests wrap originals in
  `GamePrecisionScope` and require 0 ULP.
- fsin/fcos/fpatan are not affected by precision control: emulate them in double and round
  only the following arithmetic. `TMath::ACos` etc. return their double result in ST0, so callers
  that use the value directly (e.g. `TQuaternion::Slerp`) see a double.
- Ghidra prints commutative operands in arbitrary order and reassociates x87 sums: take float
  evaluation order from `tools/x87_expr.py`.
- Launch 1 reached, also on **our DLLs**: x86 builds `TKernelInterface.dll` and
  `PPropertyParser.dll` (exports from `tools/make_module_def.py`) and installs them into `game/`
  (`JPOG_REPLACE_KERNEL` / `JPOG_REPLACE_PPROPERTYPARSER`, default ON; `=OFF` restores the
  original). Drop-in DLLs other than the kernel import it: kernel data they use must be
  `TIMPORT_DATA` (`JPOG_IMPORT_ENGINE_DATA`). The engine cannot delete its renderer
  (teardown frees the display texture before `DestroyDisplay` uses it, #20): `AApplication` keeps
  it alive until process exit; fix this in our own `TRenderD3DInterface`.
- SimJP.exe: many calls go through one pointer Ghidra cannot resolve (`DAT_00d8cec0`) and its
  locals are often merged; infer those calls from context and read the assembly (REA
  `procedure_assembly`, jump tables from the file) for anything that matters.
- Static constants in the DLLs are BSS, initialised at runtime: verify them through the loaded
  module, not the file.

# JPOG Decompilation Project

Reverse engineering / decompilation of *Jurassic Park: Operation Genesis* (2003, Blue Tongue / Vivendi).
**Goal:** a native Windows x86-64 build of the game that runs the original `Data/`.

## Strategy (agreed with the user)
- Hook-based incremental reimplementation (re3/OpenJPOG style): portable C++ replaces original
  functions inside the running 32-bit game (`game/`), one at a time, toggled per function for A/B
  testing. When coverage reaches 100%, the same source builds as a standalone x64 exe.
- Clean-room: no community decomp sources (see Status).
- Order: TKernelInterface → file formats → other engine modules → SimJP game code → platform swap.
- Platform targets for x64: **Direct3D 11** (replaces D3D8 + shader DLLs), Raw Input (replaces
  DInput8), **miniaudio** (replaces FMOD 3), **FFmpeg/libavcodec** DLL (replaces Bink 1).
  Ask the user before downloading any of these.
- Code rules: no pointer↔int casts and no 32-bit-pointer assumptions. Parse file formats
  explicitly (never cast raw file bytes onto structs that contain pointers). In the x86 build,
  `static_assert` class layouts against the original (`src/common/layout.h`).
- Names come from the exported mangled symbols (`symbols/`). Notes live in `docs/` (REA cannot
  rename items in the Ghidra database on Windows).
- Full plan: `docs/plan.md`.

## Ground rules (from the user — always follow)
1. **Never modify the system.** Only create/modify/delete files inside the project root (this repo's folder).
   No global installs (pip/npm/choco/winget -g etc.), no registry/env-var/PATH changes,
   no writes to user profile or system directories. Session scratch/temp files are the only exception.
   Exception: files that tools write on their own while running (Ghidra settings in `%APPDATA%`,
   npm/npx caches, temp files, etc.) are fine — no need to redirect or ask about those.
2. **Need an external tool or dependency? Ask the user first.** Do not download or install anything on your own.
   Approved tools live under `toolchain/`.
3. **Keep this CLAUDE.md up to date.** Whenever structure, tools, findings, or workflows change, update this file in the same step.
4. **Token budget:** once 95% of the context/token limit is used, stop starting new work: save
   the current task (commit and push verified work, note unfinished steps in the relevant issue),
   then end the task.

## License
GPL-3.0-or-later (`LICENSE`, the official GNU text), chosen 2026-10-17. It covers our code, tools
and docs only: not the game, its assets, the *Jurassic Park* trademarks or logo, or vendored
third-party code (which keeps its own license). New dependencies must be GPLv3-compatible:
MIT, BSD, zlib, or LGPL linked dynamically.

## Version control
- Remote: `git@github.com:Voikanaa/jpog-decomp.git` (private, SSH key authenticates as
  Voikanaa). Branch `main`.
- **Never commit** game binaries or data (`JPOG/`, `game/`, the ISO, any `.exe`/`.dll`/assets),
  `toolchain/`, `build/`, `.rea-tmp/`, `rea-ghidra-*/`, `.claude/settings.local.json` or `.mcp.json` (machine-specific).
  `.gitignore` covers these; run `git add -A --dry-run` before larger commits to double-check.
- Commit in small, logical units with descriptive messages. `symbols/` is committed (regenerate
  it with `tools/dump_exports.py`).
- Line endings are set in `.gitattributes`: LF for source, docs and CSV; CRLF for `.bat`.
- **Issues** (user instruction: keep them current). Use `toolchain/gh/bin/gh.exe -R Voikanaa/jpog-decomp`.
  Milestones M0b to M6 mirror `docs/plan.md`; labels are `area: *`, `type: research` and `needs: owner decision`.
  Every work item has an issue. Reference it in commits (`Refs #n`; `Fixes #n` closes it), tick
  checklists as work lands, open new issues for discovered work, and close finished ones with
  the commit hash. Templates are in `.github/ISSUE_TEMPLATE/`.
- **Keep README.md up to date** (user instruction): status table, tools table, layout and setup
  must reflect reality in the same commit that changes them.
- **Commit frequently** (user instruction): commit each logical unit as soon as it works and is
  verified, then push to `origin/main`. Update CLAUDE.md in the same commit whenever structure,
  tools, findings or workflow change.

## Layout
- `JPOG/` — original game install (treat as read-only reference; do not modify originals)
  - `SimJP.exe` — main executable
  - `T*Interface.dll`, `TApplication.dll` — engine modules (kernel, render D3D, input, GUI, terrain)
  - `Units.dll`, `SimJPTerrain.dll`, `PGUICore.dll`, `PGUIRenderer.dll`, `PPropertyParser.dll` — game/UI modules
  - Third-party: `binkw32.dll` (Bink video), `fmod.dll` (audio), `libmmd.dll` (Intel math lib)
  - `Data/` — game data (ini configs, Models, Terrain, Units, GUI, Missions, Sound…)
  - `Shaders/D3D/`, `Jurassic Park Operation Genesis.iso` (original disc image)
- `game/` — working copy of `JPOG/` without the .iso; the hook DLL is tested here (gitignored)
- `references/` — reserved; unused under the clean-room policy (gitignored)
- `symbols/` — generated by `tools/dump_exports.py`: per-module exports/imports CSV (mangled +
  demangled, RVA/VA) and `classes.csv`
- `docs/` — `plan.md` (living roadmap: decisions, phases, status), `modules.md` (binary map +
  protection notes), `progress.md` (coverage + log), later `formats/`, `classes/`
- `README.md`, `CONTRIBUTING.md` (workflow, commit and style conventions)
- `.github/workflows/ci.yml`: CI on windows-latest. It builds the `x86-debug` and `x64-debug`
  presets, then runs `check_repo.py`, a Python compile check and a clang-format check.
- `.githooks/pre-commit`: runs `check_repo.py --staged` and `format.py --check`. Enable it with
  `git config core.hooksPath .githooks` (already set in this clone).
- `.editorconfig`, `.clang-format` (tabs, Allman braces, 120 columns), `.gitattributes`
  (LF everywhere except `.bat`)
- `tools/ghidra_export.py <module.dll>|--all` + `tools/ghidra/ExportDecompilation.java`: headless
  Ghidra decompilation of every function into `analysis/decomp/<Module>.c` (one
  `// ==== <addr> <name>` block per function) and `.tsv` (index). Ghidra projects are kept in
  `analysis/ghidra/`. `analysis/` is gitignored because it is derived from the binaries. **Prefer
  grepping these exports over REA for bulk work** (REA's JSON responses are large); use REA for
  targeted queries (xrefs, bytes, single functions).
- `tools/coverage.py` — coverage = original exported functions ∩ symbols defined in our x86 libs
  (identical mangling). It regenerates the table in `docs/progress.md` and the README badge;
  `--check` runs in CI and also fails on *unmatched* symbols (our `Toshi::` symbols with no
  original export), which catches signature drift. Map new libraries to modules in its `MODULES` list.
- `tools/format.py [--check]` — formats tracked C++ in `src/` and `tests/` with the pinned
  clang-format (version-checked). Runs in CI and in the pre-commit hook. **Run it before committing C++.**
- `tools/decomp_show.py <Module> <regex>` — print functions from `analysis/decomp/`.
- `tools/dump_vtables.py` — writes `symbols/vtables.csv`: the virtual method order of all 704
  exported vtables (entries resolved per module, including import thunks and `_purecall`).
  **Our class declarations must reproduce this order.**
- `tools/resolve_imports.py` — writes `analysis/decomp/SimJP.resolved.c`, the SimJP decompilation
  with calls through the game's own IAT (0x739000) annotated with import names (about 21k sites).
  **Use `SimJP.resolved` when reading game code.** The remaining `(*DAT_00d8cec0)(...)` calls are
  indirect imports; identify them from context (neighbouring identical sequences, arguments).
- `tools/` — `setup_mcp.py` (writes `.mcp.json` for this clone), `check_repo.py` (hygiene: no game files or files over 5 MB), `msvc.py` (finds VS/MSVC tools via vswhere; no hardcoded paths), `dump_exports.py`, `build.bat <preset>` (finds VS via vswhere, sets up the MSVC env per-process, then runs CMake + Ninja)
- `src/` — reimplementation: `common/` (`abi.h` TSTDCALL, `layout.h`), `toshi/kernel/` (static lib
  `toshi_kernel`: TMath, TVector2, TVector3, TVector4 implemented; TCString/TObject/TClass/TKernelInterface
  declared only), `toshi/application/` and `toshi/render/` (declarations only), `game/` (**jpog.exe**:
  WinMain, `AApplication`, `CrashHandler`), `hook/` (x86 DLL, deferred, `JPOG_HOOK=ON` only)
- **Hybrid launch (x86):** `jpog.exe` links against the original, unprotected engine DLLs through
  import libs generated from `symbols/` (`tools/make_import_lib.py`, `cmake/OriginalModules.cmake`,
  `jpog_original_module(Name)` -> `original::Name`). Declarations in `src/toshi/` must match the
  original signatures and vtable order (`symbols/vtables.csv`). The post-build step copies
  `jpog.exe` into `game/`, and it must run from there. On a crash it writes `game/jpog_crash.log`
  (exception, module+offset, stack).
- `tests/` — doctest (`third_party/doctest`, v2.4.11) suite `jpog_tests`, run by ctest from
  `tools/build.bat`. `support/OriginalModule` loads the original DLLs from `JPOG_ORIGINAL_DIR`
  (CMake cache var or env var; default `<root>/JPOG`) for **differential tests** (x86 only;
  skipped when the game is absent, e.g. in CI). `support/FloatCompare.h` provides the ULP
  comparison and a deterministic input generator. Call original `__thiscall` members through
  `Method<R, Args...>` (`OriginalModule.h`), which goes via `__fastcall` with `this` in ECX; for
  members returning a class by value, the hidden result pointer is the first argument.
- `third_party/` — vendored, pinned deps with versions, licenses and hashes in `third_party/README.md`
- `CMakeLists.txt`, `CMakePresets.json` — presets `x86-debug`, `x64-debug`, `x64-release` → `build/<preset>/`
- `toolchain/` — project-local tools
  - `ghidra/` — Ghidra 12.1.4 (launch via `toolchain/ghidra/ghidraRun.bat`)
  - `clang-format/` — clang-format 20.1.8 (exe from the official PyPI wheel, SHA-256 verified;
    LICENSE.md alongside). Pinned: CI installs the same version. Use `python tools/format.py`.
  - `gh/` — GitHub CLI 2.102.0 (official portable zip, SHA-256 verified), used for issues,
    milestones, labels and CI status: `toolchain/gh/bin/gh.exe`. The user logs in once
    (`gh auth login`); gh stores its token in `%APPDATA%\GitHub CLI`.
- `.mcp.json` — project-scoped REA MCP registration, machine-specific and gitignored. Generate it
  from `.mcp.json.example` with `python tools/setup_mcp.py`, then restart Claude Code.
- `.rea-tmp/` — REA/Ghidra temp projects + npm cache (disposable)
- `.claude/settings.local.json` — enables the project `.mcp.json` server (`enabledMcpjsonServers: ["rea"]`)

## Tools available
- VS Build Tools 2026 (MSVC 14.51, x86 + x64), with bundled CMake/Ninja and Windows SDK
  10.0.26100, already installed system-wide. `undname.exe`/`dumpbin.exe` are under
  `VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64`. git, Python 3.15.
- Ghidra 12.1.4 (local, in `toolchain/ghidra`). Java: system JDK 25 (Eclipse Adoptium), already installed.
- **REA MCP server** (`rea-agents@6.5.0`) — use the `reverse-engineer-anything` skill.
  - Registered per-project in `.mcp.json` (overrides the user-level `rea` entry) with:
    - `GHIDRA_INSTALL_DIR` → `toolchain\ghidra` (enables the Ghidra provider)
    - `TEMP`/`TMP` → `.rea-tmp` so Ghidra's ephemeral projects stay inside this folder
    - `npm_config_cache` → `.rea-tmp\npm-cache` so npx doesn't write to the user profile
  - Ghidra provider on Windows: native x86 PE exe/DLL analysis (decompile, xrefs, strings, etc.);
    database annotation/renaming is NOT supported on Windows ("P0"), so keep names/notes in project files.
  - Usage: `open_binary` with absolute path (e.g. `<project root>\JPOG\SimJP.exe`), then
    `binary_overview`, `search_procedures`, `analyze_function`, `procedure_pseudo_code`… `close_binary` when done.

## Status / notes
- REA configured via `.mcp.json` + enabled in `.claude/settings.local.json`. Needs a full Claude Code
  restart (not just `/mcp` reconnect) to pick it up. Verify with `binary_session`: the ghidra provider
  candidate should not say "GHIDRA_INSTALL_DIR is not set".
- 2026-10-17: verified — `binary_session` reports Ghidra provider `available` (12.1.4, JDK 25.0.4,
  headless + native decompiler checks all OK).
- **Key finding (precision):** JPOG creates its D3D8 device without `D3DCREATE_FPU_PRESERVE`
  (behaviour flags are only 0x20/0x40/0x80, set in TRenderD3DInterface), so gameplay runs the x87
  at 24-bit precision. Implement float code as plain float ops; transcendentals and CRT calls go
  through double with one rounding. Differential tests wrap the original in `GamePrecisionScope`
  (`tests/support/OriginalModule.h`) and use 0-ULP tolerance.
- 2026-10-17 **Launch 1 reached:** our `jpog.exe` opens the 640x480 `TRenderD3DInterface` window with
  a D3D8 device through the original DLLs and runs the frame loop. Open problem (issue): crash on
  exit. The renderer's `DestroyDisplay` calls through a broken object at `this+0xF0` (a texture
  made from system resource 18, `TextureFactory`, in `CreateDisplay`). The shader DLLs *are*
  loaded, so the cause is still open; compare with the game's full init (ARootTask::OnCreate:
  ARenderer, material libraries, ...).
- 2026-10-17: Phase 2 started. TMath scalar functions are done (55 functions; differential tests
  are exact). All modules are decompiled to `analysis/decomp/` (~27k functions).
- 2026-10-17 Phase 0: game copied to `game/`; symbols dumped (923 classes); CMake skeleton builds
  `jpog_hook.dll` (x86) OK. Binary map + protection notes are in `docs/modules.md`.
- **Clean-room policy (user decision, 2026-10-17):** the code is derived ONLY from our own analysis
  of the shipped binaries and data. Do NOT clone, read or copy community decomp source
  (AdventureT/OpenJPOG, InfiniteC0re/OpenBarnyard/OpenToshi, jpog-blender, etc.), so their
  licenses never apply to this project. `references/` stays empty unless the user changes this.

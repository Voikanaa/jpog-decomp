<div align="center">

<img src="docs/jpog_logo.png" alt="Jurassic Park: Operation Genesis" width="420">

# jpog-decomp

**A clean-room reimplementation of *Jurassic Park: Operation Genesis* (2003) as a native Windows x64 game**

<p>
<a href="https://github.com/Voikanaa/jpog-decomp/actions/workflows/ci.yml"><img alt="CI" src="https://github.com/Voikanaa/jpog-decomp/actions/workflows/ci.yml/badge.svg"></a>
<a href="docs/progress.md"><!-- coverage-badge:begin --><img alt="Reimplemented" src="https://img.shields.io/badge/reimplemented-0.3%25-red?style=flat-square"><!-- coverage-badge:end --></a>
<a href="LICENSE"><img alt="License: GPL-3.0-or-later" src="https://img.shields.io/badge/license-GPL--3.0--or--later-blue?style=flat-square"></a>
<img alt="Status: early development" src="https://img.shields.io/badge/status-early%20development-orange?style=flat-square">
<br>
<img alt="Platform: Windows x64" src="https://img.shields.io/badge/platform-Windows%20x64-0078D6?style=flat-square">
<img alt="C++20" src="https://img.shields.io/badge/C%2B%2B-20-00599C?style=flat-square&logo=cplusplus&logoColor=white">
<img alt="CMake + Ninja" src="https://img.shields.io/badge/build-CMake%20%2B%20Ninja-064F8C?style=flat-square&logo=cmake&logoColor=white">
<img alt="Direct3D 11" src="https://img.shields.io/badge/renderer-Direct3D%2011-5C2D91?style=flat-square">
<img alt="Ghidra" src="https://img.shields.io/badge/analysis-Ghidra-C21325?style=flat-square">
</p>

[Roadmap](docs/plan.md) · [Binary map](docs/modules.md) · [Progress](docs/progress.md) · [Issues](https://github.com/Voikanaa/jpog-decomp/issues) · [Milestones](https://github.com/Voikanaa/jpog-decomp/milestones) · [Contributing](CONTRIBUTING.md)

</div>

---

## About

*Jurassic Park: Operation Genesis* was built by Blue Tongue Software on their in-house **Toshi**
engine. It shipped as a 32-bit executable with Direct3D 8, FMOD 3, Bink video and SecuROM copy
protection, all of which are increasingly hard to run on modern systems.

This project rebuilds the game and engine from scratch in modern, portable C++. The goal is a
**native 64-bit Windows build** that plays the original game data unchanged.

- **Clean room:** every line comes from our own analysis of the shipped binaries and data.
- **Verified piece by piece:** each function is checked against the original game while it runs.
- **Bring your own game:** the repository contains no game code, assets or binaries.

## How it works

```mermaid
flowchart LR
    A["Original game<br/>32-bit SimJP.exe + Toshi DLLs"] -->|Ghidra analysis| B["Clean-room C++<br/>src/"]
    B -->|x86 build| C["Hook DLL<br/>replaces original functions<br/>one at a time"]
    C -->|A/B test in the real game| A
    B -->|"x64 build (100% coverage)"| D["Native x64 game<br/>D3D11 · miniaudio · FFmpeg"]
```

1. **Analyse.** Headless Ghidra decompiles every module. More than 22,000 exported C++ symbols
   give us the original class and method names.
2. **Reimplement.** Readable, portable C++20 that mirrors the original API (`Toshi::TMath`,
   `AUnitDinosaur`, …).
3. **Verify.** Our functions are hooked into the running 32-bit game and differential tests
   compare them with the original DLL exports.
4. **Port.** Once everything is replaced, the same source builds as a standalone x64 executable on
   Direct3D 11, Raw Input, miniaudio and FFmpeg.

## Status

| Milestone | Description | |
|---|---|:-:|
| Foundations | Symbols, build system, CI, analysis tooling | ✅ |
| Runnable original | Retail exe for in-game A/B testing (needs the physical disc) | ⛔ |
| **M1** | Game boots with the hook DLL (deferred: needs a runnable original) | ⏸️ |
| **M2** | Toshi kernel fully reimplemented (`TMath` done) | 🚧 |
| **M3** | A mission loads through our renderer and resource code | ⏳ |
| **M4** | Full game playable on hooks | ⏳ |
| **M5** | Native x64 build reaches the main menu | ⏳ |
| **M6** | Native x64 build plays a full mission with original saves | ⏳ |

Detailed per-module coverage is in [docs/progress.md](docs/progress.md).

## Getting started

### Requirements
| | |
|---|---|
| Compiler | Visual Studio 2022+ or Build Tools, with the *C++ x86/x64 build tools* (found automatically via `vswhere`) |
| Tools | Python 3.10+, Git, clang-format 20.1.8 (`pip install clang-format==20.1.8`, or put the exe in `toolchain/clang-format/`) |
| Analysis (optional) | Ghidra 12.1.4 in `toolchain/ghidra/` and JDK 21+ |
| Game | Your own copy of *Jurassic Park: Operation Genesis* (PC) |

### Setup
```bat
git clone git@github.com:Voikanaa/jpog-decomp.git
cd jpog-decomp
git config core.hooksPath .githooks       :: enable the pre-commit hygiene check

:: Put the installed game in JPOG\ (read-only reference) and a working copy in game\
python tools\dump_exports.py              :: regenerate symbols\ (optional; already committed)
python tools\setup_mcp.py                 :: optional: REA/Ghidra MCP config for Claude Code
```

### Build
```bat
tools\build.bat x86-hook-debug   :: 32-bit hook DLL + tests  ->  build\x86-hook-debug\
tools\build.bat x64-debug        :: native x64 build + tests  ->  build\x64-debug\
```

## Repository layout

```
├── src/
│   ├── common/        shared headers (ABI, layout checks)
│   ├── toshi/         Toshi engine reimplementation, one folder per original DLL
│   ├── game/          SimJP game code
│   ├── platform/      Win32, D3D11, audio and video backends
│   └── hook/          x86 hook DLL injected into the original game
├── tests/             unit tests + differential tests against the original DLLs
├── symbols/           demangled export/import tables of every original module
├── docs/              roadmap, binary map, progress, file formats
├── tools/             analysis, build and maintenance scripts
└── third_party/       pinned, vendored dependencies
```

## Tools

| Script | Purpose |
|---|---|
| `tools/build.bat` | Configure, build and test a CMake preset |
| `tools/ghidra_export.py` | Headless Ghidra decompilation of the original modules into `analysis/` (gitignored) |
| `tools/decomp_show.py` | Look up decompiled functions by name or address |
| `tools/dump_exports.py` | Regenerate `symbols/` from the original binaries |
| `tools/coverage.py` | Measure reimplementation coverage; regenerates [docs/progress.md](docs/progress.md) and the README badge |
| `tools/format.py` | Format all C++ with the pinned clang-format 20.1.8 (`--check` in CI and pre-commit) |
| `tools/check_repo.py` | Hygiene check: no game files, nothing over 5 MB (CI + pre-commit) |
| `tools/setup_mcp.py` | Generate the machine-specific `.mcp.json` for the REA MCP server |

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for the clean-room rules, workflow, commit conventions and
code style.

## License

The project's own code, tools and documentation are licensed under the
[GNU General Public License v3.0 or later](LICENSE). Vendored third-party code keeps its own
license (see [third_party/README.md](third_party/README.md)). The license does not cover the
game, its assets, or the *Jurassic Park* names and logo (`docs/jpog_logo.png`); see below.

## Legal

*Jurassic Park*, *Jurassic Park: Operation Genesis* and their logos are trademarks of Universal
City Studios and Amblin Entertainment. The game and the Toshi engine belong to their respective
rights holders. This is an unofficial, non-commercial fan project and is not affiliated with or
endorsed by them.

The repository contains no game code or assets: only original reimplementation code, tools and
documentation, plus symbol names extracted from the binaries for interoperability. To use it you
need your own legally obtained copy of the game.

# jpog-decomp

Reverse engineering of *Jurassic Park: Operation Genesis* (2003, Blue Tongue Software). The goal
is a **native Windows x86-64** reimplementation that runs the original game data.

- **Clean room:** all code comes from our own analysis of the shipped binaries and data. No
  community decompilation sources are used.
- **No game files in this repo.** Bring your own copy: put the installed game in `JPOG/` (read-only
  reference) and a working copy in `game/`. Both are gitignored.

## Approach
Hook-based incremental reimplementation. Portable C++ replaces functions of the original 32-bit
game one at a time, built as an x86 hook DLL, so every piece is verified in the real game. Once
coverage is complete, the same source builds as a standalone x64 exe:
Direct3D 11, Raw Input, miniaudio, and FFmpeg for Bink video.

## Setup
- Visual Studio Build Tools 2026 (MSVC x86 + x64, bundled CMake/Ninja, Windows SDK)
- Python 3
- Ghidra 12.1.4 in `toolchain/ghidra/` (gitignored), plus a JDK 21+
- Optional: the REA MCP server for Ghidra-backed analysis from Claude Code. Run
  `python tools/setup_mcp.py` to generate the machine-specific `.mcp.json` from `.mcp.json.example`.

## Build
```
tools\build.bat x86-hook-debug   # 32-bit hook DLL -> build/x86-hook-debug/
tools\build.bat x64-debug        # standalone x64 build (later)
```

After cloning, enable the repository's git hooks once: `git config core.hooksPath .githooks`.
See [CONTRIBUTING.md](CONTRIBUTING.md) for the workflow.

## Tools
- `tools/ghidra_export.py`: headless Ghidra decompilation of the original modules into
  `analysis/decomp/` (gitignored)
- `tools/dump_exports.py`: regenerates `symbols/` (demangled exports and imports of every
  original module, plus `classes.csv`)
- `tools/check_repo.py`: repository hygiene check (no game files, nothing over 5 MB); runs in CI
  and in the pre-commit hook
- `tools/msvc.py`: finds MSVC tools through vswhere (shared helper)

## Legal
*Jurassic Park: Operation Genesis* and the Toshi engine are the property of their respective
rights holders. This repository contains no game code or assets: only original reimplementation
code, tools and documentation, plus symbol names extracted from the binaries for
interoperability. You need your own legally obtained copy of the game to use it.

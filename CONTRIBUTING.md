# Contributing

## Ground rules
- **Clean room.** Derive everything from your own analysis of the shipped binaries and data.
  Do not read, copy or port code from other decompilation projects.
- **No game files in git.** Never commit binaries, assets, saves or the ISO. CI and the pre-commit
  hook enforce this.
- **New third-party dependencies need explicit approval.** This covers libraries, tools and
  GitHub Actions beyond `actions/checkout`.

## One-time setup per clone
```
git config core.hooksPath .githooks   # enable the pre-commit hygiene check
```

## Workflow
1. Analyse the function or format in Ghidra (directly, or through REA). Record findings in
   `docs/`: `modules.md` for binary-level facts, `docs/formats/<ext>.md` for file formats.
2. Implement clean C++ under `src/`, in the module directory that matches the original DLL.
3. Verify it: toggle the hook in `game/`, and add a differential test where practical.
4. Update `docs/progress.md`, plus `docs/plan.md` if a phase or decision changed, and `CLAUDE.md`
   if the structure, tools or workflow changed.
5. Commit.

## Commits
- Keep commits small and logical, and make each one build.
- Subject line: imperative and at most about 72 characters, optionally prefixed with an area
  (`Kernel: implement TCString`, `Docs: ...`, `Tools: ...`, `CI: ...`).
- The body explains *why* and anything non-obvious, such as original addresses or quirks
  reproduced on purpose.

## Code style
- C++20, formatted by `.clang-format` (tabs, Allman braces, 120 columns). CI checks formatting.
- Names follow the original symbols (`Toshi::TCString`, `AUnitDinosaur::...`), and member
  variables keep the original's `m_` prefix where known.
- Code must stay portable to x64 (see "Code rules" in `docs/plan.md`).
- Python tools target Python 3.10+ with PEP 8 style and a 120-column limit. Use only the standard
  library unless a dependency is approved.

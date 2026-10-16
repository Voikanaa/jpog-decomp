# Contributing

## License
By contributing you agree that your contribution is licensed under the project license,
GPL-3.0-or-later (see `LICENSE`).

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
3. Verify it with a differential test in `tests/` that calls the original export
   (`OriginalModule::Kernel().Get<Fn>("<mangled name>")`) and our function on the same inputs.
   Floats must match exactly unless a comment justifies a tolerance. Once the hook framework
   lands, also toggle the hook in `game/`.
4. Run `python tools/coverage.py`, which regenerates the coverage table in `docs/progress.md`
   and the README badge (CI checks both are current).
5. Keep the docs current in the same commit: the log in `docs/progress.md`, `docs/plan.md` if a
   phase or decision changed, `README.md` (status, tools, layout, setup) and `CLAUDE.md` if the
   structure, tools or workflow changed.
6. Commit.

## Issues
Every piece of work has a GitHub issue, grouped by the milestones in `docs/plan.md` and labelled
by area. Reference the issue in commits (`Refs #12`, or `Fixes #12` to close it), and open new
issues for work discovered along the way.

## Commits
- Keep commits small and logical, and make each one build.
- Subject line: imperative and at most about 72 characters, optionally prefixed with an area
  (`Kernel: implement TCString`, `Docs: ...`, `Tools: ...`, `CI: ...`).
- The body explains *why* and anything non-obvious, such as original addresses or quirks
  reproduced on purpose.

## Code style
- C++20, formatted by `.clang-format` (tabs, Allman braces, 120 columns). Run
  `python tools/format.py` before committing; it uses the pinned clang-format 20.1.8, and CI and
  the pre-commit hook reject unformatted code.
- Names follow the original symbols (`Toshi::TCString`, `AUnitDinosaur::...`), and member
  variables keep the original's `m_` prefix where known.
- Code must stay portable to x64 (see "Code rules" in `docs/plan.md`).
- Python tools target Python 3.10+ with PEP 8 style and a 120-column limit. Use only the standard
  library unless a dependency is approved.

# Contributing

## License
By contributing you agree that your contribution is licensed under the project license,
GPL-3.0-or-later (see `LICENSE`). New dependencies must be GPLv3-compatible (MIT, BSD, zlib,
Apache-2.0, LGPL linked dynamically).

## Ground rules
- **No game files in git.** Never commit binaries, assets, saves or the ISO. CI and the pre-commit
  hook enforce this.
- **No copy-protection material.** The retail `SimJP.exe` is SecuROM-protected. Never add,
  document or distribute a way around it, and never use a cracked or no-CD executable.
- **New third-party dependencies need explicit approval.** This covers libraries, tools and
  GitHub Actions beyond `actions/checkout`. Approved libraries are vendored pinned and unmodified
  in `third_party/`, with version, license, upstream and hash in its README.
- **Other projects: facts only.** Cite and verify findings from OpenJPOG, modding tools etc.,
  but never copy, translate or port their code. Credit new sources in [CREDITS.md](CREDITS.md).

## One-time setup per clone
```
git config core.hooksPath .githooks   # pre-commit: repo hygiene, clang-format, ruff
uv sync                               # .venv with the pinned clang-format and ruff
```
Put your game install in `JPOG/` and a working copy in `game/`. See the README for the toolchain.

## Workflow
1. Analyse the function or format in Ghidra (`tools/ghidra_export.py` output, or REA). Record
   findings in `docs/`: [modules.md](docs/modules.md) for binary-level facts,
   [docs/formats/](docs/formats/README.md) for file formats.
2. Implement clean C++ under `src/`, in the module directory that matches the original DLL.
3. Verify it with a differential test in `tests/` that calls the original export
   (`Original<Fn>("<mangled name>")` from `tests/support/Differential.h`) and our function on
   the same inputs. Floats must match exactly unless a comment justifies a tolerance. Take the
   evaluation order of float code from `tools/x87_expr.py`, not from Ghidra, which does not
   preserve the association of x87 sums.
4. Build and test `x86-debug` and `x64-debug` (`tools\build.bat <preset>`).
5. Run `python tools/coverage.py`, which regenerates the coverage table in `docs/progress.md`
   and the README badge (CI checks both are current).
6. Keep the docs current in the same commit: the log in `docs/progress.md`, `docs/plan.md` if a
   phase or decision changed, the format catalogue and `CREDITS.md` for new findings and sources,
   `README.md` (status, tools, layout, setup) and `CLAUDE.md` if the structure, tools or workflow
   changed.
7. Commit, push and open a pull request.

## Issues and pull requests
- Every piece of work has a GitHub issue, grouped by the milestones in `docs/plan.md` and
  labelled by area. Reference the issue in commits (`Refs #12`, or `Fixes #12` to close it), and
  open new issues for work discovered along the way.
- Pull requests are always merged with **squash and merge** (`gh pr merge --squash`), never with a
  merge commit or rebase. CI must be green first.

## Commits
- Keep commits small and logical, and make each one build.
- Subject line: imperative and at most about 72 characters, optionally prefixed with an area
  (`Kernel: implement TCString`, `Docs: ...`, `Tools: ...`, `CI: ...`).
- The body explains *why* and anything non-obvious, such as original addresses or quirks
  reproduced on purpose.

## Code style
- C++20, formatted by `.clang-format` (tabs, Allman braces, 120 columns). Run
  `python tools/format.py` before committing; it uses the pinned clang-format 20.1.8, and CI and
  the pre-commit hook reject unformatted code. Code must build warning-free (`JPOG_WERROR=ON`).
- Names follow the original symbols (`Toshi::TCString`, `AUnitDinosaur::...`), and member
  variables keep the original's `m_` prefix where known.
- Signatures must match the original exports exactly, including quirks such as non-const
  parameters or `const float* const` (MSVC mangles top-level pointer `const`).
  `tools/coverage.py --check` fails on any `Toshi::` symbol that matches no original export.
  Class declarations follow the vtable order in `symbols/vtables.csv`.
- Define members out of line (not inline in the header), so every function exists as a symbol
  that coverage can count.
- Code must be portable: follow the "Code rules" in [docs/plan.md](docs/plan.md).
- Python tools target Python 3.10+ with PEP 8 style and a 120-column limit, use only the standard
  library unless a dependency is approved, and must pass `ruff check tools` (pinned ruff 0.17.1).

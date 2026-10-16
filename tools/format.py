"""Format (or check) all tracked C++ sources with the pinned clang-format version.

Usage:
  python tools/format.py           reformat src/ and tests/ in place
  python tools/format.py --check   report violations and exit non-zero (used in CI)

clang-format is looked up in $CLANG_FORMAT, toolchain/clang-format/, then PATH, and must be
exactly the pinned version: formatting differs between releases, so local runs and CI must agree.
"""
import argparse
import os
import shutil
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PINNED_VERSION = "20.1.8"
PATTERNS = ["src/*.cpp", "src/*.h", "src/*.hpp", "src/*.inl", "tests/*.cpp", "tests/*.h"]


def find_clang_format():
    candidates = [
        os.environ.get("CLANG_FORMAT"),
        os.path.join(ROOT, "toolchain", "clang-format", "clang-format.exe"),
        shutil.which("clang-format"),
    ]
    for exe in filter(None, candidates):
        if not os.path.exists(exe):
            continue
        version = subprocess.run([exe, "--version"], capture_output=True, text=True).stdout
        if f"version {PINNED_VERSION}" in version:
            return exe
        print(f"skipping {exe}: {version.strip()} (need {PINNED_VERSION})", file=sys.stderr)
    sys.exit(f"clang-format {PINNED_VERSION} not found; see toolchain/ in CLAUDE.md or "
             f"`pip install clang-format=={PINNED_VERSION}`")


def tracked_sources():
    out = subprocess.run(["git", "ls-files", "-z", "--", *PATTERNS], cwd=ROOT,
                         capture_output=True, check=True).stdout.decode("utf-8")
    return [f for f in out.split("\0") if f]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true", help="only check, do not modify files")
    args = parser.parse_args()

    exe = find_clang_format()
    files = tracked_sources()
    if not files:
        return 0
    mode = ["--dry-run", "-Werror"] if args.check else ["-i"]
    result = subprocess.run([exe, *mode, *files], cwd=ROOT)
    if not args.check and result.returncode == 0:
        print(f"formatted {len(files)} files")
    return result.returncode


if __name__ == "__main__":
    sys.exit(main())

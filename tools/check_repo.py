"""Repository hygiene check: no game binaries/assets and no oversized files under version control.

Usage:
  python tools/check_repo.py            check all tracked files (used by CI)
  python tools/check_repo.py --staged   check files staged for commit (used by .githooks/pre-commit)
"""
import os
import subprocess
import sys

MAX_BYTES = 5 * 1024 * 1024
FORBIDDEN_EXTENSIONS = {
    ".exe", ".dll", ".iso", ".bik", ".mpg", ".dds", ".tga", ".ppm", ".wav", ".ogg",
    ".tmd", ".tml", ".tkl", ".fsm", ".inx", ".nwt", ".ttx", ".tto", ".mis", ".sav",
    ".s00", ".s01", ".s02", ".raw", ".hm", ".pdb", ".obj", ".lib",
}
FORBIDDEN_PREFIXES = ("JPOG/", "game/", "build/", "analysis/", ".venv/", ".cache/")


def git_files(staged):
    cmd = ["git", "diff", "--cached", "--name-only", "--diff-filter=ACMR", "-z"] if staged \
        else ["git", "ls-files", "-z"]
    out = subprocess.run(cmd, capture_output=True, check=True).stdout.decode("utf-8")
    return [f for f in out.split("\0") if f]


def main():
    staged = "--staged" in sys.argv[1:]
    problems = []
    for path in git_files(staged):
        if path.startswith(FORBIDDEN_PREFIXES):
            problems.append(f"{path}: path is excluded from version control")
        elif os.path.splitext(path)[1].lower() in FORBIDDEN_EXTENSIONS:
            problems.append(f"{path}: binary/game-asset file type")
        elif os.path.isfile(path) and os.path.getsize(path) > MAX_BYTES:
            problems.append(f"{path}: larger than {MAX_BYTES // (1024 * 1024)} MB")
    for p in problems:
        print(f"check_repo: {p}", file=sys.stderr)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())

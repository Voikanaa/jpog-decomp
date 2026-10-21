"""Run Ghidra headless analysis on original modules and export full decompilation.

Usage:
  python tools/ghidra_export.py TKernelInterface.dll [more modules...]
  python tools/ghidra_export.py --all

Modules are looked up in JPOG/ (and JPOG/Shaders/D3D/). For each module this writes:
  analysis/decomp/<Module>.c    pseudocode of every function ("// ==== <addr> <name>" headers)
  analysis/decomp/<Module>.tsv  function index (address, size, name, signature)
Ghidra projects are kept in analysis/ghidra/ so later runs can reuse the analysis.
analysis/ is derived from copyrighted binaries and is therefore gitignored.
"""
import argparse
import glob
import os
import subprocess
import sys

import setup_mcp

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GAME = os.path.join(ROOT, "JPOG")
SCRIPT_DIR = os.path.join(ROOT, "tools", "ghidra")
PROJECTS = os.path.join(ROOT, "analysis", "ghidra")
OUTPUT = os.path.join(ROOT, "analysis", "decomp")


def ghidra_dir():
    # $GHIDRA_INSTALL_DIR, then the install configured by tools/setup_mcp.py.
    for candidate in (os.environ.get("GHIDRA_INSTALL_DIR"), setup_mcp.configured_ghidra()):
        if setup_mcp.is_ghidra(candidate):
            return candidate
    sys.exit("Ghidra not found: run tools/setup_mcp.py --ghidra <dir> or set GHIDRA_INSTALL_DIR")


def all_modules():
    paths = [os.path.join(GAME, "SimJP.exe")] + sorted(glob.glob(os.path.join(GAME, "*.dll"))) \
        + sorted(glob.glob(os.path.join(GAME, "Shaders", "D3D", "*.dll")))
    third_party = {"binkw32.dll", "fmod.dll", "libmmd.dll"}
    return [p for p in paths if os.path.basename(p).lower() not in third_party]


def resolve(name):
    for path in (os.path.join(GAME, name), os.path.join(GAME, "Shaders", "D3D", name)):
        if os.path.exists(path):
            return path
    sys.exit(f"module not found in JPOG/: {name}")


def export(module_path, headless):
    stem = os.path.splitext(os.path.basename(module_path))[0]
    os.makedirs(PROJECTS, exist_ok=True)
    os.makedirs(OUTPUT, exist_ok=True)
    cmd = [headless, PROJECTS, stem, "-import", module_path, "-overwrite",
           "-scriptPath", SCRIPT_DIR, "-postScript", "ExportDecompilation.java", OUTPUT]
    print(f"== {stem}", flush=True)
    result = subprocess.run(cmd, capture_output=True, text=True, errors="replace")
    summary = [line for line in result.stdout.splitlines() if "ExportDecompilation:" in line]
    if result.returncode != 0 or not summary:
        sys.stderr.write(result.stdout[-4000:] + result.stderr[-4000:])
        sys.exit(f"Ghidra export failed for {stem} (exit {result.returncode})")
    print(summary[-1].split("ExportDecompilation:", 1)[1].strip())


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("modules", nargs="*", help="module file names, e.g. TKernelInterface.dll")
    parser.add_argument("--all", action="store_true", help="export every game/engine module")
    args = parser.parse_args()
    if not args.modules and not args.all:
        parser.error("give module names or --all")

    headless = os.path.join(ghidra_dir(), "support", "analyzeHeadless.bat")
    for path in all_modules() if args.all else [resolve(m) for m in args.modules]:
        export(path, headless)


if __name__ == "__main__":
    main()

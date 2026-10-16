"""Measure reimplementation coverage and update docs/progress.md and the README badge.

Usage: python tools/coverage.py [--build build/x86-hook-debug] [--check]

On x86, our reimplemented functions have the same signatures and calling conventions as the
originals, so MSVC mangles them to the same names. Coverage is therefore the set of original
exported functions (symbols/<Module>.exports.csv) that are also defined in our static library
for that module. Requires an up-to-date x86 build.

--check exits non-zero if either file is out of date (used in CI).
"""
import argparse
import csv
import os
import re
import subprocess
import sys

import msvc

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PROGRESS = os.path.join(ROOT, "docs", "progress.md")
README = os.path.join(ROOT, "README.md")
TABLE_MARKERS = ("<!-- coverage:begin -->", "<!-- coverage:end -->")
BADGE_MARKERS = ("<!-- coverage-badge:begin -->", "<!-- coverage-badge:end -->")

# Original module -> our static library (relative to the build directory). Modules without a
# library yet report 0%.
MODULES = [
    ("TKernelInterface", "src/toshi/kernel/toshi_kernel.lib"),
    ("TApplication", None),
    ("TInputInterface", None),
    ("TInputDXInterface", None),
    ("TRenderInterface", None),
    ("TRenderD3DInterface", None),
    ("TTerrainInterface", None),
    ("PPropertyParser", None),
    ("TGuiInterface", None),
    ("PGUICore", None),
    ("PGUIRenderer", None),
    ("SimJPTerrain", None),
    ("Units", None),
    ("TLineShaderD3D", None),
    ("TSkinShaderD3D", None),
    ("TSpriteShaderD3D", None),
    ("TSysShaderD3D", None),
    ("TTerrainShaderD3D", None),
    ("SimJP", None),
]

DEFINED_SYMBOL = re.compile(r"\bSECT[0-9A-F]+\s.*External\s+\|\s+(\S+)")


def original_functions(module):
    """Mangled names of exported functions (data symbols such as vtables are excluded)."""
    path = os.path.join(ROOT, "symbols", f"{module}.exports.csv")
    with open(path, newline="", encoding="utf-8") as f:
        return {row["name"] for row in csv.DictReader(f) if "(" in row["demangled"]}


def defined_symbols(lib_path):
    out = subprocess.run([msvc.tool("dumpbin.exe"), "/nologo", "/symbols", lib_path],
                         capture_output=True, text=True, check=True, errors="replace").stdout
    return {m.group(1) for m in map(DEFINED_SYMBOL.search, out.splitlines()) if m}


def measure(build_dir):
    """Returns the markdown table and the overall percentage."""
    rows = ["| Module | Functions | Reimplemented | Coverage |", "|---|---:|---:|---:|"]
    total = done = 0
    for module, lib in MODULES:
        functions = original_functions(module)
        implemented = 0
        if lib:
            lib_path = os.path.join(build_dir, lib)
            if not os.path.exists(lib_path):
                sys.exit(f"{lib_path} not found; build the x86-hook-debug preset first")
            implemented = len(functions & defined_symbols(lib_path))
        total += len(functions)
        done += implemented
        rows.append(f"| {module} | {len(functions)} | {implemented} | {implemented / len(functions):.1%} |")
    rows.append(f"| **Total** | **{total}** | **{done}** | **{done / total:.1%}** |")
    return "\n".join(rows), 100.0 * done / total


def badge(percent):
    color = "red" if percent < 10 else "orange" if percent < 50 else "yellow" if percent < 90 else "brightgreen"
    return f"![Coverage](https://img.shields.io/badge/reimplemented-{percent:.1f}%25-{color}?style=flat-square)"


def replace_between(text, markers, content):
    begin, end = markers
    start, stop = text.index(begin) + len(begin), text.index(end)
    return text[:start] + content + text[stop:]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--build", default=os.path.join(ROOT, "build", "x86-hook-debug"))
    parser.add_argument("--check", action="store_true", help="fail if the generated sections are out of date")
    args = parser.parse_args()

    table, percent = measure(args.build)
    targets = [
        (PROGRESS, TABLE_MARKERS, "\n" + table + "\n"),
        (README, BADGE_MARKERS, badge(percent)),
    ]

    stale = []
    for path, markers, content in targets:
        with open(path, encoding="utf-8") as f:
            text = f.read()
        updated = replace_between(text, markers, content)
        if updated == text:
            continue
        stale.append(os.path.relpath(path, ROOT))
        if not args.check:
            with open(path, "w", encoding="utf-8", newline="\n") as f:
                f.write(updated)

    if args.check:
        if stale:
            sys.exit(f"out of date: {', '.join(stale)}; run python tools/coverage.py")
        return
    print(table)


if __name__ == "__main__":
    main()

"""Measure reimplementation coverage and update docs/progress.md and the README badge.

Usage: python tools/coverage.py [--build build/x86-debug] [--check]

On x86, our reimplemented functions have the same signatures and calling conventions as the
originals, so MSVC mangles them to the same names. Coverage is therefore the set of original
exported functions (symbols/<Module>.exports.csv) that are also defined in our static library
for that module. Exports our design deliberately replaces (SUPERSEDED) are listed separately and
left out of the percentage. Requires an up-to-date x86 build.

It also reports "unmatched" symbols: Toshi:: symbols we define that match no original export.
These usually mean a signature drifted from the original (e.g. `const float*` vs
`const float* const`, which MSVC mangles differently); --check fails on them.

--check exits non-zero if either file is out of date or a symbol is unmatched (used in CI).
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
    ("PPropertyParser", "src/ppropertyparser/ppropertyparser.lib"),
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


# Exports deliberately not reimplemented because our design replaces them, so they are left out
# of the coverage denominator and listed in their own column. Patterns are substrings of the
# mangled name.
SUPERSEDED = {
    # The original 32-bit packed heap: TMemory runs on the C runtime heap instead (docs/plan.md,
    # "Memory"). No other module imports these.
    "TKernelInterface": (
        "MemBlockSlot@TMemory@Toshi@@", "?AllocMem@TMemory@", "?ConvertProcessToHole@TMemory@",
        "?DumpMemInfoToFile@TMemory@", "?ExtendNodeSize@TMemory@", "?FreeMem@TMemory@",
        "?GetGlobalFlags@TMemory@", "?GetMemNodeFromAddress@TMemory@", "?GetNodeSize@TMemory@",
        "?GetProcessMemBlock@TMemory@", "?GetSlotIndex@TMemory@", "?IsAllocMB@TMemory@",
        "?IsMemNodeMB@TMemory@", "?IsProcess@TMemory@", "?MapSizeToFreeList@TMemory@",
        "?MemRecurse@TMemory@", "?MemRecurseR@TMemory@", "?SetHoleSize@TMemory@",
        "?SetProcess@TMemory@", "?m_szInvalidMemoryBlockId@TMemory@", "?m_szUnsedMemory@TMemory@",
        "?m_szValidMemoryBlockId@TMemory@",
    ),
}
# Compiler-generated "default constructor closures" (emitted only where a class with default
# arguments is default-constructed in an array); superseded in every module.
SUPERSEDED_EVERYWHERE = ("??_F",)


def superseded(module, name):
    patterns = SUPERSEDED.get(module, ()) + SUPERSEDED_EVERYWHERE
    return any(p in name for p in patterns)


# Helpers the compiler emits on its own (scalar/vector deleting destructors); never our code.
COMPILER_GENERATED = ("??_G", "??_E", "??_R")  # + C++ RTTI descriptors
# Classes the original defines entirely inline (no exported members), so ours are header-only too.
HEADER_ONLY_CLASSES = ("?$TArray@", "?$TQList@", "?$TManagedPtr@", "?$TDList@", "?$TEmitter@", "?$TListener@",
                       "?$TNodeList@", "?$TMap@", "?$TNodeTree@", "?$TComparator@", "Comparator@TPCString@")


def original_exports(module):
    """All exported mangled names (functions and data)."""
    path = os.path.join(ROOT, "symbols", f"{module}.exports.csv")
    with open(path, newline="", encoding="utf-8") as f:
        return {row["name"] for row in csv.DictReader(f)}


# Library and compiler symbols that can appear in a module's library.
NOT_MODULE_SYMBOLS = ("@std@@", "??_C@", "__empty_global_delete", "??2@Y", "??3@Y", "??_U@Y", "??_V@Y")


def original_name(symbol):
    """The original (VC6) mangling of one of our symbols. VC6 encodes a const pointer static data
    member (`static const T* const x`) as `2PB...B`, modern MSVC as `2QB...B`."""
    return symbol.replace("@@2QB", "@@2PB")


def our_name(name):
    """Our compiler's mangling of an original (VC6) export name; the inverse of original_name."""
    if "@@2PB" in name and name.endswith("B"):
        return name.replace("@@2PB", "@@2QB")
    return name


def other_exports(module):
    """Exports of every other module (cached)."""
    if module not in _OTHER_EXPORTS:
        _OTHER_EXPORTS[module] = set().union(*(original_exports(m) for m, _ in MODULES if m != module))
    return _OTHER_EXPORTS[module]


_OTHER_EXPORTS = {}


def original_functions(module):
    """Mangled names of exported functions (data symbols such as vtables are excluded)."""
    path = os.path.join(ROOT, "symbols", f"{module}.exports.csv")
    with open(path, newline="", encoding="utf-8") as f:
        return {row["name"] for row in csv.DictReader(f) if "(" in row["demangled"]}


def defined_symbols(lib_path):
    out = subprocess.run([msvc.tool("dumpbin.exe"), "/nologo", "/symbols", lib_path],
                         capture_output=True, text=True, check=True, errors="replace").stdout
    return {m.group(1) for m in map(DEFINED_SYMBOL.search, out.splitlines()) if m}


def total_skipped(modules):
    return sum(len({f for f in original_functions(m) if superseded(m, f)}) for m, _ in modules)


def measure(build_dir):
    """Returns the markdown table, the overall percentage and the unmatched symbols."""
    rows = ["| Module | Functions | Superseded | Reimplemented | Coverage |", "|---|---:|---:|---:|---:|"]
    total = done = 0
    unmatched = []
    for module, lib in MODULES:
        functions = original_functions(module)
        skipped = {f for f in functions if superseded(module, f)}
        counted = functions - skipped
        implemented = 0
        if lib:
            lib_path = os.path.join(build_dir, lib)
            if not os.path.exists(lib_path):
                sys.exit(f"{lib_path} not found; build the x86-debug preset first")
            ours = defined_symbols(lib_path)
            implemented = len(counted & ours)
            exports = original_exports(module)
            # C++ symbols of this module (Toshi:: for the kernel, all for the others; symbols
            # another module exports, e.g. inline kernel code, are not this module's).
            unmatched += [f"{module}: {s}" for s in sorted(ours)
                          if s.startswith("?") and (module != "TKernelInterface" or "@Toshi@@" in s)
                          and "?A0x" not in s and original_name(s) not in exports
                          and original_name(s) not in other_exports(module)
                          and not any(x in s for x in NOT_MODULE_SYMBOLS)
                          and not s.startswith(COMPILER_GENERATED)
                          and not any(c in s for c in HEADER_ONLY_CLASSES)
                          and "@jpog@@" not in s]  # our own helpers (namespace jpog)
        total += len(counted)
        done += implemented
        rows.append(f"| {module} | {len(functions)} | {len(skipped)} | {implemented} | "
                    f"{implemented / len(counted):.1%} |")
    rows.append(f"| **Total** | **{total + total_skipped(MODULES)}** | **{total_skipped(MODULES)}** | **{done}** | "
                f"**{done / total:.1%}** |")
    return "\n".join(rows), 100.0 * done / total, unmatched


def badge(percent):
    color = "red" if percent < 10 else "orange" if percent < 50 else "yellow" if percent < 90 else "brightgreen"
    # Raw HTML rather than markdown image syntax: the README badge block is an HTML <p>.
    url = f"https://img.shields.io/badge/reimplemented-{percent:.1f}%25-{color}?style=flat-square"
    return f'<img alt="Reimplemented" src="{url}">'


def replace_between(text, markers, content):
    begin, end = markers
    start, stop = text.index(begin) + len(begin), text.index(end)
    return text[:start] + content + text[stop:]


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--build", default=os.path.join(ROOT, "build", "x86-debug"))
    parser.add_argument("--check", action="store_true", help="fail if the generated sections are out of date")
    args = parser.parse_args()

    table, percent, unmatched = measure(args.build)
    for symbol in unmatched:
        print(f"unmatched: {symbol}", file=sys.stderr)
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
        if unmatched:
            sys.exit(f"{len(unmatched)} symbol(s) match no original export (signature mismatch?)")
        return
    print(table)


if __name__ == "__main__":
    main()

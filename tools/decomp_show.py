"""Print decompiled functions from analysis/decomp/ (produced by tools/ghidra_export.py).

Usage:
  python tools/decomp_show.py <Module> <regex> [--max-size N]

<regex> is matched against the namespaced function name or the address (e.g. "TMath::Abs$",
"0x1000e8f0"). Example: python tools/decomp_show.py TKernelInterface "^Toshi::TMath::" --max-size 64
"""
import argparse
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
HEADER = re.compile(r"^// ==== (0x[0-9a-f]+) (.*) \((\d+) bytes\)$")


def blocks(path):
    current = None
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            m = HEADER.match(line.rstrip("\n"))
            if m:
                if current:
                    yield current
                current = {"addr": m.group(1), "name": m.group(2), "size": int(m.group(3)), "lines": [line]}
            elif current:
                current["lines"].append(line)
    if current:
        yield current


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("module", help="module stem, e.g. TKernelInterface or SimJP")
    parser.add_argument("pattern", help="regex over function name or address")
    parser.add_argument("--max-size", type=int, default=None, help="skip functions larger than N bytes")
    args = parser.parse_args()

    path = os.path.join(ROOT, "analysis", "decomp", f"{args.module}.c")
    if not os.path.exists(path):
        sys.exit(f"{path} not found; run tools/ghidra_export.py {args.module}.dll first")
    rx = re.compile(args.pattern)
    for b in blocks(path):
        if (rx.search(b["name"]) or rx.search(b["addr"])) and (args.max_size is None or b["size"] <= args.max_size):
            sys.stdout.write("".join(b["lines"]))


if __name__ == "__main__":
    main()

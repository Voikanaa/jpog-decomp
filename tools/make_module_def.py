"""Write the module-definition (.def) file for our drop-in replacement of an original DLL.

Usage: python tools/make_module_def.py <Module> <output.def>
       e.g. python tools/make_module_def.py TKernelInterface build/x86-debug/TKernelInterface.def

The original modules import from each other by mangled name. Our replacement DLL exports the same
names (with the original ordinals) from symbols/<Module>.exports.csv, except the exports our design
supersedes (tools/coverage.py, SUPERSEDED), which nothing imports.
"""
import csv
import os
import sys

from coverage import our_name, superseded

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    module, output = sys.argv[1], os.path.abspath(sys.argv[2])
    with open(os.path.join(ROOT, "symbols", f"{module}.exports.csv"), newline="", encoding="utf-8") as f:
        exports = list(csv.DictReader(f))

    os.makedirs(os.path.dirname(output), exist_ok=True)
    lines = [f"LIBRARY {module}.dll", "EXPORTS"]
    for row in exports:
        if not row["name"] or superseded(module, row["name"]):
            continue
        kind = "" if "(" in row["demangled"] else " DATA"
        # Exported under the original name, defined under ours where the manglings differ.
        ours = our_name(row["name"])
        alias = f"={ours}" if ours != row["name"] else ""
        lines.append(f"    {row['name']}{alias} @{row['ordinal']}{kind}")
    text = "\n".join(lines) + "\n"
    # Rewrite only on change so the DLL does not relink needlessly.
    if not os.path.exists(output) or open(output, encoding="utf-8").read() != text:
        with open(output, "w", encoding="utf-8", newline="\n") as f:
            f.write(text)


if __name__ == "__main__":
    main()

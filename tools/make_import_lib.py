"""Generate an x86 import library for an original engine DLL from its committed export table.

Usage: python tools/make_import_lib.py <Module> <output.lib>
       e.g. python tools/make_import_lib.py TKernelInterface build/x86/original/TKernelInterface.lib

Our x86 executable links against the original (unprotected) engine DLLs for everything we have
not reimplemented yet. The import library is built from symbols/<Module>.exports.csv, so no game
files are needed at build time (CI included). Requires lib.exe (MSVC environment or vswhere).
"""
import csv
import os
import shutil
import subprocess
import sys

import msvc
from coverage import our_name

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    module, output = sys.argv[1], os.path.abspath(sys.argv[2])
    with open(os.path.join(ROOT, "symbols", f"{module}.exports.csv"), newline="", encoding="utf-8") as f:
        exports = list(csv.DictReader(f))

    os.makedirs(os.path.dirname(output), exist_ok=True)
    def_path = os.path.splitext(output)[0] + ".def"
    with open(def_path, "w", encoding="utf-8", newline="\n") as f:
        f.write(f"LIBRARY {module}.dll\nEXPORTS\n")
        for row in exports:
            if not row["name"]:
                continue  # exported by ordinal only
            # Variables (vtables, static members) must be imported as data.
            kind = "" if "(" in row["demangled"] else " DATA"
            # Where our compiler mangles a name differently from VC6, import it by ordinal under
            # our name.
            ours = our_name(row["name"])
            noname = " NONAME" if ours != row["name"] else ""
            f.write(f"    {ours} @{row['ordinal']}{noname}{kind}\n")

    lib_exe = shutil.which("lib.exe") or msvc.tool("lib.exe")
    result = subprocess.run([lib_exe, "/nologo", f"/def:{def_path}", "/machine:x86", f"/out:{output}"],
                            capture_output=True, text=True)
    if result.returncode != 0:
        sys.exit(result.stdout + result.stderr)


if __name__ == "__main__":
    main()

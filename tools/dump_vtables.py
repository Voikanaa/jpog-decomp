"""Recover virtual method order from the exported vtables of the original modules.

Usage: python tools/dump_vtables.py [game_dir]   (default: JPOG/)

Every exported vtable symbol (??_7<Class>@...@6B@) points at an array of function addresses in
.rdata. Mapping each entry back to the exported function at that address gives the virtual
method order, which our class declarations must reproduce so that calls through the vtable
match the original.

Output: symbols/vtables.csv  module, class, index, offset, target_module, mangled, demangled
Entries pointing at non-exported code are listed with an empty name (address only).
"""
import csv
import glob
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from dump_exports import PE  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def load_exports(module_stem):
    with open(os.path.join(ROOT, "symbols", f"{module_stem}.exports.csv"), newline="", encoding="utf-8") as f:
        return list(csv.DictReader(f))


def load_imports(module_stem):
    """IAT slot VA -> (dll, mangled, demangled) for a module's imports."""
    with open(os.path.join(ROOT, "symbols", f"{module_stem}.imports.csv"), newline="", encoding="utf-8") as f:
        return {int(r["iat_rva"], 16): (r["dll"], r["name"], r["demangled"]) for r in csv.DictReader(f)}


def main():
    game = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "JPOG")
    paths = [os.path.join(game, "SimJP.exe")] + sorted(glob.glob(os.path.join(game, "*.dll"))) \
        + sorted(glob.glob(os.path.join(game, "Shaders", "D3D", "*.dll")))
    third_party = {"binkw32", "fmod", "libmmd"}

    rows = []
    vtable_count = 0
    for path in paths:
        stem = os.path.splitext(os.path.basename(path))[0]
        if stem.lower() in third_party:
            continue
        pe = PE(path)
        exports = load_exports(stem)
        # Exported functions of this module, by address (modules share base addresses).
        functions = {int(r["va"], 16): (stem, r["name"], r["demangled"]) for r in exports if "(" in r["demangled"]}
        objects = {int(r["va"], 16) for r in exports if "(" not in r["demangled"]}
        imports = load_imports(stem)
        text = [(va + pe.image_base, va + pe.image_base + size) for va, size, _ in pe.sections[:1]]

        def resolve(target):
            if target in functions:
                return functions[target]
            try:
                code = pe.data[pe.off(target - pe.image_base):][:6]
            except ValueError:
                return None
            if code[:2] == b"\xff\x25":  # jmp [IAT slot]: an imported function
                slot = struct.unpack_from("<I", code, 2)[0] - pe.image_base
                dll, name, dem = imports.get(slot, ("?", "", ""))
                if name == "_purecall":
                    return ("", "_purecall", "pure virtual")
                return (os.path.splitext(dll)[0], name, dem)
            return (stem, "", "")

        for row in exports:
            if not row["name"].startswith("??_7"):
                continue
            vtable_count += 1
            cls = row["demangled"].replace("const ", "").replace("::`vftable'", "").split("{")[0].strip()
            va = int(row["va"], 16)
            for index in range(1024):
                entry = va + 4 * index
                if index and entry in objects:
                    break  # next exported object starts here
                try:
                    target = struct.unpack_from("<I", pe.data, pe.off(entry - pe.image_base))[0]
                except (ValueError, struct.error):
                    break
                if not any(lo <= target < hi for lo, hi in text):
                    break  # not a code pointer: end of this vtable
                module, mangled, demangled = resolve(target) or (stem, "", "")
                rows.append([stem, cls, index, f"{4 * index:#x}", module, mangled, demangled, f"{target:#x}"])
    with open(os.path.join(ROOT, "symbols", "vtables.csv"), "w", newline="", encoding="utf-8") as f:
        w = csv.writer(f, lineterminator="\n")
        w.writerow(["module", "class", "index", "offset", "target_module", "mangled", "demangled", "address"])
        w.writerows(rows)
    print(f"{vtable_count} vtables, {len(rows)} entries -> symbols/vtables.csv")


if __name__ == "__main__":
    main()

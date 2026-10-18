"""Annotate the SimJP decompilation with the game's own import names.

Usage: python tools/resolve_imports.py
       python tools/resolve_imports.py --slots 0x739A14 0x73932C ...   (name IAT slots from disassembly)

Ghidra reads the retail SimJP.exe through its outer import directory, so calls through the
game's own import address table (0x739000, see docs/modules.md) show up as calls to raw
numbers, e.g. `(*(code *)0x344f2c)(...)`. Those numbers are the RVAs of the import's hint/name
entries. This rewrites them as the demangled import name, e.g.
`(*(code *)0x344f2c /* Toshi::TFreeList::SetCapacity */)(...)`.

Input:  analysis/decomp/SimJP.c (tools/ghidra_export.py SimJP.exe)
Output: analysis/decomp/SimJP.resolved.c (use with tools/decomp_show.py SimJP.resolved ...)
"""
import csv
import os
import re
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = os.path.join(ROOT, "JPOG", "SimJP.exe")
IAT_START, IAT_END = 0x739000, 0x73B000
IMAGE_BASE = 0x400000


def load_sections(data):
    pe = struct.unpack_from("<I", data, 0x3C)[0]
    count = struct.unpack_from("<H", data, pe + 6)[0]
    so = pe + 24 + struct.unpack_from("<H", data, pe + 20)[0]
    sections = []
    for _ in range(count):
        _, va, size, raw = struct.unpack_from("<IIII", data, so + 8)
        sections.append((va, size, raw))
        so += 40
    return sections


def demangled_names():
    names = {}
    for path in os.listdir(os.path.join(ROOT, "symbols")):
        if path.endswith(".exports.csv"):
            with open(os.path.join(ROOT, "symbols", path), newline="", encoding="utf-8") as f:
                for row in csv.DictReader(f):
                    names[row["name"]] = row["demangled"]
    return names


def short_name(demangled):
    """'public: void __thiscall Toshi::TTask::Create(void)' -> 'Toshi::TTask::Create'."""
    head = demangled.split("(")[0]
    return head.split()[-1] if head.split() else demangled


def main():
    data = open(EXE, "rb").read()
    sections = load_sections(data)

    def offset(rva):
        for va, size, raw in sections:
            if va <= rva < va + size:
                return rva - va + raw
        return None

    demangled = demangled_names()
    # Map hint/name RVA -> import name, for every slot of the game's own IAT.
    by_rva = {}
    by_slot = {}
    o = offset(IAT_START - IMAGE_BASE)
    for slot in range(IAT_START, IAT_END, 4):
        value = struct.unpack_from("<I", data, o + slot - IAT_START)[0]
        if not value or value & 0x80000000:
            continue
        name_offset = offset(value)
        if name_offset is None:
            continue
        end = data.find(b"\0", name_offset + 2)
        name = data[name_offset + 2:end].decode("latin-1")
        if name:
            by_rva[value] = short_name(demangled.get(name, name))
            by_slot[slot] = demangled.get(name, name)

    if len(sys.argv) > 1 and sys.argv[1] == "--slots":
        # Disassembly reads imports as `call dword ptr [slot]` / `mov reg, [slot]` (data imports).
        for arg in sys.argv[2:]:
            print(f"{int(arg, 16):#x}  {by_slot.get(int(arg, 16), '(not an import slot)')}")
        return

    src = os.path.join(ROOT, "analysis", "decomp", "SimJP.c")
    if not os.path.exists(src):
        sys.exit("analysis/decomp/SimJP.c not found; run tools/ghidra_export.py SimJP.exe first")
    text = open(src, encoding="utf-8", errors="replace").read()
    count = 0

    def annotate(m):
        nonlocal count
        name = by_rva.get(int(m.group(1), 16))
        if not name:
            return m.group(0)
        count += 1
        return f"(*(code *){m.group(1)} /* {name} */)"

    text = re.sub(r"\(\*\(code \*\)(0x[0-9a-f]+)\)", annotate, text)
    with open(os.path.join(ROOT, "analysis", "decomp", "SimJP.resolved.c"), "w", encoding="utf-8") as f:
        f.write(text)
    print(f"{len(by_rva)} import names; annotated {count} call sites -> analysis/decomp/SimJP.resolved.c")


if __name__ == "__main__":
    main()

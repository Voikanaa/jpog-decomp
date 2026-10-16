"""Dump PE exports/imports of every original JPOG binary into symbols/*.csv.

Usage: python tools/dump_exports.py [game_dir]   (default: JPOG/)

Outputs:
  symbols/<module>.exports.csv  ordinal, rva, va, name, demangled
  symbols/<module>.imports.csv  dll, name_or_ordinal, iat_rva, demangled
  symbols/classes.csv           class, module, exported members, has_vtable
"""
import csv
import glob
import os
import struct
import subprocess
import sys
import tempfile
from collections import defaultdict

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
UNDNAME = r"C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\VC\Tools\MSVC\14.51.36231\bin\Hostx64\x64\undname.exe"


class PE:
    def __init__(self, path):
        self.path = path
        self.data = d = open(path, "rb").read()
        pe = struct.unpack_from("<I", d, 0x3C)[0]
        nsec = struct.unpack_from("<H", d, pe + 6)[0]
        optsz = struct.unpack_from("<H", d, pe + 20)[0]
        opt = pe + 24
        self.image_base = struct.unpack_from("<I", d, opt + 28)[0]
        self.dirs = [struct.unpack_from("<II", d, opt + 96 + 8 * i) for i in range(16)]
        self.sections = []
        so = opt + optsz
        for _ in range(nsec):
            vs, va, rs, ro = struct.unpack_from("<IIII", d, so + 8)
            self.sections.append((va, max(vs, rs), ro))
            so += 40

    def off(self, rva):
        for va, size, ro in self.sections:
            if va <= rva < va + size:
                return rva - va + ro
        raise ValueError(f"rva {rva:#x} not in any section")

    def cstr(self, rva):
        o = self.off(rva)
        return self.data[o:self.data.index(b"\0", o)].decode("latin-1")

    def exports(self):
        rva, _ = self.dirs[0]
        if not rva:
            return []
        o = self.off(rva)
        base, nfun, nnames, afun, anames, aords = struct.unpack_from("<IIIIII", self.data, o + 16)
        funcs = struct.unpack_from(f"<{nfun}I", self.data, self.off(afun)) if nfun else ()
        names = struct.unpack_from(f"<{nnames}I", self.data, self.off(anames)) if nnames else ()
        ords = struct.unpack_from(f"<{nnames}H", self.data, self.off(aords)) if nnames else ()
        by_index = {i: self.cstr(n) for n, i in zip(names, ords)}
        return [(base + i, f, by_index.get(i, "")) for i, f in enumerate(funcs) if f]

    def imports(self):
        rva, _ = self.dirs[1]
        out = []
        o = self.off(rva)
        while True:
            olt, _, _, name, ft = struct.unpack_from("<IIIII", self.data, o)
            if not name:
                break
            dll = self.cstr(name)
            t = self.off(olt or ft)
            slot = ft
            while True:
                e = struct.unpack_from("<I", self.data, t)[0]
                if not e:
                    break
                sym = f"#{e & 0xFFFF}" if e & 0x80000000 else self.cstr(e + 2)
                out.append((dll, sym, slot))
                t += 4
                slot += 4
            o += 20
        return out


def demangle_all(names):
    """Batch-demangle with undname (it accepts a file with one name per line)."""
    names = sorted({n for n in names if n.startswith("?")})
    if not names:
        return {}
    with tempfile.NamedTemporaryFile("w", suffix=".txt", delete=False) as f:
        f.write("\n".join(names) + "\n")
        tmp = f.name
    try:
        out = subprocess.run([UNDNAME, tmp], capture_output=True, text=True, errors="replace").stdout
    finally:
        os.unlink(tmp)
    lines = [l for l in out.splitlines() if l.strip()]
    # undname echoes a banner first; the final len(names) lines are the results in order.
    lines = lines[-len(names):]
    return dict(zip(names, lines))


def class_of(mangled):
    """Outermost-scope class path of a member symbol, e.g. ?Foo@TBar@Toshi@@... -> Toshi::TBar."""
    if not mangled.startswith("?"):
        return None
    body = mangled[1:]
    if body.startswith("?$"):
        return None  # templated function name; skip
    if body.startswith("?"):
        body = body[2:] if body[1] != "_" else body[3:]  # special names like ??0 / ??_7
    else:
        body = body[body.index("@") + 1:] if "@" in body else ""
    scopes = []
    for part in body.split("@"):
        if part == "" or part[0].isdigit() or part.startswith("?$"):
            break
        scopes.append(part)
    return "::".join(reversed(scopes)) or None


def main():
    game = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "JPOG")
    outdir = os.path.join(ROOT, "symbols")
    os.makedirs(outdir, exist_ok=True)
    paths = [os.path.join(game, "SimJP.exe")] + sorted(glob.glob(os.path.join(game, "*.dll"))) \
        + sorted(glob.glob(os.path.join(game, "Shaders", "D3D", "*.dll")))

    pes = {os.path.basename(p): PE(p) for p in paths}
    exps = {m: pe.exports() for m, pe in pes.items()}
    imps = {m: pe.imports() for m, pe in pes.items()}
    dem = demangle_all([e[2] for v in exps.values() for e in v] + [i[1] for v in imps.values() for i in v])

    classes = defaultdict(lambda: [None, 0, False])
    for mod, pe in pes.items():
        stem = os.path.splitext(mod)[0]
        with open(os.path.join(outdir, f"{stem}.exports.csv"), "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["ordinal", "rva", "va", "name", "demangled"])
            for ordinal, rva, name in exps[mod]:
                w.writerow([ordinal, f"{rva:#x}", f"{rva + pe.image_base:#x}", name, dem.get(name, name)])
                cls = class_of(name)
                if cls:
                    c = classes[cls]
                    c[0] = c[0] or mod
                    c[1] += 1
                    c[2] |= name.startswith("??_7")
        with open(os.path.join(outdir, f"{stem}.imports.csv"), "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["dll", "name", "iat_rva", "demangled"])
            for dll, name, slot in imps[mod]:
                w.writerow([dll, name, f"{slot:#x}", dem.get(name, name)])
        print(f"{mod:28} exports={len(exps[mod]):6} imports={len(imps[mod]):5}")

    with open(os.path.join(outdir, "classes.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["class", "module", "exported_members", "has_vtable"])
        for cls, (mod, n, vt) in sorted(classes.items()):
            w.writerow([cls, mod, n, int(vt)])
    print(f"classes: {len(classes)}")


if __name__ == "__main__":
    main()

"""Recover the exact floating-point expressions of an original x87 function.

Ghidra prints commutative operands in arbitrary order and does not preserve the association of
x87 sums, but bit-exact reimplementation needs the real evaluation order. This tool disassembles
one function with dumpbin, symbolically executes its x87 and stack instructions along the
straight-line path, and prints every value stored to memory as a fully parenthesised expression.

Usage:
  python tools/x87_expr.py <Module> <function regex | 0xADDRESS> [--args this,a,b] [--matrix]

Examples:
  python tools/x87_expr.py TKernelInterface "TQuaternion::operator\\*=$" --args this,q
  python tools/x87_expr.py TKernelInterface 0x1001b0a0 --args this,m --matrix

Pointer parameters are named with --args (ECX first for __thiscall, then stack arguments in
order). Memory is shown as name[i] (float index), or name[r][c] for 4x4 matrices with --matrix.
Constants in the module's data sections are shown by value. Branches are not followed: each
conditional jump is listed and execution continues with the fall-through path, so only use the
output for straight-line code or the branch you are interested in (--follow ADDR takes a jump).
"""
import argparse
import os
import re
import struct
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import msvc  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GAME = os.path.join(ROOT, "JPOG")
LINE = re.compile(r"^\s+([0-9A-F]{8}): (?:[0-9A-F]{2} )+\s*(\w+)\s*(.*?)\s*$")
MEM = re.compile(r"^(?:(byte|word|dword|qword|tbyte) ptr )?(?:ds:)?\[([^\]]+)\]$")
STI = re.compile(r"^st\((\d)\)$")


def module_path(name):
    for path in (os.path.join(GAME, name + ext) for ext in (".dll", ".exe")):
        if os.path.exists(path):
            return path
    path = os.path.join(GAME, "Shaders", "D3D", name + ".dll")
    if os.path.exists(path):
        return path
    sys.exit(f"module not found in JPOG/: {name}")


class Image:
    """Reads initialised data from the PE file by virtual address."""

    def __init__(self, path):
        with open(path, "rb") as f:
            self.data = f.read()
        pe = struct.unpack_from("<I", self.data, 0x3C)[0]
        count = struct.unpack_from("<H", self.data, pe + 6)[0]
        opt_size = struct.unpack_from("<H", self.data, pe + 20)[0]
        self.base = struct.unpack_from("<I", self.data, pe + 24 + 28)[0]
        self.sections = []
        for i in range(count):
            _, vsize, va, rsize, raw = struct.unpack_from("<8sIIII", self.data, pe + 24 + opt_size + i * 40)
            self.sections.append((va, vsize, rsize, raw))

    def read(self, va, size):
        rva = va - self.base
        for sva, vsize, rsize, raw in self.sections:
            if sva <= rva < sva + max(vsize, rsize):
                if rva + size > sva + rsize:
                    return None  # uninitialised (BSS): only known at run time
                return self.data[raw + rva - sva: raw + rva - sva + size]
        return None


def function_range(module, pattern):
    index = os.path.join(ROOT, "analysis", "decomp", f"{module}.tsv")
    if not os.path.exists(index):
        sys.exit(f"{index} missing; run tools/ghidra_export.py {module} first")
    rows = []
    with open(index, encoding="utf-8") as f:
        next(f)
        for line in f:
            addr, size, name = line.rstrip("\n").split("\t")[:3]
            rows.append((int(addr, 16), int(size), name))
    if re.fullmatch(r"0x[0-9a-fA-F]+", pattern):
        hits = [r for r in rows if r[0] == int(pattern, 16)]
    else:
        hits = [r for r in rows if re.search(pattern, r[2])]
    if len(hits) != 1:
        listing = "\n".join(f"  {a:#x} {n}" for a, _, n in hits[:20])
        sys.exit(f"pattern matched {len(hits)} functions" + (f":\n{listing}" if hits else ""))
    return hits[0]


def disassemble(path, start, size):
    out = subprocess.run([msvc.tool("dumpbin.exe"), "/nologo", "/disasm", f"/range:{start:#x},{start + size:#x}",
                          path], capture_output=True, text=True, check=True).stdout
    code = []
    for line in out.splitlines():
        m = LINE.match(line)
        if m:
            addr = int(m.group(1), 16)
            if start <= addr < start + size:
                code.append((addr, m.group(2), m.group(3)))
    return code


def fmt_float(value):
    text = repr(value)
    return text if "e" in text or "." in text or "n" in text else text + ".0"


class Machine:
    def __init__(self, image, args, matrix, tsv_rows):
        self.image = image
        self.matrix = matrix
        self.rows = tsv_rows
        self.args = args
        self.st = []  # x87 stack, top at the end
        self.regs = {r: ("sym", r) for r in ("eax", "ebx", "edx", "esi", "edi", "ebp")}
        self.regs["esp"] = ("ptr", "sp", 0)
        self.regs["ecx"] = ("ptr", args[0], 0) if args else ("sym", "ecx")
        self.memory = {}
        self.events = []
        self.stack_args = args[1:]
        self.temps = 0

    # -- values ----------------------------------------------------------------------------------
    def name(self, base, offset):
        if self.matrix and offset % 4 == 0 and 0 <= offset < 64:
            return f"{base}[{offset // 16}][{offset % 16 // 4}]"
        if offset % 4 == 0:
            return f"{base}[{offset // 4}]"
        return f"{base}[+{offset:#x}]"

    def address(self, text):
        """Returns (base, offset) for a memory operand."""
        m = MEM.match(text)
        if not m:
            raise ValueError(f"unsupported operand {text}")
        expr = m.group(2).replace(" ", "")
        if re.fullmatch(r"[0-9A-F]+h", expr):
            return ("abs", int(expr[:-1], 16))
        terms = re.findall(r"[+-]?[^+-]+", expr)
        base, offset = None, 0
        for term in terms:
            sign = -1 if term.startswith("-") else 1
            term = term.lstrip("+-")
            if re.fullmatch(r"[0-9A-F]+h|\d+", term):
                offset += sign * (int(term[:-1], 16) if term.endswith("h") else int(term))
            elif term in self.regs and self.regs[term][0] == "int":
                offset += sign * self.regs[term][1]
            elif "*" in term and term.split("*")[0] in self.regs and self.regs[term.split("*")[0]][0] == "int":
                offset += sign * self.regs[term.split("*")[0]][1] * int(term.split("*")[1])
            elif term in self.regs and base is None:
                value = self.regs[term]
                if value[0] != "ptr":
                    return ("unknown", f"[{expr}]")
                base, offset = value[1], offset + value[2]
            else:
                return ("unknown", f"[{expr}]")
        return (base, offset)

    def load(self, operand, kind="float"):
        size = MEM.match(operand).group(1)
        addr = self.address(operand)
        if addr in self.memory:
            return self.memory[addr]
        base, offset = addr
        if base == "abs":
            raw = self.image.read(offset, 8 if size == "qword" else 4)
            if raw is not None and kind == "float":
                value = struct.unpack("<d" if size == "qword" else "<f", raw)[0]
                return ("sym", fmt_float(value))
            return ("sym", f"mem_{offset:08x}")
        if base == "unknown":
            return ("sym", offset)
        if base == "sp" and offset >= 4:
            index = (offset - 4) // 4
            name = self.stack_args[index] if index < len(self.stack_args) else f"arg{index}"
            return ("ptr", name, 0) if kind == "int" else ("sym", name)
        if kind == "int":
            return ("sym", self.name(base, offset))
        return ("sym", self.name(base, offset))

    def store(self, operand, value):
        addr = self.address(operand)
        # Arithmetic already rounds to float at gameplay precision, but transcendental and call
        # results carry extended precision until they are spilled to a float local.
        if addr[0] == "sp" and MEM.match(operand).group(1) == "dword" and value[0] == "sym":
            if value[1].startswith(("sin(", "cos(", "atan2(", "sqrt(")) or value[1].endswith("()"):
                value = ("sym", f"float({value[1]})")
        # Name compound values spilled to the stack, so long expressions stay readable.
        if addr[0] == "sp" and value[0] == "sym" and " " in value[1]:
            name = f"t{self.temps}"
            self.temps += 1
            self.events.append(f"{name} = {self.text(value)}")
            value = ("sym", name)
        self.memory[addr] = value
        base, offset = addr
        if base not in ("sp", "unknown"):
            label = f"mem_{offset:08x}" if base == "abs" else self.name(base, offset)
            self.events.append(f"{label} = {self.text(value)}")

    def raw(self, value):
        """Expression text for use as an operand (keeps its parentheses)."""
        return value[1] if value[0] == "sym" else self.text(value)

    @staticmethod
    def text(value):
        if value[0] == "ptr":
            return f"&{value[1]}" + (f"+{value[2]:#x}" if value[2] else "")
        expr = value[1]
        if not (expr.startswith("(") and expr.endswith(")")):
            return expr
        depth = 0
        for i, ch in enumerate(expr):
            depth += {"(": 1, ")": -1}.get(ch, 0)
            if depth == 0 and i < len(expr) - 1:
                return expr  # the first "(" closes before the end: not one enclosing pair
        return expr[1:-1]

    # -- x87 stack -------------------------------------------------------------------------------
    def sti(self, i):
        return len(self.st) - 1 - i

    def push(self, expr):
        self.st.append(expr)

    def pop(self):
        return self.st.pop()

    def operand_expr(self, operand):
        m = STI.match(operand)
        if m:
            return self.st[self.sti(int(m.group(1)))]
        return self.raw(self.load(operand))

    def binop(self, op, mnemonic, operands):
        pop = mnemonic.endswith("p")
        reverse = mnemonic.rstrip("p").endswith("r")
        ops = [o.strip() for o in operands.split(",")] if operands else []
        if not ops:
            ops = ["st(1)", "st"]
        if len(ops) == 1:  # memory or st(i) with st(0) as destination
            dst, src = 0, self.operand_expr(ops[0])
            a, b = self.st[self.sti(0)], src
        else:
            dst = 0 if ops[0] == "st" else int(STI.match(ops[0]).group(1))
            other = ops[1] if ops[0] == "st" else ops[0]
            a = self.st[self.sti(dst)]
            b = self.st[self.sti(0)] if ops[0] != "st" else self.operand_expr(other)
        if reverse:
            a, b = b, a
        self.st[self.sti(dst)] = f"({a} {op} {b})"
        if pop:
            self.pop()

    def run(self, code, follow):
        jumps = {}
        i = 0
        while i < len(code):
            addr, mnem, ops = code[i]
            i += 1
            try:
                target = self.step(addr, mnem, ops)
            except (ValueError, IndexError, AttributeError) as error:
                self.events.append(f"; {addr:08X} stopped at '{mnem} {ops}': {error}")
                return
            if target is not None and (target in follow or mnem == "jmp"):
                jumps[addr] = jumps.get(addr, 0) + 1
                if jumps[addr] > 1:
                    self.events.append(f"; {addr:08X} loop detected, stopping")
                    return
                i = next((k for k, c in enumerate(code) if c[0] == target), len(code))

    def step(self, addr, mnem, ops):
        regs = self.regs
        parts = [o.strip() for o in ops.split(",")] if ops else []
        if mnem in ("fld", "fild"):
            if STI.match(ops):
                self.push(self.st[self.sti(int(STI.match(ops).group(1)))])
            else:
                value = self.raw(self.load(ops))
                self.push(f"int({value})" if mnem == "fild" else value)
        elif mnem in ("fld1", "fldz", "fldpi"):
            self.push({"fld1": "1.0", "fldz": "0.0", "fldpi": "pi"}[mnem])
        elif mnem in ("fst", "fstp", "fistp", "fist"):
            value = self.st[self.sti(0)]
            m = STI.match(ops)
            if m:
                self.st[self.sti(int(m.group(1)))] = value
            else:
                self.store(ops, ("sym", f"int({value})" if mnem.startswith("fi") else value))
            if mnem.endswith("p"):
                self.pop()
        elif mnem == "fxch":
            j = self.sti(int(STI.match(ops).group(1)) if ops else 1)
            self.st[-1], self.st[j] = self.st[j], self.st[-1]
        elif mnem.rstrip("p").rstrip("r") in ("fadd", "fsub", "fmul", "fdiv"):
            op = {"fadd": "+", "fsub": "-", "fmul": "*", "fdiv": "/"}[mnem.rstrip("p").rstrip("r")]
            self.binop(op, mnem, ops)
        elif mnem in ("fchs", "fabs", "fsqrt", "fsin", "fcos", "frndint"):
            fn = {"fchs": "-", "fabs": "fabs", "fsqrt": "sqrt", "fsin": "sin", "fcos": "cos", "frndint": "rint"}
            arg = self.st[-1]
            self.st[-1] = f"-{arg}" if mnem == "fchs" else f"{fn[mnem]}({self.text(('sym', arg))})"
        elif mnem == "fsincos":
            arg = self.text(("sym", self.st[-1]))
            self.st[-1] = f"sin({arg})"
            self.push(f"cos({arg})")
        elif mnem == "fpatan":
            x = self.pop()
            y = self.st[-1]
            self.st[-1] = f"atan2({self.text(('sym', y))}, {self.text(('sym', x))})"
        elif mnem in ("fcom", "fcomp", "fcompp", "fucom", "fucomp", "fucompp", "ftst"):
            other = self.operand_expr(ops) if ops else (self.st[self.sti(1)] if mnem != "ftst" else "0.0")
            self.events.append(f"; {addr:08X} compare {self.text(('sym', self.st[-1]))} with {self.text(('sym', other))}")
            for _ in range(mnem.count("p")):
                self.pop()
        elif mnem in ("fnstsw", "sahf", "test", "cmp", "fldcw", "fnstcw", "fwait", "nop", "int3"):
            pass
        elif mnem.startswith("j"):
            target = int(ops.split()[-1], 16) if re.fullmatch(r"[0-9A-F]{8}", ops.split()[-1]) else None
            if mnem != "jmp":
                self.events.append(f"; {addr:08X} {mnem} {ops} (not taken)")
            return target if mnem != "jmp" else (target if target is not None else None)
        elif mnem == "push":
            regs["esp"] = ("ptr", "sp", regs["esp"][2] - 4)
            value = regs[ops] if ops in regs else (self.load(ops, "int") if "[" in ops else ("sym", ops))
            self.memory[("sp", regs["esp"][2])] = value
        elif mnem == "pop":
            regs[ops] = self.memory.get(("sp", regs["esp"][2]), ("sym", ops))
            regs["esp"] = ("ptr", "sp", regs["esp"][2] + 4)
        elif mnem in ("sub", "add") and parts[0] == "esp":
            amount = int(parts[1].rstrip("h"), 16) if parts[1].endswith("h") else int(parts[1])
            if amount >= 0x80000000:
                amount -= 0x100000000
            regs["esp"] = ("ptr", "sp", regs["esp"][2] + (-amount if mnem == "sub" else amount))
        elif mnem == "xor" and parts[0] == parts[1] and parts[0] in regs:
            regs[parts[0]] = ("int", 0)
        elif mnem in ("add", "sub", "inc", "dec", "shl") and parts and parts[0] in regs and regs[parts[0]][0] in ("int", "ptr"):
            reg = parts[0]
            amount = 1 if mnem in ("inc", "dec") else (
                int(parts[1].rstrip("h"), 16) if parts[1].endswith("h") else int(parts[1]))
            kind, *rest = regs[reg]
            current = rest[-1]
            if mnem == "shl":
                current <<= amount
            else:
                current += amount if mnem in ("add", "inc") else -amount
            regs[reg] = ("int", current) if kind == "int" else ("ptr", rest[0], current)
        elif mnem == "mov":
            dst, src = parts
            if dst in regs:
                regs[dst] = regs[src] if src in regs else (self.load(src, "int") if "[" in src else ("sym", src))
            else:
                value = regs[src] if src in regs else ("sym", self.const_bits(src))
                self.store(dst, value)
        elif mnem == "lea":
            base, offset = self.address(parts[1])
            regs[parts[0]] = ("ptr", base, offset) if base not in ("abs", "unknown") else ("sym", parts[1])
        elif mnem == "call":
            self.call(addr, ops)
        elif mnem == "ret":
            return None
        else:
            self.events.append(f"; {addr:08X} ignored: {mnem} {ops}")
        return None

    @staticmethod
    def const_bits(text):
        if re.fullmatch(r"[0-9A-F]+h|\d+", text):
            bits = int(text[:-1], 16) if text.endswith("h") else int(text)
            return f"bits({bits:#x})={fmt_float(struct.unpack('<f', struct.pack('<I', bits & 0xFFFFFFFF))[0])}"
        return text

    def call(self, addr, ops):
        target = int(ops, 16) if re.fullmatch(r"[0-9A-F]{8}", ops) else None
        name = next((n for a, _, n in self.rows if a == target), ops)
        esp = self.regs["esp"][2]
        args = [self.text(self.memory.get(("sp", esp + 4 * k), ("sym", "?"))) for k in range(4)]
        self.events.append(f"; {addr:08X} call {name} (ecx={self.text(self.regs['ecx'])}, stack: {', '.join(args)}, ...)")
        # The callee pops its own arguments for __stdcall/__thiscall: take the count from its ret.
        popped = 0
        entry = next((r for r in self.rows if r[0] == target), None)
        if entry:
            code = disassemble(self.path, entry[0], entry[1])
            rets = [c for c in code if c[1] == "ret"]
            if rets and rets[0][2]:
                popped = int(rets[0][2].rstrip("h"), 16) if rets[0][2].endswith("h") else int(rets[0][2])
        self.regs["esp"] = ("ptr", "sp", esp + popped)
        self.push(f"{name.split('::')[-1]}()")
        self.regs["eax"] = ("sym", f"{name.split('::')[-1]}()")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("module", help="module stem, e.g. TKernelInterface")
    parser.add_argument("function", help="regex over the function name, or its address")
    parser.add_argument("--args", default="this", help="names for ECX and the stack arguments, comma-separated")
    parser.add_argument("--matrix", action="store_true", help="show 4x4 float matrices as name[row][col]")
    parser.add_argument("--follow", action="append", default=[], help="take the conditional jump at ADDR")
    parser.add_argument("--asm", action="store_true", help="also print the disassembly")
    args = parser.parse_args()

    path = module_path(args.module)
    start, size, name = function_range(args.module, args.function)
    code = disassemble(path, start, size)
    rows = []
    with open(os.path.join(ROOT, "analysis", "decomp", f"{args.module}.tsv"), encoding="utf-8") as f:
        next(f)
        for line in f:
            a, s, n = line.rstrip("\n").split("\t")[:3]
            rows.append((int(a, 16), int(s), n))
    machine = Machine(Image(path), [a.strip() for a in args.args.split(",")], args.matrix, rows)
    machine.path = path
    print(f"// {name} at {start:#x} ({size} bytes)")
    if args.asm:
        for addr, mnem, ops in code:
            print(f"//   {addr:08X} {mnem} {ops}")
    follow = set()
    for item in args.follow:
        jump = next((c for c in code if c[0] == int(item, 16)), None)
        if jump:
            follow.add(int(jump[2].split()[-1], 16))
    machine.run(code, follow)
    for event in machine.events:
        print(event)
    if machine.st:
        print(f"; x87 stack at exit (top last): {', '.join(machine.text(('sym', e)) for e in machine.st)}")


if __name__ == "__main__":
    main()

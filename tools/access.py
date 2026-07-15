#!/usr/bin/env python3
"""Memory-access map of a function: offset -> width / signedness / direction.

Neither Ghidra (invents bogus union-member names) nor m2c (opaque `unkN`)
discloses the ACCESS WIDTH of a struct field — but that's exactly what picks
`sw` vs `sh` vs `sb` (and `lh` vs `lhu` = s16 vs u16) when you reach a field
through an offset cast. This reads it straight from the load/store mnemonics of
the ORIGINAL bytes, grouped by base register + offset, so you don't hand-trace
the `.s`.

  tools/access.py <Name>            all accesses, grouped by base reg + offset
  tools/access.py <Name> --order    accesses in INSTRUCTION ORDER (the store
                                    sequence grouping hides — e.g. interleaved
                                    lhu/sh pairs, which fields write before which)
  tools/access.py <Name> --reg a1   only accesses off $a1 (e.g. the param)

Base registers a0-a3 hold the incoming params at entry (a0=1st, ...). A value
loaded into a callee-saved reg is often a cached pointer — cross-reference with
the disassembly. Run inside the nix devShell.
"""
import argparse, os, re, subprocess, sys

import function_inventory as FI

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

TEXT_START = 0x80011000
FILE_OFF = 0x800
ORIG = "disks/tenchu/main.exe"
SYMBOLS = "config/symbols.main.exe.txt"
TSV = ".shake/ghidra-export/functions.tsv"
OBJDUMP = "mipsel-unknown-linux-gnu-objdump"

# mnemonic -> (bytes, kind)
LOADS = {"lw": (4, "s32"), "lh": (2, "s16"), "lhu": (2, "u16"),
         "lb": (1, "s8"), "lbu": (1, "u8"),
         "lwl": (4, "s32?"), "lwr": (4, "s32?")}
STORES = {"sw": (4, "32"), "sh": (2, "16"), "sb": (1, "8"),
          "swl": (4, "32?"), "swr": (4, "32?")}


def bounds(name, symbols=SYMBOLS, tsv=TSV):
    addr = None
    with open(symbols) as fh:
        for line in fh:
            m = re.match(
                rf"{re.escape(name)}\s*=\s*(0x[0-9A-Fa-f]+)\s*;", line
            )
            if m:
                addr = int(m.group(1), 16)
                break
    if addr is None:
        sys.exit(f"access: {name} not in {symbols}")
    for fn_addr, size, _ in FI.load_functions(tsv):
        if fn_addr == addr:
            return addr, size
    sys.exit(f"access: no size for {name} in {tsv}")


def disasm(addr, size):
    data = open(ORIG, "rb").read()
    off = FILE_OFF + (addr - TEXT_START)
    open("/tmp/access.bin", "wb").write(data[off:off + size])
    out = subprocess.run(
        [OBJDUMP, "-D", "-b", "binary", "-m", "mips", "-EL",
         "--adjust-vma", hex(addr), "/tmp/access.bin"],
        capture_output=True, text=True).stdout
    r = []
    for line in out.splitlines():
        m = re.match(r"\s*([0-9a-f]+):\t[0-9a-f]{8} \t(\w+)\s+(.*)", line)
        if m:
            r.append((int(m.group(1), 16), m.group(2), m.group(3).strip()))
    return r


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name")
    ap.add_argument("--reg", help="only accesses off this base register (e.g. a1)")
    ap.add_argument("--order", action="store_true",
                    help="list accesses in instruction order (shows the store "
                         "sequence) instead of grouping by offset")
    args = ap.parse_args()
    addr, size = bounds(args.name)
    insns = disasm(addr, size)

    # collect: {(base, offset): {"widths": set, "dirs": set, "n": count, "at": [addr]}}
    acc = {}
    mem = re.compile(r"^\$?(\w+),\s*(-?\d+|-?0x[0-9a-f]+)\(\$?(\w+)\)")
    for ia, mn, ops in insns:
        info = LOADS.get(mn) or STORES.get(mn)
        if not info:
            continue
        m = mem.match(ops)
        if not m:
            continue
        _, off_s, base = m.groups()
        off = int(off_s, 0)
        if args.reg and base != args.reg:
            continue
        kind = "load" if mn in LOADS else "store"
        key = (base, off)
        e = acc.setdefault(key, {"ty": set(), "dir": set(), "n": 0, "mn": set()})
        e["ty"].add(info[1]); e["dir"].add(kind); e["n"] += 1; e["mn"].add(mn)

    # instruction-ordered view: shows the store/load SEQUENCE (which grouping
    # hides) — the interleaved lhu/sh pattern, base register, offset, width.
    PARAM = {"a0": "param1", "a1": "param2", "a2": "param3", "a3": "param4"}
    if args.order:
        print(f"{args.name} @ {addr:#x} ({size}B) — accesses in instruction order:")
        for ia, mn, ops in insns:
            info = LOADS.get(mn) or STORES.get(mn)
            if not info:
                continue
            m = mem.match(ops)
            if not m:
                continue
            _, off_s, base = m.groups()
            if args.reg and base != args.reg:
                continue
            off = int(off_s, 0)
            kind = "load " if mn in LOADS else "store"
            tag = f" ({PARAM[base]})" if base in PARAM else ""
            print(f"  {ia:#010x}  {kind} ${base:<3}{tag:9} +{off:#05x} "
                  f"{info[1]:5} [{mn}]")
        return

    if not acc:
        print(f"access: {args.name} has no register+offset memory accesses"
              + (f" off ${args.reg}" if args.reg else ""))
        return

    print(f"{args.name} @ {addr:#x} ({size}B) — base reg + offset -> type / dir "
          f"(--order shows the store sequence):")
    for (base, off) in sorted(acc, key=lambda k: (k[0], k[1])):
        e = acc[(base, off)]
        tag = f" ({PARAM[base]})" if base in PARAM else ""
        ty = "/".join(sorted(e["ty"]))
        dr = "/".join(sorted(e["dir"]))
        mns = ",".join(sorted(e["mn"]))
        print(f"  ${base:<3}{tag:9} +{off:#05x}  {ty:8} {dr:11} [{mns}] x{e['n']}")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Estimate how hard each function is to byte-match, and why.

Reads features straight from the original disassembly (size, jump table,
float/mul-div, loops, gp-relative refs, frame size, callee count, indirect
calls) plus how similar the function is to something already matched, and turns
them into a difficulty score + a plain-English "why" + the cookbook sections
that will matter. Pick a function that fits your time/skill (humans) or route/
size an agent (orchestrator).

  tools/triage.py                 all UNMATCHED game functions, easiest first
  tools/triage.py --all           include SDK-region + matched
  tools/triage.py --hard          hardest first
  tools/triage.py --leverage      sort by call in-degree (high-impact first)
  tools/triage.py <Name>          detail for one function (features + docs)
  tools/triage.py --max-size N    only functions <= N bytes

`relevant_docs(name)` is importable (reverse.py annotates its seed with it).
Run inside the nix devShell.
"""
import argparse, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

TEXT_START = 0x80011000
TEXT_END = 0x80098000
SDK_START = 0x80061000
FILE_OFF = 0x800
ORIG = "disks/tenchu/main.exe"
TSV = ".shake/ghidra-export/functions.tsv"
SYMBOLS = "config/symbols.main.exe.txt"
SRC = "src/main.exe"
OBJDUMP = "mipsel-unknown-linux-gnu-objdump"
COOK = "docs/matching-cookbook.md"

_cache = {}


def load():
    if _cache:
        return _cache
    # config/symbols is the source of truth for names (reverse.py names the .c
    # file after it), so prefer the symbol name over the Ghidra TSV's FUN_ name —
    # otherwise a function matched under its real name shows as unmatched here.
    symname, switchdata = {}, set()
    for line in open(SYMBOLS):
        m = re.match(r"([A-Za-z_$][\w$]*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;", line)
        if m:
            symname[int(m.group(2), 16)] = m.group(1)
        m = re.search(r"switchD_([0-9a-fA-F]+)__switchdataD_", line)
        if m:
            switchdata.add(int(m.group(1), 16))
    funcs = []
    for line in open(TSV):
        p = line.rstrip("\n").split("\t")
        if len(p) == 3 and re.match(r"^[A-Za-z_]\w*$", p[2]):
            a, s, n = int(p[0], 16), int(p[1]), p[2]
            if TEXT_START <= a < TEXT_END and s >= 8:
                funcs.append((a, s, symname.get(a, n)))
    funcs.sort()
    out = subprocess.run(
        [OBJDUMP, "-D", "-b", "binary", "-m", "mips", "-EL",
         "--adjust-vma", hex(TEXT_START - FILE_OFF), ORIG],
        capture_output=True, text=True).stdout
    insns = {}  # addr -> (mnemonic, operands)
    pat = re.compile(r"^\s*([0-9a-f]+):\t[0-9a-f]{8} \t(\S+)\s*(.*)")
    for line in out.splitlines():
        m = pat.match(line)
        if m and int(m.group(1), 16) >= TEXT_START:
            insns[int(m.group(1), 16)] = (m.group(2), m.group(3))
    _cache.update(funcs=funcs, switchdata=switchdata, insns=insns)
    return _cache


def matched(name):
    f = os.path.join(SRC, name + ".c")
    return os.path.exists(f) and not re.search(r"^\s*INCLUDE_ASM", open(f).read(), re.M)


FLOAT = re.compile(r"^(lwc1|swc1|mtc1|mfc1|ctc1|cfc1|.*\.s|.*\.d|c\.)")
MULDIV = {"mult", "multu", "div", "divu"}


def features(a, s):
    """Extract the difficulty-relevant features of a function's body."""
    c = load()
    mns = []
    floats = muldiv = gp = callees = indirect = loops = 0
    frame = 0
    seen_callees = set()
    for ia in range(a, a + s, 4):
        mn, ops = c["insns"].get(ia, ("?", ""))
        mns.append(mn)
        if FLOAT.match(mn):
            floats += 1
        if mn in MULDIV:
            muldiv += 1
        if "(gp)" in ops:
            gp += 1
        if mn == "jal":
            m = re.search(r"0x([0-9a-f]+)", ops)
            if m:
                seen_callees.add(int(m.group(1), 16))
        if mn == "jalr":
            indirect += 1
        if mn in ("beq", "bne", "beqz", "bnez", "bgez", "bltz", "blez", "bgtz", "b"):
            m = re.search(r"0x([0-9a-f]+)", ops)
            if m and int(m.group(1), 16) < ia:  # backward branch ~ loop
                loops += 1
        if mn == "addiu" and ops.startswith("sp,sp,-"):
            frame = -int(ops.split(",")[-1])
    return dict(mns=mns, floats=floats, muldiv=muldiv, gp=gp,
                callees=len(seen_callees), indirect=indirect, loops=loops,
                frame=frame, switch=a in c["switchdata"])


def ngrams(mns, N=4):
    return {tuple(mns[i:i + N]) for i in range(len(mns) - N + 1)}


def nearest_matched(a, s, name):
    """(similarity, matched_name) of the closest already-matched function."""
    c = load()
    g = ngrams(features(a, s)["mns"])
    best = (0.0, None)
    for ma, ms, mn in c["funcs"]:
        if mn == name or not matched(mn):
            continue
        gm = ngrams(features(ma, ms)["mns"])
        if not g or not gm:
            continue
        j = len(g & gm) / len(g | gm)
        if j > best[0]:
            best = (j, mn)
    return best


# feature -> (cookbook section, why it's relevant)
def docs_for(f):
    d = []
    if f["switch"]:
        d.append(("Split functions / Dispatch",
                  "jump-table switch — scaffold with tools/split-scaffold.py"))
    if any(m in ("slti", "sltiu") for m in f["mns"]) and f["loops"] == 0 and not f["switch"]:
        d.append(("Dispatch", "if/switch ladder — reload vs CSE, signed vs unsigned"))
    if f["loops"]:
        d.append(("Loops", f"{f['loops']} back-edge(s) — for/while/do vs goto shape"))
    if f["muldiv"]:
        d.append(("Expressions", "mult/div — magic-multiply constants, fold"))
    if f["gp"]:
        d.append(("gp vs absolute globals", "gp-relative smalls — tools/gpsyms.py"))
    if f["frame"] > 0x200:
        d.append(("Stack objects", f"0x{f['frame']:x} frame — buffer casts / spills"))
    if f["indirect"]:
        d.append(("Register allocation steering", "indirect call — null-check-var/call-field"))
    if f["floats"]:
        d.append(("(sparse in cookbook)", "float/GTE ops — few documented idioms yet"))
    return d


def score(f, sim):
    """Higher = harder. Size-driven, penalised by hard features, discounted by
    a matched worked example / near-clone twin."""
    base = len(f["mns"])
    base += 320 * bool(f["floats"])
    base += 150 * f["switch"]
    base += 60 * bool(f["muldiv"])
    base += 40 * f["loops"]
    base += 30 * (f["frame"] > 0x200)
    base += 5 * f["callees"]
    if sim >= 0.99:
        return 15                      # near-clone: clone the twin
    if sim >= 0.6:
        base *= 0.5                    # strong worked example
    elif sim >= 0.4:
        base *= 0.75
    return base


def bucket(sc):
    return ("trivial" if sc < 60 else "easy" if sc < 150 else "medium"
            if sc < 400 else "hard" if sc < 900 else "very-hard")


def relevant_docs(name):
    """Importable: cookbook sections likely needed for `name` (for reverse.py)."""
    c = load()
    hit = next((x for x in c["funcs"] if x[2] == name), None)
    if not hit:
        return []
    return [sec for sec, _why in docs_for(features(hit[0], hit[1]))]


def why(f, sim, twin):
    bits = [f"{len(f['mns'])} insns"]
    if f["switch"]:
        bits.append("JUMP TABLE")
    if f["floats"]:
        bits.append(f"{f['floats']} float/GTE")
    if f["muldiv"]:
        bits.append("mul/div")
    if f["loops"]:
        bits.append(f"{f['loops']} loop")
    if f["frame"] > 0x200:
        bits.append(f"frame 0x{f['frame']:x}")
    if f["indirect"]:
        bits.append("indirect-call")
    bits.append(f"{f['callees']} callees")
    if sim >= 0.99:
        bits.append(f"NEAR-CLONE of {twin} — clone it")
    elif twin:
        bits.append(f"~{sim:.2f} to {twin}")
    return ", ".join(bits)


def indegree():
    c = load()
    out = subprocess.run(
        [OBJDUMP, "-D", "-b", "binary", "-m", "mips", "-EL",
         "--adjust-vma", hex(TEXT_START - FILE_OFF), ORIG],
        capture_output=True, text=True).stdout
    import collections
    callers = collections.defaultdict(set)
    pat = re.compile(r"^\s*([0-9a-f]+):\t[0-9a-f]{8} \tjal\s+0x([0-9a-f]+)")
    for l in out.splitlines():
        m = pat.match(l)
        if m and int(m.group(1), 16) >= TEXT_START:
            callers[int(m.group(2), 16)].add(int(m.group(1), 16))
    funcs = c["funcs"]

    def containing(ia):
        for a, s, n in funcs:
            if a <= ia < a + s:
                return n
    deg = {}
    for a, s, n in funcs:
        deg[n] = len({containing(i) for i in callers.get(a, ())} - {None})
    return deg


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("name", nargs="?")
    ap.add_argument("--all", action="store_true", help="include SDK + matched")
    ap.add_argument("--hard", action="store_true", help="hardest first")
    ap.add_argument("--leverage", action="store_true",
                    help="sort by call in-degree (high-impact first)")
    ap.add_argument("--max-size", type=int, default=0)
    ap.add_argument("--top", type=int, default=40)
    args = ap.parse_args()
    c = load()

    if args.name:
        hit = next((x for x in c["funcs"] if x[2] == args.name), None)
        if not hit:
            sys.exit(f"triage: {args.name} not in {TSV}")
        a, s, n = hit
        f = features(a, s)
        sim, twin = nearest_matched(a, s, n)
        sc = score(f, sim)
        print(f"{n} @ {a:#x} ({s} bytes) — {bucket(sc).upper()} (score {sc:.0f})")
        print(f"  why: {why(f, sim, twin)}")
        print(f"  matched: {matched(n)}")
        print("  relevant cookbook sections:")
        for sec, w in docs_for(f) or [("(base rules)", "small/plain function")]:
            print(f"    - {sec}: {w}")
        return

    deg = indegree() if args.leverage else None
    rows = []
    for a, s, n in c["funcs"]:
        if not args.all:
            if a >= SDK_START or matched(n):
                continue
        if args.max_size and s > args.max_size:
            continue
        f = features(a, s)
        sim, twin = nearest_matched(a, s, n)
        sc = score(f, sim)
        rows.append((sc, a, s, n, f, sim, twin))
    if args.leverage:
        rows.sort(key=lambda r: -deg.get(r[3], 0))
    else:
        rows.sort(key=lambda r: -r[0] if args.hard else r[0])

    hdr = "lev " if args.leverage else ""
    print(f"{'diff':9} {hdr}{'size':>5}  {'function':26} why")
    for sc, a, s, n, f, sim, twin in rows[:args.top]:
        lev = f"{deg.get(n,0):3} " if args.leverage else ""
        print(f"{bucket(sc):9} {lev}{s:5}  {n:26} {why(f, sim, twin)}")
    print(f"\n({len(rows)} functions; `tools/triage.py <Name>` for detail + docs)")


if __name__ == "__main__":
    main()

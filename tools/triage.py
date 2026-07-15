#!/usr/bin/env python3
"""Estimate how hard each function is to byte-match, and why.

Reads features straight from the original disassembly (size, jump table,
float/mul-div, loops, gp-relative refs, frame size, callee count, indirect
calls) plus how similar the function is to something already matched, and turns
them into a difficulty score + a plain-English "why" + the cookbook sections
that will matter. Pick a function that fits your time/skill (humans) or route/
size an agent (orchestrator).

  tools/triage.py                 all UNMATCHED, UNPARKED game funcs, easiest first
  tools/triage.py --all           include SDK-region + matched + parked
  tools/triage.py --hard          hardest first
  tools/triage.py --leverage      sort by call in-degree (high-impact first)
  tools/triage.py <Name>          detail for one function (features + docs)
  tools/triage.py --max-size N    only functions <= N bytes
  tools/triage.py --parked        only the shelved functions, with their reason

PARKED functions are ones we already investigated and consciously shelved (a
documented `STATUS: NON_MATCHING` in their .c — sub-C register ties, the
sll16/sra12/sra4 sign-extension class, etc). They are hidden by default: their
features still look easy, so leaving them in the list sends agents down dead
ends we have already paid for. Read the .c's STATUS note before reviving one.

`relevant_docs(name)` is importable (reverse.py annotates its seed with it).
Run inside the nix devShell.
"""
import argparse, os, re, subprocess, sys

import function_inventory as FI

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

TEXT_START = 0x80011000
TEXT_END = 0x80098000
SDK_START = 0x80060000  # 0x80060xxx is CRT/libcd/libapi (Exec/__main/Cd*/PC*/
# EVENT_OBJ*/DeliverEvent/_SN*) — a differently-compiled PsyQ SDK block that won't
# byte-match our cc1 (verified: mips_expand_epilogue shape we never emit); not game.
FILE_OFF = 0x800
ORIG = "disks/tenchu/main.exe"
TSV = ".shake/ghidra-export/functions.tsv"
SYMBOLS = "config/symbols.main.exe.txt"
SRC = "src/main.exe"
OBJDUMP = "mipsel-unknown-linux-gnu-objdump"
COOK = "docs/matching-cookbook.md"
YAML = "config/splat.main.exe.yaml"

_cache = {}


def load_functions(tsv=TSV, symbols=SYMBOLS, splat=YAML):
    """Reviewed function rows, preferring current carved/source names."""
    symname = {}
    with open(symbols) as stream:
        for line in stream:
            m = re.match(r"([A-Za-z_$][\w$]*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;", line)
            if m:
                symname[int(m.group(2), 16)] = m.group(1)
    carved_names = FI.load_splat_c_names(splat)
    return sorted(
        (a, s, carved_names.get(a, symname.get(a, n)))
        for a, s, n in FI.load_functions(tsv)
        if TEXT_START <= a < TEXT_END and s >= 8
        and re.match(r"^[A-Za-z_]\w*$", n)
    )


def load():
    if _cache:
        return _cache
    # Keep switch-table discovery in config/symbols; load_functions() resolves
    # reviewed extents and prefers the current splat C name over stale aliases.
    switchdata = set()
    for line in open(SYMBOLS):
        m = re.search(r"switchD_([0-9a-fA-F]+)__switchdataD_", line)
        if m:
            switchdata.add(int(m.group(1), 16))
    funcs = load_functions()
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


# `STATUS: NON_MATCHING — <reason>` (em dash, en dash or ASCII hyphen).
STATUS_RE = re.compile(r"STATUS:\s*NON_MATCHING\s*[—–-]?\s*(.*)")


def parked(name):
    """Reason if <name>.c exists, is still INCLUDE_ASM-backed, and is documented
    NON_MATCHING — i.e. we tried it and shelved it. None otherwise.

    A carve with no NON_MATCHING note is just an unfinished draft, still a fair
    target; only a deliberate park counts.
    """
    f = os.path.join(SRC, name + ".c")
    if not os.path.exists(f):
        return None
    src = open(f).read()
    if not re.search(r"^\s*INCLUDE_ASM", src, re.M):
        return None  # matched
    if "NON_MATCHING" not in src:
        return None  # fresh carve / in-progress draft
    m = STATUS_RE.search(src)
    return " ".join(m.group(1).split()) if m else "parked (see the .c)"


FLOAT = re.compile(r"^(lwc1|swc1|mtc1|mfc1|ctc1|cfc1|.*\.s|.*\.d|c\.)")
MULDIV = {"mult", "multu", "div", "divu"}
# COP2 data moves: `as` assembles these, but there is no C spelling for them.
COP2_MOVES = {"lwc2", "swc2", "mfc2", "mtc2", "cfc2", "ctc2"}
# objdump renders a GTE *command* (RTPS/NCLIP/MVMVA/...) as a bare `c2`.
# `as` cannot assemble those mnemonics on its own, but splat >= 0.4x generates
# include/gte_macros.inc (68 macros) which macro.inc includes, so the region IS
# splittable and every one of these functions is carved. What still blocks a
# MATCH is that no C construct emits a GTE opcode: that needs the inline-asm
# policy decision (same one as GetPad/PClseek). Keep them de-ranked, say why.
GTE_CMD = "c2"


def _shamt(ops):
    try:
        return int(ops.split(",")[-1].strip(), 0)
    except ValueError:
        return None


def signext_split(win):
    """True for the `sll r,r,16 / sra r,r,k / sra r,r,16-k` (k != 16) triple.

    That's a short's sign-extension FUSED with a left-scale, and it has no
    natural-C spelling: cc1 refolds every plain-arithmetic respelling back to
    the textbook 2-shift sll16/sra16. Whole game code has exactly three (the
    GetPad/FUN_8001b174/GetPadXY trio), all parked pending the inline-asm
    policy — so flag it rather than let it read as a TRIVIAL target.
    """
    if len(win) != 3:
        return False
    (m1, o1), (m2, o2), (m3, o3) = win
    if (m1, m2, m3) != ("sll", "sra", "sra"):
        return False
    a1, a2, a3 = _shamt(o1), _shamt(o2), _shamt(o3)
    return a1 == 16 and a2 is not None and a3 is not None \
        and a2 != 16 and a2 + a3 == 16


def features(a, s):
    """Extract the difficulty-relevant features of a function's body."""
    c = load()
    mns = []
    floats = muldiv = gp = callees = indirect = loops = 0
    frame = 0
    signext = gtecmd = cop2 = 0
    win = []
    seen_callees = set()
    for ia in range(a, a + s, 4):
        mn, ops = c["insns"].get(ia, ("?", ""))
        mns.append(mn)
        win = (win + [(mn, ops)])[-3:]
        if signext_split(win):
            signext += 1
        if mn == GTE_CMD:
            gtecmd += 1
        if mn in COP2_MOVES:
            cop2 += 1
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
                frame=frame, switch=a in c["switchdata"], signext=signext,
                gtecmd=gtecmd, cop2=cop2)


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
        d.append(("gp vs absolute globals", "gp-relative smalls — tools/maspsxflags.py"))
    if f["frame"] > 0x200:
        d.append(("Stack objects", f"0x{f['frame']:x} frame — buffer casts / spills"))
    if f["indirect"]:
        d.append(("Register allocation steering", "indirect call — null-check-var/call-field"))
    if f["floats"]:
        d.append(("(sparse in cookbook)", "float/GTE ops — few documented idioms yet"))
    if f["signext"]:
        d.append(("Toolchain gotchas",
                  "sll16/sra-split sign-extension — no natural-C form; see GetPad.c"))
    if f["gtecmd"]:
        d.append(("Toolchain gotchas",
                  "GTE command opcodes — splat's generated gte_macros.inc makes "
                  "the split assemble; a MATCH still needs the inline-asm policy, "
                  "since no C construct emits a GTE opcode"))
    elif f["cop2"]:
        d.append(("Toolchain gotchas",
                  "COP2 data moves — assemble, but have no C spelling; blocked on "
                  "the same inline-asm policy as GetPad/PClseek"))
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
    base += 400 * bool(f["signext"])  # no natural-C form (GetPad class)
    base += 800 * bool(f["gtecmd"])   # splittable, but no C form for GTE ops
    base += 400 * bool(f["cop2"])     # assembles, but no C spelling
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
    if f["gtecmd"]:
        bits.append(f"{f['gtecmd']} GTE CMD — split OK, no C form (inline-asm policy)")
    elif f["cop2"]:
        bits.append(f"{f['cop2']} COP2 moves — no C form (inline-asm policy)")
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
    if f["signext"]:
        bits.append("SIGNEXT-SPLIT (GetPad class — likely unmatchable)")
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
    ap.add_argument("--parked", action="store_true",
                    help="only the shelved (documented NON_MATCHING) functions")
    args = ap.parse_args()
    c = load()

    if args.parked:
        rows = [(a, s, n, parked(n)) for a, s, n in c["funcs"] if parked(n)]
        rows.sort(key=lambda r: r[1])
        print(f"{'size':>5}  {'function':26} why it was shelved")
        for a, s, n, p in rows:
            print(f"{s:5}  {n:26} {p[:72]}")
        print(f"\n({len(rows)} parked; read the .c's STATUS note before reviving one)")
        return

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
        p = parked(n)
        if p:
            print(f"  PARKED: {p}")
            print(f"          (read {os.path.join(SRC, n + '.c')} before reviving)")
        print("  relevant cookbook sections:")
        for sec, w in docs_for(f) or [("(base rules)", "small/plain function")]:
            print(f"    - {sec}: {w}")
        return

    deg = indegree() if args.leverage else None
    rows = []
    n_parked = 0
    for a, s, n in c["funcs"]:
        if not args.all:
            if a >= SDK_START or matched(n):
                continue
            if parked(n):
                n_parked += 1
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
        mark = "PARKED: " if args.all and parked(n) else ""
        print(f"{bucket(sc):9} {lev}{s:5}  {n:26} {mark}{why(f, sim, twin)}")
    print(f"\n({len(rows)} functions; `tools/triage.py <Name>` for detail + docs)")
    if n_parked:
        print(f"({n_parked} parked/shelved hidden; `tools/triage.py --parked` to see why)")


if __name__ == "__main__":
    main()

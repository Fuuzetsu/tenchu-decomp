#!/usr/bin/env python3
"""Diagnose cc1's register-allocation decisions — the tool for sub-C reg ties.

When a draft is arithmetically correct and the RIGHT LENGTH but a few bytes off
in pure register choice (Sound: OR result in $a1 vs $v0; InsertConflict: id in
$v1 vs $s0), the residual is cc1's global allocator coloring a value differently
than the original. Blindly re-rolling the permuter or restructuring is slow.

This runs `cc1 -dg` (the post-global-allocation RTL dump) on the function as the
build sees it and surfaces WHY each value landed where:
  * the pseudo -> hard-register dispositions,
  * which values are live across a call (cc1 only spends a callee-saved $s0-$s7
    on a value that survives a call — so the register CLASS reveals the pressure),
  * the register copy-chains (`reg <- reg` moves) that bias the coloring — the
    thing you actually break to move a value to a different register.

It turns "guess a source form and re-score" into "the dump says $s0 holds `id`
because of the move-chain at insns N/M — break that chain."

  tools/regalloc.py <Name>              # diagnose src/main.exe/<Name>.c as built
  tools/regalloc.py <Name> --rtl        # + the raw greg RTL for the function
  tools/regalloc.py <Name> --func NAME  # a specific function if the TU has several
  tools/regalloc.py <file> --raw        # run on an already-preprocessed .c

For a NON_MATCHING partial it automatically diagnoses the DRAFT (-DNON_MATCHING).
Run inside the nix devShell.
"""
import argparse, os, re, subprocess, sys, tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)
SRC = "src/main.exe"

# The build's own cpp + cc1 invocation (mirrors permute.py / Build.hs so the
# allocation matches the real object exactly).
CPP = ("mipsel-unknown-linux-gnu-cpp -Iinclude -undef -Wall -lang-c -fno-builtin "
       "-gstabs -Dmips -D__GNUC__=2 -D__OPTIMIZE__ -D__mips__ -D__mips -Dpsx "
       "-D__psx__ -D__psx -D_PSYQ -D__EXTENSIONS__ -D_MIPSEL -D_LANGUAGE_C "
       "-DLANGUAGE_C -DHACKS -I src/main.exe").split()
CC = "cc1-281"
CC_FLAGS = ("-mcpu=3000 -quiet -G8 -w -O2 -funsigned-char -fpeephole -ffunction-cse "
            "-fpcc-struct-return -fcommon -fverbose-asm -fgnu-linker -mgas "
            "-msoft-float").split()

# MIPS o32 register classes + number->name.
CALLER = set("v0 v1 a0 a1 a2 a3 t0 t1 t2 t3 t4 t5 t6 t7 t8 t9".split())
CALLEE = set(f"s{i}" for i in range(8))
REGNAME = ({0: "zero", 1: "at", 2: "v0", 3: "v1", 28: "gp", 29: "sp",
            30: "fp", 31: "ra"}
           | {4 + i: f"a{i}" for i in range(4)}
           | {8 + i: f"t{i}" for i in range(8)}
           | {16 + i: f"s{i}" for i in range(8)}
           | {24: "t8", 25: "t9", 26: "k0", 27: "k1"})


def rn(num):
    return REGNAME.get(num, f"r{num}")


def preprocess(name):
    """Preprocess src/main.exe/<name>.c the way the build does; returns text.
    Diagnoses the DRAFT of a NON_MATCHING partial."""
    path = os.path.join(SRC, name + ".c")
    if not os.path.exists(path):
        sys.exit(f"regalloc: {path} not found")
    nm = ["-DNON_MATCHING"] if "ifndef NON_MATCHING" in open(path).read() else []
    r = subprocess.run(CPP + nm + [path], capture_output=True, text=True)
    if r.returncode != 0:
        sys.exit("regalloc: cpp failed:\n" + r.stderr)
    return r.stdout


def run_greg(pp_text):
    """cc1 -dg on preprocessed text; returns the .greg dump text."""
    with tempfile.TemporaryDirectory() as d:
        src = os.path.join(d, "f.c")
        open(src, "w").write(pp_text)
        r = subprocess.run([CC] + CC_FLAGS + ["-dg", "f.c", "-o", "f.s"],
                           cwd=d, capture_output=True, text=True)
        greg = os.path.join(d, "f.c.greg")
        if not os.path.exists(greg):
            sys.exit("regalloc: cc1 produced no greg dump:\n" + r.stderr[:2000])
        return open(greg).read()


REG = re.compile(r"\(reg(?:/\w+)?:\w+ (\d+) (\w+)\)")          # (reg/v:SI 17 s1)
MOVE = re.compile(r"\(set (\(reg(?:/\w+)?:\w+ \d+ \w+\)) "     # dst reg
                  r"(\(reg(?:/\w+)?:\w+ \d+ \w+\))\)")          # src reg (a copy)
SETOP = re.compile(r"\(set \(reg(?:/\w+)?:\w+ \d+ (\w+)\)\s*\((\w+):")  # dst name, op
DEAD = re.compile(r"REG_DEAD \(reg(?:/\w+)?:\w+ \d+ (\w+)\)")


def split_functions(dump):
    """{func_name: body_text} for each ';; Function NAME' section."""
    out, cur, name = {}, [], None
    for line in dump.splitlines():
        m = re.match(r";; Function (\S+)", line)
        if m:
            if name is not None:
                out[name] = "\n".join(cur)
            name, cur = m.group(1), []
        else:
            cur.append(line)
    if name is not None:
        out[name] = "\n".join(cur)
    return out


def insns(body):
    """Yield (kind, num, text) for each top-level RTL object (col-0 '(')."""
    obj, kind, num = [], None, None
    for line in body.splitlines():
        m = re.match(r"\((\w+) (\d+)", line)
        if m:
            if kind is not None:
                yield kind, num, "\n".join(obj)
            kind, num, obj = m.group(1), int(m.group(2)), [line]
        elif kind is not None:
            obj.append(line)
    if kind is not None:
        yield kind, num, "\n".join(obj)


def analyse(name, body):
    disp = {}
    for m in re.finditer(r"(\d+) in (\d+)", body.split(";; Hard regs")[0]):
        disp[int(m.group(1))] = int(m.group(2))
    um = re.search(r";; Hard regs used:\s*(.*)", body)
    used_str = " ".join(rn(int(x)) for x in um.group(1).split()) if um else ""
    moves, calls, setops = [], [], {}
    reg_first = {}
    for kind, num, text in insns(body):
        flat = " ".join(text.split())
        if kind == "call_insn":
            calls.append(num)
        for mm in MOVE.finditer(flat):
            dst = REG.search(mm.group(1)).group(2)
            src = REG.search(mm.group(2)).group(2)
            if dst != src:
                moves.append((num, dst, src))
        so = SETOP.search(flat)
        if so:
            setops.setdefault(so.group(1), []).append((num, so.group(2)))
        for rm in REG.finditer(flat):
            nm = rm.group(2)
            reg_first.setdefault(nm, num)
    return dict(disp=disp, used=used_str,
                moves=moves, calls=calls, setops=setops, reg_first=reg_first)


def report(name, a, show_rtl, body):
    hardnames = sorted({n for _, d, s in a["moves"] for n in (d, s)}
                       | set(a["setops"]) | set(a["reg_first"]),
                       key=lambda x: (x not in CALLEE, x))
    callee = [r for r in hardnames if r in CALLEE]
    print(f"function {name}:")
    print(f"  hard regs used: {a['used']}")
    print(f"  calls at insns: {', '.join(map(str, a['calls'])) or '(none)'}")
    if callee:
        print(f"  values LIVE ACROSS A CALL (callee-saved — the pressure):")
        for r in callee:
            ops = a["setops"].get(r, [])
            life = "; ".join(f"i{n}={op}" for n, op in ops) or "(copy target)"
            print(f"    ${r}: {life}")
    print(f"  register copy-chains (moves — break these to re-color a value):")
    if a["moves"]:
        for num, dst, src in a["moves"]:
            tag = ""
            if dst in CALLEE or src in CALLEE:
                tag = "   <- crosses caller/callee boundary"
            print(f"    i{num:<4} {src:>3} -> {dst:<3}{tag}")
    else:
        print("    (none)")
    print(f"  pseudo dispositions: " +
          "  ".join(f"p{p}->{rn(h)}" for p, h in sorted(a["disp"].items())))
    print("  reading: a value in $s0-$s7 is there ONLY because it is live across a"
          "\n  call; to move it, shorten its live range past the call or break the"
          "\n  copy-chain above that ties it to that register.")
    if show_rtl:
        print("\n--- greg RTL ---")
        print(body.strip())


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("target")
    ap.add_argument("--raw", action="store_true",
                    help="target is an already-preprocessed .c (skip cpp)")
    ap.add_argument("--func", help="only this function (if the TU has several)")
    ap.add_argument("--rtl", action="store_true", help="also print the greg RTL")
    args = ap.parse_args()

    if args.raw:
        pp = open(args.target).read()
        default_func = None
    else:
        pp = preprocess(args.target)
        default_func = args.target
    dump = run_greg(pp)
    funcs = split_functions(dump)
    if not funcs:
        sys.exit("regalloc: no functions in the greg dump (did it compile?)")

    want = args.func or default_func
    chosen = [f for f in funcs if want is None or f == want] or list(funcs)
    for i, f in enumerate(chosen):
        if i:
            print()
        report(f, analyse(f, funcs[f]), args.rtl, funcs[f])


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Diagnose cc1's register-allocation decisions — the tool for sub-C reg ties.

When a draft is arithmetically correct and the RIGHT LENGTH but a few bytes off
in pure register choice (Sound: OR result in $a1 vs $v0; InsertConflict: id in
$v1 vs $s0), the residual is cc1's global allocator coloring a value differently
than the original. Blindly re-rolling the permuter or restructuring is slow.

This runs `cc1 -dl -dg` (liveness plus post-global-allocation RTL) on the
function as the build sees it and surfaces WHY each value landed where:
  * the pseudo -> hard-register dispositions,
  * which values are live across a call (cc1 only spends a callee-saved $s0-$s7
    on a value that survives a call — so the register CLASS reveals the pressure),
  * the register copy-chains (`reg <- reg` moves) that bias the coloring — the
    thing you actually break to move a value to a different register.

It turns "guess a source form and re-score" into "the dump says $s0 holds `id`
because of the move-chain at insns N/M — break that chain."

It also reports each REG_EQUIV/REG_EQUAL note's OWNER and whether the note is
LIVE, because both facts are easy to get wrong and a round was lost to exactly
that: the note belongs to SET_DEST, not to whatever pseudo its VALUE mentions,
and a note whose owner got a hard register is INERT (every reload path keyed on
`reg_equiv_memory_loc[N]` is gated on `reg_renumber[N] < 0`). Only LIVE notes
print by default — AdtSelect has 36 notes and 5 of them can matter.

  tools/regalloc.py <Name>              # diagnose src/main.exe/<Name>.c as built
  tools/regalloc.py <Name> --rtl        # + the raw greg RTL for the function
  tools/regalloc.py <Name> --compare 80 81 --enclosed-refs 3
                                        # exact weighted-ref / fence-depth lever
  tools/regalloc.py <Name> --between 80 81 82 --enclosed-refs 3
                                        # keep p80 above p81 but below p82
  tools/regalloc.py <Name> --prefer a0   # allocnos carrying an $a0 preference
  tools/regalloc.py <Name> --notes      # ALL REG_EQUIV/REG_EQUAL notes (default:
                                        # only the LIVE ones — see below)
  tools/regalloc.py <Name> --func NAME  # a specific function if the TU has several
  tools/regalloc.py <file> --raw        # run on an already-preprocessed .c

For a NON_MATCHING partial it automatically diagnoses the DRAFT (-DNON_MATCHING).
Run inside the nix devShell.
"""
import argparse, os, re, subprocess, sys, tempfile

import rtldump

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
REGNUM = {name: number for number, name in REGNAME.items()}


def rn(num):
    return REGNAME.get(num, f"r{num}")


def parse_hard_register(value):
    value = value.lower().lstrip("$")
    if value in REGNUM:
        return REGNUM[value]
    try:
        number = int(value, 0)
    except ValueError:
        raise ValueError(f"unknown hard register: {value}")
    if number not in REGNAME:
        raise ValueError(f"unknown hard register number: {number}")
    return number


def preferred_allocnos(analysis, hard_register):
    """Allocnos that inherit a hard-register preference, with donor evidence."""
    visible = analysis["allocnos"] or set(analysis["disp"])
    rows = []
    for pseudo in sorted(visible):
        if hard_register not in analysis["preferences"].get(pseudo, []):
            continue
        usage = analysis["usage"].get(pseudo, {})
        rows.append({
            "pseudo": pseudo,
            "assigned": rn(analysis["disp"].get(pseudo, -1)),
            "refs": usage.get("refs"),
            "live_length": usage.get("live_length"),
            "calls": usage.get("calls"),
            "priority": usage.get("priority"),
        })
    return rows


def conflicts_with_hard_register(analysis, pseudo, hard_register):
    """Whether global allocation forbids ``pseudo`` from ``hard_register``.

    A hard-register conflict is categorically different from a low allocation
    priority: adding loop-depth weight can change which allocno is considered
    first, but it can never make an allocno legal in a register in its conflict
    set.  Callers can use this as an early-stop oracle before trying fences.
    """
    return hard_register in analysis["conflicts"].get(pseudo, [])


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


# `(insn 48 47 49 (set (reg:SI 93) (mem:SI (reg:SI 81))) -1 (nil)` — the SET_DEST
# pseudo, which update_equiv_regs uses as the note's owner
# (`regno = REGNO (SET_DEST (set))`).
SET_DEST_PSEUDO = re.compile(r"\(set \(reg(?:/\w+)?:\w+ (\d+)\)")
NOTE = re.compile(r"(REG_EQUIV|REG_EQUAL)\s+(.*)")


def balanced(text):
    """The first fully-parenthesised expression in `text`.

    The note's value runs to the end of the flattened insn otherwise, dragging
    the enclosing `(nil))))` tail in with it and burying the fact being read.
    """
    depth = 0
    for i, ch in enumerate(text):
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
            if depth == 0:
                return text[:i + 1]
            if depth < 0:
                return text[:i]
    return text


def analyse_notes(lreg_body, disp):
    """Map each REG_EQUIV/REG_EQUAL note to its OWNING pseudo and its home.

    Exists because a lane lost a round to the two traps this reports, and I set
    that trap myself in the brief that sent it there:

      1. The note belongs to SET_DEST, NOT to whatever pseudo its VALUE mentions.
         `REG_EQUIV (mem (mem (plus sp 32972)))` does not mean the pseudo spilled
         at sp+32972 owns it.
      2. A note whose owner GOT a hard register is INERT: everything in reload1.c
         keyed on reg_equiv_memory_loc[N] is gated on reg_renumber[N] < 0.

    Reading those two facts off a dump is a five-minute round. Hand-tracing
    .lreg -> .greg to recover them is a whole one. See docs/matching-cookbook.md,
    "REG_EQUIV: correct mechanics, and the two traps that make it useless anyway".
    """
    found = []
    for kind, num, text in insns(lreg_body):
        if "REG_EQUIV" not in text and "REG_EQUAL" not in text:
            continue
        flat = " ".join(text.split())
        owner = SET_DEST_PSEUDO.search(flat)
        note = NOTE.search(flat)
        if not owner or not note:
            continue
        pseudo = int(owner.group(1))
        found.append(dict(insn=num, pseudo=pseudo, kind=note.group(1),
                          value=balanced(note.group(2))[:100], home=disp.get(pseudo)))
    return found


REG = re.compile(r"\(reg(?:/\w+)?:\w+ (\d+) (\w+)\)")          # (reg/v:SI 17 s1)
MOVE = re.compile(r"\(set (\(reg(?:/\w+)?:\w+ \d+ \w+\)) "     # dst reg
                  r"(\(reg(?:/\w+)?:\w+ \d+ \w+\))\)")          # src reg (a copy)
SETOP = re.compile(r"\(set \(reg(?:/\w+)?:\w+ \d+ (\w+)\)\s*\((\w+):")  # dst name, op
DEAD = re.compile(r"REG_DEAD \(reg(?:/\w+)?:\w+ \d+ (\w+)\)")
USAGE = re.compile(
    r"^Register (\d+) used (\d+) times across (\d+) insns([^\n]*)$",
    re.M,
)


def alloc_priority(refs, live_length):
    """gcc-2.8.1 global-alloc priority, omitting common mode-size factor."""
    if refs <= 0 or live_length <= 0:
        return 0
    return ((refs.bit_length() - 1) * refs * 10000) // live_length


def extra_refs_to_outrank(first, second, max_extra=10000):
    """Exact extra weighted refs needed for `first` to outrank `second`.

    flow.c adds one unit of weight per enclosed reference for each loop depth.
    The lreg total may already include real loops, so guessing a wrapper count
    directly from that total is unsound; report the exact required delta first.
    """
    for extra in range(max_extra + 1):
        if alloc_priority(first["refs"] + extra, first["live_length"]) > \
                second["priority"]:
            return extra
    return None


def wrapper_depths(extra_refs, enclosed_refs):
    """Loop depths needed when one wrapper weights `enclosed_refs` uses."""
    if extra_refs is None or enclosed_refs <= 0:
        return None
    return (extra_refs + enclosed_refs - 1) // enclosed_refs


def extra_refs_for_window(subject, lower, upper, max_extra=10000):
    """Inclusive extra-ref interval placing subject strictly between rivals.

    ``global.c`` priority is monotone but discontinuous at powers of two. A
    useful allocation can therefore require a narrow window rather than merely
    outranking one peer. Return the first/last weighted-ref delta in that
    window, or ``None`` when the floor-log2 jump skips it entirely.
    """
    low = lower["priority"]
    high = upper["priority"]
    if low >= high:
        return None
    first = last = None
    for extra in range(max_extra + 1):
        priority = alloc_priority(subject["refs"] + extra,
                                  subject["live_length"])
        if low < priority < high:
            if first is None:
                first = extra
            last = extra
        elif first is not None and priority >= high:
            break
    return None if first is None else (first, last)


def wrapper_depth_window(extra_window, enclosed_refs):
    """Wrapper-depth interval whose weighted refs stay in ``extra_window``."""
    if extra_window is None or enclosed_refs <= 0:
        return None
    low, high = extra_window
    first = (low + enclosed_refs - 1) // enclosed_refs
    last = high // enclosed_refs
    return (first, last) if first <= last else None


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


def analyse(name, body, usage_body=None):
    disp = {}
    for m in re.finditer(r"(\d+) in (\d+)", body.split(";; Hard regs")[0]):
        disp[int(m.group(1))] = int(m.group(2))
    preferences = {}
    conflicts = {}
    for line in body.splitlines():
        m = re.match(r";; (\d+) preferences:\s*(.*)", line)
        if m:
            preferences[int(m.group(1))] = [int(x) for x in m.group(2).split()]
        m = re.match(r";; (\d+) conflicts:\s*(.*)", line)
        if m:
            conflicts[int(m.group(1))] = [int(x) for x in m.group(2).split()]
    um = re.search(r";; Hard regs used:\s*(.*)", body)
    used_str = " ".join(rn(int(x)) for x in um.group(1).split()) if um else ""
    alloc = re.search(r";; \d+ regs to allocate:\s*([0-9 ]+)", body)
    allocnos = {int(x) for x in alloc.group(1).split()} if alloc else set()
    usage = {}
    for pseudo, refs, live_length, tail in USAGE.findall(usage_body or body):
        refs_i, length_i = int(refs), int(live_length)
        calls = re.search(r"crosses (\d+) calls", tail)
        usage[int(pseudo)] = dict(
            refs=refs_i,
            live_length=length_i,
            calls=int(calls.group(1)) if calls else 0,
            priority=alloc_priority(refs_i, length_i),
        )
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
    # Notes are only legible BEFORE allocation: .greg has already substituted
    # hard registers, so the owning pseudo's number is gone. Use the lreg body
    # when we have one, and resolve each owner's home from .greg's dispositions.
    notes = analyse_notes(usage_body, disp) if usage_body else []
    return dict(disp=disp, preferences=preferences, conflicts=conflicts, used=used_str,
                allocnos=allocnos, usage=usage, moves=moves, calls=calls, setops=setops,
                reg_first=reg_first, notes=notes)


def report(name, a, show_rtl, body, compare=None, between=None,
           enclosed_refs=None, prefer=None, show_notes=False):
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
    notes = a.get("notes") or []
    if notes:
        live = [n for n in notes if n["home"] is None]
        inert = [n for n in notes if n["home"] is not None]
        # Only LIVE notes can matter, and they are rare (AdtSelect: 5 of 36).
        # Printing all of them buries the signal, so summarise the inert ones.
        shown = notes if show_notes else live
        print(f"  REG_EQUIV/REG_EQUAL notes: {len(notes)} "
              f"({len(live)} LIVE, {len(inert)} inert)")
        for n in shown:
            if n["home"] is None:
                state = "LIVE  — owner SPILLED, so reload paths keyed on it apply"
            else:
                state = f"inert — owner got ${rn(n['home'])} (reg_renumber >= 0)"
            print(f"    i{n['insn']:<4} {n['kind']} owned by pseudo {n['pseudo']}"
                  f"  {state}")
            print(f"          value: {n['value']}")
        if not show_notes and inert:
            print(f"    ({len(inert)} inert note(s) hidden — --notes for the full table.")
            print("     An owner with a hard register makes its note INERT: every")
            print("     reload path keyed on reg_equiv_memory_loc[N] is gated on")
            print("     reg_renumber[N] < 0.)")
        print("    NB a note belongs to SET_DEST, NOT to any pseudo its VALUE")
        print("    mentions. Both traps cost a round once; see the cookbook's")
        print('    "REG_EQUIV: correct mechanics, and the two traps".')
    dispositions = []
    visible = a["allocnos"] or set(a["disp"])
    for p, h in sorted(a["disp"].items()):
        if p not in visible:
            continue
        pref = a["preferences"].get(p, [])
        suffix = (" pref=" + "/".join(rn(x) for x in pref)) if pref else ""
        dispositions.append(f"p{p}->{rn(h)}{suffix}")
    print(f"  pseudo dispositions: " + "  ".join(dispositions))
    allocated_usage = [(p, a["usage"][p]) for p in sorted(visible)
                       if p in a["usage"]]
    if allocated_usage:
        print("  global-allocation priorities (refs / live insns -> score):")
        for pseudo, item in sorted(
                allocated_usage, key=lambda x: (-x[1]["priority"], x[0])):
            print(f"    p{pseudo}: {item['refs']} / {item['live_length']} -> "
                  f"{item['priority']}  (crosses {item['calls']} calls)")
    if prefer is not None:
        rows = preferred_allocnos(a, prefer)
        print(f"  allocnos preferring ${rn(prefer)} (candidate preference donors):")
        if not rows:
            print("    (none)")
        for row in rows:
            usage = "usage unavailable"
            if row["refs"] is not None:
                usage = (f"{row['refs']} refs / {row['live_length']} insns / "
                         f"{row['calls']} calls -> priority {row['priority']}")
            print(f"    p{row['pseudo']} -> ${row['assigned']}: {usage}")
    if compare:
        first, second = compare
        if first not in a["usage"] or second not in a["usage"]:
            print(f"  priority comparison unavailable: need p{first} and p{second} "
                  "in the lreg usage table")
        else:
            extra = extra_refs_to_outrank(a["usage"][first], a["usage"][second])
            if extra is None:
                result = "more than 10000 extra weighted refs"
            elif extra == 0:
                result = "already higher priority"
            else:
                result = f"needs +{extra} weighted ref(s)"
                if enclosed_refs:
                    depths = wrapper_depths(extra, enclosed_refs)
                    result += (f" = {depths} wrapper depth(s) if each encloses "
                               f"{enclosed_refs} ref(s)")
            print(f"  p{first} > p{second}: {result}")
            print("    (priority comparison assumes equal register mode/size)")
    if between:
        subject, lower, upper = between
        missing = [pseudo for pseudo in between if pseudo not in a["usage"]]
        if missing:
            values = ", ".join(f"p{pseudo}" for pseudo in missing)
            print(f"  priority window unavailable: {values} missing from the "
                  "lreg usage table")
        else:
            window = extra_refs_for_window(
                a["usage"][subject], a["usage"][lower], a["usage"][upper]
            )
            low_priority = a["usage"][lower]["priority"]
            high_priority = a["usage"][upper]["priority"]
            if window is None:
                result = ("no weighted-ref delta <=10000 lands strictly in "
                          f"({low_priority}, {high_priority})")
            else:
                lo, hi = window
                result = f"needs +{lo}" if lo == hi else f"needs +{lo}..+{hi}"
                result += " weighted ref(s)"
                if enclosed_refs:
                    depths = wrapper_depth_window(window, enclosed_refs)
                    if depths is None:
                        result += (f"; no whole wrapper depth enclosing "
                                   f"{enclosed_refs} ref(s) lands in-window")
                    else:
                        dlo, dhi = depths
                        depth_text = str(dlo) if dlo == dhi else f"{dlo}..{dhi}"
                        result += (f" = wrapper depth {depth_text} when each "
                                   f"encloses {enclosed_refs} ref(s)")
            print(f"  p{lower} < p{subject} < p{upper}: {result}")
            print("    (priority window assumes equal register mode/size)")
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
    ap.add_argument("--notes", action="store_true",
                    help="show ALL REG_EQUIV/REG_EQUAL notes, not just the LIVE ones")
    ap.add_argument("--compare", nargs=2, type=int, metavar=("FIRST", "SECOND"),
                    help="weighted refs needed for FIRST to outrank SECOND")
    ap.add_argument("--between", nargs=3, type=int,
                    metavar=("SUBJECT", "LOWER", "UPPER"),
                    help="weighted refs keeping SUBJECT above LOWER and below UPPER")
    ap.add_argument("--enclosed-refs", type=int, metavar="N",
                    help="with --compare/--between, refs one wrapper encloses")
    ap.add_argument("--prefer", metavar="HARD",
                    help="focus allocnos carrying a hard-register preference (a0, v1, ...)")
    args = ap.parse_args()
    if args.enclosed_refs is not None and args.enclosed_refs <= 0:
        ap.error("--enclosed-refs must be positive")
    if args.enclosed_refs is not None and not (args.compare or args.between):
        ap.error("--enclosed-refs requires --compare or --between")
    try:
        prefer = parse_hard_register(args.prefer) if args.prefer else None
    except ValueError as error:
        ap.error(str(error))

    if args.raw:
        pp = open(args.target).read()
        default_func = None
    else:
        path = os.path.join(SRC, args.target + ".c")
        if not os.path.exists(path):
            sys.exit(f"regalloc: {path} not found")
        draft = "ifndef NON_MATCHING" in open(path).read()
        try:
            result = rtldump.compile_rtl(
                args.target, ["lreg", "greg"], draft=draft
            )
        except (FileNotFoundError, ValueError, RuntimeError) as e:
            sys.exit(f"regalloc: {e}")
        greg = next((p for p in result["dumps"] if p.endswith(".greg")), None)
        lreg = next((p for p in result["dumps"] if p.endswith(".lreg")), None)
        if greg is None:
            sys.exit("regalloc: cc1 produced no greg dump")
        dump = open(greg).read()
        usage_dump = open(lreg).read() if lreg is not None else ""
        default_func = args.target
    if args.raw:
        dump = run_greg(pp)
        usage_dump = ""
    funcs = split_functions(dump)
    usage_funcs = split_functions(usage_dump)
    if not funcs:
        sys.exit("regalloc: no functions in the greg dump (did it compile?)")

    want = args.func or default_func
    chosen = [f for f in funcs if want is None or f == want] or list(funcs)
    for i, f in enumerate(chosen):
        if i:
            print()
        report(
            f,
            analyse(f, funcs[f], usage_funcs.get(f)),
            args.rtl,
            funcs[f],
            args.compare,
            args.between,
            args.enclosed_refs,
            prefer,
            args.notes,
        )


if __name__ == "__main__":
    main()

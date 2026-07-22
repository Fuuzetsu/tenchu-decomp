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
  tools/regalloc.py <Name> --order      # cc1's OWN allocation order + our model +
                                        # a self-validation verdict (refuses on
                                        # divergence). Asked for by three lanes.
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
CPP = ("mipsel-unknown-linux-gnu-cpp -Iinclude -undef -Wall -lang-c "
       "-gstabs -Dmips -D__GNUC__=2 -D__OPTIMIZE__ -D__mips__ -D__mips -Dpsx "
       "-D__psx__ -D__psx -D_PSYQ -D__EXTENSIONS__ -D_MIPSEL -D_LANGUAGE_C "
       "-DLANGUAGE_C -DHACKS -I src/main.exe").split()
CC_DEFAULT = "cc1-281"
# `-fno-builtin` is a cc1 flag, NOT cpp. It sat in CPP (doing nothing) here just as it
# did in permute.py before today's fix -- so `--raw` on a builtin-calling function
# (abs/memset/...) computed allocation for a DIFFERENT program than the build. SetupTelop
# calls memset and surfaced it. Same one-line move; the default path already delegates to
# rtldump.compile_rtl, which had it right, so only --raw was affected.
CC_FLAGS = ("-mcpu=3000 -quiet -fno-builtin -G8 -w -O2 -funsigned-char -fpeephole "
            "-ffunction-cse -fpcc-struct-return -fcommon -fverbose-asm -fgnu-linker "
            "-mgas -msoft-float").split()

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


def inseparable_pairs(analysis):
    """Allocno pairs that share a hard register and do NOT conflict.

    gcc-2.8.1 has **no coalescing pass**: two allocnos that do not conflict
    simply land on the same hard register, and any copy between them vanishes as
    `move a0,a0`. So an alias copy survives ONLY if the alias conflicts with its
    source — i.e. the source is still live after the copy.

    That makes this an early-stop ORACLE, and a decisive one: if the source is
    absent from the alias's conflict list, **no priority or preference lever can
    ever separate them**, and the register tie is unreachable from C. SetupSpline
    parked at 12 bytes on exactly this fact (`85 in 4`, `86 in 4`, 85 absent from
    86's conflicts, measured 224 vs 240) — and the lane had to read `;; N
    conflicts:` by hand to find it, because this tool printed priorities and
    dispositions but never the conflict sets.
    """
    visible = analysis["allocnos"] or set(analysis["disp"])
    by_reg = {}
    for pseudo, hard in analysis["disp"].items():
        if pseudo in visible:
            by_reg.setdefault(hard, []).append(pseudo)
    pairs = []
    for hard, group in sorted(by_reg.items()):
        for i, a in enumerate(sorted(group)):
            for b in sorted(group)[i + 1:]:
                if (b not in analysis["conflicts"].get(a, [])
                        and a not in analysis["conflicts"].get(b, [])):
                    pairs.append((a, b, hard))
    return pairs



def allocation_order(a):
    """cc1's OWN allocation order, plus our model's, plus a self-validation verdict.

    `;; N regs to allocate: 134 313 129 ...` is printed AFTER global.c's
    `allocno_compare` qsort, so **it IS the allocation order** — and this tool used to
    discard it into a `set()`. A lane hand-reproduced all 25 of SetupTelop's allocnos
    to derive an order cc1 prints on one line, and called that the single
    highest-value tooling gap it hit.

    We ALSO compute the order from the documented priority
    (`floor_log2(n_refs) * n_refs / live_length * 10000 * size`) and compare. That
    comparison is the point: a model that disagrees with cc1's own output is WRONG,
    and saying so beats emitting a confident number. Validated 12/12 functions,
    267 allocnos, zero divergence — which also settles that `.lreg`'s ref count is
    ALREADY loop-depth-weighted (each mention adds its depth, not 1).
    """
    order = a["alloc_order"]
    mine = sorted(order, key=lambda p: -a["usage"].get(p, {}).get("priority", 0))
    agree = mine == order
    first = None
    if not agree:
        first = next((i for i, (x, y) in enumerate(zip(mine, order)) if x != y), None)
    return order, mine, agree, first


def needed_refs(a, pseudo, target_priority):
    """Smallest n_refs that lifts `pseudo` above `target_priority`, or None."""
    u = a["usage"].get(pseudo)
    if not u or not u["live_length"]:
        return None
    for refs in range(u["refs"] + 1, u["refs"] + 400):
        if alloc_priority(refs, u["live_length"]) > target_priority:
            return refs
    return None


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


def run_greg(pp_text, name=None):
    """Build-profile cc1 -dg on preprocessed text; returns the .greg dump."""
    cc = rtldump.cc_executable_for(name) if name else CC_DEFAULT
    flags = rtldump.cc_flags_for(name) if name else CC_FLAGS
    with tempfile.TemporaryDirectory() as d:
        src = os.path.join(d, "f.c")
        open(src, "w").write(pp_text)
        r = subprocess.run([cc] + flags + ["-dg", "f.c", "-o", "f.s"],
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
    alloc_order = [int(x) for x in alloc.group(1).split()] if alloc else []
    allocnos = set(alloc_order)
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
                allocnos=allocnos, alloc_order=alloc_order, usage=usage, moves=moves,
                calls=calls, setops=setops, reg_first=reg_first, notes=notes)


def report(name, a, show_rtl, body, compare=None, between=None,
           enclosed_refs=None, prefer=None, show_notes=False, show_order=False):
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
    # The early-stop oracle: gcc-2.8.1 has no coalescing pass, so allocnos that
    # share a register without conflicting can NEVER be separated by any priority
    # or preference lever, and a copy between them vanishes as `move a0,a0`.
    pairs = inseparable_pairs(a)
    if pairs:
        print("  INSEPARABLE (share a register, do NOT conflict — no lever can "
              "split them):")
        for x, y, hard in pairs:
            print(f"    p{x} & p{y} both in ${rn(hard)}  (p{y} absent from p{x}'s "
                  f"conflicts)")
        print("    gcc-2.8.1 has NO coalescing pass: non-conflicting allocnos "
              "simply land on")
        print("    one hard register and any copy between them disappears. An "
              "alias copy")
        print("    survives ONLY if the alias CONFLICTS with its source — i.e. "
              "the source is")
        print("    still live AFTER the copy. If it is not, the register tie is "
              "unreachable")
        print("    from C; do not spend a round on priority. See "
              "docs/matching-cookbook.md,")
        print('    "An alias copy survives only if the alias CONFLICTS with its '
              'source".')

    if show_order:
        order, mine, agree, first = allocation_order(a)
        if not order:
            print("  --order: no `;; N regs to allocate:` line — nothing global to "
                  "allocate here (everything is local_alloc's).")
        else:
            print(f"\n  ALLOCATION ORDER ({len(order)} allocnos) — cc1 allocates in "
                  "THIS order, taking the")
            print("  lowest free register each time (MIPS defines no REG_ALLOC_ORDER, "
                  "so find_reg walks")
            print("  hard regs numerically).")
            if not agree:
                # Refuse rather than emit a number the dump contradicts.
                print()
                print("  *** SELF-VALIDATION FAILED — OUTPUT REFUSED ***")
                print(f"  Our priority model and cc1's own printed order first differ "
                      f"at index {first}:")
                print(f"    ours: p{mine[first]}   cc1: p{order[first]}")
                print("  cc1's line is ground truth, so the MODEL is wrong here. Do "
                      "not reason from the")
                print("  priorities below; report this divergence — it is a real "
                      "finding about the model.")
            else:
                print("  (model self-validates against cc1's own line: our computed "
                      "order == cc1's)")
                print()
                print(f"  {'#':>3} {'pseudo':>7} {'reg':>5} {'refs':>5} {'live':>5} "
                      f"{'priority':>9}  conflicts")
                for i, p in enumerate(order):
                    u = a["usage"].get(p, {})
                    home = a["disp"].get(p)
                    cf = a["conflicts"].get(p, [])
                    hard = " ".join(rn(x) for x in cf if x < 32)
                    print(f"  {i:>3} {('p' + str(p)):>7} "
                          f"{(rn(home) if home is not None else '—'):>5} "
                          f"{u.get('refs', '?'):>5} {u.get('live_length', '?'):>5} "
                          f"{u.get('priority', '?'):>9}  {hard[:34]}")
                print()
                print("  priority = floor_log2(n_refs) * n_refs / live_length * 10000 "
                      "* size, and n_refs is")
                print("  LOOP-DEPTH-WEIGHTED (each RTL mention adds its DEPTH, not 1) "
                      "— `.lreg`'s count is")
                print("  already weighted. floor_log2 is a STEP, so ONE ref can swing "
                      "a score (16 refs ->")
                print("  21333 vs 15 -> 15000). To move a pseudo EARLIER, raise "
                      "refs/live; to move it LATER,")
                print("  split it per site — see the cookbook's demote-the-winner "
                      "rule.")

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

    # Which pseudos global_alloc never saw. A lane spent hours applying global
    # levers (priority weighting, per-site splitting) to a value local_alloc had
    # already claimed -- they cannot move it, because it is not in the global
    # allocno list at all. Saying so is the whole point: `;; N regs to allocate:`
    # IS the global list, so anything holding a register but absent from it was
    # coloured by local_alloc, which orders quantities shortest-lived-first.
    if a["allocnos"]:
        local = sorted(set(a["disp"]) - a["allocnos"])
        if local:
            # Tag each entry. The dispositions line above prints the SAME
            # `pNNN->reg` shape, so a lane that grepped the whole FILE for
            # `p398->` got a hit from the dispositions map and concluded p398 was
            # local-only when it was not -- which would have inverted its lever
            # choice. `LOCAL:pNNN` is unambiguous to grep.
            print(f"  LOCAL-ONLY (local_alloc coloured these — global_alloc never "
                  f"saw them): " + "  ".join(f"LOCAL:p{p}->{rn(a['disp'][p])}"
                                             for p in local))
            print("    (grep `LOCAL:pNNN` — the dispositions line above uses the "
                  "same pNNN->reg")
            print("    shape, so grepping the whole file conflates the two.)")
            print("    Weighting/splitting levers DO NOT APPLY to these. A "
                  "same-length register")
            print("    residual among them is a quantity-ORDER tie: local_alloc "
                  "colours")
            print("    shortest-lived-first and the first takes $v0. Seed the "
                  "loser with a")
            print("    redundant `x = src;` before its first operation to birth it "
                  "earlier;")
            print("    the copy coalesces away. See docs/matching-cookbook.md, "
                  "\"A same-length")
            print("    register residual inside ONE basic block\".")
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


# ===================================================================
#  --local: the LOCAL-alloc quantity walk (block_alloc, local-alloc.c)
# ===================================================================
#
# `--order` above models GLOBAL allocnos (global.c). It cannot see the
# per-basic-block quantity allocation that colours the pseudos printed as
# `;; Register N in M` in the `.lreg` dump (dump_local_alloc, local-alloc.c:2490)
# -- and those are exactly the local-alloc-tie residuals (SetupTelop's
# col/font/bits 4-clique, SetLightningI's qty chains, DrawImpact's a0/v0) that
# four matcher lanes hand-traced this session because no tool printed the walk.
#
# local-alloc prints ONLY the final pseudo->hard-reg homes, never the walk
# order. So there is no printed line to check the order against directly.
# Instead we SIMULATE block_alloc in the computed QTY_CMP_PRI order and require
# the resulting assignment to match cc1's printed homes for EVERY pseudo. If it
# diverges we ABORT the derived view and print only the raw table -- exactly the
# self-validate-or-refuse contract `--order` follows. Validated with zero
# divergence across ~100 functions (SetupTelop, SetLightningI, DrawImpact,
# PlayVoice, ControlHumanoid, DrawGore/237 homes, ...).

FIRST_PSEUDO = 76               # MIPS mips.h:1371
# GR_REGS integer allocation. find_free_reg's always-excluded set within 0..31 =
# fixed {0,1,26,27,28,29,31} + fp 30 + eliminables 'from' (arg-ptr 0, rap 75).
GR_EXCLUDED = {0, 1, 26, 27, 28, 29, 30, 31}
GR_CANDIDATES = [r for r in range(2, 26) if r not in GR_EXCLUDED]      # v0..t9
# call_used int regs (caller-saved) = 0-15,24-29,31; so a quantity that crosses
# a call may use only the callee-saved GR set {16..23} = s0..s7 (fp 30 aside).
GR_CALLEE_CANDIDATES = list(range(16, 24))                            # s0..s7

# x86 cvttsd2si returns 0x80000000 (INT_MIN) for (int) of an out-of-range/NaN
# double -- what a zero-span quantity's QTY_CMP_PRI (divide by qty_death -
# qty_birth == 0 -> +inf/NaN) evaluates to. Such a qty sorts last.
QTY_PRI_INT_MIN = -2147483648


def floor_log2(n):
    """gcc's floor_log2: index of the highest set bit (>=1)."""
    return n.bit_length() - 1 if n > 0 else -1


def qty_cmp_pri(n_refs, size_words, span):
    """QTY_CMP_PRI (local-alloc.c:1727), the local-alloc quantity priority:

        (int)((double)(floor_log2(n_refs) * n_refs * size) / (death-birth) * 10000)

    NB `size` is qty_size, in WORDS (PSEUDO_REGNO_SIZE) -- 1 for QI/HI/SI, 2 for
    DI -- NOT GET_MODE_SIZE in bytes. Same shape as global allocno_compare, so a
    same-length register residual among LOCAL pseudos is this walk's tie."""
    if span <= 0:                                   # zero-length quantity
        return QTY_PRI_INT_MIN
    return int(float(floor_log2(n_refs) * n_refs * size_words) / span * 10000)


def block_order(n, cmp):
    """Replicate block_alloc's qty_order sort (local-alloc.c:1638-1662).

    For next_qty <= 3 cc1 does NOT qsort: it runs a fixed compare-and-exchange
    sequence whose compares take LITERAL qty numbers 0/1/2 (not qty_order[i]),
    an INCOMPLETE sort that orders differently than a full sort would (this is
    why SetupTelop's block 34 gives the LOWER-priority quantity the LOWER
    register). For next_qty > 3 it qsorts with the _1 comparator (cmp, then qty
    number). `cmp(a, b)` is qty_compare / qty_sugg_compare on qty NUMBERS a, b.
    """
    order = list(range(n))
    if n == 2:
        if cmp(0, 1) > 0:
            order[0], order[1] = order[1], order[0]
    elif n == 3:
        if cmp(0, 1) > 0:
            order[0], order[1] = order[1], order[0]
        if cmp(1, 2) > 0:
            order[2], order[1] = order[1], order[2]
        if cmp(0, 1) > 0:
            order[0], order[1] = order[1], order[0]
    elif n > 3:
        import functools
        order.sort(key=functools.cmp_to_key(lambda a, b: cmp(a, b) or (a - b)))
    return order


# ---- .lreg parsing ----

_CENSUS = re.compile(
    r"^Register (\d+) used (\d+) times across (\d+) insns"
    r"(?: in block (\d+))?;([^\n]*)$", re.M)
_BLOCK = re.compile(r"^Basic block (\d+): first insn (\d+), last (\d+)\.$", re.M)
_LIVE = re.compile(r"^Registers live at start:([^\n]*)$", re.M)
_HOME = re.compile(r"^;; Register (\d+) in (\d+)\.$", re.M)
# `(insn/i 430 ...)`, `(call_insn/i ...)`, `(jump_insn/u ...)`: the header may
# carry /flags. Missing them merges an object into its predecessor and corrupts
# the per-block insn numbering (and every birth/death read off it).
_OBJ = re.compile(r"\((\w+)(?:/\w+)* (\d+)")
_CLOBBER = re.compile(r"\(clobber \(reg(?:/\w+)?:\w+ (\d+)")
# REG_DEAD/REG_UNUSED must match BOTH pseudos `(reg:SI 130)` and NAMED hard regs
# `(reg:SI 4 a0)`, so do not anchor on the `)` after the number: a missed
# hard-reg death leaves an incoming arg live all block and hides its register.
_DEAD = re.compile(r"REG_DEAD \(reg(?:/\w+)?:\w+ (\d+)")
_UNUSED = re.compile(r"REG_UNUSED \(reg(?:/\w+)?:\w+ (\d+)")
_REG_HEAD = re.compile(r"^\(reg(?:/\w+)?:\w+ (\d+)")
_SUBREG_HEAD = re.compile(r"^\(subreg:\w+ \(reg(?:/\w+)?:\w+ (\d+)\)")


def parse_local_census(body):
    """Per-pseudo {refs, live_length, block, size_words, calls, pointer}."""
    out = {}
    for m in _CENSUS.finditer(body):
        tail = m.group(5)
        nbytes = int(bm.group(1)) if (bm := re.search(r"(\d+) bytes", tail)) else 4
        # "crosses 1 call" (singular) as well as "crosses N calls": missing the
        # singular reads a single-call crosser as 0 calls and hands it a
        # caller-saved register where cc1 needs a callee-saved ($s0-$s7).
        cm = re.search(r"crosses (\d+) calls?", tail)
        out[int(m.group(1))] = dict(
            refs=int(m.group(2)), live_length=int(m.group(3)),
            block=int(m.group(4)) if m.group(4) else None,
            size_words=(nbytes + 3) // 4, nbytes=nbytes,
            calls=int(cm.group(1)) if cm else 0, pointer="pointer" in tail)
    return out


def parse_local_blocks(body):
    """{block: {num, first, last, live_start}} from the .lreg block census."""
    blocks, cur = {}, None
    for line in body.splitlines():
        if (bm := _BLOCK.match(line)):
            cur = int(bm.group(1))
            blocks[cur] = dict(num=cur, first=int(bm.group(2)),
                               last=int(bm.group(3)), live_start=set())
        elif cur is not None and (lm := _LIVE.match(line)):
            blocks[cur]["live_start"] = {int(x) for x in lm.group(1).split()}
            cur = None
    return blocks


def parse_local_homes(body):
    """{pseudo: hard reg} from `;; Register N in M.` -- the GROUND TRUTH the
    derived walk self-validates against (dump_local_alloc, local-alloc.c:2490)."""
    return {int(m.group(1)): int(m.group(2)) for m in _HOME.finditer(body)}


def local_rtl_objects(body):
    """(kind, uid, text) for each col-0 RTL object, in dump (chain == exec)
    order. Flag-suffixed headers (`insn/i`) are handled; comment/census lines
    (which never start with `(`) are ignored."""
    obj, kind, num = [], None, None
    for line in body.splitlines():
        if (m := _OBJ.match(line)):
            if kind is not None:
                yield kind, num, "\n".join(obj)
            kind, num, obj = m.group(1), int(m.group(2)), [line]
        elif kind is not None:
            obj.append(line)
    if kind is not None:
        yield kind, num, "\n".join(obj)


def _balanced_from(text, i):
    """text[i]=='('. Return (substring, index-after-close)."""
    depth = 0
    for j in range(i, len(text)):
        if text[j] == "(":
            depth += 1
        elif text[j] == ")":
            depth -= 1
            if depth == 0:
                return text[i:j + 1], j + 1
    return text[i:], len(text)


def reg_of(expr):
    """Regno if expr is `(reg N ...)` or `(subreg .. (reg N) ..)` at top, else
    None. Matches pseudos (no name) and hard regs (named) alike."""
    expr = expr.strip()
    m = _REG_HEAD.match(expr) or _SUBREG_HEAD.match(expr)
    return int(m.group(1)) if m else None


def _split_children(expr):
    """(head, [child substrings]) of a parenthesised `(HEAD c1 c2 ...)`; a `[..]`
    vector body is skipped (its sets are found by the top-level (set ...) scan)."""
    inner = expr.strip()[1:-1]
    n, i = len(inner), 0
    while i < n and not inner[i].isspace() and inner[i] not in "([":
        i += 1
    head, kids = inner[:i], []
    while i < n:
        c = inner[i]
        if c.isspace():
            i += 1
        elif c == "(":
            sub, i = _balanced_from(inner, i)
            kids.append(sub)
        elif c == "[":
            depth = 0
            while i < n:
                depth += (inner[i] == "[") - (inner[i] == "]")
                i += 1
                if depth == 0:
                    break
        else:
            j = i
            while j < n and not inner[j].isspace() and inner[j] not in "()[]":
                j += 1
            kids.append(inner[i:j])
            i = j
    return head, kids


def src_operands(src):
    """The direct reg operands of a SET_SRC, in order -- the tie candidates
    block_alloc's loop sees. A MEM is ONE opaque memory operand: the registers
    inside its address are NOT tie candidates (the load-tie trap that made a
    pointer's home merge with the value it loads)."""
    if (r := reg_of(src)) is not None:
        return [r]
    head, kids = _split_children(src)
    if head.startswith("mem"):
        return []
    return [rk for rk in (reg_of(k) for k in kids) if rk is not None]


def top_level_sets(text):
    """[(dest_expr, src_expr)] for every top-level (set DEST SRC) in the insn
    PATTERN -- a PARALLEL (divmod, call) has several. Only the FIRST (operand 0)
    is a tie/suggestion candidate; the rest merely birth their dest."""
    flat = " ".join(text.split())
    out, i = [], 0
    while (k := flat.find("(set ", i)) >= 0:
        group, _ = _balanced_from(flat, k)
        _, kids = _split_children(group)
        if len(kids) >= 2:
            out.append((kids[0], kids[1]))
        i = k + 5
    return out


def _local_pseudos_in_block(homes, census, b):
    """The homed local pseudos whose census block is b."""
    return {p for p in homes if census.get(p, {}).get("block") == b}


def local_alloc_view(name, lreg_body):
    """Simulate block_alloc for every basic block and self-validate.

    Returns a dict: homes (cc1's printed truth), result (our simulated homes),
    per_block (quantities/walk/assignment/hard-live), divergences, validated,
    diagnostics. See the module comment for the contract."""
    census = parse_local_census(lreg_body)
    blocks = parse_local_blocks(lreg_body)
    homes = parse_local_homes(lreg_body)
    objects = list(local_rtl_objects(lreg_body))
    pos = {uid: i for i, (_, uid, _) in enumerate(objects)}

    result, diagnostics, per_block = {}, [], {}

    for b in sorted(blocks):
        locals_b = _local_pseudos_in_block(homes, census, b)
        if not locals_b:
            continue
        first, last = blocks[b]["first"], blocks[b]["last"]
        if first not in pos or last not in pos:
            diagnostics.append(f"block {b}: first/last insn not in RTL body")
            continue
        # Objects of the block, in chain order, numbered like block_alloc
        # (every NON-note object -- insn/jump/call/label/barrier -- increments).
        objs, numbering, k = [], {}, 0
        for j in range(pos[first], pos[last] + 1):
            kind, uid, text = objects[j]
            if kind != "note":
                k += 1
            numbering[uid] = k
            objs.append((kind, uid, text))

        birth, death, ties = {}, {}, []
        copy_sugg, arith_sugg, hard_events = {}, {}, []

        for kind, uid, text in objs:
            if kind == "note":
                continue
            k = numbering[uid]
            flat = " ".join(text.split())
            # Deaths: REG_DEAD at 2k, REG_UNUSED (dead output) at 2k+1.
            for p in (int(x) for x in _DEAD.findall(flat)):
                if p >= FIRST_PSEUDO:
                    if p in locals_b:
                        death[p] = 2 * k
                elif p < 32:
                    hard_events.append((p, "dead", 2 * k))
            for p in (int(x) for x in _UNUSED.findall(flat)):
                if p >= FIRST_PSEUDO:
                    if p in locals_b:
                        death[p] = 2 * k + 1
                elif p < 32:
                    hard_events.append((p, "unused", 2 * k + 1))
            # Births (every set), then tie + suggestions (ONLY operand 0).
            born_before = set(birth)
            sets = top_level_sets(text)
            for dest_expr, _src in sets:
                dest = reg_of(dest_expr)
                if dest is None:
                    continue
                if dest < FIRST_PSEUDO:
                    if dest < 32:
                        hard_events.append((dest, "set", 2 * k))
                elif dest in locals_b and dest not in birth:
                    birth[dest] = 2 * k
            if sets:
                dest = reg_of(sets[0][0])                 # r0 = operand 0
                ops = src_operands(sets[0][1])
                bucket = copy_sugg if reg_of(sets[0][1]) is not None else arith_sugg
                if dest is not None and dest >= FIRST_PSEUDO and dest in locals_b:
                    # A HARD operand records a suggestion for r0 AND reg_is_born's
                    # r0 (line 1906), committing it so no later pseudo can tie; a
                    # tie also needs r0 born THIS insn (reg_qty==-2, line 1946).
                    committed = dest in born_before
                    for r1 in ops:
                        if r1 < FIRST_PSEUDO:
                            if 2 <= r1 <= 25:
                                bucket.setdefault(dest, set()).add(r1)
                            committed = True
                        elif not committed and r1 in locals_b and r1 in death \
                                and death[r1] == 2 * k:
                            ties.append((dest, r1))
                            break
                elif dest is not None and 2 <= dest <= 25:
                    # r0 a hard reg -> suggested to each pseudo operand (call-arg
                    # / return copies): combine_regs' sreg-hard branch.
                    for r1 in ops:
                        if r1 >= FIRST_PSEUDO and r1 in locals_b:
                            bucket.setdefault(r1, set()).add(dest)
            for c in (int(x) for x in _CLOBBER.findall(flat)):
                if c >= FIRST_PSEUDO:
                    if c in locals_b and c not in birth:
                        birth[c] = 2 * k - 1
                elif c < 32:
                    hard_events.append((c, "set", 2 * k - 1))

        # Quantities: union-find over ties (output joins the dying input's qty).
        parent = {p: p for p in locals_b}

        def find(x):
            while parent[x] != x:
                parent[x] = parent[parent[x]]
                x = parent[x]
            return x

        for out, src in ties:
            if out in parent and src in parent:
                parent[find(out)] = find(src)
        members_of = {}
        for p in locals_b:
            members_of.setdefault(find(p), set()).add(p)

        quantities = []
        for members in members_of.values():
            bs = [birth[m] for m in members if m in birth]
            ds = [death[m] for m in members if m in death]
            if not bs or not ds:
                diagnostics.append(
                    f"block {b}: quantity {sorted(members)} has no "
                    f"birth/death in the RTL body (model incomplete)")
            qb = min(bs) if bs else 0
            qd = max(ds) if ds else qb
            refs = sum(census[m]["refs"] for m in members)
            size = max(census[m]["size_words"] for m in members)
            calls = sum(census[m]["calls"] for m in members)
            cs, ars = set(), set()
            for m in members:
                cs |= copy_sugg.get(m, set())
                ars |= arith_sugg.get(m, set())
            quantities.append(dict(
                members=members, birth=qb, death=qd, refs=refs, size=size,
                calls=calls, copy_sugg=cs, arith_sugg=ars,
                pri=qty_cmp_pri(refs, size, qd - qb)))
        # qty number = birth (alloc) order; lower birth -> lower number.
        quantities.sort(key=lambda q: (q["birth"], min(q["members"])))
        for i, q in enumerate(quantities):
            q["qnum"] = i

        # Hard-reg live intervals. A hard reg (call arg / return value) is set
        # and killed repeatedly; track precise [set, dead] intervals -- a 'set'
        # opens if not already open, a death closes an open one, an unmatched
        # death is IGNORED (never a spurious block-long [0, X] that hides a reg).
        hard_live, cur_open = {}, {}
        for r in blocks[b]["live_start"]:
            if r < 32:
                cur_open[r] = 0
        for r, ev, idx in sorted(hard_events, key=lambda e: (e[2], e[1] == "set")):
            if ev == "set":
                cur_open.setdefault(r, idx)
            elif r in cur_open:
                hard_live.setdefault(r, []).append((cur_open.pop(r), idx))
        end_idx = 2 * (max((numbering[o[1]] for o in objs), default=0)) + 2
        for r, b0 in cur_open.items():
            hard_live.setdefault(r, []).append((b0, end_idx))

        def hard_busy(reg, lo, hi):
            return any(a < hi and lo < z for (a, z) in hard_live.get(reg, []))

        assigned, occupied = {}, {}
        byq = {q["qnum"]: q for q in quantities}
        n = len(quantities)

        def cand_list(q):
            return GR_CALLEE_CANDIDATES if q["calls"] > 0 else GR_CANDIDATES

        def is_free(reg, lo, hi):
            # A zero-length quantity (birth==death) has an EMPTY [born,dead)
            # range: find_free_reg's live-scan never runs, so it conflicts with
            # nothing and takes the lowest candidate.
            if lo >= hi:
                return True
            if hard_busy(reg, lo, hi):
                return False
            return not any(a < hi and lo < z for (a, z) in occupied.get(reg, []))

        def place(q, reg):
            assigned[q["qnum"]] = reg
            if q["birth"] < q["death"]:
                occupied.setdefault(reg, []).append((q["birth"], q["death"]))
            for m in q["members"]:
                result[m] = reg

        def cmp_pri(a, bb):                              # qty_compare
            return byq[bb]["pri"] - byq[a]["pri"]

        def sugg_val(q):                                 # QTY_CMP_SUGG
            ncs, ns = len(q["copy_sugg"]), len(q["arith_sugg"])
            return ncs if ncs else ns * FIRST_PSEUDO

        def cmp_sugg(a, bb):                             # qty_sugg_compare
            t = sugg_val(byq[a]) - sugg_val(byq[bb])
            return t if t != 0 else byq[bb]["pri"] - byq[a]["pri"]

        # Pass 1: quantities with a suggested hard reg get first crack at it, in
        # qty_sugg_compare order: copy-suggs (lowest free), then arith-suggs.
        for qn in block_order(n, cmp_sugg):
            q = byq[qn]
            if not (q["copy_sugg"] or q["arith_sugg"]):
                continue
            cand = set(cand_list(q))
            lo, hi = q["birth"], q["death"]
            pools = ([q["copy_sugg"], q["arith_sugg"]] if q["copy_sugg"]
                     else [q["arith_sugg"]])
            for pool in pools:
                if (hit := next((r for r in sorted(pool)
                                 if r in cand and is_free(r, lo, hi)), None)):
                    place(q, hit)
                    break
        # Pass 2: everything still unplaced, in qty_compare order, lowest free.
        main_order = block_order(n, cmp_pri)
        for qn in main_order:
            q = byq[qn]
            if qn in assigned:
                continue
            lo, hi = q["birth"], q["death"]
            if (hit := next((r for r in cand_list(q)
                             if is_free(r, lo, hi)), None)) is not None:
                place(q, hit)

        per_block[b] = dict(
            quantities=quantities, walk=[byq[qn] for qn in main_order],
            assigned=assigned, hard_live=hard_live, locals=locals_b)

    divergences = validate_local(result, homes)
    return dict(name=name, homes=homes, census=census, blocks=blocks,
                per_block=per_block, result=result, divergences=divergences,
                validated=not divergences and bool(homes),
                diagnostics=diagnostics)


def validate_local(result, homes):
    """[(pseudo, our_reg, cc1_reg)] where the simulation disagrees with cc1's
    printed home. Empty == every home reproduced. This is the guard: a non-empty
    return ABORTS the derived view (the walk/assignment model is wrong)."""
    bad = []
    for p in sorted(homes):
        if result.get(p) != homes[p]:
            bad.append((p, result.get(p), homes[p]))
    return bad


def conflicts_in_block(quantities):
    """{qnum: [qnums whose live range overlaps]} within a block -- the mutual
    conflicts the walk resolves (two overlapping quantities cannot share a
    register). Empty [birth,death) ranges (zero-length qtys) conflict nothing."""
    out = {}
    for a in quantities:
        la, ha = a["birth"], a["death"]
        out[a["qnum"]] = [c["qnum"] for c in quantities
                          if c["qnum"] != a["qnum"]
                          and la < c["death"] and c["birth"] < ha]
    return out


def report_local(view, dump_path):
    """Print the RAW per-pseudo table (always) then, only if the simulation
    self-validated against cc1's printed homes, the DERIVED quantities + walk."""
    name, homes = view["name"], view["homes"]
    census, per_block = view["census"], view["per_block"]
    print(f"function {name}: LOCAL-alloc quantity walk (block_alloc)")
    print(f"  dump: {dump_path}")
    if not homes:
        # A guarded source measures the DRAFT; an empty homes list is a real
        # ERROR (cc1 produced no `;; Register N in M`), never a vacuous success.
        print("  ERROR: no `;; Register N in M` homes in this .lreg dump -- "
              "nothing was locally allocated (or the dump is empty/for the wrong")
        print("  variant). This is NOT 'no quantities'; check the dump path above.")
        return
    for d in view["diagnostics"]:
        print(f"  note: {d}")

    # ---- RAW: always shown, straight from the dump (the hand-trace). ----
    print(f"\n  RAW homes ({len(homes)} local pseudos, grouped by block; birth/"
          "death are per-block insn*2 indices):")
    shown = set()
    for b in sorted(per_block):
        pb = per_block[b]
        qby_member = {m: q for q in pb["quantities"] for m in q["members"]}
        rows = sorted(pb["locals"])
        if not rows:
            continue
        print(f"    block {b}:")
        print(f"      {'pseudo':>7} {'home':>5} {'refs':>4} {'sz':>2} "
              f"{'birth':>5} {'death':>5} {'QTY_CMP_PRI':>11}  calls")
        for p in rows:
            q = qby_member.get(p, {})
            c = census.get(p, {})
            print(f"      {('p'+str(p)):>7} {rn(homes[p]):>5} "
                  f"{c.get('refs','?'):>4} {c.get('size_words','?'):>2} "
                  f"{q.get('birth','?'):>5} {q.get('death','?'):>5} "
                  f"{q.get('pri','?'):>11}  {c.get('calls',0)}")
            shown.add(p)
    orphans = sorted(set(homes) - shown)
    if orphans:
        print("    (homes whose census block was not matched to a block body: "
              + " ".join(f"p{p}->{rn(homes[p])}" for p in orphans) + ")")

    # ---- DERIVED: only if the simulated assignment self-validated. ----
    div = view["divergences"]
    print()
    if not view["validated"]:
        print("  *** SELF-VALIDATION FAILED -- DERIVED WALK REFUSED ***")
        print(f"  The simulated assignment diverges from cc1's printed homes at "
              f"{len(div)} pseudo(s):")
        for p, mine, cc1 in div[:12]:
            b = census.get(p, {}).get("block")
            print(f"    p{p} (block {b}): simulated "
                  f"{rn(mine) if mine is not None else '(unplaced)'} != cc1 "
                  f"{rn(cc1)}")
        if len(div) > 12:
            print(f"    ... and {len(div) - 12} more")
        print("  The walk order or assignment model is WRONG here, so the raw "
              "table above is the")
        print("  trustworthy view; the derived quantities/order are withheld. "
              "This divergence is")
        print("  itself a finding about the local-alloc model -- report it. "
              "(blind spot: the")
        print("  suggestion/coalescing model may be incomplete for this block.)")
        return

    print(f"  self-validated: simulated assignment reproduced all {len(homes)} "
          "of cc1's printed homes, 0 divergences.")
    print("  DERIVED walk -- cc1 colours each block's quantities in DESCENDING "
          "QTY_CMP_PRI order")
    print("  (the small-block quirk of local-alloc.c:1638 aside), taking the "
          "lowest free reg; a")
    print("  quantity that crosses a call may use only $s0-$s7. blind spot: the "
          "quantity/tie")
    print("  structure is INFERRED (local-alloc prints only final homes), but "
          "the assignment it")
    print("  implies matches cc1's homes exactly, which is the check.")
    for b in sorted(per_block):
        pb = per_block[b]
        if not pb["quantities"]:
            continue
        conf = conflicts_in_block(pb["quantities"])
        print(f"\n    block {b} -- walk order ({len(pb['quantities'])} "
              "quantities):")
        print(f"      {'#':>2} {'qty (pseudos)':>18} {'home':>5} {'refs':>4} "
              f"{'[birth,death)':>13} {'pri':>9}  suggests / conflicts")
        for q in pb["walk"]:
            reg = pb["assigned"].get(q["qnum"])
            sg = sorted(q["copy_sugg"]) + sorted(q["arith_sugg"])
            sgs = "sugg " + "/".join(rn(r) for r in sg) if sg else ""
            cf = conf.get(q["qnum"], [])
            cfs = ("conflicts q" + ",q".join(str(x) for x in cf)) if cf else ""
            tag = "  ".join(x for x in (sgs, cfs) if x)
            mem = ",".join("p" + str(m) for m in sorted(q["members"]))
            print(f"      {q['qnum']:>2} {mem:>18} "
                  f"{(rn(reg) if reg is not None else '-'):>5} {q['refs']:>4} "
                  f"{('['+str(q['birth'])+','+str(q['death'])+')'):>13} "
                  f"{q['pri']:>9}  {tag}")
    print("\n  reading: to move a LOCAL pseudo's register, change its quantity's "
          "QTY_CMP_PRI rank")
    print("  (refs / live span) so the walk reaches it earlier/later, or seed a "
          "copy so it")
    print("  coalesces into a differently-homed quantity. See "
          "docs/matching-cookbook.md.")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("target")
    ap.add_argument("--raw", action="store_true",
                    help="target is an already-preprocessed .c (skip cpp; use "
                         "--func to select a non-default object compiler)")
    ap.add_argument("--func", help="only this function (if the TU has several)")
    ap.add_argument("--rtl", action="store_true", help="also print the greg RTL")
    ap.add_argument("--order", action="store_true",
                    help="print cc1's OWN allocation order (the `;; N regs to "
                         "allocate:` line IS the post-qsort order), our computed "
                         "order, and a SELF-VALIDATION verdict — divergence means the "
                         "model is wrong and the output is refused. Asked for by three "
                         "lanes; two called it the highest-value gap they hit.")
    ap.add_argument("--local", action="store_true",
                    help="the LOCAL-alloc quantity walk (block_alloc): per-block "
                         "quantities, their QTY_CMP_PRI priorities, the descending "
                         "walk order and conflicts, plus a SELF-VALIDATION verdict — "
                         "the derived walk is shown ONLY if a simulated assignment "
                         "reproduces cc1's printed `;; Register N in M` homes, else it "
                         "is refused and only the raw table prints. This is the "
                         "local-alloc-tie view `--order` (global allocnos) cannot see.")
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
    if args.local and args.raw:
        ap.error("--local needs the .lreg dump; it is not compatible with --raw "
                 "(which produces only a greg dump)")
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
        profile_name = args.func or os.path.splitext(os.path.basename(args.target))[0]
        dump = run_greg(pp, profile_name)
        usage_dump = ""
    funcs = split_functions(dump)
    usage_funcs = split_functions(usage_dump)

    if args.local:
        # The local-alloc walk lives entirely in the .lreg dump.
        want = args.func or default_func
        chosen = [f for f in usage_funcs if want is None or f == want] \
            or list(usage_funcs)
        if not chosen:
            sys.exit("regalloc: no functions in the lreg dump (did it compile?)")
        for i, f in enumerate(chosen):
            if i:
                print()
            report_local(local_alloc_view(f, usage_funcs[f]), lreg)
        return

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
            args.order,
        )


if __name__ == "__main__":
    main()

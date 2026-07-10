#!/usr/bin/env python3
"""Recover data-symbol names by aligning data *references* inside known functions.

The function matchers (symmatch / xbuildnames / callmatch) name code.  Globals
need a different handle: PSX.SYM's data addresses belong to a lost build, and the
data segment shifted far more than .text did, so ordering alone is unreliable.

But a function we have already identified on both sides touches the same globals,
in the same order.  So: for every retail function whose name we share with the
demo's PSX.EXE, decode both bodies' data references (`lui`/`%lo` pairs and
`$gp`-relative accesses), and when the two reference sequences have the same
length, zip them.  Each zip is a vote that retail address X is the demo's symbol
Y.  A global touched by several known functions collects several independent
votes.

Accepted only when every vote for X agrees, the name is claimed by no other
address, and the name exists in PSX.SYM (provenance).

Precision is measured, not assumed: retail addresses that *already* carry a real
name form a control set -- we report how often the votes reproduce it.
"""
from __future__ import annotations

import argparse, collections, difflib, os, re, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import psxsym as P

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PLACEHOLDER = re.compile(r"^(D_|DAT_|LAB_|jtbl_|switchD_|__)")

# (text_lo, text_hi) so we can tell a data reference from a code one
RETAIL_TEXT = (0x80016134, 0x80086763)
DEMO_TEXT = (0x8001389C, 0x8008B383)
RETAIL_GP, DEMO_GP = 0x80097698, 0x80097DA8

LOADS_STORES = set(range(0x20, 0x2F)) | {0x30, 0x31, 0x32, 0x38, 0x39, 0x3A}


def sext(x: int) -> int:
    return x - 0x10000 if x & 0x8000 else x


def norm(w: int) -> int:
    """Opcode-level token: enough to align two builds of the same function."""
    op = w >> 26
    if op == 0:
        return (0, w & 0x3F, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31)
    if op in (2, 3):
        return (op,)
    return (op, (w >> 21) & 31, (w >> 16) & 31)


def data_refs(text: bytes, base: int, addr: int, size: int, gp: int,
              tlo: int, thi: int):
    """(refs, tokens) -- refs is [(instr_index, data_address)], tokens aligns builds."""
    off = addr - base
    hi: dict[int, int] = {}
    out: list[tuple[int, int]] = []
    toks: list = []
    for i in range(0, size, 4):
        w = struct.unpack_from("<I", text, off + i)[0]
        toks.append(norm(w))
        op = w >> 26
        rs, rt = (w >> 21) & 31, (w >> 16) & 31
        imm = w & 0xFFFF
        if op == 15:                                   # lui rt, imm
            hi[rt] = imm << 16
            continue
        target = None
        if op == 9 and rs in hi:                       # addiu rt, rs(hi), lo
            target = hi[rs] + sext(imm)
        elif op == 9 and rs == 28:                     # addiu rt, $gp, off
            target = gp + sext(imm)
        elif op in LOADS_STORES and rs in hi:
            target = hi[rs] + sext(imm)
        elif op in LOADS_STORES and rs == 28:
            target = gp + sext(imm)
        if target is not None and not (tlo <= target <= thi):
            out.append((i // 4, target))
        # invalidate the destination register unless it just became a pointer
        if op == 0:                                    # R-type writes rd
            rd = (w >> 11) & 31
            hi.pop(rd, None)
        elif op in (9, 0x0C, 0x0D, 0x0E, 8, 0x0A, 0x0B) or op in LOADS_STORES and op < 0x28:
            if rt != rs:
                hi.pop(rt, None)
        elif op == 3:                                  # jal clobbers caller-saved
            for r in list(hi):
                if r in range(1, 16) or r in (24, 25):
                    hi.pop(r, None)
    return out, toks


def align_refs(rr, rtok, dr, dtok):
    """Map retail refs to demo refs through an instruction-level alignment.

    The two builds of a function differ by inserted/removed instructions, so a
    positional zip only works when the reference counts happen to agree.  Aligning
    the opcode token streams first recovers the correspondence even when they do
    not -- which is most of the time.
    """
    dref = dict(dr)
    sm = difflib.SequenceMatcher(a=rtok, b=dtok, autojunk=False)
    imap = {}
    for i, j, n in sm.get_matching_blocks():
        for k in range(n):
            imap[i + k] = j + k
    out = []
    for i, x in rr:
        j = imap.get(i)
        if j is not None and j in dref:
            out.append((x, dref[j]))
    return out


def load_tsv(path, cols=3):
    for line in open(path):
        if line.startswith("#"):
            continue
        p = line.rstrip("\n").split("\t")
        if len(p) >= cols:
            yield p


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("sym", nargs="?", default=f"{REPO}/disks/demo/PSX.SYM")
    ap.add_argument("--psxexe", default=f"{REPO}/disks/demo/PSX.EXE")
    ap.add_argument("--psxexe-funcs", default=f"{REPO}/reference/demo-psxexe.functions.tsv")
    ap.add_argument("--psxexe-labels", default=f"{REPO}/reference/demo-psxexe.labels.tsv")
    ap.add_argument("--functions", default=f"{REPO}/.shake/ghidra-export/functions.tsv")
    ap.add_argument("--exe", default=f"{REPO}/disks/tenchu/main.exe")
    ap.add_argument("--symbols", default=f"{REPO}/config/symbols.main.exe.txt")
    ap.add_argument("--min-votes", type=int, default=2)
    ap.add_argument("--apply", metavar="TSV")
    ap.add_argument("--candidates", metavar="TSV",
                    help="write the single-vote (unproposed) suggestions here")
    args = ap.parse_args()

    sf = P.load(args.sym)
    known = {s.name for s in sf.syms} | {f.name for f in sf.funcs}

    # ---- demo side
    dexe = open(args.psxexe, "rb").read()
    dt_addr, dt_size = struct.unpack_from("<II", dexe, 0x18)
    dtext = dexe[0x800:0x800 + dt_size]
    dfuncs = {p[2]: (int(p[0], 16), int(p[1])) for p in load_tsv(args.psxexe_funcs)}
    dlabels = {int(p[0], 16): p[1] for p in load_tsv(args.psxexe_labels, 2)}
    for n, (a, s) in dfuncs.items():
        dlabels.setdefault(a, n)

    # ---- retail side
    rexe = open(args.exe, "rb").read()
    rtext, rbase = rexe[0x800:], 0x80011000
    rfuncs = {p[2]: (int(p[0], 16), int(p[1])) for p in load_tsv(args.functions)}
    cur: dict[int, str] = {}
    for line in open(args.symbols):
        m = re.match(r"\s*([A-Za-z_]\w*)\s*=\s*(0x[0-9A-Fa-f]+)\s*;", line)
        if m:
            cur[int(m.group(2), 16)] = m.group(1)
    taken = set(cur.values())

    shared = [n for n in rfuncs if n in dfuncs]
    print(f"functions named on both sides: {len(shared)}")

    # how many shared-name demo functions touch each demo symbol?  A proposal that
    # only reproduces a fraction of those references is weaker than its vote count
    # suggests.
    demo_refs: collections.Counter = collections.Counter()
    for n in shared:
        da, ds = dfuncs[n]
        if ds < 8 or da - dt_addr + ds > len(dtext):
            continue
        dr, _ = data_refs(dtext, dt_addr, da, ds, DEMO_GP, *DEMO_TEXT)
        for y in {y for _, y in dr}:
            nm = dlabels.get(y)
            if nm:
                demo_refs[nm] += 1

    votes: dict[int, collections.Counter] = collections.defaultdict(collections.Counter)
    zipped = skipped = 0
    for n in shared:
        ra, rs = rfuncs[n]
        da, ds = dfuncs[n]
        if rs < 8 or ds < 8 or ra - rbase + rs > len(rtext) or da - dt_addr + ds > len(dtext):
            continue
        rr, rtok = data_refs(rtext, rbase, ra, rs, RETAIL_GP, *RETAIL_TEXT)
        dr, dtok = data_refs(dtext, dt_addr, da, ds, DEMO_GP, *DEMO_TEXT)
        pairs = align_refs(rr, rtok, dr, dtok) if rr and dr else []
        if not pairs:
            skipped += 1
            continue
        zipped += 1
        # one vote per (function, address) -- a function that touches a global
        # five times is still one independent witness
        for x, y in {(x, y) for x, y in pairs}:
            nm = dlabels.get(y)
            if nm and nm in known and not PLACEHOLDER.match(nm):
                votes[x][nm] += 1

    print(f"function pairs contributing votes: {zipped}   no aligned refs: {skipped}")
    print(f"retail data addresses with >=1 vote: {len(votes)}")

    # unanimous, and the name is claimed by exactly one address
    unanimous = {x: c.most_common(1)[0] for x, c in votes.items() if len(c) == 1}
    byname = collections.Counter(nm for nm, _ in unanimous.values())
    clean = {x: (nm, v) for x, (nm, v) in unanimous.items() if byname[nm] == 1}

    # ---- CONTROL: only addresses whose CURRENT name is itself an original
    # PSX.SYM name.  A retail name absent from PSX.SYM is one of our guesses, so a
    # mismatch there is a recovery, not an error -- report those separately.
    ctrl = [(x, cur[x], nm, v) for x, (nm, v) in clean.items()
            if x in cur and cur[x] in known]
    agree = [c for c in ctrl if c[1] == c[2]]
    print(f"\ncontrol (retail name is itself a PSX.SYM name): {len(ctrl)}   "
          f"agree: {len(agree)} ({100*len(agree)/max(1,len(ctrl)):.1f}%)")
    for x, ours, theirs, v in [c for c in ctrl if c[1] != c[2]][:10]:
        print(f"    DISAGREE {x:08x} ours={ours:24s} demo={theirs} ({v} votes)")

    guessed = [(x, cur[x], nm, v) for x, (nm, v) in clean.items()
               if x in cur and cur[x] not in known and not PLACEHOLDER.match(cur[x])
               and nm not in taken and v >= args.min_votes]
    print(f"\n=== guessed repo names the demo can replace ({len(guessed)}) ===")
    for x, ours, theirs, v in sorted(guessed):
        cov = v / max(1, demo_refs.get(theirs, 1))
        print(f"  {x:08x} {ours:30s} -> {theirs:24s} {v} votes, "
              f"{cov:.0%} of demo referencers")

    # ---- proposals: placeholder or absent, name not already used
    props = {}
    for x, (nm, v) in clean.items():
        if nm in taken:
            continue
        old = cur.get(x)
        if old is not None and not PLACEHOLDER.match(old):
            continue
        if v < args.min_votes:
            continue
        props[x] = (old or f"D_{x:08X}", nm, v)

    print(f"\n=== proposals (>= {args.min_votes} votes): {len(props)} ===")
    dtype = {}
    for p in load_tsv(f"{REPO}/reference/psxsym-data.tsv", 4):
        dtype[p[3]] = (p[1], p[2])
    for x in sorted(props):
        old, nm, v = props[x]
        st, ty = dtype.get(nm, ("?", "?"))
        newname = "(new)" if x not in cur else old
        cov = v / max(1, demo_refs.get(nm, 1))
        print(f"  {x:08x} {newname:22s} -> {nm:28s} {v}v {cov:>4.0%}  {st:8s} {ty}")

    singles = {x: (cur.get(x) or f"D_{x:08X}", nm, v) for x, (nm, v) in clean.items()
               if v == 1 and nm not in taken
               and (x not in cur or PLACEHOLDER.match(cur[x]))}
    print(f"\n(single-vote candidates, not proposed: {len(singles)})")

    if args.candidates:
        with open(args.candidates, "w") as fh:
            fh.write("# Data symbols with only ONE witnessing function -- not adopted.\n"
                     "# A single reference-alignment vote can come from a misaligned\n"
                     "# instruction block; two independent functions rarely agree by\n"
                     "# accident.  Re-run tools/datamatch.py --min-votes 1 to see them\n"
                     "# in context, and confirm by reading the referencing function.\n"
                     "#addr\tcurrent\tcandidate\tstorage\ttype\n")
            for x in sorted(singles):
                old, nm, v = singles[x]
                st, ty = dtype.get(nm, ("?", "?"))
                fh.write(f"{x:08x}\t{old}\t{nm}\t{st}\t{ty}\n")
        print(f"wrote {len(singles)} single-vote candidates to {args.candidates}")

    if args.apply:
        with open(args.apply, "w") as fh:
            fh.write("#addr\tprevious_name\tadopted_name\tmatcher\tevidence\n")
            for x in sorted(props):
                old, nm, v = props[x]
                fh.write(f"{x:08x}\t{old}\t{nm}\tdatamatch\tVOTES{v}\n")
            for x, ours, nm, v in sorted(guessed):
                fh.write(f"{x:08x}\t{ours}\t{nm}\tdatamatch\tVOTES{v}\n")
        print(f"\nwrote {len(props)+len(guessed)} renames to {args.apply}")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Recover original function names for retail ``main.exe`` from the demo's PSX.SYM.

PSX.SYM (carved from the Japanese demo's AFS archive) is the Psy-Q debug output
for an *earlier, lost build*.  Its addresses are useless against retail, but two
structural facts survive:

* the linker emits each translation unit contiguously, and
* each build has a stable text-emission order within a translation unit.

That emission order is normally source order, but GCC 2.8 defers some
address-taken ``static inline`` copies until the end of the object.  Both sides
of this matcher are address-ordered, so those copies remain in their true
emission positions rather than being mistaken for source-line order.

So: tag every retail function whose name we already share with the demo, cut the
retail address-ordered list into runs that agree on TU, and inside each run align
the retail functions against that TU's demo functions with Needleman-Wunsch.
Alignment is scored on facts recomputed from the retail binary:

* ``fsize`` -- frame size from ``addiu $sp,$sp,-N``
* ``mask``  -- saved-register bitmask from the prologue's ``sw`` stores
* code size

PSX.SYM records fsize/mask for every function.  Across the anchors they agree
63% of the time (the rest are functions genuinely edited between the two
builds), so agreement is strong positive evidence; disagreement is only weakly
negative.  Confidence of a proposed name:

  HIGH  fsize and mask both agree
  MED   exactly one of fsize/mask agrees, and the code size is within 25%
  LOW   neither agrees (printed, never applied)

Nothing is mutated here.  ``--apply FILE`` writes a TSV of accepted renames for
tools/import_symbols.py.
"""
from __future__ import annotations

import argparse, bisect, collections, os, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import psxsym as P
import function_inventory as FI

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VB, FO = 0x80011000, 0x800
GAME_END = 0x80060000
PLACEHOLDER = ("FUN_", "LAB_", "DAT_", "DEAD_", "thunk")
GAP = -2


def is_placeholder(n: str) -> bool:
    return n.startswith(PLACEHOLDER)


def frame_of(exe: bytes, addr: int, size: int) -> tuple[int, int]:
    off = addr - VB + FO
    fsize = mask = 0
    for i in range(min(size // 4, 16)):
        w = struct.unpack_from("<I", exe, off + 4 * i)[0]
        op, rs, rt = w >> 26, (w >> 21) & 31, (w >> 16) & 31
        imm = w & 0xFFFF
        simm = imm - 0x10000 if imm & 0x8000 else imm
        if op == 9 and rs == 29 and rt == 29 and simm < 0:
            fsize = -simm
        elif op == 0x2B and rs == 29 and (16 <= rt <= 23 or rt in (30, 31)):
            mask |= 1 << rt
    return fsize, mask


class R:  # retail function
    __slots__ = ("addr", "size", "name", "fsize", "mask", "tu")

    def __init__(self, addr, size, name, exe):
        self.addr, self.size, self.name = addr, size, name
        self.fsize, self.mask = frame_of(exe, addr, size)
        self.tu = None


def longest_increasing(pairs: list[tuple[int, int]]) -> list[tuple[int, int]]:
    """LIS on the second coordinate; `pairs` is already sorted by the first."""
    tails: list[int] = []
    idx: list[int] = []
    prev = [-1] * len(pairs)
    for k, (_, j) in enumerate(pairs):
        p = bisect.bisect_left(tails, j)
        if p == len(tails):
            tails.append(j); idx.append(k)
        else:
            tails[p] = j; idx[p] = k
        prev[k] = idx[p - 1] if p else -1
    if not idx:
        return []
    out, k = [], idx[len(tails) - 1]
    while k != -1:
        out.append(pairs[k]); k = prev[k]
    return out[::-1]


def score(r: R, d: P.Func, dsize: int) -> tuple[int, bool, bool]:
    f_ok = r.fsize == d.fsize
    m_ok = r.mask == (d.mask & 0xFFFFFFFF)
    s = 0
    if f_ok: s += 3
    if m_ok: s += 3
    if dsize and abs(r.size - dsize) <= 0.25 * max(r.size, dsize):
        s += 2
    if s == 0:
        s = -1
    return s, f_ok, m_ok


def align(rs: list[R], ds: list[tuple[P.Func, int]]) -> list[tuple[R, P.Func, int, bool, bool]]:
    """Needleman-Wunsch; returns aligned (retail, demo) pairs."""
    n, m = len(rs), len(ds)
    M = [[0] * (m + 1) for _ in range(n + 1)]
    for i in range(1, n + 1): M[i][0] = i * GAP
    for j in range(1, m + 1): M[0][j] = j * GAP
    for i in range(1, n + 1):
        for j in range(1, m + 1):
            s, _, _ = score(rs[i - 1], ds[j - 1][0], ds[j - 1][1])
            M[i][j] = max(M[i - 1][j - 1] + s, M[i - 1][j] + GAP, M[i][j - 1] + GAP)
    out, i, j = [], n, m
    while i > 0 and j > 0:
        s, f_ok, m_ok = score(rs[i - 1], ds[j - 1][0], ds[j - 1][1])
        if M[i][j] == M[i - 1][j - 1] + s:
            out.append((rs[i - 1], ds[j - 1][0], s, f_ok, m_ok)); i -= 1; j -= 1
        elif M[i][j] == M[i - 1][j] + GAP:
            i -= 1
        else:
            j -= 1
    return out[::-1]


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("sym", nargs="?", default=f"{REPO}/disks/demo/PSX.SYM")
    ap.add_argument("--functions", default=f"{REPO}/.shake/ghidra-export/functions.tsv")
    ap.add_argument("--splat", default=f"{REPO}/config/splat.main.exe.yaml",
                    help="current named C subsegments overlaid onto the function inventory")
    ap.add_argument("--no-name-overlay", action="store_true",
                    help="use names in --functions verbatim (normally stale after renames)")
    ap.add_argument("--exe", default=f"{REPO}/disks/tenchu/main.exe")
    ap.add_argument("--apply", metavar="TSV")
    ap.add_argument("--min", choices=("HIGH", "MED"), default="MED")
    ap.add_argument("--quiet", action="store_true")
    ap.add_argument("--candidates", metavar="TSV",
                    help="write EVERY proposal (incl. rejected LOW ones) here")
    ap.add_argument("--unplaced", metavar="TSV",
                    help="write demo functions with no retail counterpart here")
    ap.add_argument("--left", metavar="TSV",
                    help="write retail placeholders with no candidate at all here")
    args = ap.parse_args()

    sf = P.load(args.sym)
    exe = open(args.exe, "rb").read()

    demo = sorted(sf.funcs, key=lambda f: f.addr)
    for k, f in enumerate(demo):        # code size = distance to next function
        nxt = demo[k + 1].addr if k + 1 < len(demo) else f.end_addr + 8
        f_size = max(0, min(nxt - f.addr, f.end_addr - f.addr + 8) if f.end_addr else nxt - f.addr)
        setattr(f, "csize", f_size)
    dname = {f.name: f for f in demo}
    dtu = collections.defaultdict(list)
    for f in demo:
        dtu[f.file.split("\\")[-1]].append(f)

    rows = FI.load_functions(args.functions)
    if not args.no_name_overlay:
        rows, renamed = FI.overlay_current_names(rows, args.splat)
        print(f"retail current-name overlays: {renamed}")
    retail = []
    for a, s, n in rows:
        if a < GAME_END and s > 0:
            retail.append(R(a, s, n, exe))
    retail.sort(key=lambda r: r.addr)

    # ---- tag anchors, cut into TU runs
    for r in retail:
        if r.name in dname:
            r.tu = dname[r.name].file.split("\\")[-1]
    anchors = [i for i, r in enumerate(retail) if r.tu]
    runs = []
    for i in anchors:
        if runs and runs[-1][0] == retail[i].tu:
            runs[-1][2] = i
        else:
            runs.append([retail[i].tu, i, i])
    print(f"retail funcs: {len(retail)}   demo debug funcs: {len(demo)}   "
          f"anchors: {len(anchors)}   TU runs: {len(runs)}")

    taken_names = {r.name for r in retail}

    # A retail function that already carries an original (PSX.SYM) name is
    # correct by construction -- never propose a new name for it.  Only
    # placeholders and names absent from the SYM are rename-eligible.
    eligible = lambda r: is_placeholder(r.name) or r.name not in dname

    proposals, placed = [], set()
    for tu, lo, hi in runs:
        rs = retail[lo:hi + 1]
        # anchors of THIS TU inside the run, monotone in demo source order
        av = [(k, dtu[tu].index(dname[r.name]))
              for k, r in enumerate(rs)
              if r.name in dname and dname[r.name] in dtu[tu]]
        keep = set(id(x) for x in longest_increasing(av))
        av = [x for x in av if id(x) in keep] if av else []
        for (k0, j0), (k1, j1) in zip(av, av[1:]):
            placed.add(dtu[tu][j0].name); placed.add(dtu[tu][j1].name)
            rgap = [r for r in rs[k0 + 1:k1] if eligible(r)]
            frozen = [r for r in rs[k0 + 1:k1] if not eligible(r)]
            dgap = [d for d in dtu[tu][j0 + 1:j1]
                    if d.name not in taken_names and d.name not in {f.name for f in frozen}]
            if not rgap or not dgap:
                continue
            for r, d, s, f_ok, m_ok in align(rgap, [(d, d.csize) for d in dgap]):
                placed.add(d.name)
                size_ok = bool(d.csize) and abs(r.size - d.csize) <= 0.25 * max(r.size, d.csize)
                conf = ("HIGH" if (f_ok and m_ok and size_ok) else
                        "MED" if ((f_ok and m_ok) or ((f_ok or m_ok) and size_ok)) else "LOW")
                proposals.append((r.addr, r.name, d.name, tu, conf, f_ok, m_ok, r.size, d.csize))
        for _, j in av:
            placed.add(dtu[tu][j].name)

    # a demo name must not be assigned to two retail functions
    bycount = collections.Counter(p[2] for p in proposals)
    conflicts = {n for n, c in bycount.items() if c > 1}
    # nor may we hand out a name that some other retail function already owns
    taken = {r.name for r in retail}
    proposals = [p for p in proposals if p[2] not in conflicts and p[2] not in taken]

    order = {"HIGH": 0, "MED": 1, "LOW": 2}
    ph = [p for p in proposals if is_placeholder(p[1])]
    guessed = [p for p in proposals if not is_placeholder(p[1])]
    unplaced = [f for f in demo if f.name not in placed and f.name not in taken]
    named_addrs = {p[0] for p in proposals}
    left = [r for r in retail if is_placeholder(r.name) and r.addr not in named_addrs]

    if args.candidates:
        with open(args.candidates, "w") as fh:
            fh.write("# Every name PSX.SYM's TU alignment can suggest for a retail function.\n"
                     "# LOW rows are NOT applied: positional evidence only, neither the frame\n"
                     "# size nor the saved-register mask agrees.  Corroborate with\n"
                     "#   tools/callmatch.py --verify <this file>\n"
                     "# before adopting one.\n"
                     "#addr\tcurrent\tcandidate\tsource\tconfidence\ttu\tretail_size\tdemo_size\n")
            for a, rn, dn, tu, conf, f_ok, m_ok, rsz, dsz in sorted(proposals):
                fh.write(f"{a:08x}\t{rn}\t{dn}\tsymmatch\t{conf}\t{tu}\t{rsz}\t{dsz}\n")
        print(f"wrote {len(proposals)} candidates to {args.candidates}")

    if args.left:
        with open(args.left, "w") as fh:
            fh.write("# Retail functions still named FUN_/DEAD_ for which PSX.SYM's TU\n"
                     "# alignment offers no candidate at all.  Most are library code (no\n"
                     "# debug info) or functions added after the demo.  Try, in order:\n"
                     "#   tools/xbuildnames.py   (instruction identity vs demo PSX.EXE)\n"
                     "#   tools/callmatch.py     (call signature -- survives a rewritten body)\n"
                     "#addr\tsize\tname\n")
            for r in sorted(left, key=lambda x: x.addr):
                fh.write(f"{r.addr:08x}\t{r.size}\t{r.name}\n")
        print(f"wrote {len(left)} unnamed retail placeholders to {args.left}")

    if args.unplaced:
        with open(args.unplaced, "w") as fh:
            fh.write("# Functions PSX.SYM knows that we could NOT locate in retail main.exe.\n"
                     "# Either removed/inlined between the builds, moved to MENU.EXE/ENDING.EXE,\n"
                     "# or still hiding behind one of the unnamed retail placeholders.\n"
                     "#demo_addr\tdemo_size\tfsize\tmask\tfile\tline\tname\n")
            for f in sorted(unplaced, key=lambda x: x.addr):
                fh.write(f"{f.addr:08x}\t{f.csize}\t{f.fsize}\t{f.mask & 0xFFFFFFFF:#010x}\t"
                         f"{f.file.split(chr(92))[-1]}\t{f.line}\t{f.name}\n")
        print(f"wrote {len(unplaced)} unplaced demo functions to {args.unplaced}")

    if not args.quiet:
        for label, group in (("PLACEHOLDER -> original name", ph),
                             ("GUESSED NAME -> original name", guessed)):
            print(f"\n================ {label} ({len(group)}) ================")
            for conf in ("HIGH", "MED", "LOW"):
                sel = sorted(p for p in group if p[4] == conf)
                if not sel: continue
                print(f"  --- {conf} ({len(sel)})")
                for a, rn, dn, tu, c, f_ok, m_ok, rsz, dsz in sel:
                    ev = ("f" if f_ok else "-") + ("m" if m_ok else "-")
                    print(f"    {a:08x} {rn:34s} -> {dn:30s} [{tu:12s}] {ev} "
                          f"size {rsz}/{dsz}")

        print(f"\n================ demo functions with NO retail counterpart "
              f"({len(unplaced)}) ================")
        for tu, c in collections.Counter(f.file.split('\\')[-1] for f in unplaced).most_common():
            print(f"  {c:3d}  {tu}")
        print("  " + ", ".join(sorted(f.name for f in unplaced)))

        print(f"\n================ retail placeholders still unnamed ({len(left)}) ================")
        print("  " + ", ".join(f"{r.name}" for r in left))

    print(f"\nSUMMARY: proposals={len(proposals)} "
          f"HIGH={sum(1 for p in proposals if p[4]=='HIGH')} "
          f"MED={sum(1 for p in proposals if p[4]=='MED')} "
          f"LOW={sum(1 for p in proposals if p[4]=='LOW')}  "
          f"(placeholder {len(ph)}, guessed {len(guessed)}); "
          f"dropped {len(conflicts)} conflicting demo names")

    if args.apply:
        with open(args.apply, "w") as fh:
            n = 0
            for a, rn, dn, tu, conf, f_ok, m_ok, rsz, dsz in sorted(proposals):
                if order[conf] <= order[args.min]:
                    fh.write(f"{a:08x}\t{rn}\t{dn}\t{tu}\t{conf}\n"); n += 1
        print(f"wrote {n} renames (>= {args.min}) to {args.apply}")


if __name__ == "__main__":
    main()

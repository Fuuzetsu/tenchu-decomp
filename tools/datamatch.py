#!/usr/bin/env python3
"""Recover data-symbol names by aligning data *references* inside known functions.

The function matchers (symmatch / xbuildnames / callmatch) name code.  Globals
need a different handle: PSX.SYM's data addresses belong to a lost build, and the
data segment shifted far more than .text did, so ordering alone is unreliable
against retail.

But a function we have already identified on both sides touches the same globals,
in the same order.  So: for every retail function whose name we share with the
demo's PSX.EXE, decode both bodies' data references (`lui`/`%lo` pairs and
`$gp`-relative accesses), and when the two reference sequences have the same
length, zip them.  Each zip is a vote that retail address X is the demo's symbol
Y.  A global touched by several known functions collects several independent
votes.

The demo label export is incomplete.  Unique names prove a rigid PSX.SYM-data to
demo-PSX.EXE relocation, so the tool first infers that delta (rather than baking
it in), requires at least 64 anchors and 99% dominance, and synthesizes omitted
demo labels.  Duplicate static names are accepted only when the referencing
function's original translation unit disambiguates them.

Accepted only when every vote for X agrees, the name is claimed by no other raw
vote address, and the name exists in PSX.SYM (provenance).

Precision is measured, not assumed: retail addresses that *already* carry a real
name form a control set -- we report how often the votes reproduce it.
"""
from __future__ import annotations

import argparse, collections, difflib, os, re, struct, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import psxsym as P
import function_inventory as FI

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PLACEHOLDER = re.compile(r"^(D_|DAT_|LAB_|jtbl_|switchD_|__)")
IDENT = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
LINKER_DATA = re.compile(
    r"^(?:__(?:data|text|bss|sdata|sbss)(?:_.*)?|"
    r"_(?:text|bss)_(?:obj|org)(?:end)?)$"
)

MIN_DELTA_ANCHORS = 64
MIN_DELTA_DOMINANCE = 0.99
MIN_CONTROL_FOR_APPLY = 64

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
              tlo: int, thi: int, *, direct_only: bool = False):
    """(refs, tokens) -- refs is [(instr_index, data_address)], tokens aligns builds.

    ``direct_only`` stops following a register after an address-materializing
    instruction.  This is sufficient for symbol ownership (the materialized
    base is itself a reference) and avoids interpreting later ``0(reg)``
    accesses as ``high(global) + 0`` after a ``lui``/``addiu`` pair.
    """
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
            if direct_only or rt != rs:
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


def tu_key(path: str) -> str:
    """Normalize the DOS/Unix source paths carried by PSX.SYM."""
    return re.split(r"[\\/]", path)[-1].upper() if path else ""


def is_semantic_data_name(name: str) -> bool:
    """Reject linker boundaries and compiler-disambiguated local statics."""
    return bool(IDENT.fullmatch(name)) and not LINKER_DATA.match(name)


def load_label_sets(path: str) -> dict[int, set[str]]:
    """Load every label at an address; never silently overwrite aliases."""
    out: dict[int, set[str]] = collections.defaultdict(set)
    for p in load_tsv(path, 2):
        out[int(p[0], 16)].add(p[1])
    return dict(out)


def reverse_labels(labels: dict[int, set[str]]) -> dict[str, set[int]]:
    out: dict[str, set[int]] = collections.defaultdict(set)
    for addr, names in labels.items():
        for name in names:
            out[name].add(addr)
    return dict(out)


def psxsym_data_index(sf: P.SymFile):
    """Return semantic PSX.SYM data labels plus static/TU provenance.

    ``__data_org`` is emitted by the lost link itself and is a safer boundary
    than guessing from the last debug-described function (most Psy-Q library
    functions have no 0x8c record).
    """
    starts = {s.addr for s in sf.syms if s.name == "__data_org"}
    if len(starts) != 1:
        raise ValueError(
            "PSX.SYM data calibration needs exactly one __data_org; "
            f"found {len(starts)}"
        )
    data_lo = next(iter(starts))
    by_addr: dict[int, set[str]] = collections.defaultdict(set)
    by_name: dict[str, set[int]] = collections.defaultdict(set)
    for s in sf.syms:
        if s.addr < data_lo or not is_semantic_data_name(s.name):
            continue
        by_addr[s.addr].add(s.name)
        by_name[s.name].add(s.addr)

    def_files: dict[tuple[int, str], set[str]] = collections.defaultdict(set)
    for d in sf.defs:
        key = tu_key(d.file)
        if d.offset >= data_lo and key and d.name in by_addr.get(d.offset, ()):
            def_files[(d.offset, d.name)].add(key)

    func_tus: dict[str, set[str]] = collections.defaultdict(set)
    for f in sf.funcs:
        key = tu_key(f.file)
        if key:
            func_tus[f.name].add(key)
    return dict(by_addr), dict(by_name), dict(def_files), dict(func_tus), data_lo


def infer_data_delta(psx_by_name: dict[str, set[int]],
                     demo_by_name: dict[str, set[int]],
                     min_anchors: int = MIN_DELTA_ANCHORS,
                     min_dominance: float = MIN_DELTA_DOMINANCE):
    """Infer demo_addr - PSX.SYM_addr from names unique in both images."""
    deltas: collections.Counter[int] = collections.Counter()
    for name, paddrs in psx_by_name.items():
        daddrs = demo_by_name.get(name, set())
        if len(paddrs) == 1 and len(daddrs) == 1:
            deltas[next(iter(daddrs)) - next(iter(paddrs))] += 1
    total = sum(deltas.values())
    if not deltas:
        raise ValueError("no unique PSX.SYM/demo data anchors found")
    delta, support = deltas.most_common(1)[0]
    dominance = support / total
    if support < min_anchors or dominance < min_dominance:
        raise ValueError(
            "unsafe PSX.SYM/demo data relocation: "
            f"best delta {delta:+#x} has {support}/{total} anchors "
            f"({dominance:.1%}); require >= {min_anchors} and "
            f">= {min_dominance:.1%}"
        )
    return delta, support, total


def synthesize_demo_data_labels(actual: dict[int, set[str]],
                                psx_by_addr: dict[int, set[str]],
                                delta: int):
    """Merge calibrated PSX.SYM labels into the lossy Ghidra demo export."""
    merged = {addr: set(names) for addr, names in actual.items()}
    added_pairs = 0
    added_addrs: set[int] = set()
    for psx_addr, names in psx_by_addr.items():
        demo_addr = psx_addr + delta
        dst = merged.setdefault(demo_addr, set())
        for name in names:
            if not is_semantic_data_name(name) or name in dst:
                continue
            dst.add(name)
            added_pairs += 1
            added_addrs.add(demo_addr)
    return merged, added_pairs, len(added_addrs)


def resolve_data_name(demo_addr: int, labels: dict[int, set[str]],
                      psx_by_name: dict[str, set[int]], delta: int,
                      function_tus: set[str] | None = None,
                      def_files: dict[tuple[int, str], set[str]] | None = None):
    """Resolve one demo data address, respecting duplicate-static scope."""
    psx_addr = demo_addr - delta
    function_tus = function_tus or set()
    def_files = def_files or {}
    resolved = []
    for name in labels.get(demo_addr, ()):
        paddrs = psx_by_name.get(name, set())
        if psx_addr not in paddrs:
            continue
        if len(paddrs) > 1:
            owners = def_files.get((psx_addr, name), set())
            if not owners or owners.isdisjoint(function_tus):
                continue
        resolved.append(name)
    return resolved[0] if len(resolved) == 1 else None


def select_clean_votes(votes: dict[int, collections.Counter],
                       demo_targets: dict[tuple[int, str], set[int]]):
    """Apply unanimity and reverse uniqueness to *all* raw vote claims."""
    unanimous = {x: c.most_common(1)[0] for x, c in votes.items() if len(c) == 1}
    raw_claimants: dict[str, set[int]] = collections.defaultdict(set)
    for x, counter in votes.items():
        for name in counter:
            raw_claimants[name].add(x)
    clean = {
        x: (name, count)
        for x, (name, count) in unanimous.items()
        if len(raw_claimants[name]) == 1
        and len(demo_targets.get((x, name), ())) == 1
    }
    stats = {
        "nonunanimous": len(votes) - len(unanimous),
        "reverse_conflicts": sum(
            1 for x, (name, _) in unanimous.items()
            if len(raw_claimants[name]) != 1
        ),
        "target_conflicts": sum(
            1 for x, (name, _) in unanimous.items()
            if len(raw_claimants[name]) == 1
            and len(demo_targets.get((x, name), ())) != 1
        ),
    }
    return clean, stats


def load_retail_functions(functions: str, splat: str, overlay: bool = True):
    """Load authoritative boundaries with the decomp's current C names."""
    rows = FI.load_functions(functions)
    renamed = 0
    if overlay:
        rows, renamed = FI.overlay_current_names(rows, splat)
    return {name: (addr, size) for addr, size, name in rows}, renamed


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("sym", nargs="?", default=f"{REPO}/disks/demo/PSX.SYM")
    ap.add_argument("--psxexe", default=f"{REPO}/disks/demo/PSX.EXE")
    ap.add_argument("--psxexe-funcs", default=f"{REPO}/reference/demo-psxexe.functions.tsv")
    ap.add_argument("--psxexe-labels", default=f"{REPO}/reference/demo-psxexe.labels.tsv")
    ap.add_argument("--functions", default=f"{REPO}/.shake/ghidra-export/functions.tsv")
    ap.add_argument("--splat", default=f"{REPO}/config/splat.main.exe.yaml",
                    help="current named C subsegments overlaid onto the function inventory")
    ap.add_argument("--no-name-overlay", action="store_true",
                    help="use names in --functions verbatim (normally stale after renames)")
    ap.add_argument("--exe", default=f"{REPO}/disks/tenchu/main.exe")
    ap.add_argument("--symbols", default=f"{REPO}/config/symbols.main.exe.txt")
    ap.add_argument("--min-votes", type=int, default=2)
    ap.add_argument("--apply", metavar="TSV")
    ap.add_argument("--candidates", metavar="TSV",
                    help="write the single-vote (unproposed) suggestions here")
    args = ap.parse_args()

    sf = P.load(args.sym)
    (psx_by_addr, psx_by_name, def_files, func_tus,
     psx_data_lo) = psxsym_data_index(sf)
    known = set(psx_by_name)

    # ---- demo side
    dexe = open(args.psxexe, "rb").read()
    dt_addr, dt_size = struct.unpack_from("<II", dexe, 0x18)
    dtext = dexe[0x800:0x800 + dt_size]
    dfuncs = {p[2]: (int(p[0], 16), int(p[1])) for p in load_tsv(args.psxexe_funcs)}
    actual_dlabels = load_label_sets(args.psxexe_labels)
    for n, (a, s) in dfuncs.items():
        actual_dlabels.setdefault(a, set()).add(n)
    actual_by_name = reverse_labels(actual_dlabels)
    try:
        data_delta, delta_support, delta_total = infer_data_delta(
            psx_by_name, actual_by_name
        )
    except ValueError as e:
        sys.exit(f"datamatch: {e}")
    dlabels, synth_pairs, synth_addrs = synthesize_demo_data_labels(
        actual_dlabels, psx_by_addr, data_delta
    )
    print(
        f"PSX.SYM data starts at {psx_data_lo:08x}; inferred demo relocation "
        f"{data_delta:+#x} from {delta_support}/{delta_total} unique anchors "
        f"({100 * delta_support / delta_total:.1f}%)"
    )
    print(
        f"synthesized {synth_pairs} missing label/address pairs at "
        f"{synth_addrs} demo data addresses"
    )

    # ---- retail side
    rexe = open(args.exe, "rb").read()
    rtext, rbase = rexe[0x800:], 0x80011000
    rfuncs, renamed = load_retail_functions(
        args.functions, args.splat, not args.no_name_overlay
    )
    print(f"retail current-name overlays: {renamed}")
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
            nm = resolve_data_name(
                y, dlabels, psx_by_name, data_delta,
                func_tus.get(n, set()), def_files
            )
            if nm:
                demo_refs[nm] += 1

    votes: dict[int, collections.Counter] = collections.defaultdict(collections.Counter)
    witnesses: dict[tuple[int, str], set[str]] = collections.defaultdict(set)
    demo_targets: dict[tuple[int, str], set[int]] = collections.defaultdict(set)
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
        # One vote per (function, address, name): a function that touches a
        # global five times is still one independent witness.
        resolved = set()
        for x, y in set(pairs):
            nm = resolve_data_name(
                y, dlabels, psx_by_name, data_delta,
                func_tus.get(n, set()), def_files
            )
            if nm and nm in known and not PLACEHOLDER.match(nm):
                resolved.add((x, nm))
                demo_targets[(x, nm)].add(y)
        for x, nm in resolved:
            if n not in witnesses[(x, nm)]:
                witnesses[(x, nm)].add(n)
                votes[x][nm] += 1

    print(f"function pairs contributing votes: {zipped}   no aligned refs: {skipped}")
    print(f"retail data addresses with >=1 vote: {len(votes)}")

    clean, conflict_stats = select_clean_votes(votes, demo_targets)
    print(
        "vote safety rejects: "
        f"{conflict_stats['nonunanimous']} non-unanimous addresses, "
        f"{conflict_stats['reverse_conflicts']} raw reverse-name conflicts, "
        f"{conflict_stats['target_conflicts']} multi-target conflicts"
    )

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

    if args.apply and (len(ctrl) < MIN_CONTROL_FOR_APPLY or len(agree) != len(ctrl)):
        sys.exit(
            "datamatch: refusing --apply: control must have at least "
            f"{MIN_CONTROL_FOR_APPLY} entries and 100% precision; got "
            f"{len(agree)}/{len(ctrl)}"
        )

    guessed = [(x, cur[x], nm, v) for x, (nm, v) in clean.items()
               if x in cur and cur[x] not in known and not PLACEHOLDER.match(cur[x])
               and nm not in taken and v >= args.min_votes]
    print(f"\n=== guessed repo names the demo can replace ({len(guessed)}) ===")
    for x, ours, theirs, v in sorted(guessed):
        cov = v / max(1, demo_refs.get(theirs, 1))
        y = next(iter(demo_targets[(x, theirs)]))
        ws = ",".join(sorted(witnesses[(x, theirs)]))
        print(f"  {x:08x} {ours:30s} -> {theirs:24s} {v} votes, "
              f"{cov:.0%} of demo referencers; psx={y-data_delta:08x} "
              f"demo={y:08x}; witnesses={ws}")

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
        y = next(iter(demo_targets[(x, nm)]))
        ws = ",".join(sorted(witnesses[(x, nm)]))
        print(f"  {x:08x} {newname:22s} -> {nm:28s} {v}v {cov:>4.0%}  "
              f"{st:8s} {ty}  psx={y-data_delta:08x} demo={y:08x} "
              f"witnesses={ws}")

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
                     "#addr\tcurrent\tcandidate\tstorage\ttype\tpsxsym_addr\t"
                     "demo_addr\twitnesses\n")
            for x in sorted(singles):
                old, nm, v = singles[x]
                st, ty = dtype.get(nm, ("?", "?"))
                y = next(iter(demo_targets[(x, nm)]))
                ws = ",".join(sorted(witnesses[(x, nm)]))
                fh.write(f"{x:08x}\t{old}\t{nm}\t{st}\t{ty}\t"
                         f"{y-data_delta:08x}\t{y:08x}\t{ws}\n")
        print(f"wrote {len(singles)} single-vote candidates to {args.candidates}")

    if args.apply:
        with open(args.apply, "w") as fh:
            fh.write("#addr\tprevious_name\tadopted_name\tmatcher\tevidence\n")
            for x in sorted(props):
                old, nm, v = props[x]
                y = next(iter(demo_targets[(x, nm)]))
                ws = ",".join(sorted(witnesses[(x, nm)]))
                fh.write(f"{x:08x}\t{old}\t{nm}\tdatamatch-reloc\t"
                         f"VOTES{v};PSX={y-data_delta:08x};DEMO={y:08x};"
                         f"WITNESSES={ws}\n")
            for x, ours, nm, v in sorted(guessed):
                y = next(iter(demo_targets[(x, nm)]))
                ws = ",".join(sorted(witnesses[(x, nm)]))
                fh.write(f"{x:08x}\t{ours}\t{nm}\tdatamatch-reloc\t"
                         f"VOTES{v};PSX={y-data_delta:08x};DEMO={y:08x};"
                         f"WITNESSES={ws}\n")
        print(f"\nwrote {len(props)+len(guessed)} renames to {args.apply}")


if __name__ == "__main__":
    main()

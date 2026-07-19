#!/usr/bin/env python3
"""Locate main.exe's functions inside the other five executables.

Tenchu links the same engine into six PS-EXEs. `trial.exe` (the Mission Editor)
is a near-complete relink of the game; `menu.exe`, `ending.exe` and `slps_019.01`
carry the engine core plus the whole statically-linked Sony SDK; `run.exe` is the
loader and shares little but the SDK.

Two modes:

  exact       byte-for-byte identical function bodies. Every immediate, jal target
              and gp-relative offset coincides, so a function we have already
              byte-matched in main.exe reproduces those bytes for free. Rare: the
              two builds put data at different addresses, so anything touching a
              global differs. Useful only as a drop-in list.

  normalized  (default) mask out the relocatable fields -- j/jal targets, lui
              immediates, and every I-type immediate -- before comparing. This
              finds the *same code* regardless of where the linker put things.
              It is the same normalisation `xbuildnames.py` uses against the demo.

The normalized mode doubles as a symbol-recovery tool: a unique hit gives that
function's address in the other exe. Hits are cross-checked for adjacency (a run
of functions landing end-to-end at consecutive addresses is very strong evidence
the boundaries are right), and ambiguous hits -- a normalized body occurring more
than once, which happens for small identical helpers -- are dropped.

The demo PSX.EXE is a second name source (--source demo): the monolithic demo
build contains translation units main.exe never shipped (OPENING.C, OPMOVIE.C,
MOJI.C), so its PSX.SYM-backed names can land in MENU.EXE/ENDING.EXE where ours
cannot. --merged-tsv composes both sources with strict precedence: a name our
main.exe already establishes is copied over, and demo names only fill target
ranges no main.exe function claims; same-span name disagreements are reported,
never silently resolved.

    tools/xexe.py                       # summary table, all targets
    tools/xexe.py --target trial.exe    # per-function detail
    tools/xexe.py --target trial.exe --exact
    tools/xexe.py --target menu.exe --source demo
    tools/xexe.py --target menu.exe --merged-tsv reference/xexe-menu.exe.tsv

Nothing here writes to config/. Minting config/symbols.<target>.txt from a --tsv
is a deliberate, reviewable step -- see PLAN.md.
"""
from __future__ import annotations

import argparse
import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)
sys.path.insert(0, os.path.join(ROOT, "tools"))

from xbuildnames import norm  # noqa: E402  (same normalisation as the demo matcher)

FUNCS_TSV = os.path.join(".shake", "ghidra-export", "functions.tsv")
MAIN = ("disks/tenchu/main.exe", 0x80011000)
MAIN_TEXT_END = 0x80098000

# The demo's monolithic PSX.EXE: boundaries and names from boricj's PSX.SYM
# propagation (reference/demo-psxexe.functions.tsv), provenance re-filtered
# through PSX.SYM proper below.
DEMO = ("disks/demo/PSX.EXE", 0x80010100)
DEMO_FUNCS_TSV = os.path.join("reference", "demo-psxexe.functions.tsv")
PSXSYM_UNPLACED_TSV = os.path.join("reference", "psxsym-unplaced.tsv")
PSXSYM_PROTOS_H = os.path.join("reference", "psxsym-protos.h")

# name -> (image, text vram). run.exe loads high; the rest share main's base.
OTHERS = {
    "trial.exe": ("disks/tenchu/trial.exe", 0x80011000),
    "menu.exe": ("disks/tenchu/menu.exe", 0x80011000),
    "ending.exe": ("disks/tenchu/ending.exe", 0x80011000),
    "run.exe": ("disks/tenchu/run.exe", 0x80110000),
    "slps_019.01": ("disks/slps_019.01", 0x80011000),
}

# A function shorter than this is a `jr $ra` stub or a BIOS thunk: its body occurs
# all over every image and a "hit" means nothing.
MIN_SIZE = 32


def normtext(blob: bytes, off0: int) -> bytes:
    """Normalise a whole image's .text so a normalised body can be `find`-ed in it."""
    return b"".join(
        struct.pack("<I", norm(struct.unpack_from("<I", blob, i)[0]))
        for i in range(off0, len(blob) - 3, 4)
    )


def normbody(b: bytes) -> bytes:
    return normtext(b, 0)


def config_symbol_names() -> dict[int, str]:
    """{address: adopted name} from config/symbols.main.exe.txt.

    The Ghidra export is a snapshot; the config is where adopted names live.
    Data labels (D_*) are excluded so a data alias sharing a function address
    cannot rename the function.
    """
    import re

    out: dict[int, str] = {}
    for line in open("config/symbols.main.exe.txt"):
        match = re.match(r"\s*(\w+)\s*=\s*(0x[0-9A-Fa-f]+)\s*;", line)
        if match and not match.group(1).startswith("D_"):
            out[int(match.group(2), 16)] = match.group(1)
    return out


def main_functions(min_size: int = MIN_SIZE) -> list[tuple[str, int, int, bytes]]:
    """(name, vram, size, body) for every main.exe function of at least min_size.

    Names are overlaid with the repo's current identities: splat's named C
    subsegments for game code, and config/symbols adoptions for everything
    the snapshot still calls FUN_*.
    """
    import function_inventory as FI

    img = open(MAIN[0], "rb").read()
    rows, _ = FI.overlay_current_names(
        FI.load_functions(FUNCS_TSV), "config/splat.main.exe.yaml"
    )
    adopted = config_symbol_names()
    out = []
    for a, sz, name in rows:
        if not (MAIN[1] <= a < MAIN_TEXT_END) or sz < min_size:
            continue
        # config/symbols is where adopted spellings live (FUN_ renames, but
        # also assembler-safe forms like _2D_SP0_OBJ_* for PSY-Q names the
        # snapshot keeps in raw digit-leading form).
        name = adopted.get(a, name)
        off = a - MAIN[1] + 0x800
        b = img[off : off + sz]
        if len(b) == sz:
            out.append((name, a, sz, b))
    return out


def demo_functions(min_size: int = MIN_SIZE) -> list[tuple[str, int, int, bytes]]:
    """(name, vram, size, body) for every named demo PSX.EXE function.

    FUN_/LAB_ rows are skipped: a placeholder transfers no information.
    """
    img = open(DEMO[0], "rb").read()
    out = []
    for line in open(DEMO_FUNCS_TSV):
        parts = line.rstrip("\n").split("\t")
        if len(parts) < 3 or line.startswith("#"):
            continue
        a, sz, name = int(parts[0], 16), int(parts[1]), parts[2]
        # DEAD_* is boricj's marker for unreferenced code; like FUN_/LAB_
        # it names nothing.
        if name.startswith(("FUN_", "LAB_", "DEAD_")) or sz < min_size:
            continue
        off = a - DEMO[1] + 0x800
        if off < 0x800:
            continue
        b = img[off : off + sz]
        if len(b) == sz:
            out.append((name, a, sz, b))
    return out


def psxsym_names() -> set[str]:
    """Function names PSX.SYM itself records (prototypes + unplaced list)."""
    import re

    names: set[str] = set()
    with open(PSXSYM_PROTOS_H) as protos:
        for line in protos:
            match = re.search(r"(\w+)\s*\(", line)
            if match:
                names.add(match.group(1))
    with open(PSXSYM_UNPLACED_TSV) as unplaced:
        for line in unplaced:
            if line.startswith("#"):
                continue
            parts = line.rstrip("\n").split("\t")
            if len(parts) >= 7:
                names.add(parts[6])
    return names


def merge_hits(main_hits, demo_hits):
    """Compose the two sources with strict main.exe precedence.

    Returns (demo_new, agreements, disagreements, covered, ambiguous_demo):
    demo hits whose target span overlaps any main hit are dropped as covered,
    except that an identical span with a different name is recorded as a
    disagreement instead of silently resolved; demo hits contending for one
    target address under different names are dropped as ambiguous.
    """
    by_start = {hit[2]: hit for hit in main_hits}
    spans = sorted((hit[2], hit[2] + hit[3], hit[0]) for hit in main_hits)

    claims: dict[int, list] = {}
    for hit in demo_hits:
        claims.setdefault(hit[2], []).append(hit)
    ambiguous_demo = 0
    demo_new, agreements, disagreements, covered = [], 0, [], []
    for target, contenders in sorted(claims.items()):
        names = {hit[0] for hit in contenders}
        if len(names) > 1:
            ambiguous_demo += 1
            continue
        hit = contenders[0]
        name, _, start, size = hit
        exact = by_start.get(start)
        if exact is not None and exact[3] == size:
            if exact[0] == name:
                agreements += 1
            else:
                disagreements.append((start, exact[0], name))
            continue
        end = start + size
        if any(s < end and start < e for s, e, _ in spans):
            covered.append(hit)
            continue
        demo_new.append(hit)
    return demo_new, agreements, disagreements, covered, ambiguous_demo


def matched_set() -> set[str]:
    """The functions we have already byte-matched (same rule as tools/progress.py)."""
    import re

    yaml = open("config/splat.main.exe.yaml").read()
    carved = set(re.findall(r"^\s+- \[0x[0-9A-Fa-f]+,\s*c,\s*(\S+)\]", yaml, re.M))
    out = set()
    src = "src/main.exe"
    for f in os.listdir(src):
        if not f.endswith(".c"):
            continue
        if re.search(r"^\s*INCLUDE_ASM", open(os.path.join(src, f)).read(), re.M):
            continue
        if f[:-2] in carved:
            out.add(f[:-2])
    return out


def find_all(hay: bytes, needle: bytes, limit: int = 3) -> list[int]:
    occ, i = [], hay.find(needle)
    while i != -1 and len(occ) < limit:
        occ.append(i)
        i = hay.find(needle, i + 4)
    return occ


def scan(exe: str, exact: bool, min_size: int, funcs=None):
    path, vram = OTHERS[exe]
    blob = open(path, "rb").read()
    hay = blob[0x800:] if exact else normtext(blob, 0x800)
    if funcs is None:
        funcs = main_functions(min_size)

    hits, ambiguous = [], 0
    for name, addr, sz, body in funcs:
        occ = find_all(hay, body if exact else normbody(body))
        if not occ:
            continue
        if len(occ) > 1:
            ambiguous += 1
            continue
        off = occ[0]
        if off % 4:
            continue
        hits.append((name, addr, vram + off, sz))
    return funcs, hits, ambiguous


def adjacency(hits) -> int:
    """How many hits are immediately followed by another hit, end-to-end.

    Functions keep their source order within a translation unit, so a long run of
    end-to-end hits is independent evidence that the boundaries and addresses are
    right. (vinit/vgetmaxsize/vgetfreesize/vcalloc do exactly this in trial.exe.)
    """
    ends = {h[2] + h[3] for h in hits}
    starts = {h[2] for h in hits}
    return len(ends & starts)


JAL_OP = 3
JR_RA = 0x03E00008
MAX_CANDIDATE_SIZE = 0x8000


def jal_target(word: int, pc_region: int) -> int | None:
    if word >> 26 != JAL_OP:
        return None
    return (pc_region & 0xF0000000) | ((word & 0x03FFFFFF) << 2)


def named_callees(blob: bytes, base: int, addr: int, size: int,
                  names: dict[int, str]) -> tuple:
    """Sorted multiset of named jal targets inside [addr, addr+size)."""
    out = []
    off0 = addr - base + 0x800
    for i in range(0, size, 4):
        word = struct.unpack_from("<I", blob, off0 + i)[0]
        target = jal_target(word, addr)
        if target is not None:
            name = names.get(target)
            if name:
                out.append(name)
    return tuple(sorted(out))


def candidate_functions(blob: bytes, base: int,
                        anchors: dict[int, tuple[int, str]]):
    """Unnamed candidate (addr, extent) pairs from the target's own call graph.

    Every jal target is a function start.  Extents run to the next known
    start (anchor or candidate) with a hard cap; that is enough for a callee
    multiset even without exact boundaries.  False starts decoded from data
    words self-filter later: they essentially never accumulate MIN_CALLS
    distinct named callees.
    """
    top = base + len(blob) - 0x800
    starts: set[int] = set()
    for off in range(0x800, len(blob) - 3, 4):
        target = jal_target(struct.unpack_from("<I", blob, off)[0],
                            base + off - 0x800)
        if target is not None and base <= target < top and target % 4 == 0:
            starts.add(target)
    all_starts = sorted(starts | set(anchors))
    out = []
    for index, addr in enumerate(all_starts):
        if addr in anchors:
            continue
        following = (
            all_starts[index + 1] if index + 1 < len(all_starts) else top
        )
        extent = min(following - addr, MAX_CANDIDATE_SIZE, top - addr)
        if extent >= 8:
            out.append((addr, extent))
    return out


def place_by_calls(target: str, blob: bytes, base: int,
                   rows: list, min_calls: int = 3):
    """Place evolved demo bodies by callee multiset against the named anchors.

    rows are the merged-table entries (addr, size, name, source, provenance).
    Returns (proposals, contested) where each proposal is
    (target_addr, extent, demo_name, tier, evidence).

    Demo callee multisets are restricted to names the target side knows: the
    OPMOVIE.C/MOJI.C family calls its own siblings, so each accepted
    placement widens the shared vocabulary, and the search iterates to a
    fixpoint.
    """
    import collections
    import math

    demo_img = open(DEMO[0], "rb").read()
    demo_all = demo_functions(8)
    demo_names = {a: n for n, a, _, _ in demo_all}

    anchors = {addr: (size, name) for addr, size, name, _, _ in rows}
    anchor_names = {addr: name for addr, size, name, _, _ in rows}
    placed_names = {name for _, _, name, _, _ in rows}
    candidates = candidate_functions(blob, base, anchors)

    proposals, used_addrs, used_names = [], set(), set()
    contested = 0
    while True:
        known = placed_names | used_names
        demo_sigs = {}
        for name, addr, size, _ in demo_all:
            if name in known:
                continue
            calls = tuple(
                c for c in named_callees(demo_img, DEMO[1], addr, size,
                                         demo_names)
                if c in known
            )
            if len(set(calls)) >= min_calls:
                demo_sigs[name] = (calls, size)

        candidate_sigs = {}
        for addr, extent in candidates:
            if addr in used_addrs:
                continue
            calls = named_callees(blob, base, addr, extent, anchor_names)
            if len(set(calls)) >= min_calls:
                candidate_sigs[addr] = (calls, extent)

        round_proposals = []

        by_sig_demo: dict[tuple, list[str]] = {}
        for name, (calls, _) in demo_sigs.items():
            by_sig_demo.setdefault(calls, []).append(name)
        by_sig_candidate: dict[tuple, list[int]] = {}
        for addr, (calls, _) in candidate_sigs.items():
            by_sig_candidate.setdefault(calls, []).append(addr)
        for sig, names_for in sorted(by_sig_demo.items()):
            addrs_for = by_sig_candidate.get(sig, [])
            if len(names_for) == 1 and len(addrs_for) == 1:
                name, addr = names_for[0], addrs_for[0]
                round_proposals.append(
                    (addr, candidate_sigs[addr][1], name, "exact",
                     f"{len(sig)} calls")
                )

        taken_addrs = {p[0] for p in round_proposals}
        taken_names = {p[2] for p in round_proposals}

        def contain_fits(want, demo_size, exclude_addrs):
            """Bounded containment fits: extras and size gap both limited.

            A tiny multiset is contained in every huge dispatcher, so a fit
            is only evidence when the candidate does little else and is
            size-plausible.
            """
            total = sum(want.values())
            fits = []
            for addr, (have_calls, extent) in candidate_sigs.items():
                if addr in exclude_addrs:
                    continue
                have = collections.Counter(have_calls)
                if want - have:
                    continue
                extra = sum((have - want).values())
                gap = abs(math.log(max(1, extent) / max(1, demo_size)))
                if extra <= 2 * total and gap <= math.log(3):
                    fits.append((extra, gap, addr, extent))
            return sorted(fits)

        for name, (calls, demo_size) in sorted(demo_sigs.items()):
            if name in taken_names:
                continue
            want = collections.Counter(calls)
            fits = contain_fits(want, demo_size, taken_addrs)
            if not fits:
                continue
            best = fits[0]
            if [f for f in fits[1:] if f[0] <= best[0] and f[1] <= best[1]]:
                contested += 1
                continue
            # Bidirectional check: if another unplaced demo signature also
            # fits this candidate within bounds, the pairing is a coin flip.
            rivals = [
                other
                for other, (other_calls, other_size) in demo_sigs.items()
                if other != name and other not in taken_names
                and any(
                    f[2] == best[2]
                    for f in contain_fits(
                        collections.Counter(other_calls), other_size,
                        taken_addrs,
                    )
                )
            ]
            if rivals:
                contested += 1
                continue
            round_proposals.append(
                (best[2], best[3], name, "contain",
                 f"{len(calls)} calls +{best[0]} extra")
            )
            taken_addrs.add(best[2])
            taken_names.add(name)

        if not round_proposals:
            break
        for addr, extent, name, tier, evidence in round_proposals:
            proposals.append((addr, extent, name, tier, evidence))
            used_addrs.add(addr)
            used_names.add(name)
            anchor_names[addr] = name
    return sorted(proposals), contested


def merged(target: str, min_size: int, tsv: str,
           place_calls: bool = False) -> None:
    """Compose main.exe and demo names for one target and write the table."""
    main_funcs, main_hits, main_ambiguous = scan(target, False, min_size)
    demo_funcs, demo_hits, demo_ambiguous = scan(
        target, False, min_size, funcs=demo_functions(min_size)
    )
    demo_new, agreements, disagreements, covered, contested = merge_hits(
        main_hits, demo_hits
    )
    psxsym = psxsym_names()
    unplaced = {
        line.split("\t")[6].rstrip("\n")
        for line in open(PSXSYM_UNPLACED_TSV)
        if not line.startswith("#") and line.count("\t") >= 6
    }

    print(
        f"{target}: {len(main_hits)}/{len(main_funcs)} main.exe functions "
        f"copied ({main_ambiguous} ambiguous, {adjacency(main_hits)} adjacent); "
        f"demo adds {len(demo_new)} new names from {len(demo_hits)} hits "
        f"({demo_ambiguous} ambiguous bodies, {contested} contested targets, "
        f"{len(covered)} already covered, {agreements} span-identical "
        f"agreements)"
    )
    if disagreements:
        print("\n  span-identical NAME DISAGREEMENTS (review, do not adopt blind):")
        for addr, ours, theirs in disagreements:
            print(f"    {addr:#010x} main={ours} demo={theirs}")
    placed = sorted(hit[0] for hit in demo_new if hit[0] in unplaced)
    if placed:
        print(f"\n  PSX.SYM-unplaced functions now placed here ({len(placed)}):")
        print("    " + ", ".join(placed))

    rows = sorted(
        [(addr, size, name, "main", "-") for name, _, addr, size in main_hits]
        + [
            (addr, size, name, "demo", "psxsym" if name in psxsym else "ghidra")
            for name, _, addr, size in demo_new
        ]
    )

    if place_calls:
        path, vram = OTHERS[target]
        blob = open(path, "rb").read()
        proposals, contested = place_by_calls(target, blob, vram, rows)
        print(f"\n  call-graph placement: {len(proposals)} proposals "
              f"({contested} contested containment fits dropped)")
        for addr, extent, name, tier, evidence in proposals:
            flag = "psxsym" if name in psxsym else "ghidra"
            print(f"    {addr:#010x} ~{extent:>5}b {name:<24} "
                  f"[{tier}, {evidence}] {flag}")
            rows.append((addr, extent, name, f"calls-{tier}", flag))
        rows.sort()

    with open(tsv, "w") as out:
        out.write("#addr\tsize\tname\tsource\tprovenance\n")
        for addr, size, name, source, provenance in rows:
            out.write(f"{addr:#010x}\t{size}\t{name}\t{source}\t{provenance}\n")
    print(f"\n  wrote {len(rows)} rows to {tsv}")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--target", choices=sorted(OTHERS), help="default: all, summary only")
    ap.add_argument("--exact", action="store_true", help="byte-identical bodies only")
    ap.add_argument("--min-size", type=int, default=MIN_SIZE)
    ap.add_argument("--tsv", help="write name<TAB>addr<TAB>size for the hits")
    ap.add_argument("--source", choices=("main", "demo"), default="main",
                    help="whose functions to look for (default: main.exe's)")
    ap.add_argument("--merged-tsv",
                    help="compose main.exe + demo names with main precedence "
                         "and write addr/size/name/source/provenance rows "
                         "(needs --target)")
    ap.add_argument("--place-by-calls", action="store_true",
                    help="after composing, place evolved demo bodies by "
                         "named-callee multiset against the anchors "
                         "(needs --merged-tsv)")
    args = ap.parse_args()

    if not os.path.exists(FUNCS_TSV):
        sys.exit(f"xexe: no {FUNCS_TSV} -- run the Ghidra export first")

    if args.place_by_calls and not args.merged_tsv:
        sys.exit("xexe: --place-by-calls needs --merged-tsv")
    if args.merged_tsv:
        if not args.target:
            sys.exit("xexe: --merged-tsv needs --target")
        merged(args.target, args.min_size, args.merged_tsv,
               place_calls=args.place_by_calls)
        return

    matched = matched_set() if args.source == "main" else set()
    mode = "exact" if args.exact else "normalized"
    source_funcs = (
        None if args.source == "main" else demo_functions(args.min_size)
    )
    targets = [args.target] if args.target else sorted(OTHERS)

    if not args.target:
        n = len(source_funcs if source_funcs is not None
                else main_functions(args.min_size))
        print(f"{n} {args.source} functions >= {args.min_size} bytes; "
              f"{len(matched)} matched overall; mode={mode}\n")
        print(f"  {'exe':<12} {'hits':>5} {'ambig':>6} {'adjacent':>9} {'ours':>5}")
        print("  " + "-" * 41)

    for exe in targets:
        funcs, hits, ambiguous = scan(exe, args.exact, args.min_size,
                                      funcs=source_funcs)
        ours = [h for h in hits if h[0] in matched]
        if not args.target:
            print(f"  {exe:<12} {len(hits):>5} {ambiguous:>6} "
                  f"{adjacency(hits):>9} {len(ours):>5}")
            continue

        print(f"{exe}: {len(hits)}/{len(funcs)} main.exe functions found ({mode}), "
              f"{ambiguous} ambiguous, {adjacency(hits)} in end-to-end runs, "
              f"{len(ours)} already matched by us\n")
        for name, addr, there, sz in sorted(hits, key=lambda h: h[2]):
            star = " *" if name in matched else "  "
            print(f"  {there:#010x} {sz:>5}b{star} {name:<34} (main {addr:#010x})")
        print("\n  * = we have byte-matching C for this function already")

        if args.tsv:
            with open(args.tsv, "w") as f:
                for name, addr, there, sz in sorted(hits, key=lambda h: h[2]):
                    f.write(f"{name}\t{there:#010x}\t{sz}\n")
            print(f"\n  wrote {len(hits)} rows to {args.tsv}")


if __name__ == "__main__":
    main()

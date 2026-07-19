#!/usr/bin/env python3
"""Turn a blob-split PS-EXE config into a decomp-ready function scaffold.

`tools/newexe.py` gives an executable a byte-identical single-blob split.
This tool rewrites the code segment of `config/splat.<target>.yaml` into one
`c` subsegment per known function start, so splat generates an INCLUDE_ASM
stub translation unit for every function — the same day-zero state main.exe
started from, with no decompilation implied.

Function starts come from three sources, composed:

- the composed name table `reference/xexe-<target>.tsv` (our main.exe names
  transferred by normalized identity, demo names for unclaimed ranges, and
  call-graph placements), which also names the subsegments;
- every `jal` target inside the code segment (unnamed starts become
  FUN_<vram> stubs); and
- the executable entry point from the PS-X header.

Existing `c` subsegments (functions somebody already carved) are preserved
exactly.  Gaps before the first start stay `textbin`.  Boundaries do not
affect byte identity — a mis-split range still reassembles exactly — they
only shape the stub files, and can be refined later with tools/reverse.py.

Also writes `config/symbols.<target>.txt` (named starts) and
`config/functions.<target>.tsv` (addr/size/name for progress reporting).

    tools/scaffold_exe.py menu.exe ending.exe trial.exe
"""
from __future__ import annotations

import argparse
import os
import re
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

IMAGES = {
    "menu.exe": "disks/tenchu/menu.exe",
    "ending.exe": "disks/tenchu/ending.exe",
    "trial.exe": "disks/tenchu/trial.exe",
}

JAL_OP = 3


def read_header(path: str) -> tuple[int, int]:
    data = open(path, "rb").read()
    if data[:8] != b"PS-X EXE":
        sys.exit(f"{path}: not a PS-EXE")
    pc0 = struct.unpack_from("<I", data, 0x10)[0]
    t_addr = struct.unpack_from("<I", data, 0x18)[0]
    return pc0, t_addr


def code_frame(yaml_text: str) -> tuple[int, int, int]:
    """(vram, code_start_off, code_end_off) of the single code segment."""
    vram_match = re.search(r"^\s+vram: (0x[0-9A-Fa-f]+)$", yaml_text, re.M)
    start_match = re.search(r"^\s+start: (0x[0-9A-Fa-f]+)$", yaml_text, re.M)
    end_match = re.search(r"^  - \[(0x[0-9A-Fa-f]+)\]\s*$", yaml_text, re.M)
    if not (vram_match and start_match and end_match):
        sys.exit("could not parse the code segment frame from the yaml")
    return (
        int(vram_match.group(1), 16),
        int(start_match.group(1), 16),
        int(end_match.group(1), 16),
    )


def existing_c_subsegments(yaml_text: str) -> dict[int, str]:
    return {
        int(offset, 16): name
        for offset, name in re.findall(
            r"^\s+- \[(0x[0-9A-Fa-f]+), c, (\S+)\]\s*$", yaml_text, re.M
        )
    }


def table_rows(target: str) -> list[tuple[int, str]]:
    path = os.path.join("reference", f"xexe-{target}.tsv")
    rows = []
    for line in open(path):
        if line.startswith("#"):
            continue
        addr, _size, name, _source, _prov = line.rstrip("\n").split("\t")
        rows.append((int(addr, 16), name))
    return rows


BRANCH_OPS = {0x01, 0x04, 0x05, 0x06, 0x07}  # REGIMM, beq, bne, blez, bgtz
COP_OPS = {0x10, 0x11, 0x12, 0x13}


def branch_target(word: int, pc: int) -> int | None:
    op = word >> 26
    if op in BRANCH_OPS or (op in COP_OPS and (word >> 21) & 0x1F == 0x08):
        offset = word & 0xFFFF
        if offset >= 0x8000:
            offset -= 0x10000
        return pc + 4 + offset * 4
    return None


def merge_split_functions(blob: bytes, vram: int, start_off: int,
                          end_off: int, starts: list[int]) -> list[int]:
    """Drop starts that a PC-relative branch proves to be mid-function.

    Handwritten SDK objects use jal entries into shared tails, so a jal
    target is not always a whole function.  A branch never leaves its real
    function, so any start strictly between a branch and its target is
    false; removing it merges the chunks.  Iterates to a fixpoint.
    """
    kept = sorted(set(starts))
    # Trust only control flow inside the credible code window and of sane
    # span: the region tail may be rodata whose words can decode as branches
    # with wild offsets, and one such fake would merge away real boundaries.
    window_lo = kept[0]
    window_hi = min(vram + (end_off - start_off), kept[-1] + 0x8000)
    branches, jumps = [], []
    for off in range(start_off, end_off - 3, 4):
        pc = vram + off - start_off
        if not window_lo <= pc < window_hi:
            continue
        word = struct.unpack_from("<I", blob, off)[0]
        target = branch_target(word, pc)
        if target is None and word >> 26 == 0x02:  # j: local jump OR tail call
            target = (pc & 0xF0000000) | ((word & 0x03FFFFFF) << 2)
            kind = jumps
        else:
            kind = branches
        if target is None or not window_lo <= target < window_hi:
            continue
        if abs(target - pc) > 0x8000:
            continue
        kind.append((min(pc, target), max(pc, target), target))
    while True:
        kept_set = set(kept)
        false_starts = {
            s
            for low, high, _target in branches
            for s in kept
            if low < s <= high
        }
        # A j whose target is a known function start is a tail call; a j
        # into the middle of a chunk is a local jump, so any start between
        # it and its target is false.
        false_starts |= {
            s
            for low, high, target in jumps
            if target not in kept_set
            for s in kept
            if low < s <= high
        }
        if not false_starts:
            return kept
        kept = [s for s in kept if s not in false_starts]


GP_MEMOPS = set(range(0x20, 0x27)) | set(range(0x28, 0x2F)) | {0x09, 0x23}
GP_REGISTER = 28


def gp_referenced(blob: bytes, start_off: int, end_off: int,
                  gp_value: int) -> set[int]:
    """Addresses referenced through $gp in [start_off, end_off).

    splat reliably auto-labels %hi/%lo pairs but can miss pure %gp_rel data
    references; defining these addresses in the symbols script keeps GNU ld
    from truncating R_MIPS_GPREL16 against an undefined symbol.
    """
    out = set()
    for off in range(start_off, end_off - 3, 4):
        word = struct.unpack_from("<I", blob, off)[0]
        if word >> 26 in GP_MEMOPS and (word >> 21) & 0x1F == GP_REGISTER:
            imm = word & 0xFFFF
            if imm >= 0x8000:
                imm -= 0x10000
            out.add((gp_value + imm) & 0xFFFFFFFF)
    return out


def jal_starts(blob: bytes, vram: int, start_off: int, end_off: int) -> set[int]:
    lo, hi = vram, vram + (end_off - start_off)
    out = set()
    for off in range(start_off, end_off - 3, 4):
        word = struct.unpack_from("<I", blob, off)[0]
        if word >> 26 != JAL_OP:
            continue
        target = (vram & 0xF0000000) | ((word & 0x03FFFFFF) << 2)
        if lo <= target < hi and target % 4 == 0:
            out.add(target)
    return out


def scaffold(target: str) -> None:
    image = IMAGES[target]
    yaml_path = os.path.join("config", f"splat.{target}.yaml")
    yaml_text = open(yaml_path).read()
    vram, start_off, end_off = code_frame(yaml_text)
    pc0, _ = read_header(image)
    blob = open(image, "rb").read()

    pinned = {
        vram + off - start_off: name
        for off, name in existing_c_subsegments(yaml_text).items()
    }
    names = dict(table_rows(target))

    # Real text ends at the last `jr ra`; the remainder of the code frame is
    # rodata/data whose D_* references splat can only resolve when the tail
    # is split as a data subsegment, not swallowed by the last function.
    last_jr = None
    for off in range(start_off, end_off - 3, 4):
        if struct.unpack_from("<I", blob, off)[0] == 0x03E00008:
            last_jr = off
    text_end_off = min(end_off, last_jr + 8) if last_jr is not None else end_off
    text_end = vram + text_end_off - start_off

    starts = set(names) | set(pinned) | jal_starts(blob, vram, start_off, end_off)
    starts.add(pc0)
    starts = sorted(a for a in starts if vram <= a < text_end)
    before = len(starts)
    starts = merge_split_functions(blob, vram, start_off, text_end_off, starts)
    merged_away = before - len(starts)

    lines = []
    first_off = starts[0] - vram + start_off
    if first_off > start_off:
        lines.append(f"      - [0x{start_off:x}, textbin]")
    ordered = []
    for addr in starts:
        # A transferred FUN_ name encodes main.exe's address; in this
        # executable the local placeholder is the honest spelling.
        table_name = names.get(addr)
        if table_name is not None and table_name.startswith("FUN_"):
            table_name = None
        name = pinned.get(addr) or table_name or f"FUN_{addr:08x}"
        if not re.match(r"^[A-Za-z_$][A-Za-z0-9_$]*$", name):
            name = "_" + name  # assembler-safe, matching main's convention
        offset = addr - vram + start_off
        lines.append(f"      - [0x{offset:x}, c, {name}]")
        ordered.append((addr, offset, name))
    if text_end_off < end_off:
        lines.append(f"      - [0x{text_end_off:x}, data]")

    block = re.search(
        r"(    subsegments:\n)((?: {6}- \[.*\]\s*\n)+)", yaml_text
    )
    if not block:
        sys.exit(f"{yaml_path}: could not locate the subsegments block")
    yaml_text = (
        yaml_text[: block.start(2)] + "\n".join(lines) + "\n"
        + yaml_text[block.end(2):]
    )
    # splat groups subsegments by output section, not address: with the
    # original blob-era `.data`-first order, the tail data subsegment would
    # be laid out before all text and scramble the image.  Text comes first
    # in these executables' address order.
    order_block = re.search(r"(  section_order:\n)((?:    - \..*\n)+)", yaml_text)
    if not order_block:
        sys.exit(f"{yaml_path}: could not locate section_order")
    yaml_text = (
        yaml_text[: order_block.start(2)]
        + "    - .text\n    - .rodata\n    - .data\n"
        + "    - .sdata\n    - .sbss\n    - .bss\n"
        + yaml_text[order_block.end(2):]
    )
    open(yaml_path, "w").write(yaml_text)

    # Symbols cover every named start, including branch-merged ones: a
    # handwritten-SDK label entry fused into its object still gets its name
    # as a mid-file global label, and jals to it keep linking by name.
    # _gp must be defined for the linker: GNU ld resolves R_MIPS_GPREL16
    # against the _gp symbol, and without it every %gp_rel() truncates.
    gp_match = re.search(r"^\s+gp_value: (0x[0-9A-Fa-f]+)$", yaml_text, re.M)
    named_addrs = set(names) | set(pinned)
    with open(os.path.join("config", f"symbols.{target}.txt"), "w") as sym:
        if gp_match:
            gp_value = int(gp_match.group(1), 16)
            sym.write(f"_gp = {gp_match.group(1)};\n")
            for addr in sorted(
                gp_referenced(blob, start_off, text_end_off, gp_value)
            ):
                # gp_value itself already exists as _gp on the splat side;
                # its data label comes from the data subsegment.
                if addr not in named_addrs and addr != gp_value:
                    sym.write(f"D_{addr:08X} = 0x{addr:08x};\n")
        for addr in sorted(set(names) | set(pinned)):
            if not (vram <= addr < vram + (end_off - start_off)):
                continue
            name = pinned.get(addr) or names[addr]
            if name.startswith("FUN_"):
                continue
            if not re.match(r"^[A-Za-z_$][A-Za-z0-9_$]*$", name):
                name = "_" + name
            sym.write(f"{name} = 0x{addr:08x};\n")

    with open(os.path.join("config", f"functions.{target}.tsv"), "w") as fns:
        fns.write("#addr\tsize\tname\n")
        for index, (addr, offset, name) in enumerate(ordered):
            following = (
                ordered[index + 1][1] if index + 1 < len(ordered)
                else text_end_off
            )
            fns.write(f"0x{addr:08x}\t{following - offset}\t{name}\n")

    named = sum(1 for _, _, n in ordered if not n.startswith("FUN_"))
    print(
        f"{target}: {len(ordered)} function stubs "
        f"({named} named, {len(ordered) - named} FUN_ placeholders, "
        f"{merged_away} branch-crossed starts merged), "
        f"code 0x{start_off:x}..0x{end_off:x}"
    )


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("targets", nargs="+", choices=sorted(IMAGES))
    args = parser.parse_args()
    for target in args.targets:
        scaffold(target)


if __name__ == "__main__":
    raise SystemExit(main())

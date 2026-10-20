#!/usr/bin/env python3
"""Recover sibling-executable symbols from verified shared C instructions.

Unlike xexe's broad discovery fingerprint, this comparison permits differences
only in operands which the compiled MAIN object actually relocates. Constants,
registers, branches, and instruction order must agree. Output is a proposal;
adoption still requires recompiling the C and checking the complete target.

    tools/shared_symbols.py FILEIO --target menu.exe --at 0x80015f74
    tools/shared_symbols.py ACTION --function UpdateMotion --at 0x80020564
    tools/shared_symbols.py VALLOC --at 0x80013ac0 --size 0x920
"""
import argparse
from collections import defaultdict
from pathlib import Path
import re
import struct

try:
    from tools.reloc_c_literals import ElfObject, Relocation
except ModuleNotFoundError:
    from reloc_c_literals import ElfObject, Relocation


ROOT = Path(__file__).resolve().parent.parent


def word(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def signed_low(value):
    return (value & 0x7FFF) - (value & 0x8000)


def infer_symbols(original, target, relocations, symbols, original_gp, target_gp):
    """Require fixed operands to match, then solve and reconcile relocations."""
    if len(original) != len(target) or len(original) % 4:
        raise ValueError("instruction spans must have equal, word-aligned sizes")
    masks = {}
    for relocation in relocations:
        if relocation.type not in (4, 5, 6, 7, 10):
            raise ValueError(f"unsupported relocation: {relocation}")
        if (relocation.offset < 0 or relocation.offset % 4 or
                relocation.offset + 4 > len(original)):
            raise ValueError(f"relocation outside the compared span: {relocation}")
        masks[relocation.offset] = (
            0x3FFFFFF if relocation.type == 4 else
            0xFFFF if relocation.type in (5, 6, 7) else 0
        )
    for offset in range(0, len(original), 4):
        changed = word(original, offset) ^ word(target, offset)
        if changed & (~masks.get(offset, 0) & 0xFFFFFFFF):
            raise ValueError(f"non-relocation instruction difference at +0x{offset:x}")

    proposals = defaultdict(set)
    highs = {}
    # ELF can defer a HI16 record until the last of several LO16 uses.
    # Recover the address from instruction order, not relocation-table order.
    for relocation in sorted(relocations, key=lambda item: item.offset):
        name, offset, kind = (
            relocation.symbol, relocation.offset, relocation.type
        )
        # Section-relative references do not carry a public symbol name.
        # They are covered by the final whole-image rebuild, not named here.
        if not name or kind == 10:
            continue
        if kind == 5:
            highs[name] = offset
            continue
        if name not in symbols:
            raise ValueError(f"no original address for {name}")
        old, new = word(original, offset), word(target, offset)
        if kind == 4:
            delta = ((new & 0x3FFFFFF) - (old & 0x3FFFFFF)) * 4
        elif kind == 7:
            delta = target_gp - original_gp + signed_low(new) - signed_low(old)
        else:
            if name not in highs:
                raise ValueError(f"LO16 without a preceding HI16 for {name}")
            high = highs[name]
            delta = (
                ((word(target, high) & 0xFFFF) -
                 (word(original, high) & 0xFFFF)) * 0x10000
                + signed_low(new) - signed_low(old)
            )
        proposals[name].add((symbols[name] + delta) & 0xFFFFFFFF)
    for name, values in proposals.items():
        if len(values) != 1:
            raise ValueError(
                f"conflicting placements for {name}: "
                + ", ".join(hex(v) for v in sorted(values))
            )
    return {name: next(iter(values)) for name, values in proposals.items()}


def load_symbols(path):
    return {
        name: int(address, 16)
        for name, address in re.findall(
            r"^([\w$]+)\s*=\s*(0x[\da-fA-F]+)", path.read_text(), re.M
        )
    }


def image_span(image, address, size):
    base = word(image, 0x18)
    offset = address - base + 0x800
    if offset < 0x800 or offset + size > len(image):
        raise ValueError("requested span is outside the executable")
    return image[offset:offset + size]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("unit", help="MAIN.EXE translation-unit stem")
    parser.add_argument("--target", default="menu.exe",
                        choices=("menu.exe", "ending.exe", "trial.exe"))
    parser.add_argument("--at", required=True, type=lambda v: int(v, 0))
    parser.add_argument("--function", help="compare only this unit member")
    parser.add_argument("--size", type=lambda v: int(v, 0),
                        help="limit the span, e.g. before an altered suffix")
    args = parser.parse_args()
    original = (ROOT / "disks/tenchu/main.exe").read_bytes()
    if (ROOT / ".shake/build/tenchu/main.exe").read_bytes() != original:
        raise ValueError("rebuild MAIN.EXE with ./Build check before transferring")
    obj = ElfObject(ROOT / f".shake/build/main.exe/{args.unit}.c.o")
    symbols = load_symbols(ROOT / "config/symbols.main.exe.txt")
    functions = [
        s for s in obj.symbols if s.size and
        obj.sections[s.section_index].name == ".text"
    ]
    first = obj.symbol(args.function) if args.function else min(
        functions, key=lambda s: s.value
    )
    start = first.value
    available = first.size if args.function else obj.section(".text").size - start
    size = args.size if args.size is not None else available
    if not 0 < size <= available:
        raise ValueError("--size is outside the selected C span")
    text_relocations = (
        obj.relocations(".rel.text")
        if any(section.name == ".rel.text" for section in obj.sections)
        else []
    )
    relocations = [
        Relocation(r.offset - start, r.type, r.symbol)
        for r in text_relocations
        if start <= r.offset < start + size
    ]

    def gp(target):
        config = (ROOT / f"config/splat.{target}.yaml").read_text()
        return int(re.search(r"gp_value:\s*(0x[\da-fA-F]+)", config)[1], 16)

    target = (ROOT / f"disks/tenchu/{args.target}").read_bytes()
    proposals = infer_symbols(
        image_span(original, symbols[first.name], size),
        image_span(target, args.at, size), relocations, symbols,
        gp("main.exe"), gp(args.target)
    )
    for function in functions:
        if start <= function.value and function.value + function.size <= start + size:
            address = args.at + function.value - start
            if function.name in proposals and proposals[function.name] != address:
                raise ValueError(f"function placement conflicts for {function.name}")
            proposals[function.name] = address
    for name, address in sorted(proposals.items(), key=lambda p: (p[1], p[0])):
        print(f"{name} = 0x{address:08x};")


if __name__ == "__main__":
    main()

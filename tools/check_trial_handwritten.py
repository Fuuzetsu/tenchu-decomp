#!/usr/bin/env python3
"""Verify TRIAL's copies of MAIN's already-classified handwritten routines.

Run after `./Build check-all`. Fixed instruction operands must agree; only
operands relocated by MAIN's actual object may differ. Also check internal
jump destinations and all sixteen entries in the primitive dispatch table.
`--write` regenerates the reference TSV; the default checks the committed copy.
This does not classify new routines or count assembly as recovered C.
"""
import argparse
from pathlib import Path
import re
import struct

try:
    from tools.shared_symbols import (
        ElfObject, Relocation, image_span, infer_symbols, load_source_symbols,
        load_symbols,
    )
except ModuleNotFoundError:
    from shared_symbols import (
        ElfObject, Relocation, image_span, infer_symbols, load_source_symbols,
        load_symbols,
    )

ROOT = Path(__file__).resolve().parent.parent
REFERENCE = ROOT / "reference/trial-handwritten-asm.tsv"


def names(path):
    return [name for line in path.read_text().splitlines()
            if (name := line.split("#", 1)[0].strip())]


def verify():
    original = (ROOT / "disks/tenchu/main.exe").read_bytes()
    if (ROOT / ".shake/build/tenchu/main.exe").read_bytes() != original:
        raise ValueError("run ./Build check-main.exe before comparing source objects")
    target = (ROOT / "disks/tenchu/trial.exe").read_bytes()
    approved = set(names(ROOT / "config/handwritten-asm.txt"))
    selected = names(ROOT / "config/handwritten-asm.trial.exe.txt")
    if len(selected) != len(set(selected)) or not set(selected) <= approved:
        raise ValueError("TRIAL claims duplicate or unclassified handwritten routines")
    rows = {
        name: (int(address, 16), int(size))
        for line in (ROOT / "config/functions.trial.exe.tsv").read_text().splitlines()
        if line and not line.startswith("#")
        for address, size, name in [line.split("\t")]
    }
    source_symbols = load_source_symbols("main.exe")
    target_symbols = load_symbols(ROOT / "config/symbols.trial.exe.txt")

    def gp(executable):
        config = (ROOT / f"config/splat.{executable}.yaml").read_text()
        return int(re.search(r"gp_value:\s*(0x[\da-fA-F]+)", config)[1], 16)

    evidence, dispatch = [], None
    for name in selected:
        obj = ElfObject(ROOT / f".shake/build/main.exe/{name}.c.o")
        function = obj.symbol(name)
        address, size = rows[name]
        if function.size != size:
            raise ValueError(f"{name}: function lengths differ")
        relocations = obj.relocations(".rel.text") if any(
            section.name == ".rel.text" for section in obj.sections
        ) else []
        relocations = [
            Relocation(r.offset - function.value, r.type, r.symbol)
            for r in relocations
            if function.value <= r.offset < function.value + size
        ]
        source_address = source_symbols[name]
        old = image_span(original, source_address, size)
        new = image_span(target, address, size)
        proposals = infer_symbols(old, new, relocations, source_symbols,
                                  gp("main.exe"), gp("trial.exe"))
        for symbol, value in proposals.items():
            if symbol in target_symbols and value != target_symbols[symbol]:
                raise ValueError(f"{name}: {symbol} has a conflicting target address")
        for relocation in relocations:
            if relocation.type != 4:
                continue
            old_word, = struct.unpack_from("<I", old, relocation.offset)
            new_word, = struct.unpack_from("<I", new, relocation.offset)
            old_jump = 0x80000000 | ((old_word & 0x3ffffff) << 2)
            new_jump = 0x80000000 | ((new_word & 0x3ffffff) << 2)
            if source_address <= old_jump < source_address + size:
                if new_jump != address + old_jump - source_address:
                    raise ValueError(f"{name}: internal jump destination changed")
        changed = sum(a != b for a, b in zip(struct.iter_unpack("<I", old),
                                           struct.iter_unpack("<I", new)))
        evidence.append((address, name, source_address, size, changed))
        if name == "DrawTMD":
            dispatch = (source_symbols["D_8001480C"], proposals["D_8001480C"])

    if dispatch is None:
        raise ValueError("the reviewed renderer inventory must include DrawTMD")
    handlers = [name for _, name, _, _, _ in sorted(evidence) if name.startswith("draw")]
    if len(handlers) != 16:
        raise ValueError("the reviewed renderer inventory must contain sixteen handlers")
    for image, address, values in (
        (original, dispatch[0], [source_symbols[name] for name in handlers]),
        (target, dispatch[1], [rows[name][0] for name in handlers]),
    ):
        if image_span(image, address, 64) != struct.pack("<16I", *values):
            raise ValueError("primitive dispatch table does not preserve handler order")

    output = "# Verified by tools/check_trial_handwritten.py; no C-completion credit.\n"
    output += "# name\tmain_address\ttrial_address\tbytes\trelocated_words_changed\n"
    output += "".join(
        f"{name}\t0x{source:08x}\t0x{address:08x}\t{size}\t{changed}\n"
        for address, name, source, size, changed in sorted(evidence)
    )
    return output, len(evidence), sum(row[3] for row in evidence)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true")
    args = parser.parse_args()
    output, count, size = verify()
    if args.write:
        REFERENCE.write_text(output)
    elif not REFERENCE.exists() or REFERENCE.read_text() != output:
        raise ValueError("handwritten reference is stale; review before regenerating")
    print(f"TRIAL handwritten assembly verified: {count} routines / {size:,} bytes; "
          "primitive dispatch table verified")


if __name__ == "__main__":
    main()

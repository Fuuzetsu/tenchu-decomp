#!/usr/bin/env python3
"""Generate the retail-exact canonical-assembly SDK-text link scripts.

This deliberately bounded relocation gate consumes the linker-owned game
lane.  It removes absolute symbol assignments for the SDK/CRT text stream now
emitted from relocatable C and canonical assembly, and can insert controlled
probe sizes immediately before that stream.  ``UnitVector2`` at ``0x80086764``
is the explicit stopping point: it begins the remaining loaded-data input and
is outside this proof.
"""

from __future__ import annotations

import argparse
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
import re
import struct

try:
    from tools.reloc_game_lane import LaneError, atomic_write, filter_symbols
except ModuleNotFoundError:  # Direct invocation adds tools/, not the repo root.
    from reloc_game_lane import LaneError, atomic_write, filter_symbols


SDK_TEXT_START = 0x800601D4
SDK_TEXT_END = 0x80086764
EXPECTED_SDK_SYMBOLS = 2010
EXPECTED_SDK_SECTION_SYMBOLS = 1892
EXPECTED_OMITTED_INTERNAL_ALIASES = 118
FIRST_SDK_INPUT = "/LIBAPI_4F9D4.s.o(.text);"

SHN_UNDEF = 0
SHN_ABS = 0xFFF1
SHT_SYMTAB = 2
SHT_NOBITS = 8
SHT_REL = 9

SHF_EXECINSTR = 0x4

R_MIPS_26 = 4
R_MIPS_HI16 = 5
R_MIPS_LO16 = 6
R_MIPS_PC16 = 10


# These are the Splat-generated canonical-assembly carves, not the many
# existing C/canonical-object islands between them.  Exact sizes and
# relocation inventories keep an accidental return to literal ``.word`` text
# from weakening this lane unnoticed.  Empty maps are intentional: six tiny
# gaps contain only padding/alignment instructions and need no relocations.
EXPECTED_CANONICAL_OBJECTS: dict[str, tuple[int, dict[int, int]]] = {
    "LIBAPI_4F9D4.s.o": (0x50, {R_MIPS_PC16: 1}),
    "CRT_SDK_4FA48.s.o": (
        0x4EB8,
        {R_MIPS_26: 285, R_MIPS_HI16: 508, R_MIPS_LO16: 508, R_MIPS_PC16: 154},
    ),
    "SDK_TEXT_5492C.s.o": (
        0xC4,
        {R_MIPS_26: 6, R_MIPS_HI16: 2, R_MIPS_LO16: 2},
    ),
    "SDK_TEXT_54B80.s.o": (
        0x1D4,
        {R_MIPS_26: 6, R_MIPS_HI16: 16, R_MIPS_LO16: 16, R_MIPS_PC16: 1},
    ),
    "SDK_TEXT_54DE4.s.o": (
        0x4B8,
        {R_MIPS_26: 7, R_MIPS_HI16: 2, R_MIPS_LO16: 2, R_MIPS_PC16: 32},
    ),
    "SDK_TEXT_5534C.s.o": (
        0x88,
        {R_MIPS_26: 5, R_MIPS_HI16: 3, R_MIPS_LO16: 3, R_MIPS_PC16: 5},
    ),
    "SDK_TEXT_55420.s.o": (0x4, {}),
    "SDK_TEXT_5544C.s.o": (0x8, {}),
    "SDK_TEXT_55478.s.o": (0xC, {}),
    "SDK_TEXT_554DC.s.o": (0x8, {}),
    "SDK_TEXT_55530.s.o": (0x4, {}),
    "SDK_TEXT_55618.s.o": (0xC, {}),
    "SDK_TEXT_556EC.s.o": (
        0x18,
        {R_MIPS_HI16: 1, R_MIPS_LO16: 1},
    ),
    "SDK_TEXT_55714.s.o": (
        0x5A0,
        {R_MIPS_26: 16, R_MIPS_HI16: 3, R_MIPS_LO16: 3, R_MIPS_PC16: 29},
    ),
    "SDK_TEXT_55D68.s.o": (
        0x59C,
        {R_MIPS_26: 13, R_MIPS_HI16: 15, R_MIPS_LO16: 15, R_MIPS_PC16: 16},
    ),
    "SDK_TEXT_58164.s.o": (
        0xF9F0,
        {R_MIPS_26: 668, R_MIPS_HI16: 837, R_MIPS_LO16: 837, R_MIPS_PC16: 624},
    ),
    "SDK_TEXT_67B78.s.o": (
        0x9C74,
        {R_MIPS_26: 374, R_MIPS_HI16: 598, R_MIPS_LO16: 598, R_MIPS_PC16: 164},
    ),
    "SDK_TEXT_71800.s.o": (
        0xAB4,
        {R_MIPS_26: 56, R_MIPS_HI16: 45, R_MIPS_LO16: 45, R_MIPS_PC16: 16},
    ),
    "SDK_TEXT_722E8.s.o": (
        0x8C8,
        {R_MIPS_26: 65, R_MIPS_HI16: 114, R_MIPS_LO16: 114, R_MIPS_PC16: 8},
    ),
    "SDK_TEXT_72CD0.s.o": (
        0x3294,
        {R_MIPS_26: 196, R_MIPS_HI16: 249, R_MIPS_LO16: 249, R_MIPS_PC16: 7},
    ),
}

EXPECTED_CANONICAL_TEXT_BYTES = 0x23ED8
EXPECTED_CANONICAL_RELOCATIONS = {
    R_MIPS_26: 1697,
    R_MIPS_HI16: 2393,
    R_MIPS_LO16: 2393,
    R_MIPS_PC16: 1057,
}

# The only raw high-half constants which still look like KSEG addresses after
# removing the artificial ``__override__prt`` function splits.  The first is a
# KSEG bit mask; the next two are packed GPU values; 0x80020009 is an SPU
# bitfield.  None is dereferenced as an in-image pointer.  Any new value in the
# 0x80000000..0x80ffffff range must be reviewed instead of silently accepted.
EXPECTED_HIGH_LITERAL_COUNTS = Counter(
    {0x80000000: 44, 0x80800000: 2, 0x80808081: 1, 0x80020009: 1}
)

# Two vendor signatures/return stubs are deliberately emitted as six literal
# words.  They move with .text but are not addresses and carry no relocation.
EXPECTED_LITERAL_WORDS = Counter(
    {
        0x25097350: 1,
        0x0043539B: 1,
        0x03E00008: 1,
        0x00000000: 1,
        0x25007350: 1,
        0x0043529B: 1,
    }
)

ELF_HEADER = struct.Struct("<16sHHIIIIIHHHHHH")
SECTION_HEADER = struct.Struct("<IIIIIIIIII")
SYMBOL_ENTRY = struct.Struct("<IIIBBH")
REL_ENTRY = struct.Struct("<II")


@dataclass(frozen=True)
class Section:
    name: str
    type: int
    flags: int
    address: int
    offset: int
    size: int
    link: int
    info: int
    alignment: int
    entry_size: int


@dataclass(frozen=True)
class Symbol:
    value: int
    section_index: int


class Elf32:
    """Small, strict ELF32 little-endian reader for the relocation gate."""

    def __init__(self, path: Path):
        self.path = path
        self.data = path.read_bytes()
        if len(self.data) < ELF_HEADER.size:
            raise LaneError(f"{path}: truncated ELF header")
        header = ELF_HEADER.unpack_from(self.data)
        ident = header[0]
        if ident[:4] != b"\x7fELF" or ident[4] != 1 or ident[5] != 1:
            raise LaneError(f"{path}: expected ELF32 little-endian input")
        if header[2] != 8:
            raise LaneError(f"{path}: expected EM_MIPS, got machine {header[2]}")

        section_offset = header[6]
        section_entry_size = header[11]
        section_count = header[12]
        section_names_index = header[13]
        if section_entry_size != SECTION_HEADER.size:
            raise LaneError(
                f"{path}: unexpected section-header size {section_entry_size}"
            )
        end = section_offset + section_count * section_entry_size
        if section_count == 0 or end > len(self.data):
            raise LaneError(f"{path}: invalid section-header table")

        raw_sections = [
            SECTION_HEADER.unpack_from(self.data, section_offset + index * section_entry_size)
            for index in range(section_count)
        ]
        if section_names_index >= len(raw_sections):
            raise LaneError(f"{path}: invalid section-name string table index")
        names_header = raw_sections[section_names_index]
        names = self._slice(names_header[4], names_header[5], "section-name table")

        self.sections: list[Section] = []
        for raw in raw_sections:
            self.sections.append(
                Section(
                    name=self._cstring(names, raw[0]),
                    type=raw[1],
                    flags=raw[2],
                    address=raw[3],
                    offset=raw[4],
                    size=raw[5],
                    link=raw[6],
                    info=raw[7],
                    alignment=raw[8],
                    entry_size=raw[9],
                )
            )

        self.symbols: dict[str, Symbol] = {}
        for section in self.sections:
            if section.type != SHT_SYMTAB:
                continue
            if section.link >= len(self.sections):
                raise LaneError(f"{path}: invalid symbol string-table link")
            strings_section = self.sections[section.link]
            strings = self._slice(
                strings_section.offset, strings_section.size, "symbol string table"
            )
            entry_size = section.entry_size or SYMBOL_ENTRY.size
            if entry_size != SYMBOL_ENTRY.size or section.size % entry_size:
                raise LaneError(f"{path}: malformed symbol table")
            table = self._slice(section.offset, section.size, "symbol table")
            for offset in range(0, len(table), entry_size):
                name_index, value, _size, _info, _other, section_index = (
                    SYMBOL_ENTRY.unpack_from(table, offset)
                )
                name = self._cstring(strings, name_index)
                if name:
                    self.symbols[name] = Symbol(value, section_index)

    def _slice(self, offset: int, size: int, description: str) -> bytes:
        end = offset + size
        if offset < 0 or size < 0 or end > len(self.data):
            raise LaneError(f"{self.path}: invalid {description} extent")
        return self.data[offset:end]

    @staticmethod
    def _cstring(table: bytes, offset: int) -> str:
        if offset >= len(table):
            return ""
        end = table.find(b"\0", offset)
        if end < 0:
            end = len(table)
        return table[offset:end].decode("ascii", errors="replace")

    def symbol(self, name: str) -> Symbol:
        try:
            return self.symbols[name]
        except KeyError as error:
            raise LaneError(f"{self.path}: missing symbol {name}") from error

    def section(self, name: str) -> Section:
        for section in self.sections:
            if section.name == name:
                return section
        raise LaneError(f"{self.path}: missing section {name}")

    def optional_section(self, name: str) -> Section | None:
        for section in self.sections:
            if section.name == name:
                return section
        return None

    def word(self, address: int) -> int:
        for section in self.sections:
            if (
                section.type != SHT_NOBITS
                and section.address <= address < section.address + section.size
            ):
                offset = section.offset + address - section.address
                raw = self._slice(offset, 4, f"word at 0x{address:08x}")
                return struct.unpack("<I", raw)[0]
        raise LaneError(f"{self.path}: address 0x{address:08x} is not file-backed")

    def relocation_counts(self, section_name: str) -> dict[int, int]:
        section = self.section(section_name)
        if section.type != SHT_REL:
            raise LaneError(f"{self.path}: {section_name} is not an SHT_REL section")
        entry_size = section.entry_size or REL_ENTRY.size
        if entry_size != REL_ENTRY.size or section.size % entry_size:
            raise LaneError(f"{self.path}: malformed relocation section {section_name}")
        table = self._slice(section.offset, section.size, section_name)
        counts: dict[int, int] = {}
        for offset in range(0, len(table), entry_size):
            _relocation_offset, info = REL_ENTRY.unpack_from(table, offset)
            relocation_type = info & 0xFF
            counts[relocation_type] = counts.get(relocation_type, 0) + 1
        return counts

    def optional_relocation_counts(self, section_name: str) -> dict[int, int]:
        if self.optional_section(section_name) is None:
            return {}
        return self.relocation_counts(section_name)


def add_probe(source: str, pad: int) -> str:
    """Insert a file-backed probe before the first canonical SDK input."""

    if pad not in (0, 4, 0x10004):
        raise LaneError(
            f"SDK relocation probe must be 0, 4, or 0x10004 bytes, got {pad}"
        )
    output: list[str] = []
    matches = 0
    for line in source.splitlines(keepends=True):
        if FIRST_SDK_INPUT in line:
            matches += 1
            if pad:
                indent = line[: len(line) - len(line.lstrip())]
                newline = "\r\n" if line.endswith("\r\n") else "\n"
                output.append(f"{indent}LONG(0x00000000);{newline}")
                if pad > 4:
                    output.append(f"{indent}. = . + 0x{pad - 4:x};{newline}")
        output.append(line)
    if matches != 1:
        raise LaneError(
            f"expected one SDK text input marker {FIRST_SDK_INPUT}, found {matches}"
        )
    return "".join(output)


def generate(
    linker_input: Path,
    symbols_input: Path,
    linker_output: Path,
    symbols_output: Path,
    *,
    pad: int = 0,
    expected_removed: int | None = None,
) -> int:
    filtered, removed = filter_symbols(
        symbols_input.read_text(), start=SDK_TEXT_START, end=SDK_TEXT_END
    )
    if expected_removed is not None and len(removed) != expected_removed:
        raise LaneError(
            f"expected {expected_removed} SDK-text aliases, found {len(removed)}"
        )
    rewritten = add_probe(linker_input.read_text(), pad)
    atomic_write(linker_output, rewritten)
    atomic_write(symbols_output, filtered)
    return len(removed)


def jump_target(word: int, pc: int) -> int:
    """Decode a MIPS J/JAL target in the current 256 MiB region."""

    return (((pc + 4) & 0xF0000000) | ((word & 0x03FFFFFF) << 2)) & 0xFFFFFFFF


def hi_lo_target(high_word: int, low_word: int) -> int:
    """Decode the value built by a LUI plus signed-immediate ADDIU pair."""

    low = low_word & 0xFFFF
    if low & 0x8000:
        low -= 0x10000
    return (((high_word & 0xFFFF) << 16) + low) & 0xFFFFFFFF


def require_symbol(
    base: Elf32,
    shifted: Elf32,
    name: str,
    retail_address: int,
    *,
    delta: int,
) -> None:
    base_symbol = base.symbol(name)
    shifted_symbol = shifted.symbol(name)
    if base_symbol.value != retail_address:
        raise LaneError(
            f"base {name}=0x{base_symbol.value:08x}, expected 0x{retail_address:08x}"
        )
    expected_shifted = (retail_address + delta) & 0xFFFFFFFF
    if shifted_symbol.value != expected_shifted:
        raise LaneError(
            f"shifted {name}=0x{shifted_symbol.value:08x}, "
            f"expected 0x{expected_shifted:08x}"
        )
    if base_symbol.section_index in (SHN_UNDEF, SHN_ABS):
        raise LaneError(f"base {name} is not section-owned")
    if shifted_symbol.section_index in (SHN_UNDEF, SHN_ABS):
        raise LaneError(f"shifted {name} is not section-owned")


def require_fixed_symbol(
    base: Elf32,
    shifted: Elf32,
    name: str,
    address: int,
    *,
    absolute: bool,
) -> None:
    base_symbol = base.symbol(name)
    shifted_symbol = shifted.symbol(name)
    if base_symbol.value != address or shifted_symbol.value != address:
        raise LaneError(f"{name} did not remain fixed at 0x{address:08x}")
    if absolute:
        if (
            base_symbol.section_index != SHN_ABS
            or shifted_symbol.section_index != SHN_ABS
        ):
            raise LaneError(f"{name} is fixed but not absolute in both links")
    elif any(
        symbol.section_index in (SHN_UNDEF, SHN_ABS)
        for symbol in (base_symbol, shifted_symbol)
    ):
        raise LaneError(f"{name} stayed fixed but is not section-owned in both links")


def require_jump(
    elf: Elf32,
    owner: str,
    offset: int,
    target: str,
    opcode: int,
) -> None:
    pc = (elf.symbol(owner).value + offset) & 0xFFFFFFFF
    word = elf.word(pc)
    actual_opcode = word >> 26
    if actual_opcode != opcode:
        raise LaneError(
            f"{elf.path}: {owner}+0x{offset:x} opcode {actual_opcode}, expected {opcode}"
        )
    actual_target = jump_target(word, pc)
    expected_target = elf.symbol(target).value
    if actual_target != expected_target:
        raise LaneError(
            f"{elf.path}: {owner}+0x{offset:x} targets 0x{actual_target:08x}, "
            f"expected {target}=0x{expected_target:08x}"
        )


def require_hi_lo(
    elf: Elf32,
    owner: str,
    offset: int,
    target: str,
) -> None:
    pc = (elf.symbol(owner).value + offset) & 0xFFFFFFFF
    high_word = elf.word(pc)
    low_word = elf.word(pc + 4)
    if high_word >> 26 != 0x0F or low_word >> 26 != 0x09:
        raise LaneError(
            f"{elf.path}: {owner}+0x{offset:x} is not LUI/ADDIU"
        )
    actual_target = hi_lo_target(high_word, low_word)
    expected_target = elf.symbol(target).value
    if actual_target != expected_target:
        raise LaneError(
            f"{elf.path}: {owner}+0x{offset:x} builds 0x{actual_target:08x}, "
            f"expected {target}=0x{expected_target:08x}"
        )


def require_words(elf: Elf32, owner: str, expected: tuple[int, ...]) -> None:
    """Check deliberately literal, non-address words carried in SDK text."""

    address = elf.symbol(owner).value
    actual = tuple(elf.word(address + index * 4) for index in range(len(expected)))
    if actual != expected:
        raise LaneError(
            f"{elf.path}: {owner} literal words "
            f"{[f'0x{word:08x}' for word in actual]}, expected "
            f"{[f'0x{word:08x}' for word in expected]}"
        )


def canonical_source_audit(source_paths: list[Path]) -> int:
    """Reject numeric code targets and unreviewed address-like literals."""

    by_name: dict[str, Path] = {}
    for path in source_paths:
        if path.name in by_name:
            raise LaneError(f"duplicate canonical SDK source {path.name}")
        by_name[path.name] = path

    expected_names = {name.removesuffix(".o") for name in EXPECTED_CANONICAL_OBJECTS}
    actual_names = set(by_name)
    if actual_names != expected_names:
        missing = sorted(expected_names - actual_names)
        extra = sorted(actual_names - expected_names)
        raise LaneError(
            f"canonical SDK source inventory changed; missing={missing}, extra={extra}"
        )

    high_literals: Counter[int] = Counter()
    literal_words: Counter[int] = Counter()
    numeric_jump = re.compile(r"\b(?:j|jal)\s+(?:0x[0-9a-f]+|[0-9]+)\b", re.I)
    high_literal = re.compile(
        r"\blui\s+\$[^,]+,\s*\(0x([0-9a-f]+)\s*>>\s*16\)", re.I
    )
    literal_word = re.compile(r"\.word\s+0x([0-9a-f]+)\b", re.I)

    for name in sorted(expected_names):
        path = by_name[name]
        source = path.read_text()
        if len(re.findall(r"^\.section\s+\.text\b", source, re.M)) != 1:
            raise LaneError(f"{path}: expected exactly one .text section directive")
        if re.search(r"^\.section\s+\.data\b", source, re.M):
            raise LaneError(f"{path}: canonical SDK text unexpectedly contains .data")
        match = numeric_jump.search(source)
        if match is not None:
            raise LaneError(f"{path}: numeric J/JAL operand {match.group(0)!r}")

        for match in high_literal.finditer(source):
            value = int(match.group(1), 16)
            if 0x80000000 <= value <= 0x80FFFFFF:
                high_literals[value] += 1
        for match in literal_word.finditer(source):
            literal_words[int(match.group(1), 16)] += 1

    if high_literals != EXPECTED_HIGH_LITERAL_COUNTS:
        raise LaneError(
            f"canonical SDK address-like high literals {dict(high_literals)}, "
            f"expected reviewed set {dict(EXPECTED_HIGH_LITERAL_COUNTS)}"
        )
    if literal_words != EXPECTED_LITERAL_WORDS:
        raise LaneError(
            f"canonical SDK literal words {dict(literal_words)}, "
            f"expected reviewed signatures {dict(EXPECTED_LITERAL_WORDS)}"
        )
    return sum(literal_words.values())


def canonical_object_inventory(
    object_paths: list[Path],
) -> tuple[int, dict[int, int]]:
    """Validate every generated canonical-assembly carve and total its relocs."""

    by_name: dict[str, Path] = {}
    for path in object_paths:
        if path.name in by_name:
            raise LaneError(f"duplicate canonical SDK object {path.name}")
        by_name[path.name] = path

    expected_names = set(EXPECTED_CANONICAL_OBJECTS)
    actual_names = set(by_name)
    if actual_names != expected_names:
        missing = sorted(expected_names - actual_names)
        extra = sorted(actual_names - expected_names)
        raise LaneError(
            f"canonical SDK object inventory changed; missing={missing}, extra={extra}"
        )

    total_size = 0
    total_counts: dict[int, int] = {}
    for name, (expected_size, expected_counts) in EXPECTED_CANONICAL_OBJECTS.items():
        elf = Elf32(by_name[name])
        text = elf.section(".text")
        if not text.flags & SHF_EXECINSTR:
            raise LaneError(f"{by_name[name]}: .text is not executable")
        if text.size != expected_size:
            raise LaneError(
                f"{by_name[name]}: .text size 0x{text.size:x}, "
                f"expected 0x{expected_size:x}"
            )
        counts = elf.optional_relocation_counts(".rel.text")
        if counts != expected_counts:
            raise LaneError(
                f"{by_name[name]}: relocation counts {counts}, "
                f"expected {expected_counts}"
            )
        total_size += text.size
        for relocation_type, count in counts.items():
            total_counts[relocation_type] = (
                total_counts.get(relocation_type, 0) + count
            )

    if total_size != EXPECTED_CANONICAL_TEXT_BYTES:
        raise LaneError(
            f"canonical SDK .text total 0x{total_size:x}, "
            f"expected 0x{EXPECTED_CANONICAL_TEXT_BYTES:x}"
        )
    if total_counts != EXPECTED_CANONICAL_RELOCATIONS:
        raise LaneError(
            f"canonical SDK relocation total {total_counts}, "
            f"expected {EXPECTED_CANONICAL_RELOCATIONS}"
        )
    return total_size, total_counts


def movable_symbol_inventory(symbols_path: Path) -> dict[str, int]:
    """Read the retail symbol script with the same strict range parser."""

    _filtered, removed = filter_symbols(
        symbols_path.read_text(), start=SDK_TEXT_START, end=SDK_TEXT_END
    )
    if len(removed) != EXPECTED_SDK_SYMBOLS:
        raise LaneError(
            f"expected {EXPECTED_SDK_SYMBOLS} SDK-text symbols, found {len(removed)}"
        )
    return removed


def verify(
    base_path: Path,
    shifted_path: Path,
    symbols_path: Path,
    object_paths: list[Path],
    source_paths: list[Path],
    *,
    delta: int,
) -> tuple[int, dict[int, int]]:
    """Prove the canonical SDK text stream reacts correctly to a probe."""

    base = Elf32(base_path)
    shifted = Elf32(shifted_path)
    if delta not in (4, 0x10004):
        raise LaneError(f"verification delta must be 4 or 0x10004, got {delta}")

    movable = movable_symbol_inventory(symbols_path)
    base_names = set(base.symbols)
    shifted_names = set(shifted.symbols)
    omitted_base = set(movable) - base_names
    omitted_shifted = set(movable) - shifted_names
    if omitted_base != omitted_shifted:
        raise LaneError(
            "base and shifted links omitted different SDK internal aliases"
        )
    if len(omitted_base) != EXPECTED_OMITTED_INTERNAL_ALIASES:
        raise LaneError(
            f"expected {EXPECTED_OMITTED_INTERNAL_ALIASES} obsolete internal "
            f"aliases to disappear, found {len(omitted_base)}"
        )
    emitted = {
        name: address
        for name, address in movable.items()
        if name not in omitted_base
    }
    if len(emitted) != EXPECTED_SDK_SECTION_SYMBOLS:
        raise LaneError(
            f"expected {EXPECTED_SDK_SECTION_SYMBOLS} emitted SDK-text symbols, "
            f"found {len(emitted)}"
        )
    for name, address in emitted.items():
        require_symbol(base, shifted, name, address, delta=delta)

    # The linker-owned game stayed before the probe.  UnitVector2 is the first
    # symbol in the remaining loaded-data input and is the checked fixed
    # boundary.
    require_fixed_symbol(base, shifted, "main", 0x800162A4, absolute=False)
    require_fixed_symbol(base, shifted, "UnitVector2", SDK_TEXT_END, absolute=True)

    # Exercise linked words from the early CRT, the first newly canonical
    # libgs carve, the middle libgte/libapi stream, and the final card input.
    # Targets which are code symbols move too; these checks therefore reject
    # a canonical source that merely preserved the retail immediate bits.
    for elf in (base, shifted):
        require_jump(elf, "__SN_ENTRY_POINT", 0x88, "InitHeap", 3)
        require_jump(elf, "__SN_ENTRY_POINT", 0x9C, "main", 3)
        require_jump(elf, "CdInit", 0x58, "EVENT_OBJ_80", 2)
        require_hi_lo(elf, "CdInit", 0x24, "EVENT_OBJ_CC")
        require_jump(elf, "GsSetLightMatrix", 0x58, "PushMatrix", 3)
        require_jump(elf, "InitGeom", 0x08, "_patch_gte", 3)
        require_hi_lo(elf, "_patch_gte", 0x30, "PATCHGTE_OBJ_AC")
        require_jump(elf, "UserFuncOpen", 0x28, "USERFUNC_OBJ_7C", 2)
        require_jump(elf, "_card_open", 0x08, "InitCARD", 3)
        require_hi_lo(elf, "_card_start", 0x20, "funcEvSpIOE")
        require_jump(elf, "StartPAD", 0x08, "StartPAD2", 3)
        require_hi_lo(elf, "FUN_80083538", 0x20, "func_800835E8")
        require_hi_lo(elf, "_ExitCard", 0x24, "D_80083A74")
        require_hi_lo(elf, "_ExitCard", 0x2C, "D_80083A80")
        require_jump(elf, "FUN_80085f0c", 0x14, "bzero", 3)

        # These six words are PsyQ signatures/return stubs, not addresses.
        # They travel with the section but intentionally carry no relocation.
        require_words(elf, "D_80077914", (0x25097350, 0x0043539B))
        require_words(
            elf,
            "INTR_OBJ_6B8",
            (0x03E00008, 0x00000000, 0x25007350, 0x0043529B),
        )

    canonical_source_audit(source_paths)
    return canonical_object_inventory(object_paths)


def arguments(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)

    generate_parser = commands.add_parser("generate")
    generate_parser.add_argument("--linker-in", type=Path, required=True)
    generate_parser.add_argument("--symbols-in", type=Path, required=True)
    generate_parser.add_argument("--linker-out", type=Path, required=True)
    generate_parser.add_argument("--symbols-out", type=Path, required=True)
    generate_parser.add_argument(
        "--pad", type=lambda value: int(value, 0), choices=(0, 4, 0x10004), default=0
    )

    verify_parser = commands.add_parser("verify")
    verify_parser.add_argument("--base-elf", type=Path, required=True)
    verify_parser.add_argument("--shifted-elf", type=Path, required=True)
    verify_parser.add_argument("--symbols-in", type=Path, required=True)
    verify_parser.add_argument(
        "--sdk-object", type=Path, action="append", required=True
    )
    verify_parser.add_argument(
        "--sdk-source", type=Path, action="append", required=True
    )
    verify_parser.add_argument(
        "--delta", type=lambda value: int(value, 0), choices=(4, 0x10004), required=True
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = arguments(argv)
    try:
        if args.command == "generate":
            removed = generate(
                args.linker_in,
                args.symbols_in,
                args.linker_out,
                args.symbols_out,
                pad=args.pad,
                expected_removed=EXPECTED_SDK_SYMBOLS,
            )
        else:
            text_size, counts = verify(
                args.base_elf,
                args.shifted_elf,
                args.symbols_in,
                args.sdk_object,
                args.sdk_source,
                delta=args.delta,
            )
    except (LaneError, OSError) as error:
        print(f"reloc-sdk lane: {error}")
        return 1
    if args.command == "generate":
        print(
            f"reloc-sdk lane: removed {removed} absolute SDK-text aliases; "
            f"probe={args.pad}"
        )
    else:
        probe = "+4" if args.delta == 4 else f"+0x{args.delta:x}"
        print(
            f"reloc-sdk lane: {probe} probe moved {EXPECTED_SDK_SECTION_SYMBOLS} "
            f"SDK-text symbols and removed {EXPECTED_OMITTED_INTERNAL_ALIASES} "
            f"obsolete aliases; repaired early/mid/late J/JAL and HI/LO references; "
            f"canonical .text=0x{text_size:x}, .rel.text={sum(counts.values())}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

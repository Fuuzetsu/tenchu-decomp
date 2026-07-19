#!/usr/bin/env python3
"""Generate the retail-exact canonical-assembly SDK-prefix link scripts.

This is the second, deliberately bounded relocation gate.  Its input is the
linker-owned game lane.  It removes absolute symbol assignments for the SDK
prefix now emitted by Splat as canonical assembly, and can insert controlled
probe sizes immediately before that prefix.  Code after the prefix is outside
this gate and still includes literal raw inputs.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import struct

try:
    from tools.reloc_game_lane import LaneError, atomic_write, filter_symbols
except ModuleNotFoundError:  # Direct invocation adds tools/, not the repo root.
    from reloc_game_lane import LaneError, atomic_write, filter_symbols


SDK_TEXT_START = 0x800601D4
SDK_TEXT_END = 0x80065100
EXPECTED_SDK_SYMBOLS = 261
FIRST_SDK_INPUT = "/LIBAPI_4F9D4.s.o(.text);"

SHN_UNDEF = 0
SHN_ABS = 0xFFF1
SHT_SYMTAB = 2
SHT_NOBITS = 8
SHT_REL = 9

R_MIPS_26 = 4
R_MIPS_HI16 = 5
R_MIPS_LO16 = 6
R_MIPS_PC16 = 10

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
            f"expected {expected_removed} SDK-prefix symbols, found {len(removed)}"
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


def verify(
    base_path: Path,
    shifted_path: Path,
    object_path: Path,
    *,
    delta: int,
) -> dict[int, int]:
    """Prove the canonical SDK prefix reacts correctly to a controlled probe."""

    base = Elf32(base_path)
    shifted = Elf32(shifted_path)
    sdk_object = Elf32(object_path)
    if delta not in (4, 0x10004):
        raise LaneError(f"verification delta must be 4 or 0x10004, got {delta}")

    movable = {
        "Exec": 0x800601D4,
        "PClseek": 0x80060224,
        "__SN_ENTRY_POINT": 0x80060268,
        "InitHeap": 0x800603E8,
        "PCread": 0x80060404,
        "_SN_read": 0x800604C4,
        "CdInit": 0x800605B4,
        "EVENT_OBJ_80": 0x80060634,
        "EVENT_OBJ_CC": 0x80060680,
        "GsInitCoord2param": 0x800650D4,
    }
    for name, address in movable.items():
        require_symbol(base, shifted, name, address, delta=delta)

    # The linker-owned game stayed before the probe.  The next SDK input is
    # deliberately outside this bounded gate and still absolute.
    require_fixed_symbol(base, shifted, "main", 0x800162A4, absolute=False)
    require_fixed_symbol(
        base, shifted, "GsSetLsMatrix", SDK_TEXT_END, absolute=True
    )

    # Exercise an internal JAL, an external JAL, an internal J, and an
    # internal code-address LUI/ADDIU pair in both links.  These are checks on
    # linked instruction words, not on source spelling.
    for elf in (base, shifted):
        require_jump(elf, "__SN_ENTRY_POINT", 0x88, "InitHeap", 3)
        require_jump(elf, "__SN_ENTRY_POINT", 0x9C, "main", 3)
        require_jump(elf, "PCread", 0x5C, "_SN_read", 3)
        require_jump(elf, "CdInit", 0x58, "EVENT_OBJ_80", 2)
        require_hi_lo(elf, "CdInit", 0x24, "EVENT_OBJ_CC")

    text = sdk_object.section(".text")
    if text.size != SDK_TEXT_END - 0x80060248:
        raise LaneError(
            f"{object_path}: .text size 0x{text.size:x}, expected 0x4eb8"
        )
    counts = sdk_object.relocation_counts(".rel.text")
    expected_counts = {
        R_MIPS_26: 285,
        R_MIPS_HI16: 498,
        R_MIPS_LO16: 498,
        R_MIPS_PC16: 154,
    }
    if counts != expected_counts:
        raise LaneError(
            f"{object_path}: relocation counts {counts}, expected {expected_counts}"
        )
    return counts


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
    verify_parser.add_argument("--sdk-object", type=Path, required=True)
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
            counts = verify(
                args.base_elf, args.shifted_elf, args.sdk_object, delta=args.delta
            )
    except (LaneError, OSError) as error:
        print(f"reloc-sdk lane: {error}")
        return 1
    if args.command == "generate":
        print(
            f"reloc-sdk lane: removed {removed} absolute SDK-prefix symbols; "
            f"probe={args.pad}"
        )
    else:
        probe = "+4" if args.delta == 4 else f"+0x{args.delta:x}"
        print(
            f"reloc-sdk lane: {probe} probe repaired J/JAL and HI/LO references; "
            f".rel.text={sum(counts.values())}"
        )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

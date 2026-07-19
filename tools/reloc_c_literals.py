#!/usr/bin/env python3
"""Audit the first symbolic-C objects used by the normal relink lane.

The retail matching sources for six functions contain numeric address
materialisation chosen solely to reproduce the shipped instruction schedule.
With ``TENCHU_RELOCATABLE`` defined, those same translation units use ordinary
symbolic C instead.  This verifier reads the resulting ELF objects directly
and requires the expected MIPS HI16/LO16 relocation records.  It also rejects
an unrelocated matching-only high half (``0x8009`` or ``0x800d``) in these
bounded objects.

This is a bounded object/link-input gate, not a claim that the whole executable
can grow yet.  Later raw code/data, BSS/$gp ownership, and PS-X EXE finalisation
remain separate normal-link blockers.
"""

from __future__ import annotations

import argparse
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
import struct

try:
    from tools.reloc_game_lane import atomic_write
except ModuleNotFoundError:  # Direct invocation adds tools/, not the repo root.
    from reloc_game_lane import atomic_write


EM_MIPS = 8
ELFCLASS32 = 1
ELFDATA2LSB = 1
SHN_UNDEF = 0
SHT_SYMTAB = 2
SHT_NOBITS = 8
SHT_REL = 9

R_MIPS_HI16 = 5
R_MIPS_LO16 = 6

ELF_HEADER = struct.Struct("<16sHHIIIIIHHHHHH")
SECTION_HEADER = struct.Struct("<IIIIIIIIII")
SYMBOL_ENTRY = struct.Struct("<IIIBBH")
REL_ENTRY = struct.Struct("<II")

EXPECTED_TEXT_SHRINK = 12
EXPECTED_LINKER_REFERENCES = 4
FIRST_SDK_TEXT_INPUT = ".shake/build/main.exe/LIBAPI_4F9D4.s.o(.text);"
SHN_ABS = 0xFFF1
SDK_TEXT_START = 0x800601D4
SDK_TEXT_END = 0x800834D0

# The natural objects start at the same place as their exact counterparts, then
# move later functions by the cumulative eight-byte shrink in ActivateHumans
# and SelectCameraOwnerOption.  A 12-byte boundary pad deliberately restores
# the still-raw SDK to retail placement.
LINKED_SYMBOL_DELTAS = {
    "valloc": 0,
    "vinit": 4,
    "ActivateHumans": 4,
    "DrawConstruction": -4,
    "ProcItemShinsoku": -4,
    "ReqItemShinsoku": -4,
    "SelectCameraOwnerOption": -4,
    "LayoutEnemyOption": -12,
    "FileOption": -12,
    "debug_menu_stage_option": -12,
    "AdtSelect": -12,
    "AdtDmyPadRead": -12,
    "Exec": 0,
}


@dataclass(frozen=True)
class Section:
    name: str
    type: int
    address: int
    offset: int
    size: int
    link: int
    info: int
    entry_size: int


@dataclass(frozen=True)
class Symbol:
    name: str
    value: int
    section_index: int


@dataclass(frozen=True)
class Relocation:
    offset: int
    type: int
    symbol: str


@dataclass(frozen=True)
class ObjectSpec:
    targets: dict[str, dict[int, int]]
    literal_high_halves: tuple[int, ...]


OBJECT_SPECS = {
    "SelectCameraOwnerOption": ObjectSpec(
        {"D_80097D70": {R_MIPS_HI16: 1, R_MIPS_LO16: 1}}, (0x8009,)
    ),
    "FileOption": ObjectSpec(
        {"D_80097D70": {R_MIPS_HI16: 1, R_MIPS_LO16: 1}}, (0x8009,)
    ),
    # The compiler births CamState's high half on both sides of a call, then
    # shares the one field load.  Both HI16 records are required evidence.
    "ProcItemShinsoku": ObjectSpec(
        {"CamState": {R_MIPS_HI16: 2, R_MIPS_LO16: 1}}, (0x8009,)
    ),
    "ActivateHumans": ObjectSpec(
        {"StageChar": {R_MIPS_HI16: 1, R_MIPS_LO16: 1}}, (0x8009,)
    ),
    "vinit": ObjectSpec(
        {"MemoryPool": {R_MIPS_HI16: 1, R_MIPS_LO16: 1}}, (0x800D,)
    ),
    "valloc": ObjectSpec(
        {"MemoryPool": {R_MIPS_HI16: 1, R_MIPS_LO16: 2}}, (0x800D,)
    ),
}


class AuditError(RuntimeError):
    """An ELF or relocation-lane invariant failed."""


class ElfObject:
    """Strict, minimal ELF32 little-endian reader for MIPS REL objects."""

    def __init__(self, path: Path):
        self.path = path
        self.data = path.read_bytes()
        if len(self.data) < ELF_HEADER.size:
            raise AuditError(f"{path}: truncated ELF header")

        header = ELF_HEADER.unpack_from(self.data)
        ident = header[0]
        if ident[:4] != b"\x7fELF":
            raise AuditError(f"{path}: not an ELF object")
        if ident[4] != ELFCLASS32 or ident[5] != ELFDATA2LSB:
            raise AuditError(f"{path}: expected ELF32 little-endian input")
        if header[2] != EM_MIPS:
            raise AuditError(f"{path}: expected EM_MIPS, got machine {header[2]}")

        section_offset = header[6]
        section_entry_size = header[11]
        section_count = header[12]
        section_names_index = header[13]
        if section_entry_size != SECTION_HEADER.size:
            raise AuditError(
                f"{path}: unexpected section-header size {section_entry_size}"
            )
        section_end = section_offset + section_count * section_entry_size
        if section_count == 0 or section_end > len(self.data):
            raise AuditError(f"{path}: invalid section-header table")

        raw_sections = [
            SECTION_HEADER.unpack_from(
                self.data, section_offset + index * section_entry_size
            )
            for index in range(section_count)
        ]
        if section_names_index >= section_count:
            raise AuditError(f"{path}: invalid section-name table index")
        names_header = raw_sections[section_names_index]
        section_names = self._slice(
            names_header[4], names_header[5], "section-name string table"
        )

        self.sections = [
            Section(
                name=self._cstring(section_names, raw[0]),
                type=raw[1],
                address=raw[3],
                offset=raw[4],
                size=raw[5],
                link=raw[6],
                info=raw[7],
                entry_size=raw[9],
            )
            for raw in raw_sections
        ]

        symbol_tables = [
            (index, section)
            for index, section in enumerate(self.sections)
            if section.type == SHT_SYMTAB
        ]
        if len(symbol_tables) != 1:
            raise AuditError(
                f"{path}: expected one symbol table, found {len(symbol_tables)}"
            )
        self.symbol_table_index, symbol_table = symbol_tables[0]
        if symbol_table.link >= len(self.sections):
            raise AuditError(f"{path}: invalid symbol string-table link")
        strings_section = self.sections[symbol_table.link]
        strings = self._slice(
            strings_section.offset, strings_section.size, "symbol string table"
        )
        entry_size = symbol_table.entry_size or SYMBOL_ENTRY.size
        if entry_size != SYMBOL_ENTRY.size or symbol_table.size % entry_size:
            raise AuditError(f"{path}: malformed symbol table")
        table = self._slice(symbol_table.offset, symbol_table.size, "symbol table")
        self.symbols: list[Symbol] = []
        for offset in range(0, len(table), entry_size):
            name_offset, value, _size, _info, _other, section_index = (
                SYMBOL_ENTRY.unpack_from(table, offset)
            )
            self.symbols.append(
                Symbol(self._cstring(strings, name_offset), value, section_index)
            )

    def _slice(self, offset: int, size: int, description: str) -> bytes:
        end = offset + size
        if offset < 0 or size < 0 or end > len(self.data):
            raise AuditError(f"{self.path}: invalid {description} extent")
        return self.data[offset:end]

    @staticmethod
    def _cstring(table: bytes, offset: int) -> str:
        if offset >= len(table):
            return ""
        end = table.find(b"\0", offset)
        if end < 0:
            end = len(table)
        return table[offset:end].decode("ascii", errors="replace")

    def section(self, name: str) -> Section:
        matches = [section for section in self.sections if section.name == name]
        if len(matches) != 1:
            raise AuditError(
                f"{self.path}: expected one {name} section, found {len(matches)}"
            )
        return matches[0]

    def section_data(self, name: str) -> bytes:
        section = self.section(name)
        return self._slice(section.offset, section.size, name)

    def symbol(self, name: str) -> Symbol:
        matches = [symbol for symbol in self.symbols if symbol.name == name]
        if len(matches) != 1:
            raise AuditError(
                f"{self.path}: expected one symbol {name}, found {len(matches)}"
            )
        return matches[0]

    def word_at_address(self, address: int) -> int:
        raw = self.bytes_at_address(address, 4)
        return struct.unpack("<I", raw)[0]

    def bytes_at_address(self, address: int, size: int) -> bytes:
        for section in self.sections:
            if (
                section.type != SHT_NOBITS
                and section.address <= address
                and address + size <= section.address + section.size
            ):
                offset = section.offset + address - section.address
                return self._slice(offset, size, "linked address range")
        raise AuditError(
            f"{self.path}: range 0x{address:08x}+0x{size:x} is not file-backed"
        )

    def file_backed_section_at(self, address: int) -> Section:
        matches = [
            section
            for section in self.sections
            if section.type != SHT_NOBITS
            and section.address <= address < section.address + section.size
        ]
        if len(matches) != 1:
            raise AuditError(
                f"{self.path}: expected one file-backed section at "
                f"0x{address:08x}, found {len(matches)}"
            )
        return matches[0]

    def relocations(self, name: str) -> list[Relocation]:
        section = self.section(name)
        if section.type != SHT_REL:
            raise AuditError(f"{self.path}: {name} is not an SHT_REL section")
        if section.link != self.symbol_table_index:
            raise AuditError(f"{self.path}: {name} does not link to the symbol table")
        entry_size = section.entry_size or REL_ENTRY.size
        if entry_size != REL_ENTRY.size or section.size % entry_size:
            raise AuditError(f"{self.path}: malformed {name}")
        table = self._slice(section.offset, section.size, name)
        output: list[Relocation] = []
        for offset in range(0, len(table), entry_size):
            relocation_offset, info = REL_ENTRY.unpack_from(table, offset)
            symbol_index = info >> 8
            if symbol_index >= len(self.symbols):
                raise AuditError(f"{self.path}: invalid relocation symbol index")
            output.append(
                Relocation(
                    offset=relocation_offset,
                    type=info & 0xFF,
                    symbol=self.symbols[symbol_index].name,
                )
            )
        return output


def instruction_word(text: bytes, offset: int, description: str) -> int:
    if offset < 0 or offset + 4 > len(text) or offset % 4:
        raise AuditError(f"{description}: relocation offset 0x{offset:x} is invalid")
    return struct.unpack_from("<I", text, offset)[0]


def find_literal_high_halves(
    text: bytes,
    relocated_hi_offsets: set[int],
    *,
    high_half: int,
) -> list[int]:
    """Return unrelocated LUI offsets for one matching-only numeric base."""

    findings: list[int] = []
    for offset in range(0, len(text) - 3, 4):
        word = struct.unpack_from("<I", text, offset)[0]
        if word >> 26 == 0x0F and word & 0xFFFF == high_half:
            if offset not in relocated_hi_offsets:
                findings.append(offset)
    return findings


def verify_contract(elf: ElfObject, spec: ObjectSpec, description: str) -> str:
    text = elf.section_data(".text")
    relocations = elf.relocations(".rel.text")
    hi_offsets = {
        relocation.offset
        for relocation in relocations
        if relocation.type == R_MIPS_HI16
    }

    for symbol_name, expected in spec.targets.items():
        symbol = elf.symbol(symbol_name)
        if symbol.section_index != SHN_UNDEF:
            raise AuditError(
                f"{description}: {symbol_name} is not an undefined linker-owned symbol"
            )
        actual = Counter(
            relocation.type
            for relocation in relocations
            if relocation.symbol == symbol_name
        )
        if actual != Counter(expected):
            raise AuditError(
                f"{description}: {symbol_name} relocations {dict(actual)}, "
                f"expected {expected}"
            )

        for relocation in relocations:
            if relocation.symbol != symbol_name:
                continue
            word = instruction_word(text, relocation.offset, description)
            opcode = word >> 26
            if relocation.type == R_MIPS_HI16 and opcode != 0x0F:
                raise AuditError(
                    f"{description}: HI16 at 0x{relocation.offset:x} is not LUI"
                )
            if relocation.type == R_MIPS_LO16 and opcode not in (0x09, 0x23):
                raise AuditError(
                    f"{description}: LO16 at 0x{relocation.offset:x} is neither "
                    "ADDIU nor LW"
                )

    for high_half in spec.literal_high_halves:
        literals = find_literal_high_halves(text, hi_offsets, high_half=high_half)
        if literals:
            rendered = ", ".join(f"0x{offset:x}" for offset in literals)
            raise AuditError(
                f"{description}: unrelocated LUI 0x{high_half:04x} at "
                f"text offset(s) {rendered}"
            )

    details = []
    for symbol_name, expected in spec.targets.items():
        details.append(
            f"{symbol_name} HI16={expected.get(R_MIPS_HI16, 0)} "
            f"LO16={expected.get(R_MIPS_LO16, 0)}"
        )
    return f"{description}: " + ", ".join(details)


def rewrite_linker(
    source: str,
    reference_objects: dict[str, Path],
    variant_objects: dict[str, Path],
    *,
    padding: int,
) -> str:
    """Substitute every audited object section and optionally pad the shrink."""

    if padding not in (0, EXPECTED_TEXT_SHRINK) or padding % 4:
        raise AuditError(
            "normal-C linker padding is "
            f"{padding}, expected 0 or {EXPECTED_TEXT_SHRINK}"
        )
    output = source
    for name in OBJECT_SPECS:
        old = str(reference_objects[name])
        new = str(variant_objects[name])
        count = output.count(old)
        if count != EXPECTED_LINKER_REFERENCES:
            raise AuditError(
                f"linker contains {count} references to {old}, "
                f"expected {EXPECTED_LINKER_REFERENCES}"
            )
        output = output.replace(old, new)

    lines = output.splitlines(keepends=True)
    marker_count = sum(FIRST_SDK_TEXT_INPUT in line for line in lines)
    if marker_count != 1:
        raise AuditError(
            f"expected one SDK text marker {FIRST_SDK_TEXT_INPUT}, "
            f"found {marker_count}"
        )
    padded: list[str] = []
    for line in lines:
        if FIRST_SDK_TEXT_INPUT in line:
            indent = line[: len(line) - len(line.lstrip())]
            newline = "\r\n" if line.endswith("\r\n") else "\n"
            for _ in range(padding // 4):
                padded.append(f"{indent}LONG(0x00000000);{newline}")
        padded.append(line)
    return "".join(padded)


def text_shrink(
    reference_objects: dict[str, Path],
    variant_objects: dict[str, Path],
) -> int:
    shrink = 0
    for name in OBJECT_SPECS:
        reference_size = ElfObject(reference_objects[name]).section(".text").size
        variant_size = ElfObject(variant_objects[name]).section(".text").size
        shrink += reference_size - variant_size
    return shrink


def generate_linker(
    linker_input: Path,
    linker_output: Path,
    reference_objects: dict[str, Path],
    variant_objects: dict[str, Path],
    *,
    restore_boundary: bool = True,
) -> int:
    shrink = text_shrink(reference_objects, variant_objects)
    if restore_boundary and shrink != EXPECTED_TEXT_SHRINK:
        raise AuditError(
            f"normal-C text shrink is {shrink}, expected {EXPECTED_TEXT_SHRINK}"
        )
    rewritten = rewrite_linker(
        linker_input.read_text(),
        reference_objects,
        variant_objects,
        padding=shrink if restore_boundary else 0,
    )
    atomic_write(linker_output, rewritten)
    return shrink


def sign_extend_16(value: int) -> int:
    value &= 0xFFFF
    return value - 0x10000 if value & 0x8000 else value


def verify_linked(
    base: ElfObject,
    variant: ElfObject,
    objects: dict[str, Path],
) -> list[str]:
    """Prove the natural objects participate in the linked placement probe."""

    reports: list[str] = []
    for name, delta in LINKED_SYMBOL_DELTAS.items():
        base_symbol = base.symbol(name)
        variant_symbol = variant.symbol(name)
        expected = (base_symbol.value + delta) & 0xFFFFFFFF
        if variant_symbol.value != expected:
            raise AuditError(
                f"linked {name}=0x{variant_symbol.value:08x}, "
                f"expected 0x{expected:08x} ({delta:+d})"
            )
        if name != "Exec" and variant_symbol.section_index in (SHN_UNDEF, SHN_ABS):
            raise AuditError(f"linked {name} is not section-owned")
        reports.append(f"{name} {delta:+d}")

    exec_address = base.symbol("Exec").value
    base_tail_section = base.file_backed_section_at(exec_address)
    variant_tail_section = variant.file_backed_section_at(exec_address)
    base_end = base_tail_section.address + base_tail_section.size
    variant_end = variant_tail_section.address + variant_tail_section.size
    if variant_end != base_end:
        raise AuditError(
            f"linked file-backed end 0x{variant_end:08x}, expected 0x{base_end:08x}"
        )
    pad_read_address = base.symbol("AdtPadRead").value
    variant_pad_read_address = variant.symbol("AdtPadRead").value
    if variant_pad_read_address != pad_read_address:
        raise AuditError(
            f"linked AdtPadRead=0x{variant_pad_read_address:08x}, "
            f"expected 0x{pad_read_address:08x}"
        )
    if not exec_address <= pad_read_address or pad_read_address + 4 > base_end:
        raise AuditError("linked AdtPadRead is outside the post-Exec file-backed range")

    base_pad_target = base.symbol("AdtDmyPadRead").value
    variant_pad_target = variant.symbol("AdtDmyPadRead").value
    base_pad_word = base.word_at_address(pad_read_address)
    variant_pad_word = variant.word_at_address(pad_read_address)
    if base_pad_word != base_pad_target:
        raise AuditError(
            f"retail AdtPadRead=0x{base_pad_word:08x}, "
            f"expected AdtDmyPadRead=0x{base_pad_target:08x}"
        )
    if variant_pad_word != variant_pad_target:
        raise AuditError(
            f"linked AdtPadRead=0x{variant_pad_word:08x}, "
            f"expected AdtDmyPadRead=0x{variant_pad_target:08x}"
        )

    base_symbols_by_value: dict[int, set[str]] = {}
    variant_symbols_by_value: dict[int, set[str]] = {}
    for elf, output in (
        (base, base_symbols_by_value),
        (variant, variant_symbols_by_value),
    ):
        for symbol in elf.symbols:
            if symbol.section_index in (SHN_UNDEF, SHN_ABS):
                continue
            output.setdefault(symbol.value, set()).add(symbol.name)

    changed_pointer_words = 0
    for address in range(exec_address, base_end, 4):
        base_word = base.word_at_address(address)
        variant_word = variant.word_at_address(address)
        if base_word == variant_word:
            continue
        common_targets = (
            base_symbols_by_value.get(base_word, set())
            & variant_symbols_by_value.get(variant_word, set())
        )
        if not common_targets:
            raise AuditError(
                f"linked post-Exec word changed unexpectedly at 0x{address:08x}: "
                f"0x{base_word:08x} -> 0x{variant_word:08x}"
            )
        changed_pointer_words += 1
    tail_size = base_end - exec_address
    reports.append(
        f"post-Exec 0x{tail_size:x} bytes changed only in "
        f"{changed_pointer_words} section-owned pointer words"
    )

    reports.extend(verify_linked_relocations(variant, objects))
    return reports


def verify_linked_relocations(
    variant: ElfObject,
    objects: dict[str, Path],
) -> list[str]:
    """Require every audited compiler relocation to be applied in one link."""

    reports: list[str] = []
    for function_name, spec in OBJECT_SPECS.items():
        obj = ElfObject(objects[function_name])
        relocations = obj.relocations(".rel.text")
        function_address = variant.symbol(function_name).value
        for target_name in spec.targets:
            target_relocations = [
                relocation
                for relocation in relocations
                if relocation.symbol == target_name
            ]
            low_relocations = [
                relocation
                for relocation in target_relocations
                if relocation.type == R_MIPS_LO16
            ]
            if not low_relocations:
                raise AuditError(
                    f"{function_name}: expected at least one {target_name} LO16 addend"
                )
            object_text = obj.section_data(".text")
            addends = {
                sign_extend_16(
                    instruction_word(object_text, relocation.offset, function_name)
                )
                for relocation in low_relocations
            }
            if len(addends) != 1:
                raise AuditError(
                    f"{function_name}: {target_name} LO16 addends differ: "
                    + ", ".join(f"{addend:+d}" for addend in sorted(addends))
                )
            addend = next(iter(addends))
            target = (variant.symbol(target_name).value + addend) & 0xFFFFFFFF
            expected_high = ((target + 0x8000) >> 16) & 0xFFFF
            expected_low = target & 0xFFFF

            for relocation in target_relocations:
                linked_word = variant.word_at_address(
                    function_address + relocation.offset
                )
                immediate = linked_word & 0xFFFF
                expected_immediate = (
                    expected_high
                    if relocation.type == R_MIPS_HI16
                    else expected_low
                )
                if immediate != expected_immediate:
                    raise AuditError(
                        f"linked {function_name} {target_name} relocation at "
                        f"0x{relocation.offset:x} has 0x{immediate:04x}, "
                        f"expected 0x{expected_immediate:04x}"
                    )
            reports.append(f"{function_name}->{target_name}=0x{target:08x}")
    return reports


def _unique_section_symbols(elf: ElfObject, start: int, end: int) -> list[Symbol]:
    candidates = [
        symbol
        for symbol in elf.symbols
        if start <= symbol.value < end
        and symbol.section_index not in (SHN_UNDEF, SHN_ABS)
    ]
    counts = Counter(symbol.name for symbol in candidates)
    return [symbol for symbol in candidates if counts[symbol.name] == 1]


def verify_normal_link(
    base: ElfObject,
    variant: ElfObject,
    reference_objects: dict[str, Path],
    objects: dict[str, Path],
    linker_source: str,
) -> list[str]:
    """Verify that the no-pad C objects reached the composed normal link.

    The extension section is allowed to have any size.  Movement of the SDK is
    derived from the compiler objects' measured net text change rather than a
    fixed address delta, so adding a normal-link helper does not weaken this
    check.
    """

    shrink = text_shrink(reference_objects, objects)
    if shrink % 4:
        raise AuditError(f"normal-C text change {shrink} is not word-aligned")

    lines = linker_source.splitlines()
    markers = [index for index, line in enumerate(lines) if FIRST_SDK_TEXT_INPUT in line]
    if len(markers) != 1:
        raise AuditError(
            f"normal linker has {len(markers)} SDK boundary markers, expected one"
        )
    marker = markers[0]
    if marker and "LONG(0x00000000)" in lines[marker - 1]:
        raise AuditError("normal linker still pads the C/SDK boundary")

    delta = -shrink
    sdk_symbols = _unique_section_symbols(base, SDK_TEXT_START, SDK_TEXT_END)
    if not sdk_symbols:
        raise AuditError("base link has no section-owned SDK symbols")
    for symbol in sdk_symbols:
        shifted = variant.symbol(symbol.name)
        expected = (symbol.value + delta) & 0xFFFFFFFF
        if shifted.value != expected:
            raise AuditError(
                f"normal-link {symbol.name}=0x{shifted.value:08x}, "
                f"expected 0x{expected:08x} ({delta:+d})"
            )
        if shifted.section_index in (SHN_UNDEF, SHN_ABS):
            raise AuditError(f"normal-link {symbol.name} is not section-owned")

    extension_start = variant.symbol("__tenchu_extension_start")
    extension_end = variant.symbol("__tenchu_extension_end")
    initialized_end = variant.symbol("main_exe_INITIALIZED_END")
    bss_start = variant.symbol("__bss_start")
    bss_end = variant.symbol("__bss_end")
    crt0_bss_end = variant.symbol("D_800CDBA8")
    heap_start = variant.symbol("HEAP_START")
    memory_pool = variant.symbol("MemoryPool")
    boundaries = (
        extension_start,
        extension_end,
        initialized_end,
        bss_start,
        bss_end,
        crt0_bss_end,
        heap_start,
        memory_pool,
    )
    if any(symbol.section_index in (SHN_UNDEF, SHN_ABS) for symbol in boundaries):
        raise AuditError("normal-link extension/BSS/pool boundary is not section-owned")
    if extension_end.value < extension_start.value:
        raise AuditError("normal-link extension end precedes its start")
    if initialized_end.value != extension_end.value:
        raise AuditError("initialized end does not follow the normal-link extension")
    expected_bss_start = (initialized_end.value + 15) & ~15
    if bss_start.value != expected_bss_start:
        raise AuditError(
            f"BSS starts at 0x{bss_start.value:08x}, "
            f"expected aligned 0x{expected_bss_start:08x}"
        )
    if crt0_bss_end.value != bss_end.value:
        raise AuditError("crt0 clear end does not follow linked BSS end")
    if heap_start.value != bss_end.value + 4:
        raise AuditError("heap start does not follow linked BSS end")
    if bss_end.value > memory_pool.value:
        raise AuditError("linked BSS overlaps MemoryPool")

    reports = [
        f"normal C changed text by {delta:+d} bytes with no boundary pad",
        f"{len(sdk_symbols)} unique section-owned SDK symbols followed {delta:+d}",
        "extension size "
        f"0x{extension_end.value - extension_start.value:x}; "
        f"BSS 0x{bss_start.value:08x}..0x{bss_end.value:08x}",
    ]
    reports.extend(verify_linked_relocations(variant, objects))

    stale_targets = []
    for name in ("D_80097D70", "CamState", "StageChar"):
        symbol = variant.symbol(name)
        if symbol.section_index == SHN_ABS:
            stale_targets.append(name)
    if stale_targets:
        reports.append(
            "known runtime blockers remain ABS: " + ", ".join(stale_targets)
        )
    return reports


def parse_objects(values: list[str], *, option: str = "--object") -> dict[str, Path]:
    output: dict[str, Path] = {}
    for value in values:
        name, separator, raw_path = value.partition("=")
        if not separator or not name or not raw_path:
            raise AuditError(f"invalid {option} {value!r}; expected NAME=PATH")
        if name in output:
            raise AuditError(f"duplicate {option} {name}")
        output[name] = Path(raw_path)
    expected = set(OBJECT_SPECS)
    actual = set(output)
    if actual != expected:
        missing = ", ".join(sorted(expected - actual)) or "none"
        extra = ", ".join(sorted(actual - expected)) or "none"
        raise AuditError(f"object inventory differs: missing {missing}; extra {extra}")
    return output


def arguments(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    verify_objects = subparsers.add_parser(
        "verify-objects", help="audit the six compiler-produced ELF objects"
    )
    verify_objects.add_argument(
        "--object",
        action="append",
        default=[],
        metavar="NAME=PATH",
        help="one of the fixed symbolic-C object inputs",
    )

    generate = subparsers.add_parser(
        "generate-linker", help="substitute the natural objects in the game lane"
    )
    generate.add_argument("--linker-in", type=Path, required=True)
    generate.add_argument("--linker-out", type=Path, required=True)
    generate.add_argument(
        "--reference-object", action="append", default=[], metavar="NAME=PATH"
    )
    generate.add_argument(
        "--object", action="append", default=[], metavar="NAME=PATH"
    )
    generate.add_argument(
        "--no-boundary-pad",
        action="store_true",
        help=(
            "substitute the natural objects without restoring their text shrink; "
            "used only by the composed normal relink"
        ),
    )

    verify_link = subparsers.add_parser(
        "verify-linked", help="audit the linked placement/substitution probe"
    )
    verify_link.add_argument("--base-elf", type=Path, required=True)
    verify_link.add_argument("--variant-elf", type=Path, required=True)
    verify_link.add_argument(
        "--object", action="append", default=[], metavar="NAME=PATH"
    )

    verify_normal = subparsers.add_parser(
        "verify-normal-link",
        help="audit the no-pad C objects in the composed growth-capable link",
    )
    verify_normal.add_argument("--base-elf", type=Path, required=True)
    verify_normal.add_argument("--variant-elf", type=Path, required=True)
    verify_normal.add_argument("--linker", type=Path, required=True)
    verify_normal.add_argument(
        "--reference-object", action="append", default=[], metavar="NAME=PATH"
    )
    verify_normal.add_argument(
        "--object", action="append", default=[], metavar="NAME=PATH"
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = arguments(argv)
    try:
        if args.command == "verify-objects":
            objects = parse_objects(args.object)
            reports = [
                verify_contract(ElfObject(objects[name]), OBJECT_SPECS[name], name)
                for name in sorted(OBJECT_SPECS)
            ]
            print("reloc-c-literals: verified six symbolic-C objects")
            for report in reports:
                print(f"  {report}")
        elif args.command == "generate-linker":
            objects = parse_objects(args.object)
            references = parse_objects(
                args.reference_object, option="--reference-object"
            )
            shrink = generate_linker(
                args.linker_in,
                args.linker_out,
                references,
                objects,
                restore_boundary=not args.no_boundary_pad,
            )
            if args.no_boundary_pad:
                print(
                    "reloc-c-literals: substituted six symbolic-C objects; "
                    f"left compiler text delta {-shrink:+d} bytes linker-owned"
                )
            else:
                print(
                    "reloc-c-literals: substituted six symbolic-C objects; "
                    f"restored SDK boundary with {shrink} bytes"
                )
        elif args.command == "verify-linked":
            objects = parse_objects(args.object)
            reports = verify_linked(
                ElfObject(args.base_elf), ElfObject(args.variant_elf), objects
            )
            print("reloc-c-literals: linked substitution probe verified")
            for report in reports:
                print(f"  {report}")
        elif args.command == "verify-normal-link":
            objects = parse_objects(args.object)
            references = parse_objects(
                args.reference_object, option="--reference-object"
            )
            reports = verify_normal_link(
                ElfObject(args.base_elf),
                ElfObject(args.variant_elf),
                references,
                objects,
                args.linker.read_text(),
            )
            print("reloc-c-literals: composed normal link verified")
            for report in reports:
                print(f"  {report}")
        else:  # argparse enforces the choices; keep type-checkers honest.
            raise AuditError(f"unknown command {args.command}")
    except (AuditError, OSError) as error:
        print(f"reloc-c-literals: {error}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

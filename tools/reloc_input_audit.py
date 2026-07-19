#!/usr/bin/env python3
"""Reject fixed movable MAIN addresses in the normal-link input objects.

This gate reads the exact object/section inventory from the generated normal
linker script, then reads MIPS ELF relocation records from those objects.  It
does not infer safety from final linked bytes: a symbolic reference must still
be visible as a relocation in its input object.

The checks are deliberately narrow and machine-verifiable:

* every direct ``j``/``jal`` instruction needs ``R_MIPS_26``;
* an unrelocated LUI, optionally followed by a canonical ADDI(U), ORI, or
  load/store low half, may not form an address in movable MAIN RAM;
* an aligned word in a linked alloc-data section which targets movable MAIN
  needs ``R_MIPS_32``.

Reviewed fixed PS1 contracts (persistent save state, executable handoff,
scratchpad/MMIO, RAM end and initial stack) are reported but accepted.  The
PS-X EXE header's entry/load placeholders are accepted because ``psxexe.py``
regenerates those fields after every link.  One stock libsnd arithmetic magic
constant is accepted by exact object and value.

This is a regression gate, not a type-system proof.  In particular it does
not try to prove arbitrary register arithmetic, byte-unaligned data fields, or
random packed assembly data.  Those limits are printed in ``--help`` and are
covered by the separate manifest/growth gates where the current image needs
them.
"""

from __future__ import annotations

import argparse
from collections import Counter
from dataclasses import asdict, dataclass
import fnmatch
import json
import os
from pathlib import Path
import re
import struct
import sys
from typing import Iterable


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_LINKER = Path(".shake/build/relink/layout/main.exe.ld")
DEFAULT_ELF = Path(".shake/build/tenchu/main_relink.exe.elf")
DEFAULT_MAP = Path(".shake/build/tenchu/main_relink.exe.map")

EM_MIPS = 8
ET_REL = 1
ET_EXEC = 2
SHT_PROGBITS = 1
SHT_SYMTAB = 2
SHT_NOBITS = 8
SHT_REL = 9
SHT_MIPS_REGINFO = 0x70000006
SHT_MIPS_ABIFLAGS = 0x7000002A
SHF_ALLOC = 0x2
SHF_EXECINSTR = 0x4
SHN_UNDEF = 0

R_MIPS_32 = 2
R_MIPS_26 = 4
R_MIPS_HI16 = 5
R_MIPS_LO16 = 6

ELF_HEADER = struct.Struct("<16sHHIIIIIHHHHHH")
SECTION_HEADER = struct.Struct("<IIIIIIIIII")
SYMBOL_ENTRY = struct.Struct("<IIIBBH")
REL_ENTRY = struct.Struct("<II")

LINKER_INPUT_RE = re.compile(
    r"(?P<object>[^\s;()]+\.o)\s*\((?P<sections>[^()]*)\)"
)
MAP_LOAD_RE = re.compile(r"^LOAD\s+(?P<object>.+\.o)\s*$")
GLOB_MAGIC_RE = re.compile(r"[*?[]")

MAIN_START = 0x80011000
HANDOFF_START = 0x80100000
PERSISTENT_START = 0x80010000
PERSISTENT_END = 0x80010E70
HANDOFF_END = 0x8010005C
SCRATCH_START = 0x1F800000
SCRATCH_END = 0x1F800400
MMIO_START = 0x1F801000
MMIO_END = 0x1F803000
INITIAL_STACK = 0x801FFFF0
RAM_END_VALUES = frozenset({0x80200000, 0x80200008})

# _SsVmSetSeqVol uses this as a multiply/divide magic value, not an address.
# Scope the exception to the canonical stock-SDK object rather than blessing
# the value in ordinary game C.
SAFE_NUMERIC_CONSTANTS = {
    ("SDK_TEXT_58164.s.o", 0x80020009): "libsnd arithmetic constant",
}

# psxexe.py regenerates PC at +0x10 and load address at +0x18 from the linked
# ELF.  No other literal in the header is exempt.
HEADER_PLACEHOLDERS = {
    ("header.s.o", ".data", 0x10): "PS-X EXE entry placeholder",
    ("header.s.o", ".data", 0x18): "PS-X EXE load placeholder",
}

# The strict linker script selects these into an INFO output guarded by a
# zero-size assertion. They are therefore owned only while empty; an unknown
# zero-sized alloc section is still rejected rather than silently discarded.
EXPECTED_EMPTY_INPUT_SECTIONS = frozenset(
    {
        ".text",
        ".data",
        ".rodata",
        ".sdata",
        ".bss",
        ".sbss",
        ".scommon",
        ".rel.dyn",
        ".rela.dyn",
    }
)

MOVABLE_OUTPUT_SECTIONS = frozenset(
    {".main_exe", ".main_exe_extension", ".main_exe_bss"}
)
MOVABLE_BOUNDARY_SYMBOLS = frozenset(
    {"D_800CDBA8", "main_exe_BSS_END", "HEAP_START", "MemoryPool"}
)

# Immediate instructions whose rs register is an address base.  Treat all
# ordinary and coprocessor loads/stores as signed-offset address uses.
MEMORY_OPCODES = frozenset(range(0x20, 0x40))
STORE_OPCODES = frozenset({0x28, 0x29, 0x2A, 0x2B, 0x2E, 0x2F}) | frozenset(
    {0x38, 0x39, 0x3A, 0x3B, 0x3E, 0x3F}
)


class AuditError(RuntimeError):
    """The requested object-level audit could not be completed."""


@dataclass(frozen=True)
class Section:
    index: int
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
    name: str
    value: int
    size: int
    info: int
    section_index: int


@dataclass(frozen=True)
class Relocation:
    offset: int
    type: int
    symbol: str


@dataclass(frozen=True)
class ObjectSelection:
    path: Path
    display: str
    patterns: tuple[str, ...]


@dataclass(frozen=True)
class LowUse:
    offset: int
    kind: str
    target: int


@dataclass(frozen=True)
class Finding:
    kind: str
    object: str
    section: str
    offset: str
    word: str
    target: str | None
    detail: str


@dataclass
class Counts:
    objects: int = 0
    reviewed_alloc_metadata_sections: int = 0
    selected_alloc_sections: int = 0
    executable_sections: int = 0
    data_sections: int = 0
    direct_jumps: int = 0
    relocated_direct_jumps: int = 0
    symbolic_hi16: int = 0
    data_words: int = 0
    symbolic_data_words: int = 0


class Elf32:
    """Strict minimal ELF32 little-endian reader for MIPS REL/EXEC files."""

    def __init__(self, path: Path):
        self.path = path
        try:
            self.data = path.read_bytes()
        except OSError as error:
            raise AuditError(f"cannot read {path}: {error}") from error
        if len(self.data) < ELF_HEADER.size:
            raise AuditError(f"{path}: truncated ELF header")

        header = ELF_HEADER.unpack_from(self.data)
        ident = header[0]
        if ident[:4] != b"\x7fELF" or ident[4] != 1 or ident[5] != 1:
            raise AuditError(f"{path}: expected ELF32 little-endian input")
        self.elf_type = header[1]
        if header[2] != EM_MIPS:
            raise AuditError(f"{path}: expected EM_MIPS, got machine {header[2]}")

        section_offset = header[6]
        section_entry_size = header[11]
        section_count = header[12]
        names_index = header[13]
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
        if names_index >= section_count:
            raise AuditError(f"{path}: invalid section-name table index")
        names_header = raw_sections[names_index]
        names = self._slice(names_header[4], names_header[5], "section names")
        self.sections = [
            Section(
                index=index,
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
            for index, raw in enumerate(raw_sections)
        ]

        self.symbol_tables: dict[int, list[Symbol]] = {}
        self.symbols: list[Symbol] = []
        for section in self.sections:
            if section.type != SHT_SYMTAB:
                continue
            if section.link >= len(self.sections):
                raise AuditError(f"{path}: invalid symbol string-table link")
            strings_section = self.sections[section.link]
            strings = self.section_data(strings_section)
            entry_size = section.entry_size or SYMBOL_ENTRY.size
            if entry_size != SYMBOL_ENTRY.size or section.size % entry_size:
                raise AuditError(f"{path}: malformed symbol table")
            table = self.section_data(section)
            symbols: list[Symbol] = []
            for offset in range(0, len(table), entry_size):
                name_offset, value, size, info, _other, section_index = (
                    SYMBOL_ENTRY.unpack_from(table, offset)
                )
                symbols.append(
                    Symbol(
                        name=self._cstring(strings, name_offset),
                        value=value & 0xFFFFFFFF,
                        size=size,
                        info=info,
                        section_index=section_index,
                    )
                )
            self.symbol_tables[section.index] = symbols
            self.symbols.extend(symbols)

    def _slice(self, offset: int, size: int, description: str) -> bytes:
        end = offset + size
        if offset < 0 or size < 0 or end > len(self.data):
            raise AuditError(f"{self.path}: invalid {description} extent")
        return self.data[offset:end]

    @staticmethod
    def _cstring(table: bytes, offset: int) -> str:
        if offset < 0 or offset >= len(table):
            return ""
        end = table.find(b"\0", offset)
        if end < 0:
            end = len(table)
        return table[offset:end].decode("ascii", errors="replace")

    def section_data(self, section: Section) -> bytes:
        if section.type == SHT_NOBITS:
            return b""
        return self._slice(section.offset, section.size, section.name or "section")

    def relocations_for(self, target: Section) -> list[Relocation]:
        output: list[Relocation] = []
        for section in self.sections:
            if section.type != SHT_REL or section.info != target.index:
                continue
            symbols = self.symbol_tables.get(section.link)
            if symbols is None:
                raise AuditError(
                    f"{self.path}: {section.name} has no linked symbol table"
                )
            entry_size = section.entry_size or REL_ENTRY.size
            if entry_size != REL_ENTRY.size or section.size % entry_size:
                raise AuditError(f"{self.path}: malformed {section.name}")
            table = self.section_data(section)
            for entry_offset in range(0, len(table), entry_size):
                offset, info = REL_ENTRY.unpack_from(table, entry_offset)
                symbol_index = info >> 8
                if symbol_index >= len(symbols):
                    raise AuditError(
                        f"{self.path}: invalid symbol in {section.name}"
                    )
                output.append(
                    Relocation(offset, info & 0xFF, symbols[symbol_index].name)
                )
        return output


@dataclass(frozen=True)
class MovableModel:
    """Final section intervals plus linker-derived boundary symbol values."""

    ranges: tuple[tuple[int, int, str], ...]
    boundary_values: frozenset[int]

    @classmethod
    def from_elf(cls, elf: Elf32) -> "MovableModel":
        if elf.elf_type != ET_EXEC:
            raise AuditError(f"{elf.path}: expected linked ET_EXEC ELF")
        ranges = tuple(
            (section.address, section.address + section.size, section.name)
            for section in elf.sections
            if section.name in MOVABLE_OUTPUT_SECTIONS and section.size
        )
        if not any(name == ".main_exe" for _, _, name in ranges):
            raise AuditError(f"{elf.path}: missing .main_exe output section")
        if not any(name == ".main_exe_bss" for _, _, name in ranges):
            raise AuditError(f"{elf.path}: missing .main_exe_bss output section")

        by_name: dict[str, set[int]] = {}
        for symbol in elf.symbols:
            if symbol.name:
                by_name.setdefault(symbol.name, set()).add(symbol.value)
        missing = sorted(MOVABLE_BOUNDARY_SYMBOLS - by_name.keys())
        if missing:
            raise AuditError(
                f"{elf.path}: missing movable boundary symbols: {', '.join(missing)}"
            )
        ambiguous = sorted(
            name for name in MOVABLE_BOUNDARY_SYMBOLS if len(by_name[name]) != 1
        )
        if ambiguous:
            raise AuditError(
                f"{elf.path}: ambiguous movable boundary symbols: "
                + ", ".join(ambiguous)
            )
        values = frozenset(
            next(iter(by_name[name])) for name in MOVABLE_BOUNDARY_SYMBOLS
        )
        return cls(ranges, values)

    @staticmethod
    def cached_alias(value: int) -> int:
        value &= 0xFFFFFFFF
        if 0xA0000000 <= value < 0xC0000000:
            return value - 0x20000000
        return value

    def in_source_owned_range(self, value: int) -> bool:
        value = self.cached_alias(value)
        return any(start <= value < end for start, end, _name in self.ranges)

    def in_text_movable_ram(self, value: int) -> bool:
        value = self.cached_alias(value)
        return MAIN_START <= value < HANDOFF_START

    def in_data_movable_ram(self, value: int, *, compiled: bool) -> bool:
        value = self.cached_alias(value)
        if self.in_source_owned_range(value) or value in self.boundary_values:
            return True
        # Compiler-generated initialized objects do not contain opaque packed
        # assets, so the whole pre-handoff MAIN/pool interval is meaningful.
        # Generated assembly data stays on exact source-owned ranges to avoid
        # treating arbitrary byte streams as pointers.
        return compiled and MAIN_START <= value < HANDOFF_START


def _strip_linker_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    return re.sub(r"#.*$", "", text, flags=re.MULTILINE)


def _display_path(path: Path, root: Path) -> str:
    try:
        return path.resolve().relative_to(root.resolve()).as_posix()
    except ValueError:
        return os.path.normpath(str(path))


def _expand_linker_object_glob(pattern: Path) -> list[Path]:
    """Expand an ld filename wildcard, whose ``*`` may match ``/``."""

    rendered = str(pattern)
    magic = GLOB_MAGIC_RE.search(rendered)
    if magic is None:
        return [pattern]
    separator = rendered.rfind(os.sep, 0, magic.start())
    if separator < 0:
        search_root = Path(".")
    elif separator == 0:
        search_root = Path(os.sep)
    else:
        search_root = Path(rendered[:separator])
    if not search_root.is_dir():
        return []
    return sorted(
        path
        for path in search_root.rglob("*")
        if path.is_file() and fnmatch.fnmatchcase(str(path), rendered)
    )


def linked_selections(linker_text: str, root: Path = ROOT) -> list[ObjectSelection]:
    """Expand exact GNU-ld object tokens and union their selected sections."""

    selected: dict[Path, set[str]] = {}
    for match in LINKER_INPUT_RE.finditer(_strip_linker_comments(linker_text)):
        token = match.group("object")
        patterns = match.group("sections").split()
        if not patterns:
            raise AuditError(f"linked object {token} has an empty section selector")
        raw_path = Path(token)
        rooted = raw_path if raw_path.is_absolute() else root / raw_path
        paths = _expand_linker_object_glob(rooted)
        # GNU ld accepts an object glob which currently matches nothing.  It is
        # not an input object yet, so there is nothing for this audit to read.
        for path in paths:
            selected.setdefault(path, set()).update(patterns)

    if not selected:
        raise AuditError("linker script names no object inputs")
    missing = [path for path in selected if not path.is_file()]
    if missing:
        rendered = ", ".join(_display_path(path, root) for path in missing[:5])
        raise AuditError(f"linked object does not exist: {rendered}")
    return [
        ObjectSelection(
            path=path,
            display=_display_path(path, root),
            patterns=tuple(sorted(patterns)),
        )
        for path, patterns in sorted(selected.items(), key=lambda item: str(item[0]))
    ]


def loaded_object_paths(map_text: str, root: Path = ROOT) -> list[Path]:
    """Return GNU ld's exact ``LOAD`` object inventory in link order."""

    result: list[Path] = []
    seen: set[Path] = set()
    for line in map_text.splitlines():
        match = MAP_LOAD_RE.match(line)
        if match is None:
            continue
        raw = Path(match.group("object"))
        path = raw if raw.is_absolute() else root / raw
        path = Path(os.path.normpath(str(path)))
        if path in seen:
            raise AuditError(
                f"link map loads object twice: {_display_path(path, root)}"
            )
        seen.add(path)
        result.append(path)
    if not result:
        raise AuditError("link map contains no LOAD object records")
    return result


def filter_loaded_selections(
    selections: Iterable[ObjectSelection], loaded: Iterable[Path], root: Path = ROOT
) -> list[ObjectSelection]:
    """Keep only objects GNU ld loaded and reject unmodelled map inputs."""

    by_path = {selection.path: selection for selection in selections}
    output: list[ObjectSelection] = []
    missing: list[Path] = []
    for path in loaded:
        selection = by_path.get(path)
        if selection is None:
            missing.append(path)
        else:
            output.append(selection)
    if missing:
        rendered = ", ".join(_display_path(path, root) for path in missing[:5])
        raise AuditError(
            "link map LOAD object is absent from linker selectors: " + rendered
        )
    return output


def selected_sections(elf: Elf32, patterns: Iterable[str]) -> list[Section]:
    patterns = tuple(pattern for pattern in patterns if pattern != "COMMON")
    return [
        section
        for section in elf.sections
        if any(fnmatch.fnmatchcase(section.name, pattern) for pattern in patterns)
    ]


def reviewed_alloc_metadata(section: Section) -> str | None:
    """Accept only exact, structurally valid allocatable MIPS metadata."""

    expected = {
        ".reginfo": (SHT_MIPS_REGINFO, 24, 4, 24),
        ".MIPS.abiflags": (SHT_MIPS_ABIFLAGS, 24, 8, 24),
    }.get(section.name)
    if expected is None:
        return None
    section_type, size, alignment, entry_size = expected
    if (
        section.type == section_type
        and section.flags == SHF_ALLOC
        and section.address == 0
        and section.size == size
        and section.link == 0
        and section.info == 0
        and section.alignment == alignment
        and section.entry_size == entry_size
    ):
        return section.name
    return None


def audit_alloc_section_ownership(
    sections: Iterable[Section],
    owned_indices: set[int],
    *,
    object_name: str,
) -> tuple[list[Finding], Counter[str]]:
    """Reject alloc sections not selected into a runtime/zero-size owner."""

    findings: list[Finding] = []
    reviewed: Counter[str] = Counter()
    for section in sections:
        if not (section.flags & SHF_ALLOC):
            continue
        if section.index in owned_indices:
            continue
        if section.size == 0 and section.name in EXPECTED_EMPTY_INPUT_SECTIONS:
            continue
        metadata = reviewed_alloc_metadata(section)
        if metadata is not None:
            reviewed[metadata] += 1
            continue
        findings.append(
            Finding(
                kind="unowned_alloc_section",
                object=object_name,
                section=section.name,
                offset="0x0",
                word=f"0x{section.size:x}",
                target=None,
                detail=(
                    "SHF_ALLOC input section is not selected by a "
                    "normal-link runtime owner and is not exact reviewed MIPS "
                    f"metadata (type=0x{section.type:x}, flags=0x{section.flags:x})"
                ),
            )
        )
    return findings, reviewed


def relocation_map(relocations: Iterable[Relocation]) -> dict[int, set[int]]:
    result: dict[int, set[int]] = {}
    for relocation in relocations:
        result.setdefault(relocation.offset, set()).add(relocation.type)
    return result


def _sign_extend_16(value: int) -> int:
    return value - 0x10000 if value & 0x8000 else value


def _written_gpr(word: int) -> int | None:
    opcode = word >> 26
    rt = (word >> 16) & 0x1F
    if opcode == 0:
        function = word & 0x3F
        if function == 8:  # jr
            return None
        if function == 9:  # jalr
            return (word >> 11) & 0x1F
        if function in {12, 13}:  # syscall, break
            return None
        return (word >> 11) & 0x1F
    if opcode == 3:
        return 31
    if 0x10 <= opcode <= 0x13:
        coprocessor_operation = (word >> 21) & 0x1F
        return rt if coprocessor_operation in {0, 2} else None
    if opcode in {1, 2, 4, 5, 6, 7} or opcode in STORE_OPCODES:
        return None
    if opcode in MEMORY_OPCODES:
        return rt if opcode not in STORE_OPCODES else None
    if 8 <= opcode <= 15:
        return rt
    return None


def scan_lui_low_uses(
    words: list[int], start_index: int, relocations: dict[int, set[int]]
) -> list[LowUse]:
    """Find canonical low-half uses until the LUI destination is overwritten."""

    high_word = words[start_index]
    register = (high_word >> 16) & 0x1F
    high = (high_word & 0xFFFF) << 16
    uses: list[LowUse] = []
    stop_after_delay = False
    # Sixty-four instructions covers every reviewed current low use while
    # bounding false cross-path associations.  High-base-in-MAIN literals are
    # rejected independently, so this window matters only for 0x8001/0x8010
    # bases whose canonical low half crosses into movable MAIN.
    for index in range(start_index + 1, min(len(words), start_index + 65)):
        word = words[index]
        offset = index * 4
        opcode = word >> 26
        rs = (word >> 21) & 0x1F
        this_is_call_delay = stop_after_delay
        if rs == register and R_MIPS_LO16 not in relocations.get(offset, set()):
            immediate = word & 0xFFFF
            if opcode in {8, 9}:
                target = (high + _sign_extend_16(immediate)) & 0xFFFFFFFF
                uses.append(LowUse(offset, "addiu", target))
            elif opcode == 13:
                uses.append(LowUse(offset, "ori", high | immediate))
            elif opcode in MEMORY_OPCODES:
                target = (high + _sign_extend_16(immediate)) & 0xFFFFFFFF
                uses.append(LowUse(offset, "memory", target))

        if _written_gpr(word) == register:
            break
        if this_is_call_delay:
            break
        # A direct/indirect call destroys caller-saved registers after its
        # delay slot.  Scan that slot, then stop before associating a later
        # reuse of the same physical register with this LUI.
        if opcode == 3 or (opcode == 0 and word & 0x3F == 9):
            if register in set(range(1, 16)) | {24, 25, 31}:
                stop_after_delay = True
    return uses


def fixed_contract(value: int) -> str | None:
    value &= 0xFFFFFFFF
    if PERSISTENT_START <= value <= PERSISTENT_END:
        return "persistent_state"
    if HANDOFF_START <= value <= HANDOFF_END:
        return "executable_handoff"
    if SCRATCH_START <= value < SCRATCH_END:
        return "scratchpad"
    if MMIO_START <= value < MMIO_END:
        return "mmio"
    if value == INITIAL_STACK:
        return "initial_stack"
    if value in RAM_END_VALUES:
        return "ram_end"
    return None


def _hex32(value: int) -> str:
    return f"0x{value & 0xFFFFFFFF:08x}"


def _safe_numeric_constant(object_name: str, uses: list[LowUse]) -> str | None:
    if len(uses) != 1 or uses[0].kind not in {"addiu", "ori"}:
        return None
    return SAFE_NUMERIC_CONSTANTS.get((Path(object_name).name, uses[0].target))


def analyse_executable_section(
    data: bytes,
    relocations: dict[int, set[int]],
    model: MovableModel,
    *,
    object_name: str,
    section_name: str,
) -> tuple[list[Finding], Counter[str], Counter[str]]:
    if len(data) % 4:
        raise AuditError(
            f"{object_name}:{section_name}: executable size is not word-aligned"
        )
    words = [
        struct.unpack_from("<I", data, offset)[0]
        for offset in range(0, len(data), 4)
    ]
    findings: list[Finding] = []
    reviewed: Counter[str] = Counter()
    evidence: Counter[str] = Counter()

    for index, word in enumerate(words):
        offset = index * 4
        types = relocations.get(offset, set())
        opcode = word >> 26
        if opcode in {2, 3}:
            evidence["direct_jumps"] += 1
            if R_MIPS_26 in types:
                evidence["relocated_direct_jumps"] += 1
            else:
                findings.append(
                    Finding(
                        kind="direct_jump_without_r_mips_26",
                        object=object_name,
                        section=section_name,
                        offset=f"0x{offset:x}",
                        word=_hex32(word),
                        target=None,
                        detail=("direct j" if opcode == 2 else "direct jal")
                        + " has no R_MIPS_26 relocation",
                    )
                )

        if opcode != 0x0F:
            continue
        if R_MIPS_HI16 in types:
            evidence["symbolic_hi16"] += 1
            continue

        high_base = (word & 0xFFFF) << 16
        uses = scan_lui_low_uses(words, index, relocations)
        safe_constant = _safe_numeric_constant(object_name, uses)
        if safe_constant is not None:
            reviewed["numeric_constant"] += 1
            continue
        bad_uses = [use for use in uses if model.in_text_movable_ram(use.target)]
        if bad_uses:
            targets = ", ".join(
                f"{_hex32(use.target)} at +0x{use.offset:x}" for use in bad_uses
            )
            findings.append(
                Finding(
                    kind="literal_lui_low_main_address",
                    object=object_name,
                    section=section_name,
                    offset=f"0x{offset:x}",
                    word=_hex32(word),
                    target=_hex32(bad_uses[0].target),
                    detail=f"unrelocated LUI/low forms movable MAIN address {targets}",
                )
            )
            continue

        if model.in_text_movable_ram(high_base):
            findings.append(
                Finding(
                    kind="literal_lui_main_base",
                    object=object_name,
                    section=section_name,
                    offset=f"0x{offset:x}",
                    word=_hex32(word),
                    target=_hex32(high_base),
                    detail="unrelocated LUI materialises a movable MAIN base",
                )
            )
            continue

        contracts = [fixed_contract(use.target) for use in uses]
        contract = fixed_contract(high_base) or next(
            (item for item in contracts if item is not None), None
        )
        if contract is not None:
            reviewed[contract] += 1
    return findings, reviewed, evidence


def _compiled_object(object_name: str) -> bool:
    path = Path(object_name)
    return path.name.endswith(".c.o") or "reloc-c-literals" in path.parts


def analyse_data_section(
    data: bytes,
    relocations: dict[int, set[int]],
    model: MovableModel,
    *,
    object_name: str,
    section_name: str,
) -> tuple[list[Finding], Counter[str], Counter[str]]:
    findings: list[Finding] = []
    reviewed: Counter[str] = Counter()
    evidence: Counter[str] = Counter()
    compiled = _compiled_object(object_name)
    for offset in range(0, len(data) - 3, 4):
        evidence["data_words"] += 1
        types = relocations.get(offset, set())
        if R_MIPS_32 in types:
            evidence["symbolic_data_words"] += 1
            continue
        value = struct.unpack_from("<I", data, offset)[0]
        if not model.in_data_movable_ram(value, compiled=compiled):
            continue
        placeholder = HEADER_PLACEHOLDERS.get(
            (Path(object_name).name, section_name, offset)
        )
        if placeholder is not None:
            reviewed["psx_header_placeholder"] += 1
            continue
        findings.append(
            Finding(
                kind="literal_alloc_data_main_address",
                object=object_name,
                section=section_name,
                offset=f"0x{offset:x}",
                word=_hex32(value),
                target=_hex32(value),
                detail="alloc-data word targets movable MAIN without R_MIPS_32",
            )
        )
    return findings, reviewed, evidence


def collect(
    root: Path, linker: Path, linked_elf: Path, link_map: Path = DEFAULT_MAP
) -> dict[str, object]:
    linker_path = linker if linker.is_absolute() else root / linker
    elf_path = linked_elf if linked_elf.is_absolute() else root / linked_elf
    map_path = link_map if link_map.is_absolute() else root / link_map
    if not linker_path.is_file() or not elf_path.is_file() or not map_path.is_file():
        raise AuditError(
            "normal-link artifacts are missing; run `nix develop -c ./Build relink`"
        )
    expanded = linked_selections(linker_path.read_text(), root)
    loaded = loaded_object_paths(map_path.read_text(), root)
    selections = filter_loaded_selections(expanded, loaded, root)
    model = MovableModel.from_elf(Elf32(elf_path))
    counts = Counts(objects=len(selections))
    findings: list[Finding] = []
    reviewed: Counter[str] = Counter()

    for selection in selections:
        elf = Elf32(selection.path)
        if elf.elf_type != ET_REL:
            raise AuditError(f"{selection.display}: expected relocatable ET_REL object")
        owned_sections = selected_sections(elf, selection.patterns)
        ownership_findings, ownership_reviewed = audit_alloc_section_ownership(
            elf.sections,
            {section.index for section in owned_sections},
            object_name=selection.display,
        )
        findings.extend(ownership_findings)
        counts.reviewed_alloc_metadata_sections += sum(
            ownership_reviewed.values()
        )
        for section in owned_sections:
            if not (section.flags & SHF_ALLOC) or section.type != SHT_PROGBITS:
                continue
            if section.size == 0:
                continue
            counts.selected_alloc_sections += 1
            section_relocations = relocation_map(elf.relocations_for(section))
            data = elf.section_data(section)
            if section.flags & SHF_EXECINSTR:
                counts.executable_sections += 1
                new_findings, new_reviewed, evidence = analyse_executable_section(
                    data,
                    section_relocations,
                    model,
                    object_name=selection.display,
                    section_name=section.name,
                )
            else:
                counts.data_sections += 1
                new_findings, new_reviewed, evidence = analyse_data_section(
                    data,
                    section_relocations,
                    model,
                    object_name=selection.display,
                    section_name=section.name,
                )
            findings.extend(new_findings)
            reviewed.update(new_reviewed)
            for name, value in evidence.items():
                setattr(counts, name, getattr(counts, name) + value)

    findings.sort(key=lambda item: (item.object, item.section, item.offset, item.kind))
    return {
        "schema": 1,
        "linker": _display_path(linker_path, root),
        "elf": _display_path(elf_path, root),
        "map": _display_path(map_path, root),
        "movable_ranges": [
            {"start": _hex32(start), "end": _hex32(end), "section": name}
            for start, end, name in model.ranges
        ],
        "counts": asdict(counts),
        "reviewed_fixed": dict(sorted(reviewed.items())),
        "findings": [asdict(finding) for finding in findings],
        "summary": {"findings": len(findings)},
    }


def render_text(report: dict[str, object]) -> str:
    counts = report["counts"]
    assert isinstance(counts, dict)
    reviewed = report["reviewed_fixed"]
    assert isinstance(reviewed, dict)
    findings = report["findings"]
    assert isinstance(findings, list)
    lines = [
        "reloc-input-audit main.exe",
        (
            f"linked objects: {counts['objects']}; selected alloc sections: "
            f"{counts['selected_alloc_sections']} "
            f"(exec={counts['executable_sections']}, data={counts['data_sections']})"
        ),
        (
            "reviewed unowned alloc metadata sections: "
            f"{counts['reviewed_alloc_metadata_sections']}"
        ),
        (
            f"direct J/JAL: {counts['direct_jumps']}; R_MIPS_26-backed: "
            f"{counts['relocated_direct_jumps']}; symbolic HI16: "
            f"{counts['symbolic_hi16']}"
        ),
        (
            f"alloc-data words: {counts['data_words']}; R_MIPS_32-backed: "
            f"{counts['symbolic_data_words']}"
        ),
        "reviewed fixed literals: "
        + (", ".join(f"{name}={value}" for name, value in reviewed.items()) or "none"),
    ]
    for finding in findings:
        assert isinstance(finding, dict)
        target = f" -> {finding['target']}" if finding["target"] else ""
        lines.append(
            f"FINDING {finding['object']}:{finding['section']}+"
            f"{finding['offset']} {finding['kind']}{target}: {finding['detail']}"
        )
    lines.append(f"findings: {len(findings)}")
    return "\n".join(lines) + "\n"


def parser() -> argparse.ArgumentParser:
    argument_parser = argparse.ArgumentParser(
        description=(
            "Gate the exact normal-link input sections on MIPS relocation records."
        ),
        epilog=(
            "Scope: R_MIPS_26-backed direct J/JAL, canonical R_MIPS_HI16/"
            "R_MIPS_LO16 LUI+low address formation, and R_MIPS_32-backed "
            "aligned alloc-data words. Arbitrary register arithmetic and "
            "unaligned/opaque packed fields are intentionally outside this "
            "heuristic gate; reviewed loaded-data manifests and the +0x10004 "
            "growth probe cover the current exceptional fields."
        ),
    )
    argument_parser.add_argument("--root", type=Path, default=ROOT)
    argument_parser.add_argument("--linker", type=Path, default=DEFAULT_LINKER)
    argument_parser.add_argument("--elf", type=Path, default=DEFAULT_ELF)
    argument_parser.add_argument("--map", type=Path, default=DEFAULT_MAP)
    argument_parser.add_argument("--json", action="store_true")
    argument_parser.add_argument(
        "--allow-findings",
        action="store_true",
        help="report findings but exit successfully (diagnostic use only)",
    )
    return argument_parser


def main(argv: list[str] | None = None) -> int:
    args = parser().parse_args(argv)
    try:
        report = collect(args.root, args.linker, args.elf, args.map)
    except (AuditError, OSError) as error:
        print(f"reloc-input-audit: {error}", file=sys.stderr)
        return 2
    if args.json:
        print(json.dumps(report, indent=2, sort_keys=True))
    else:
        print(render_text(report), end="")
    findings = report["summary"]["findings"]
    return 0 if args.allow_findings or findings == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())

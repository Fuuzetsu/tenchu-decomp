#!/usr/bin/env python3
"""Build the retail-exact proof for linker-owned MAIN.EXE BSS storage.

The shipped PS-X EXE has a subtle overlap: initialized data ends at
``0x80097eb0``, where crt0 starts clearing BSS, but the sector-sized payload
continues with 0x150 zero bytes through ``0x80098000``.  Splat consequently
places those zeros (and their labels) in a PROGBITS input section.

This opt-in proof rewrites only generated build products:

* the zero tail of ``72CD0.data.s`` becomes a NOBITS ``.bss`` section;
* the generated linker's zero-sized BSS input list moves into a real NOLOAD
  output section following initialized data;
* every script-assigned symbol in the loaded MAIN.EXE image is allowed to come
  from its real input label, including initialized data, the remaining mixed
  SDK/data carve, BSS, and ``_gp``;
* every manifest-transformed pointer target object can replace its matching
  generated data input before the final link;
* new C files under the normal-link extension directory receive ordinary
  text/data/small-BSS/BSS input patterns instead of falling into ``/DISCARD/``;
* when requested by the normal relink, small initialized/uninitialized data
  newly emitted by an existing game C object is collected in the same
  ``$gp``-near hooks (and residual COMMON storage is retained) instead of
  falling into ``/DISCARD/``; and
* the retail proof represents the fixed virtual-memory pool as an explicit
  NOBITS reservation; the composed relink preserves that base while BSS fits
  in the retail gap, then advances it to aligned BSS and shrinks the
  reservation toward its fixed upper bound.

At retail sizes objcopy intentionally emits the logical 0x876b0-byte prefix.
``validate`` proves that appending the reference image's 0x150 zero-byte sector
padding gives the shipped executable exactly. The Shake gate delegates that
padding and the mutable PS-X EXE header to ``tools/psxexe.py``; this tool does
not duplicate the finalizer role.

This remains a layout proof, not a size-changing runnable build. In particular,
the final mixed SDK instruction carve still needs symbolic instruction
relocations even after its public labels become section-owned.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import os
from pathlib import Path
import re
import subprocess
import tempfile
from typing import Sequence


INITIALIZED_END = 0x80097EB0
MAIN_LOAD_ADDRESS = 0x80011000
BSS_START = INITIALIZED_END
BSS_PAD_END = 0x80098000
BSS_END = 0x800CDBA8
HEAP_START = BSS_END + 4
GP_ADDRESS = 0x80097698
MEMORY_POOL_START = 0x800DC000
MEMORY_POOL_SIZE = 0x120000
MEMORY_POOL_END = MEMORY_POOL_START + MEMORY_POOL_SIZE
MEMORY_POOL_ALIGNMENT = 16
MINIMUM_MEMORY_POOL_SIZE = 0x10
MEMORY_POOL_CAPACITY = MEMORY_POOL_SIZE // 4 - 2
HANDOFF_START = 0x80100000
HANDOFF_END = 0x8010005C
STACK_START = 0x801FFFF0

LOGICAL_FILE_SIZE = 0x876B0
RETAIL_FILE_SIZE = 0x87800
REFERENCE_PADDING_SIZE = RETAIL_FILE_SIZE - LOGICAL_FILE_SIZE

TAIL_SPLIT_MARKER = "nonmatching OTablePt"
TAIL_SECTION_DIRECTIVE = '.section .bss, "aw", @nobits'
# PutMapMode is the byte at 0x80097bb1 inside the three-byte alignment tail
# following the ``D_80097BA0`` string pool. Splat cannot emit a dlabel in the
# implicit bytes produced by ``.align``, so retain the exact proven interior
# relationship as a relocatable in-output-section alias instead of an absolute
# retail address.
INITIALIZED_INTERIOR_ALIASES = (("PutMapMode", "D_80097BA0", 0x11),)
OLD_BSS_START_MARKER = "main_exe_BSS_START = .;"
OLD_BSS_END_MARKER = (
    "main_exe_BSS_SIZE = ABSOLUTE(main_exe_BSS_END - main_exe_BSS_START);"
)
ROM_END_MARKER = "main_exe_ROM_END = __romPos;"
VRAM_END_MARKER = "main_exe_VRAM_END = .;"

ASSIGNMENT_RE = re.compile(
    r"^\s*([A-Za-z_.$][A-Za-z0-9_.$]*)\s*=\s*"
    r"(0[xX][0-9A-Fa-f]+);\s*(?:[#].*)?$"
)
DLABEL_RE = re.compile(r"^\s*dlabel\s+([A-Za-z_.$][A-Za-z0-9_.$]*)\s*$")
NM_POSIX_RE = re.compile(
    r"^(?P<name>\S+)\s+(?P<type>[A-Za-z?])\s+"
    r"(?P<value>[0-9A-Fa-f]+)(?:\s+\S+)?\s*$"
)
READELF_SECTION_RE = re.compile(
    r"^\s*\[\s*\d+\]\s+(?P<name>\S+)\s+(?P<type>\S+)\s+"
    r"(?P<address>[0-9A-Fa-f]+)\s+[0-9A-Fa-f]+\s+"
    r"(?P<size>[0-9A-Fa-f]+)\s+"
)


class LaneError(RuntimeError):
    """A generated-input or retail-layout invariant failed."""


@dataclass(frozen=True)
class Symbol:
    name: str
    address: int


def atomic_write(path: Path, content: str | bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    mode = "wb" if isinstance(content, bytes) else "w"
    kwargs = {} if isinstance(content, bytes) else {"newline": ""}
    descriptor, temporary = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    try:
        with os.fdopen(descriptor, mode, **kwargs) as stream:
            stream.write(content)
        os.replace(temporary, path)
    except BaseException:
        try:
            os.unlink(temporary)
        except FileNotFoundError:
            pass
        raise


def parse_assignments(source: str) -> dict[str, int]:
    result: dict[str, int] = {}
    for line in source.splitlines():
        match = ASSIGNMENT_RE.fullmatch(line)
        if match is None:
            continue
        name, raw_address = match.groups()
        address = int(raw_address, 16)
        previous = result.get(name)
        if previous is not None and previous != address:
            raise LaneError(
                f"symbol {name} has conflicting assignments "
                f"0x{previous:08x} and 0x{address:08x}"
            )
        result[name] = address
    return result


def merge_assignments(*sources: str) -> dict[str, int]:
    merged: dict[str, int] = {}
    for source in sources:
        for name, address in parse_assignments(source).items():
            previous = merged.get(name)
            if previous is not None and previous != address:
                raise LaneError(
                    f"symbol {name} has conflicting assignments "
                    f"0x{previous:08x} and 0x{address:08x}"
                )
            merged[name] = address
    return merged


def is_owned_assignment(name: str, address: int) -> bool:
    return (
        MAIN_LOAD_ADDRESS <= address < BSS_END
        or name in {"D_800CDBA8", "_gp", "HEAP_START", "MemoryPool"}
    )


def filter_symbol_script(source: str) -> tuple[str, dict[str, int]]:
    removed: dict[str, int] = {}
    output: list[str] = []
    for line in source.splitlines(keepends=True):
        match = ASSIGNMENT_RE.fullmatch(line.rstrip("\r\n"))
        if match is not None:
            name, raw_address = match.groups()
            address = int(raw_address, 16)
            if is_owned_assignment(name, address):
                previous = removed.get(name)
                if previous is not None and previous != address:
                    raise LaneError(f"conflicting removed symbol {name}")
                removed[name] = address
                continue
        output.append(line)

    expected = {
        "_gp": GP_ADDRESS,
        "HEAP_START": HEAP_START,
        "MemoryPool": MEMORY_POOL_START,
    }
    for name, address in removed.items():
        if name in expected and address != expected[name]:
            raise LaneError(
                f"{name} moved from expected retail address "
                f"0x{expected[name]:08x} to 0x{address:08x}"
            )
    return "".join(output), removed


def transform_tail_source(source: str) -> tuple[str, set[str]]:
    lines = source.splitlines(keepends=True)
    marker_indices = [
        index
        for index, line in enumerate(lines)
        if line.rstrip("\r\n") == TAIL_SPLIT_MARKER
    ]
    if len(marker_indices) != 1:
        raise LaneError(
            f"expected one {TAIL_SPLIT_MARKER!r} marker, found {len(marker_indices)}"
        )
    marker = marker_indices[0]
    newline = "\r\n" if lines[marker].endswith("\r\n") else "\n"
    lines.insert(marker, TAIL_SECTION_DIRECTIVE + newline + newline)

    bss_labels = {
        match.group(1)
        for line in lines[marker + 1 :]
        if (match := DLABEL_RE.fullmatch(line.rstrip("\r\n"))) is not None
    }
    if "OTablePt" not in bss_labels or "World" not in bss_labels:
        raise LaneError("the converted tail does not cover the known padding/BSS labels")
    return "".join(lines), bss_labels


def _indent_of(line: str) -> str:
    return line[: len(line) - len(line.lstrip())]


def make_bss_body(
    *,
    indent: str,
    bss_input_lines: list[str],
    tail_object: str,
    extension_object_glob: str,
    ordinary_c_object_globs: Sequence[str],
    aliases: list[Symbol],
    dynamic_pool: bool = False,
) -> str:
    inner = indent + "    "
    lines = [
        f"{indent}.main_exe_bss (NOLOAD) : AT(main_exe_ROM_END) SUBALIGN(4)\n",
        f"{indent}{{\n",
        f"{inner}main_exe_BSS_START = .;\n",
        f"{inner}__bss_start = .;\n",
        f"{inner}{tail_object}(.bss);\n",
        f"{inner}{extension_object_glob}(.sbss .sbss.* .scommon);\n",
    ]
    if ordinary_c_object_globs:
        # cc1 is run with -G8. maspsx consequently lowers <=8-byte tentative
        # definitions (cc1's .comm/.lcomm) into .sbss, so these inputs must sit
        # next to the original gp-small BSS prefix, not at the far BSS end.
        lines.extend(
            f"{inner}{object_glob}(.sbss .sbss.* .scommon);\n"
            for object_glob in ordinary_c_object_globs
        )
    lines.extend(bss_input_lines)
    cursor = BSS_PAD_END
    for symbol in aliases:
        gap = symbol.address - cursor
        if gap < 0:
            raise LaneError(f"BSS aliases are out of order at {symbol.name}")
        if gap:
            lines.append(f"{inner}. += 0x{gap:x};\n")
        lines.append(f"{inner}{symbol.name} = .;\n")
        cursor = symbol.address
    final_gap = BSS_END - cursor
    if final_gap < 0:
        raise LaneError("the last BSS alias lies beyond crt0's clear range")
    lines.extend(
        [
            f"{inner}. += 0x{final_gap:x};\n",
            f"{inner}{extension_object_glob}(.bss .bss.* COMMON);\n",
        ]
    )
    if ordinary_c_object_globs:
        # The generated linker already enumerates each ordinary object's base
        # .bss section. Repeating it is harmless (it is empty by this point)
        # and also makes .bss.* plus any residual GNU COMMON fail-safe rather
        # than silently discarding a newly introduced definition.
        lines.extend(
            f"{inner}{object_glob}(.bss .bss.* COMMON);\n"
            for object_glob in ordinary_c_object_globs
        )
    lines.extend(
        [
            f"{inner}__bss_end = .;\n",
            f"{inner}D_800CDBA8 = .;\n",
            f"{inner}main_exe_BSS_END = .;\n",
            f"{inner}main_exe_BSS_SIZE = "
            "ABSOLUTE(main_exe_BSS_END - main_exe_BSS_START);\n",
            f"{inner}HEAP_START = . + 4;\n",
            f"{indent}}}\n",
            f"{indent}main_exe_VRAM_END = .;\n",
            "\n",
        ]
    )
    if dynamic_pool:
        lines.extend(
            [
                f"{indent}__tenchu_memory_pool_size = ABSOLUTE("
                f"0x{MEMORY_POOL_END:08x} "
                f"- MAX(0x{MEMORY_POOL_START:08x}, "
                f"ALIGN(ABSOLUTE(main_exe_BSS_END), "
                f"{MEMORY_POOL_ALIGNMENT})));\n",
                f"{indent}.main_exe_memory_pool MAX(0x{MEMORY_POOL_START:08x}, "
                f"ALIGN(ABSOLUTE(main_exe_BSS_END), "
                f"{MEMORY_POOL_ALIGNMENT})) (NOLOAD) :\n",
                f"{indent}{{\n",
                f"{inner}MemoryPool = .;\n",
                f"{inner}. += __tenchu_memory_pool_size;\n",
                f"{inner}MemoryPoolEnd = .;\n",
                f"{inner}main_exe_MEMORY_POOL_END = .;\n",
                f"{indent}}}\n",
                "\n",
            ]
        )
    else:
        lines.extend(
            [
                f"{indent}.main_exe_memory_pool 0x{MEMORY_POOL_START:08x} "
                "(NOLOAD) :\n",
                f"{indent}{{\n",
                f"{inner}MemoryPool = .;\n",
                f"{inner}. += 0x{MEMORY_POOL_SIZE:x};\n",
                f"{inner}MemoryPoolEnd = .;\n",
                f"{inner}main_exe_MEMORY_POOL_END = .;\n",
                f"{indent}}}\n",
                "\n",
            ]
        )
    lines.extend(
        [
            f"{indent}MemoryPoolCapacity = ABSOLUTE("
            "(MemoryPoolEnd - MemoryPool) / 4 - 2);\n",
            "\n",
            f"{indent}__tenchu_handoff_start = ABSOLUTE(0x{HANDOFF_START:08x});\n",
            f"{indent}__tenchu_handoff_end = ABSOLUTE(0x{HANDOFF_END:08x});\n",
            f'{indent}ASSERT(main_exe_BSS_START == ALIGN(main_exe_INITIALIZED_END, 16), '
            '"BSS does not follow aligned initialized data")\n',
            f'{indent}ASSERT(D_800CDBA8 == main_exe_BSS_END, '
            '"crt0 clear end disagrees with linker BSS end")\n',
            f'{indent}ASSERT(HEAP_START == main_exe_BSS_END + 4, '
            '"heap start does not follow BSS")\n',
            f'{indent}ASSERT(_gp >= __load_start && _gp < main_exe_INITIALIZED_END, '
            '"_gp lies outside the initialized image")\n',
        ]
    )
    if dynamic_pool:
        lines.extend(
            [
                f'{indent}ASSERT(MemoryPool == MAX(0x{MEMORY_POOL_START:08x}, '
                f'ALIGN(ABSOLUTE(main_exe_BSS_END), '
                f'{MEMORY_POOL_ALIGNMENT})), '
                '"dynamic MemoryPool does not follow its minimum/aligned BSS")\n',
            ]
        )
    else:
        lines.extend(
            [
                f'{indent}ASSERT(MemoryPool == 0x{MEMORY_POOL_START:08x}, '
                '"retail MemoryPool changed")\n',
            ]
        )
    lines.extend(
        [
            f'{indent}ASSERT(MemoryPoolEnd == main_exe_MEMORY_POOL_END, '
            '"virtual-memory pool end aliases disagree")\n',
            f'{indent}ASSERT(main_exe_MEMORY_POOL_END == 0x{MEMORY_POOL_END:08x}, '
            '"virtual-memory pool upper bound changed")\n',
            f'{indent}ASSERT(main_exe_BSS_END <= MemoryPool, '
            '"BSS overlaps the virtual-memory pool")\n',
            f'{indent}ASSERT(MemoryPoolEnd - MemoryPool >= '
            f'0x{MINIMUM_MEMORY_POOL_SIZE:x}, '
            '"virtual-memory pool is too small")\n',
            f'{indent}ASSERT((MemoryPoolEnd - MemoryPool) % 4 == 0, '
            '"virtual-memory pool is not word-aligned")\n',
            f'{indent}ASSERT(main_exe_BSS_END <= __tenchu_handoff_start, '
            '"BSS overlaps the fixed executable handoff record")\n',
            f'{indent}ASSERT(main_exe_MEMORY_POOL_END <= 0x{STACK_START:08x}, '
            '"virtual-memory pool overlaps the initial stack")\n',
        ]
    )
    return "".join(lines)


def rewrite_linker(
    source: str,
    *,
    old_tail_object: str,
    new_tail_object: str,
    extension_object_glob: str,
    aliases: list[Symbol],
    ordinary_c_object_globs: Sequence[str] = (),
    object_replacements: Sequence[tuple[str, str]] = (),
    dynamic_pool: bool = False,
) -> str:
    lines = source.splitlines(keepends=True)
    for old_object, new_object in object_replacements:
        if not old_object or not new_object or old_object == new_object:
            raise LaneError("object replacements require distinct nonempty paths")
        matches = sum(line.count(old_object) for line in lines)
        if matches != 1:
            raise LaneError(
                f"expected one linker reference to {old_object}, found {matches}"
            )
        lines = [line.replace(old_object, new_object) for line in lines]
    gp_indices: list[int] = []
    for index, line in enumerate(lines):
        assignment = ASSIGNMENT_RE.fullmatch(line.rstrip("\r\n"))
        if assignment is None or assignment.group(1) != "_gp":
            continue
        address = int(assignment.group(2), 16)
        if address != GP_ADDRESS:
            raise LaneError(
                f"generated linker _gp is 0x{address:08x}, "
                f"expected 0x{GP_ADDRESS:08x}"
            )
        gp_indices.append(index)
    if len(gp_indices) != 1:
        raise LaneError(
            f"expected one generated-linker _gp assignment, found {len(gp_indices)}"
        )
    del lines[gp_indices[0]]

    main_section_indices = [
        index
        for index, line in enumerate(lines)
        if ".main_exe 0x80011000" in line
    ]
    if len(main_section_indices) != 1:
        raise LaneError(
            "expected one retail-address .main_exe output section, found "
            f"{len(main_section_indices)}"
        )
    main_section_index = main_section_indices[0]
    brace_indices = [
        index
        for index in range(main_section_index + 1, min(main_section_index + 4, len(lines)))
        if lines[index].strip() == "{"
    ]
    if len(brace_indices) != 1:
        raise LaneError("could not locate the .main_exe output-section body")
    brace_index = brace_indices[0]
    newline = "\r\n" if lines[brace_index].endswith("\r\n") else "\n"
    body_indent = _indent_of(lines[brace_index]) + "    "
    lines.insert(brace_index + 1, f"{body_indent}__load_start = .;{newline}")
    for alias, owner, addend in reversed(INITIALIZED_INTERIOR_ALIASES):
        lines.insert(
            brace_index + 2,
            f"{body_indent}{alias} = {owner} + 0x{addend:x};{newline}",
        )

    old_tail_line = f"{old_tail_object}(.data);"
    tail_indices = [index for index, line in enumerate(lines) if old_tail_line in line]
    if len(tail_indices) != 1:
        raise LaneError(
            f"expected one linker tail owner {old_tail_line!r}, found {len(tail_indices)}"
        )
    tail_index = tail_indices[0]
    lines[tail_index] = lines[tail_index].replace(old_tail_object, new_tail_object)

    starts = [index for index, line in enumerate(lines) if OLD_BSS_START_MARKER in line]
    ends = [index for index, line in enumerate(lines) if OLD_BSS_END_MARKER in line]
    if len(starts) != 1 or len(ends) != 1 or starts[0] >= ends[0]:
        raise LaneError("could not isolate the generated in-section BSS input block")
    start, end = starts[0], ends[0]
    old_bss_body = lines[start + 1 : end]
    bss_input_lines = [line for line in old_bss_body if "(.bss);" in line]
    if not bss_input_lines or not any("(.bss);" in line for line in bss_input_lines):
        raise LaneError("generated BSS block contained no .bss inputs")
    del lines[start : end + 1]

    rom_indices = [index for index, line in enumerate(lines) if ROM_END_MARKER in line]
    vram_indices = [index for index, line in enumerate(lines) if VRAM_END_MARKER in line]
    if len(rom_indices) != 1 or len(vram_indices) != 1:
        raise LaneError("could not isolate the generated ROM/VRAM end markers")
    rom_index, vram_index = rom_indices[0], vram_indices[0]
    if vram_index != rom_index + 1:
        raise LaneError("unexpected content between generated ROM and VRAM end markers")
    indent = _indent_of(lines[rom_index])
    extension_lines = [
            f"{indent}/* Normal-link extension sources. */\n",
            f"{indent}.main_exe_extension : AT(__romPos) SUBALIGN(4)\n",
            f"{indent}{{\n",
            f"{indent}    __tenchu_extension_start = .;\n",
    ]
    if ordinary_c_object_globs:
        # Existing game objects already own fixed text/rodata/data positions,
        # but those generated clauses omit -G8's .sdata output. Collect only
        # the missing small initialized sections here, immediately after the
        # original loaded tail so their signed gp offsets stay short.
        extension_lines.extend(
            f"{indent}    {object_glob}(.sdata .sdata.*);\n"
            for object_glob in ordinary_c_object_globs
        )
    extension_lines.extend(
        [
            f"{indent}    {extension_object_glob}"
            "(.text .text.* .rodata .rodata.* .data .data.* .sdata .sdata.*);\n",
            f"{indent}    __tenchu_extension_end = .;\n",
            f"{indent}}}\n",
            f"{indent}__romPos += SIZEOF(.main_exe_extension);\n",
            f"{indent}__romPos = ALIGN(__romPos, 4);\n",
            f"{indent}. = ALIGN(., 4);\n",
        ]
    )
    extension_body = "".join(extension_lines)
    lines[rom_index] = (
        extension_body
        + lines[rom_index]
        + f"{indent}main_exe_INITIALIZED_END = .;\n"
        + make_bss_body(
            indent=indent,
            bss_input_lines=bss_input_lines,
            tail_object=new_tail_object,
            extension_object_glob=extension_object_glob,
            ordinary_c_object_globs=ordinary_c_object_globs,
            aliases=aliases,
            dynamic_pool=dynamic_pool,
        )
    )
    del lines[vram_index]
    return "".join(lines)


def generate(
    *,
    linker_input: Path,
    symbols_input: Path,
    undefined_input: Path,
    tail_input: Path,
    linker_output: Path,
    symbols_output: Path,
    undefined_output: Path,
    tail_output: Path,
    old_tail_object: str,
    new_tail_object: str,
    extension_object_glob: str,
    ordinary_c_object_globs: Sequence[str] = (),
    object_replacements: Sequence[tuple[str, str]] = (),
    dynamic_pool: bool = False,
) -> tuple[int, int, int]:
    symbol_source = symbols_input.read_text()
    undefined_source = undefined_input.read_text()
    assignments = merge_assignments(symbol_source, undefined_source)
    bss_assignments = {
        name: address
        for name, address in assignments.items()
        if BSS_START <= address < BSS_END
    }
    if not bss_assignments:
        raise LaneError("symbol scripts contain no retail BSS assignments")

    filtered_symbols, removed_symbols = filter_symbol_script(symbol_source)
    filtered_undefined, removed_undefined = filter_symbol_script(undefined_source)
    removed = set(removed_symbols) | set(removed_undefined)
    owned_assignments = {
        name
        for name, address in assignments.items()
        if is_owned_assignment(name, address)
    }
    required = owned_assignments
    missing = sorted(required - removed)
    if missing:
        raise LaneError("owned symbols were not removed from scripts: " + ", ".join(missing))

    transformed_tail, tail_bss_labels = transform_tail_source(tail_input.read_text())
    padding_symbols = {
        name
        for name, address in bss_assignments.items()
        if address < BSS_PAD_END
    }
    missing_tail = sorted(padding_symbols - tail_bss_labels)
    if missing_tail:
        raise LaneError(
            "padding-range BSS symbols lack labels in 72CD0: " + ", ".join(missing_tail)
        )

    aliases = [
        Symbol(name, address)
        for name, address in sorted(
            bss_assignments.items(), key=lambda item: (item[1], item[0])
        )
        if address >= BSS_PAD_END
    ]
    rewritten_linker = rewrite_linker(
        linker_input.read_text(),
        old_tail_object=old_tail_object,
        new_tail_object=new_tail_object,
        extension_object_glob=extension_object_glob,
        aliases=aliases,
        ordinary_c_object_globs=ordinary_c_object_globs,
        object_replacements=object_replacements,
        dynamic_pool=dynamic_pool,
    )

    atomic_write(linker_output, rewritten_linker)
    atomic_write(symbols_output, filtered_symbols)
    atomic_write(undefined_output, filtered_undefined)
    atomic_write(tail_output, transformed_tail)
    initialized_assignments = {
        name
        for name, address in assignments.items()
        if MAIN_LOAD_ADDRESS <= address < BSS_START
    }
    return len(bss_assignments), len(aliases), len(initialized_assignments)


def parse_nm_posix(source: str) -> dict[str, tuple[int, str]]:
    result: dict[str, tuple[int, str]] = {}
    for line in source.splitlines():
        match = NM_POSIX_RE.fullmatch(line)
        if match is None:
            continue
        result[match.group("name")] = (
            int(match.group("value"), 16) & 0xFFFFFFFF,
            match.group("type"),
        )
    return result


def parse_readelf_sections(source: str) -> dict[str, tuple[str, int, int]]:
    result: dict[str, tuple[str, int, int]] = {}
    for line in source.splitlines():
        match = READELF_SECTION_RE.match(line)
        if match is None:
            continue
        result[match.group("name")] = (
            match.group("type"),
            int(match.group("address"), 16),
            int(match.group("size"), 16),
        )
    return result


def validate_reference(logical: bytes, reference: bytes) -> None:
    if len(logical) != LOGICAL_FILE_SIZE:
        raise LaneError(
            f"logical binary is 0x{len(logical):x} bytes, expected 0x{LOGICAL_FILE_SIZE:x}"
        )
    if len(reference) != RETAIL_FILE_SIZE:
        raise LaneError(
            f"reference executable is 0x{len(reference):x} bytes, "
            f"expected 0x{RETAIL_FILE_SIZE:x}"
        )
    if reference[LOGICAL_FILE_SIZE:] != bytes(REFERENCE_PADDING_SIZE):
        raise LaneError("reference 0x150-byte initialized/BSS overlap is not all zero")
    rebuilt = logical + bytes(REFERENCE_PADDING_SIZE)
    if rebuilt != reference:
        mismatch = next(
            index
            for index, (actual, expected) in enumerate(zip(rebuilt, reference))
            if actual != expected
        )
        raise LaneError(f"logical binary differs from retail at file offset 0x{mismatch:x}")


def validate_elf(
    *,
    elf: Path,
    nm: str,
    readelf: str,
    expected_bss_symbols: dict[str, int],
    expected_initialized_symbols: dict[str, int] | None = None,
) -> None:
    nm_result = subprocess.run(
        [nm, "-P", "-n", str(elf)],
        check=True,
        text=True,
        stdout=subprocess.PIPE,
    )
    symbols = parse_nm_posix(nm_result.stdout)
    for name, expected_address in (expected_initialized_symbols or {}).items():
        actual = symbols.get(name)
        if actual is None:
            # C-owned switch tables and other private compiler objects can
            # make a historic Splat alias obsolete: their relocations target
            # the input section directly and no public definition survives.
            # The link itself rejects an absent alias that still has users.
            continue
        address, symbol_type = actual
        if address != expected_address:
            raise LaneError(
                f"{name} is 0x{address:08x}, expected 0x{expected_address:08x}"
            )
        if symbol_type.upper() in {"A", "U"} or symbol_type == "?":
            raise LaneError(
                f"{name} is type {symbol_type}, expected a section-owned symbol"
            )
    for name, expected_address in expected_bss_symbols.items():
        actual = symbols.get(name)
        if actual is None:
            raise LaneError(f"linked ELF lacks BSS symbol {name}")
        address, symbol_type = actual
        if address != expected_address:
            raise LaneError(
                f"{name} is 0x{address:08x}, expected 0x{expected_address:08x}"
            )
        if symbol_type not in {"B", "b"}:
            raise LaneError(f"{name} is type {symbol_type}, expected linker-owned BSS")

    boundary_expectations = {
        "__load_start": (MAIN_LOAD_ADDRESS, set("TtRrDd")),
        "main": (0x800162A4, {"T", "t"}),
        "Exec": (0x800601D4, {"T", "t"}),
        "GsInitCoord2param": (0x800650D4, {"T", "t"}),
        "__bss_start": (BSS_START, {"B", "b"}),
        "__bss_end": (BSS_END, {"B", "b"}),
        "D_800CDBA8": (BSS_END, {"B", "b"}),
        "HEAP_START": (HEAP_START, {"B", "b"}),
        "MemoryPool": (MEMORY_POOL_START, {"B", "b"}),
        "MemoryPoolEnd": (MEMORY_POOL_END, {"B", "b"}),
        "_gp": (GP_ADDRESS, set("TtDdRrSsGg")),
    }
    for name, (expected_address, allowed_types) in boundary_expectations.items():
        actual = symbols.get(name)
        if actual is None:
            raise LaneError(f"linked ELF lacks boundary symbol {name}")
        address, symbol_type = actual
        if address != expected_address or symbol_type not in allowed_types:
            raise LaneError(
                f"{name} is 0x{address:08x}/{symbol_type}, expected "
                f"0x{expected_address:08x}/{''.join(sorted(allowed_types))}"
            )
        if symbol_type == "A":
            raise LaneError(f"{name} is still absolute")

    capacity = symbols.get("MemoryPoolCapacity")
    if capacity != (MEMORY_POOL_CAPACITY, "A"):
        raise LaneError(
            f"MemoryPoolCapacity is {capacity}, expected "
            f"(0x{MEMORY_POOL_CAPACITY:x}, A)"
        )

    readelf_result = subprocess.run(
        [readelf, "-SW", str(elf)],
        check=True,
        text=True,
        stdout=subprocess.PIPE,
    )
    sections = parse_readelf_sections(readelf_result.stdout)
    expected_sections = {
        ".main_exe_bss": ("NOBITS", BSS_START, BSS_END - BSS_START),
        ".main_exe_memory_pool": (
            "NOBITS",
            MEMORY_POOL_START,
            MEMORY_POOL_SIZE,
        ),
    }
    for name, expected in expected_sections.items():
        actual = sections.get(name)
        if actual != expected:
            raise LaneError(f"{name} is {actual}, expected {expected}")


def run_validate(args: argparse.Namespace) -> None:
    symbol_source = args.symbols_in.read_text()
    undefined_source = args.undefined_in.read_text()
    assignments = merge_assignments(symbol_source, undefined_source)
    expected_bss_symbols = {
        name: address
        for name, address in assignments.items()
        if BSS_START <= address < BSS_END
    }
    expected_initialized_symbols = {
        name: address
        for name, address in assignments.items()
        if MAIN_LOAD_ADDRESS <= address < BSS_START
    }
    validate_reference(args.logical.read_bytes(), args.reference.read_bytes())
    validate_elf(
        elf=args.elf,
        nm=args.nm,
        readelf=args.readelf,
        expected_bss_symbols=expected_bss_symbols,
        expected_initialized_symbols=expected_initialized_symbols,
    )


def arguments(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    generate_parser = subparsers.add_parser("generate")
    generate_parser.add_argument("--linker-in", type=Path, required=True)
    generate_parser.add_argument("--symbols-in", type=Path, required=True)
    generate_parser.add_argument("--undefined-in", type=Path, required=True)
    generate_parser.add_argument("--tail-in", type=Path, required=True)
    generate_parser.add_argument("--linker-out", type=Path, required=True)
    generate_parser.add_argument("--symbols-out", type=Path, required=True)
    generate_parser.add_argument("--undefined-out", type=Path, required=True)
    generate_parser.add_argument("--tail-out", type=Path, required=True)
    generate_parser.add_argument("--old-tail-object", required=True)
    generate_parser.add_argument("--new-tail-object", required=True)
    generate_parser.add_argument("--extension-object-glob", required=True)
    generate_parser.add_argument(
        "--ordinary-c-object-glob",
        action="append",
        default=[],
        help=(
            "normal-relink glob for already enumerated C objects; repeat for "
            "each object family whose newly emitted "
            ".sdata/.sbss/.scommon/COMMON sections belong in "
            "gp-near/owned output hooks (omitted by the retail proof)"
        ),
    )
    generate_parser.add_argument(
        "--dynamic-pool",
        action="store_true",
        help=(
            "preserve the retail MemoryPool base until linked BSS reaches it, "
            "then align the pool after BSS and reserve through 0x801fc000; the "
            "default always keeps the retail pool layout"
        ),
    )
    generate_parser.add_argument(
        "--replace-object",
        action="append",
        default=[],
        metavar="OLD=NEW",
        help="replace one exact linker input object path; repeatable",
    )

    validate_parser = subparsers.add_parser("validate")
    validate_parser.add_argument("--logical", type=Path, required=True)
    validate_parser.add_argument("--reference", type=Path, required=True)
    validate_parser.add_argument("--elf", type=Path, required=True)
    validate_parser.add_argument("--symbols-in", type=Path, required=True)
    validate_parser.add_argument("--undefined-in", type=Path, required=True)
    validate_parser.add_argument(
        "--nm", default="mipsel-unknown-linux-gnu-nm"
    )
    validate_parser.add_argument(
        "--readelf", default="mipsel-unknown-linux-gnu-readelf"
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = arguments(argv)
    try:
        if args.command == "generate":
            replacements: list[tuple[str, str]] = []
            for replacement in args.replace_object:
                if replacement.count("=") != 1:
                    raise LaneError(
                        f"invalid --replace-object {replacement!r}; expected OLD=NEW"
                    )
                old_object, new_object = replacement.split("=", 1)
                replacements.append((old_object, new_object))
            bss_symbols, aliases, initialized_symbols = generate(
                linker_input=args.linker_in,
                symbols_input=args.symbols_in,
                undefined_input=args.undefined_in,
                tail_input=args.tail_in,
                linker_output=args.linker_out,
                symbols_output=args.symbols_out,
                undefined_output=args.undefined_out,
                tail_output=args.tail_out,
                old_tail_object=args.old_tail_object,
                new_tail_object=args.new_tail_object,
                extension_object_glob=args.extension_object_glob,
                ordinary_c_object_globs=args.ordinary_c_object_glob,
                object_replacements=replacements,
                dynamic_pool=args.dynamic_pool,
            )
            print(
                f"reloc-bss lane: moved {initialized_symbols} initialized and "
                f"{bss_symbols} BSS symbols; "
                f"generated {aliases} post-padding aliases"
            )
        else:
            run_validate(args)
            print(
                "reloc-bss lane: NOLOAD BSS/MemoryPool and retail padding are exact"
            )
    except (LaneError, OSError, subprocess.SubprocessError) as error:
        print(f"reloc-bss lane: {error}")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

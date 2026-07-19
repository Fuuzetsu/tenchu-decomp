#!/usr/bin/env python3
"""Prove a positive, ordinary-layout growth of the composed MAIN.EXE relink.

The normal relink already substitutes relocation-bearing C, canonical assembly,
and reviewed data objects.  This tool makes that claim executable for positive
growth: it inserts one temporary PROGBITS input immediately after ``main``'s
real text input, links the complete executable with GNU ``ld``, finalizes the
PS-X EXE header, and checks every downstream contract against the unmodified
normal relink.

The fixture is deliberately not a patch or trampoline.  Its bytes occupy the
same genuine input boundary that extra instructions at the end of ``main``
would occupy, and no address is assigned to it.  GNU ``ld`` chooses its address
and all later addresses.  The default size, ``0x10004``, crosses a 64 KiB
boundary so linked HI16/LO16 carry handling is exercised as well as ordinary
small growth.

Run this after ``./Build check-relink`` inside the Nix development shell::

    python3 tools/reloc_growth_probe.py
"""

from __future__ import annotations

import argparse
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
import subprocess
import sys
from typing import Iterable

try:
    from tools import psxexe, reloc_audit, reloc_c_literals, reloc_data
except ModuleNotFoundError:  # Direct invocation adds tools/, not the repo root.
    import psxexe  # type: ignore[no-redef]
    import reloc_audit  # type: ignore[no-redef]
    import reloc_c_literals  # type: ignore[no-redef]
    import reloc_data  # type: ignore[no-redef]


ROOT = Path(__file__).resolve().parents[1]

DEFAULT_GROWTH = 0x10004
POOL_ALIGNMENT = 16
POOL_MINIMUM_START = 0x800DC000

BASE_ELF = Path(".shake/build/tenchu/main_relink.exe.elf")
BASE_LOGICAL = Path(".shake/build/tenchu/main_relink.logical")
BASE_EXE = Path(".shake/build/tenchu/main_relink.exe")
LINKER = Path(".shake/build/relink/layout/main.exe.ld")
SYMBOLS = Path(".shake/build/relink/layout/symbols.main.exe.txt")
UNDEFINED_SYMBOLS = Path(
    ".shake/build/relink/layout/undefined_symbols_auto.main.exe.txt"
)
UNDEFINED_FUNCTIONS = Path(
    ".shake/gen/main.exe/meta/undefined_functions_auto.main.exe.txt"
)
MANIFEST = Path("config/reloc-data.main.exe.json")
RAW_DATA_DIR = Path(".shake/gen/main.exe/asm/data")
OUTPUT_DIR = Path(".shake/build/reloc-growth-probe")

ANCHOR_INPUT = ".shake/build/main.exe/main.c.o(.text);"
PROBE_SYMBOL = "__tenchu_growth_probe"
FIRST_SHIFTED_SYMBOL = "DoBriefingAndInventorySelection"

# These names deliberately cross ownership/provenance boundaries.  ``main`` is
# before the inserted bytes; every name in SHIFTED_SYMBOLS is after them.
UNCHANGED_SYMBOLS = (
    "__load_start",
    "main",
)
SHIFTED_SYMBOLS = (
    # game C
    "DoBriefingAndInventorySelection",
    "ActivateHumans",
    "AdtSelect",
    # CRT/PsyQ and the entry point
    "Exec",
    "__SN_ENTRY_POINT",
    "GsSetLsMatrix",
    # canonical tail / initialized game data / gp
    "UnitVector2",
    "StageChar",
    "CamState",
    "D_80097D70",
    "_gp",
    "main_exe_INITIALIZED_END",
)

C_OBJECTS = {
    name: Path(".shake/reloc-c-literals") / f"{name}.o"
    for name in reloc_c_literals.OBJECT_SPECS
}


class ProbeError(RuntimeError):
    """The growth link or one of its relocation/layout invariants failed."""


@dataclass(frozen=True)
class ProbeReport:
    growth: int
    shifted_symbols: int
    owned_symbols: int
    compiler_relocations: int
    hi16_carries: int
    manifest_pointers: int
    bss_delta: int
    pool_start_before: int
    pool_start_after: int
    pool_end: int
    pool_capacity_before: int
    pool_capacity_after: int
    pc_before: int
    pc_after: int
    load_address: int
    load_size_before: int
    load_size_after: int
    audit_findings: int


def resolve(root: Path, path: Path) -> Path:
    return path if path.is_absolute() else root / path


def parse_int(value: str) -> int:
    try:
        result = int(value, 0)
    except ValueError as error:
        raise argparse.ArgumentTypeError(f"invalid integer {value!r}") from error
    if result <= 0 or result % 4:
        raise argparse.ArgumentTypeError("growth must be a positive multiple of four")
    return result


def align_up(value: int, alignment: int) -> int:
    if alignment <= 0 or alignment & (alignment - 1):
        raise ProbeError(f"alignment {alignment} is not a positive power of two")
    return (value + alignment - 1) & ~(alignment - 1)


def inject_probe(linker_source: str, probe_object: Path) -> str:
    """Insert the fixture after the one real ``main`` text input."""

    if any(character.isspace() for character in str(probe_object)):
        raise ProbeError(f"probe object path contains whitespace: {probe_object}")
    lines = linker_source.splitlines(keepends=True)
    matches = [index for index, line in enumerate(lines) if ANCHOR_INPUT in line]
    if len(matches) != 1:
        raise ProbeError(
            f"linker has {len(matches)} {ANCHOR_INPUT!r} anchors, expected one"
        )
    index = matches[0]
    indent = lines[index][: len(lines[index]) - len(lines[index].lstrip())]
    newline = "\r\n" if lines[index].endswith("\r\n") else "\n"
    insertion = (
        f"{indent}/* Temporary ordinary-layout growth fixture. */{newline}"
        f"{indent}{probe_object}(.text);{newline}"
    )
    lines.insert(index + 1, insertion)
    return "".join(lines)


def probe_assembly(growth: int) -> str:
    return f"""\
.set noreorder
.set noat
.section .text, \"ax\", @progbits
.balign 4
.globl {PROBE_SYMBOL}
.type {PROBE_SYMBOL}, @function
{PROBE_SYMBOL}:
    .space 0x{growth:x}, 0
.size {PROBE_SYMBOL}, . - {PROBE_SYMBOL}
"""


def run(command: Iterable[str], *, root: Path) -> None:
    rendered = [str(item) for item in command]
    try:
        subprocess.run(rendered, cwd=root, check=True)
    except FileNotFoundError as error:
        raise ProbeError(f"missing command {rendered[0]!r}; enter `nix develop`") from error
    except subprocess.CalledProcessError as error:
        raise ProbeError(
            f"command exited {error.returncode}: " + " ".join(rendered)
        ) from error


def require_file(path: Path, description: str) -> None:
    if not path.is_file():
        raise ProbeError(
            f"missing {description}: {path}; run `./Build check-relink` first"
        )


def extension_object_inputs(root: Path) -> list[Path]:
    """Mirror Shake's source-backed extension-object inventory.

    Shake recursively unions the user and generated C trees by their path
    relative to ``main.exe``, then passes every object below ``reloc/`` to ld.
    Derive the growth inputs from the same two source trees so a deleted source
    cannot leave a stale build object in the proof and a user override of a
    generated relative path contributes exactly one object.
    """

    source_roots = (
        root / "src/main.exe",
        root / ".shake/gen/main.exe/src",
    )
    build_root = Path(".shake/build/main.exe")
    relative_sources: set[Path] = set()
    for source_root in source_roots:
        reloc_root = source_root / "reloc"
        if not reloc_root.is_dir():
            continue
        relative_sources.update(
            source.relative_to(source_root)
            for source in reloc_root.rglob("*.c")
            if source.is_file()
        )

    inputs: list[Path] = []
    for relative_source in sorted(relative_sources):
        relative_object = build_root / Path(f"{relative_source}.o")
        require_file(root / relative_object, f"extension object for {relative_source}")
        inputs.append(relative_object)
    return inputs


def symbol(
    elf: reloc_c_literals.ElfObject, name: str
) -> reloc_c_literals.Symbol:
    result = elf.symbol(name)
    if result.section_index in (
        reloc_c_literals.SHN_UNDEF,
        reloc_c_literals.SHN_ABS,
    ):
        raise ProbeError(f"{elf.path}: {name} is not section-owned")
    return result


def verify_symbol_movement(
    base: reloc_c_literals.ElfObject,
    grown: reloc_c_literals.ElfObject,
    growth: int,
) -> None:
    for name in UNCHANGED_SYMBOLS:
        before = symbol(base, name).value
        after = symbol(grown, name).value
        if after != before:
            raise ProbeError(
                f"{name} moved 0x{before:08x}->0x{after:08x}, expected unchanged"
            )

    first_before = symbol(base, FIRST_SHIFTED_SYMBOL).value
    probe = symbol(grown, PROBE_SYMBOL)
    if probe.value != first_before:
        raise ProbeError(
            f"probe starts at 0x{probe.value:08x}, expected former downstream "
            f"boundary 0x{first_before:08x}"
        )

    for name in SHIFTED_SYMBOLS:
        before = symbol(base, name).value
        after = symbol(grown, name).value
        if after != before + growth:
            raise ProbeError(
                f"{name} moved 0x{before:08x}->0x{after:08x}, "
                f"expected +0x{growth:x}"
            )


def unique_owned_symbols(
    elf: reloc_c_literals.ElfObject,
) -> dict[str, reloc_c_literals.Symbol]:
    candidates = [
        item
        for item in elf.symbols
        if item.name
        and item.section_index
        not in (reloc_c_literals.SHN_UNDEF, reloc_c_literals.SHN_ABS)
    ]
    counts = Counter(item.name for item in candidates)
    return {item.name: item for item in candidates if counts[item.name] == 1}


def verify_all_owned_movement(
    base: reloc_c_literals.ElfObject,
    grown: reloc_c_literals.ElfObject,
    growth: int,
    bss_delta: int,
) -> int:
    """Check every uniquely named section-owned symbol in affected regions."""

    base_symbols = unique_owned_symbols(base)
    grown_symbols = unique_owned_symbols(grown)
    first_shifted = base.symbol(FIRST_SHIFTED_SYMBOL).value
    load_start = base.symbol("__load_start").value
    initialized_end = base.symbol("main_exe_INITIALIZED_END").value
    bss_start = base.symbol("__bss_start").value
    bss_end = base.symbol("__bss_end").value

    checked = 0
    for name, before in base_symbols.items():
        if load_start <= before.value < first_shifted:
            delta = 0
        elif first_shifted <= before.value <= initialized_end:
            delta = growth
        elif bss_start <= before.value <= bss_end:
            delta = bss_delta
        else:
            continue
        after = grown_symbols.get(name)
        if after is None:
            raise ProbeError(f"grown ELF lost unique section-owned symbol {name}")
        expected = before.value + delta
        if after.value != expected:
            raise ProbeError(
                f"section-owned {name} moved 0x{before.value:08x}->"
                f"0x{after.value:08x}, expected 0x{expected:08x}"
            )
        checked += 1
    if checked == 0:
        raise ProbeError("no section-owned symbols were checked")
    return checked


def verify_bss_and_pool(
    base: reloc_c_literals.ElfObject,
    grown: reloc_c_literals.ElfObject,
) -> tuple[int, int, int, int, int, int]:
    base_initialized_end = symbol(base, "main_exe_INITIALIZED_END").value
    grown_initialized_end = symbol(grown, "main_exe_INITIALIZED_END").value
    base_bss_start = symbol(base, "__bss_start").value
    grown_bss_start = symbol(grown, "__bss_start").value
    base_bss_end = symbol(base, "__bss_end").value
    grown_bss_end = symbol(grown, "__bss_end").value

    if base_bss_start != align_up(base_initialized_end, 16):
        raise ProbeError("baseline BSS does not follow aligned initialized data")
    if grown_bss_start != align_up(grown_initialized_end, 16):
        raise ProbeError("grown BSS does not follow aligned initialized data")
    if grown_bss_end - grown_bss_start != base_bss_end - base_bss_start:
        raise ProbeError("growth fixture unexpectedly changed BSS size")
    bss_delta = grown_bss_start - base_bss_start
    if grown_bss_end - base_bss_end != bss_delta:
        raise ProbeError("BSS start and end did not move by the same amount")

    for elf, bss_end in ((base, base_bss_end), (grown, grown_bss_end)):
        if symbol(elf, "D_800CDBA8").value != bss_end:
            raise ProbeError(f"{elf.path}: crt0 BSS clear end is stale")
        if symbol(elf, "HEAP_START").value != bss_end + 4:
            raise ProbeError(f"{elf.path}: HEAP_START does not follow BSS")

    base_pool = symbol(base, "MemoryPool").value
    grown_pool = symbol(grown, "MemoryPool").value
    base_pool_end = symbol(base, "MemoryPoolEnd").value
    grown_pool_end = symbol(grown, "MemoryPoolEnd").value
    expected_base_pool = max(
        POOL_MINIMUM_START, align_up(base_bss_end, POOL_ALIGNMENT)
    )
    expected_grown_pool = max(
        POOL_MINIMUM_START, align_up(grown_bss_end, POOL_ALIGNMENT)
    )
    if base_pool != expected_base_pool:
        raise ProbeError(
            "baseline MemoryPool does not honor its retail minimum/aligned BSS"
        )
    if grown_pool != expected_grown_pool:
        raise ProbeError(
            "grown MemoryPool does not honor its retail minimum/aligned BSS"
        )
    if grown_pool <= base_pool:
        raise ProbeError("positive image growth did not advance MemoryPool")
    if grown_pool_end != base_pool_end:
        raise ProbeError("MemoryPoolEnd moved instead of remaining the fixed upper bound")
    for elf, pool_end in ((base, base_pool_end), (grown, grown_pool_end)):
        if symbol(elf, "main_exe_MEMORY_POOL_END").value != pool_end:
            raise ProbeError(f"{elf.path}: pool end aliases disagree")

    capacities: list[int] = []
    for elf, pool_start, pool_end in (
        (base, base_pool, base_pool_end),
        (grown, grown_pool, grown_pool_end),
    ):
        capacity = elf.symbol("MemoryPoolCapacity")
        expected = (pool_end - pool_start) // 4 - 2
        if capacity.section_index != reloc_c_literals.SHN_ABS:
            raise ProbeError(f"{elf.path}: MemoryPoolCapacity is not a scalar")
        if capacity.value != expected:
            raise ProbeError(
                f"{elf.path}: MemoryPoolCapacity=0x{capacity.value:x}, "
                f"expected 0x{expected:x}"
            )
        capacities.append(capacity.value)
    if capacities[1] >= capacities[0]:
        raise ProbeError("grown MemoryPool capacity did not shrink")

    return (
        bss_delta,
        base_pool,
        grown_pool,
        base_pool_end,
        capacities[0],
        capacities[1],
    )


def owner_retail_start(source: str, owner_name: str, description: Path) -> int:
    """Resolve a generated dlabel's retail start from its first byte comment."""

    owner: str | None = None
    found: list[int] = []
    for line in source.splitlines():
        start = reloc_data.DLABEL_RE.fullmatch(line.rstrip("\r\n"))
        if start is not None:
            owner = start.group(1)
            continue
        end = reloc_data.ENDDLABEL_RE.fullmatch(line.rstrip("\r\n"))
        if end is not None:
            owner = None
            continue
        if owner != owner_name:
            continue
        address = reloc_data.ADDRESS_RE.search(line)
        if address is not None:
            found.append(int(address.group("address"), 16))
            break
    if len(found) != 1:
        raise ProbeError(
            f"{description}: expected one retail start for owner {owner_name}, "
            f"found {len(found)}"
        )
    return found[0]


def manifest_owner_starts(
    entries: list[reloc_data.PointerEntry], raw_dir: Path
) -> dict[tuple[Path, str], int]:
    requested = {(entry.source_file, entry.source_owner) for entry in entries}
    sources: dict[Path, str] = {}
    starts: dict[tuple[Path, str], int] = {}
    for file_key, owner in sorted(requested, key=lambda item: (str(item[0]), item[1])):
        path = raw_dir / file_key
        require_file(path, f"generated owner source {file_key}")
        source = sources.setdefault(file_key, path.read_text())
        starts[(file_key, owner)] = owner_retail_start(source, owner, path)
    return starts


def verify_manifest_pointers(
    base: reloc_c_literals.ElfObject,
    grown: reloc_c_literals.ElfObject,
    entries: list[reloc_data.PointerEntry],
    owner_starts: dict[tuple[Path, str], int],
    growth: int,
    bss_delta: int,
) -> int:
    seen_sources: set[tuple[Path, int]] = set()
    saw_stage_config = False
    saw_bss_cursor = False
    for entry in entries:
        source_key = (entry.source_file, entry.source_address)
        if source_key in seen_sources:
            raise ProbeError(
                f"duplicate manifest source {entry.source_file}:"
                f"0x{entry.source_address:08x}"
            )
        seen_sources.add(source_key)
        owner_start = owner_starts[(entry.source_file, entry.source_owner)]
        offset = entry.source_address - owner_start
        if offset < 0:
            raise ProbeError(
                f"manifest source {entry.source_file}:0x{entry.source_address:08x} "
                f"precedes owner {entry.source_owner} at 0x{owner_start:08x}"
            )
        base_owner = symbol(base, entry.source_owner).value
        grown_owner = symbol(grown, entry.source_owner).value
        base_source = base_owner + offset
        grown_source = grown_owner + offset
        base_target = (
            symbol(base, entry.symbol).value + entry.target_addend
        ) & 0xFFFFFFFF
        grown_target = (
            symbol(grown, entry.symbol).value + entry.target_addend
        ) & 0xFFFFFFFF
        base_word = base.word_at_address(base_source)
        grown_word = grown.word_at_address(grown_source)
        if base_word != base_target:
            raise ProbeError(
                f"baseline pointer 0x{base_source:08x}=0x{base_word:08x}, "
                f"expected {entry.symbol}=0x{base_target:08x}"
            )
        if grown_word != grown_target:
            raise ProbeError(
                f"grown pointer 0x{grown_source:08x}=0x{grown_word:08x}, "
                f"expected {entry.symbol}=0x{grown_target:08x}"
            )

        if entry.source_owner == "StageConfig":
            saw_stage_config = True
            if grown_source != base_source or grown_target != base_target:
                raise ProbeError(
                    "StageConfig source/leading-data target moved across a later "
                    "game-code growth boundary"
                )
        if entry.source_address == 0x80097E98:
            saw_bss_cursor = True
            if grown_source - base_source != growth:
                raise ProbeError("D_80097E98 source did not follow initialized data")
            if grown_target - base_target != bss_delta:
                raise ProbeError(
                    "D_80097E98 target did not follow aligned linker-owned BSS"
                )
    # These cases enter the manifest together with the broadened byte/unaligned
    # audit.  Once present, keep their different movement semantics executable.
    if any(entry.source_owner == "StageConfig" for entry in entries) and not saw_stage_config:
        raise ProbeError("StageConfig manifest records were not checked")
    if any(entry.source_address == 0x80097E98 for entry in entries) and not saw_bss_cursor:
        raise ProbeError("D_80097E98 manifest record was not checked")
    return len(seen_sources)


def count_hi16_carries(
    base: reloc_c_literals.ElfObject,
    grown: reloc_c_literals.ElfObject,
    objects: dict[str, Path],
    *,
    require_carry: bool,
) -> int:
    """Count audited compiler target pairs whose linked HI16 must change."""

    carries = 0
    for function_name, spec in reloc_c_literals.OBJECT_SPECS.items():
        obj = reloc_c_literals.ElfObject(objects[function_name])
        relocations = obj.relocations(".rel.text")
        object_text = obj.section_data(".text")
        for target_name in spec.targets:
            lows = [
                relocation
                for relocation in relocations
                if relocation.symbol == target_name
                and relocation.type == reloc_c_literals.R_MIPS_LO16
            ]
            if not lows:
                raise ProbeError(
                    f"{function_name}: no LO16 record for {target_name}"
                )
            addends = {
                reloc_c_literals.sign_extend_16(
                    reloc_c_literals.instruction_word(
                        object_text, relocation.offset, function_name
                    )
                )
                for relocation in lows
            }
            if len(addends) != 1:
                raise ProbeError(
                    f"{function_name}: inconsistent LO16 addends for {target_name}"
                )
            addend = next(iter(addends))
            before = (base.symbol(target_name).value + addend) & 0xFFFFFFFF
            after = (grown.symbol(target_name).value + addend) & 0xFFFFFFFF
            before_high = ((before + 0x8000) >> 16) & 0xFFFF
            after_high = ((after + 0x8000) >> 16) & 0xFFFF
            if before_high != after_high:
                carries += 1
    if require_carry and carries == 0:
        raise ProbeError("+0x10004 probe exercised no audited HI16 carry")
    return carries


def verify_headers(
    *,
    base_elf: reloc_c_literals.ElfObject,
    grown_elf: reloc_c_literals.ElfObject,
    base_logical: Path,
    grown_logical: Path,
    base_exe: Path,
    grown_exe: Path,
    growth: int,
) -> tuple[psxexe.PsxExeHeader, psxexe.PsxExeHeader]:
    base_logical_bytes = base_logical.read_bytes()
    grown_logical_bytes = grown_logical.read_bytes()
    if len(grown_logical_bytes) - len(base_logical_bytes) != growth:
        raise ProbeError(
            "logical initialized image grew by "
            f"0x{len(grown_logical_bytes) - len(base_logical_bytes):x}, "
            f"expected 0x{growth:x}"
        )

    base_header = psxexe.validate_image(
        base_exe.read_bytes(),
        source=str(base_exe),
        expected={"gp": 0, "sp": 0x801FFFF0},
    )
    entry = symbol(grown_elf, "__SN_ENTRY_POINT").value
    load = symbol(grown_elf, "__load_start").value
    finalized, grown_header, _padding = psxexe.finalize_image(
        grown_logical_bytes,
        entry=entry,
        load_address=load,
        source=str(grown_logical),
        expected={"gp": 0, "sp": 0x801FFFF0},
    )
    grown_exe.write_bytes(finalized)

    base_entry = symbol(base_elf, "__SN_ENTRY_POINT").value
    base_load = symbol(base_elf, "__load_start").value
    if base_header.pc != base_entry or grown_header.pc != entry:
        raise ProbeError("PS-X EXE PC does not equal the linked entry symbol")
    if grown_header.pc - base_header.pc != growth:
        raise ProbeError("finalized PC did not follow the inserted code")
    if base_header.t_addr != base_load or grown_header.t_addr != load:
        raise ProbeError("PS-X EXE load address does not equal __load_start")
    if grown_header.t_addr != base_header.t_addr:
        raise ProbeError("fixed MAIN.EXE load origin unexpectedly moved")

    for logical, header, description in (
        (base_logical_bytes, base_header, "baseline"),
        (grown_logical_bytes, grown_header, "grown"),
    ):
        expected_size = align_up(
            len(logical) - psxexe.HEADER_SIZE, psxexe.SECTOR_SIZE
        )
        if header.t_size != expected_size:
            raise ProbeError(
                f"{description} t_size=0x{header.t_size:x}, "
                f"expected 0x{expected_size:x}"
            )
    if grown_header.t_size <= base_header.t_size:
        raise ProbeError("positive logical growth did not increase finalized t_size")

    for field in psxexe.HEADER_FIELDS:
        if field in {"pc", "t_addr", "t_size"}:
            continue
        if getattr(grown_header, field) != getattr(base_header, field):
            raise ProbeError(f"finalizer unexpectedly changed header field {field}")
    return base_header, grown_header


def verify_audit(
    *,
    root: Path,
    elf: Path,
    linker: Path,
    raw_dir: Path,
    nm: str,
) -> int:
    report = reloc_audit.collect(root, elf, linker, raw_dir, nm)
    summary = report["summary"]
    findings = int(summary["finding_count"])
    if findings:
        raise ProbeError(
            f"grown executable has {findings} movable fixed-address audit findings"
        )
    return findings


def arguments(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--growth", type=parse_int, default=DEFAULT_GROWTH)
    parser.add_argument("--base-elf", type=Path, default=BASE_ELF)
    parser.add_argument("--base-logical", type=Path, default=BASE_LOGICAL)
    parser.add_argument("--base-exe", type=Path, default=BASE_EXE)
    parser.add_argument("--linker", type=Path, default=LINKER)
    parser.add_argument("--symbols", type=Path, default=SYMBOLS)
    parser.add_argument("--undefined-symbols", type=Path, default=UNDEFINED_SYMBOLS)
    parser.add_argument("--undefined-functions", type=Path, default=UNDEFINED_FUNCTIONS)
    parser.add_argument("--manifest", type=Path, default=MANIFEST)
    parser.add_argument("--raw-dir", type=Path, default=RAW_DATA_DIR)
    parser.add_argument("--output-dir", type=Path, default=OUTPUT_DIR)
    parser.add_argument("--as", dest="assembler", default="mipsel-unknown-linux-gnu-as")
    parser.add_argument("--ld", default="mipsel-unknown-linux-gnu-ld")
    parser.add_argument(
        "--objcopy", default="mipsel-unknown-linux-gnu-objcopy"
    )
    parser.add_argument("--nm", default="mipsel-unknown-linux-gnu-nm")
    return parser.parse_args(argv)


def execute(args: argparse.Namespace) -> ProbeReport:
    root = args.root.resolve()
    base_elf_path = resolve(root, args.base_elf)
    base_logical = resolve(root, args.base_logical)
    base_exe = resolve(root, args.base_exe)
    linker = resolve(root, args.linker)
    symbols = resolve(root, args.symbols)
    undefined_symbols = resolve(root, args.undefined_symbols)
    undefined_functions = resolve(root, args.undefined_functions)
    manifest = resolve(root, args.manifest)
    raw_dir = resolve(root, args.raw_dir)
    output_dir = resolve(root, args.output_dir)

    for path, description in (
        (base_elf_path, "baseline relink ELF"),
        (base_logical, "baseline logical image"),
        (base_exe, "baseline finalized PS-X EXE"),
        (linker, "composed linker script"),
        (symbols, "composed symbol script"),
        (undefined_symbols, "composed undefined-symbol script"),
        (undefined_functions, "undefined-function script"),
        (manifest, "loaded-pointer manifest"),
    ):
        require_file(path, description)
    if not raw_dir.is_dir():
        raise ProbeError(f"missing generated raw-data directory: {raw_dir}")
    object_paths = {name: resolve(root, path) for name, path in C_OBJECTS.items()}
    for name, path in object_paths.items():
        require_file(path, f"symbolic-C object {name}")

    output_dir.mkdir(parents=True, exist_ok=True)
    probe_source = output_dir / "growth_probe.s"
    probe_object = output_dir / "growth_probe.s.o"
    grown_linker = output_dir / "main.exe.ld"
    grown_elf_path = output_dir / "main_growth.exe.elf"
    grown_map = output_dir / "main_growth.exe.map"
    grown_logical = output_dir / "main_growth.logical"
    grown_exe = output_dir / "main_growth.exe"

    probe_source.write_text(probe_assembly(args.growth))
    run(
        (
            args.assembler,
            "-EL",
            "-Iinclude",
            "-march=r3000",
            "-mtune=r3000",
            "-no-pad-sections",
            "-O1",
            "-G0",
            "-o",
            str(probe_object),
            str(probe_source),
        ),
        root=root,
    )
    probe_input = reloc_c_literals.ElfObject(probe_object)
    if probe_input.section(".text").size != args.growth:
        raise ProbeError(
            f"probe object text is 0x{probe_input.section('.text').size:x}, "
            f"expected 0x{args.growth:x}"
        )

    grown_linker.write_text(inject_probe(linker.read_text(), probe_object.resolve()))
    extension_objects = extension_object_inputs(root)
    link_command = [
        args.ld,
        "-EL",
        "-o",
        str(grown_elf_path),
        "-Map",
        str(grown_map),
        "-T",
        str(grown_linker),
        "-T",
        str(symbols),
        "-T",
        str(undefined_symbols),
        "-T",
        str(undefined_functions),
        "--orphan-handling=error",
        "--no-check-sections",
        "-nostdlib",
        str(probe_object),
    ]
    link_command.extend(str(path) for path in extension_objects)
    run(link_command, root=root)
    run(
        (
            args.objcopy,
            "-O",
            "binary",
            str(grown_elf_path),
            str(grown_logical),
        ),
        root=root,
    )

    base_elf = reloc_c_literals.ElfObject(base_elf_path)
    grown_elf = reloc_c_literals.ElfObject(grown_elf_path)
    verify_symbol_movement(base_elf, grown_elf, args.growth)
    (
        bss_delta,
        pool_before,
        pool_after,
        pool_end,
        capacity_before,
        capacity_after,
    ) = verify_bss_and_pool(base_elf, grown_elf)
    owned_symbols = verify_all_owned_movement(
        base_elf, grown_elf, args.growth, bss_delta
    )

    for name, path in sorted(object_paths.items()):
        reloc_c_literals.verify_contract(
            reloc_c_literals.ElfObject(path),
            reloc_c_literals.OBJECT_SPECS[name],
            name,
        )
    linked_reports = reloc_c_literals.verify_linked_relocations(
        grown_elf, object_paths
    )
    hi16_carries = count_hi16_carries(
        base_elf,
        grown_elf,
        object_paths,
        require_carry=args.growth >= 0x10000,
    )

    entries = reloc_data.load_manifest(manifest)
    owner_starts = manifest_owner_starts(entries, raw_dir)
    pointer_count = verify_manifest_pointers(
        base_elf,
        grown_elf,
        entries,
        owner_starts,
        args.growth,
        bss_delta,
    )
    base_header, grown_header = verify_headers(
        base_elf=base_elf,
        grown_elf=grown_elf,
        base_logical=base_logical,
        grown_logical=grown_logical,
        base_exe=base_exe,
        grown_exe=grown_exe,
        growth=args.growth,
    )
    findings = verify_audit(
        root=root,
        elf=grown_elf_path,
        linker=grown_linker,
        raw_dir=raw_dir,
        nm=args.nm,
    )

    return ProbeReport(
        growth=args.growth,
        shifted_symbols=len(SHIFTED_SYMBOLS),
        owned_symbols=owned_symbols,
        compiler_relocations=len(linked_reports),
        hi16_carries=hi16_carries,
        manifest_pointers=pointer_count,
        bss_delta=bss_delta,
        pool_start_before=pool_before,
        pool_start_after=pool_after,
        pool_end=pool_end,
        pool_capacity_before=capacity_before,
        pool_capacity_after=capacity_after,
        pc_before=base_header.pc,
        pc_after=grown_header.pc,
        load_address=grown_header.t_addr,
        load_size_before=base_header.t_size,
        load_size_after=grown_header.t_size,
        audit_findings=findings,
    )


def render(report: ProbeReport) -> str:
    return "\n".join(
        (
            "reloc-growth-probe: ordinary GNU ld growth verified",
            f"  inserted text: +0x{report.growth:x} after main.c.o",
            f"  downstream representatives: {report.shifted_symbols} moved",
            f"  section-owned layout symbols: {report.owned_symbols} checked",
            f"  compiler target pairs: {report.compiler_relocations} linked; "
            f"{report.hi16_carries} changed HI16",
            f"  loaded pointers: {report.manifest_pointers} resolve at moved sources",
            f"  BSS delta: +0x{report.bss_delta:x}",
            "  MemoryPool: "
            f"0x{report.pool_start_before:08x}->0x{report.pool_start_after:08x} "
            f"through 0x{report.pool_end:08x}; capacity "
            f"0x{report.pool_capacity_before:x}->0x{report.pool_capacity_after:x} words",
            "  PS-X header: "
            f"PC 0x{report.pc_before:08x}->0x{report.pc_after:08x}, "
            f"load 0x{report.load_address:08x}, "
            f"t_size 0x{report.load_size_before:x}->0x{report.load_size_after:x}",
            f"  movable fixed-address findings: {report.audit_findings}",
        )
    )


def main(argv: list[str] | None = None) -> int:
    args = arguments(argv)
    try:
        report = execute(args)
    except (
        ProbeError,
        reloc_audit.AuditError,
        reloc_c_literals.AuditError,
        psxexe.PsxExeError,
        reloc_data.RelocDataError,
        OSError,
    ) as error:
        print(f"reloc-growth-probe: {error}", file=sys.stderr)
        return 2
    print(render(report))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

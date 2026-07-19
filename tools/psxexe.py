#!/usr/bin/env python3
"""Finalize and validate a sector-aligned PS-X EXE image.

This is a standalone building block for a future normally-linked executable
lane.  It is deliberately not wired into Shake: the current byte-matching
output keeps using its generated retail header unchanged.

``finalize`` takes an EXE-shaped linker/objcopy output whose first 0x800 bytes
are a PS-X EXE header template.  It preserves that template byte-for-byte
except for the three layout fields that a normal link must regenerate:

* initial PC (header offset 0x10),
* load address / ``t_addr`` (0x18), and
* sector-padded payload size / ``t_size`` (0x1c).

The entry and load address can be explicit, read from named symbols in an ELF
or GNU ld map, or (for an ELF) derived from ``e_entry`` and its lowest loadable
PSX segment.  ``validate`` checks an existing image without changing it.

Examples::

    tools/psxexe.py finalize linked.exe -o game.exe \
        --elf game.elf --entry-symbol __SN_ENTRY_POINT \
        --load-symbol __load_start
    tools/psxexe.py finalize linked.exe -o game.exe \
        --entry 0x80020000 --load-address 0x80011000
    tools/psxexe.py validate game.exe --expect gp=0 \
        --expect sp=0x801ffff0

Run ``tools/psxexe.py <command> --help`` for the complete interface.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import os
from pathlib import Path
import re
import stat
import struct
import sys
import tempfile
from typing import Iterable, Mapping, Sequence


PSX_EXE_MAGIC = b"PS-X EXE"
HEADER_SIZE = 0x800
SECTOR_SIZE = 0x800
UINT32_LIMIT = 1 << 32

# Names intentionally follow the conventional PS-X EXE documentation while
# retaining t_addr/t_size, the names used by splat and the rest of this repo.
HEADER_FIELDS = {
    "pc": 0x10,
    "gp": 0x14,
    "t_addr": 0x18,
    "t_size": 0x1C,
    "d_addr": 0x20,
    "d_size": 0x24,
    "b_addr": 0x28,
    "b_size": 0x2C,
    "sp": 0x30,
    "sp_offset": 0x34,
}

FIELD_ALIASES = {
    "entry": "pc",
    "entry_point": "pc",
    "load": "t_addr",
    "load_address": "t_addr",
    "load_size": "t_size",
    "data_address": "d_addr",
    "data_size": "d_size",
    "bss_address": "b_addr",
    "bss_size": "b_size",
    "stack": "sp",
    "stack_offset": "sp_offset",
}


class PsxExeError(RuntimeError):
    """A PS-X EXE, ELF, map, or CLI invariant was violated."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise PsxExeError(message)


def parse_int(value: str | int) -> int:
    """Parse a CLI/map integer and reject values outside an unsigned word."""
    if isinstance(value, int):
        number = value
    else:
        try:
            number = int(value, 0)
        except ValueError as error:
            raise PsxExeError(f"invalid integer {value!r}") from error
    require(0 <= number < UINT32_LIMIT, f"value {number:#x} is not a 32-bit word")
    return number


def normalize_address(value: int, description: str) -> int:
    """Normalize a 32-bit address sign-extended by a 64-bit host linker."""
    if 0 <= value < UINT32_LIMIT:
        return value
    if 0 <= value < (1 << 64) and value >> 32 == 0xFFFFFFFF:
        return value & 0xFFFFFFFF
    raise PsxExeError(f"{description} value {value:#x} is not a 32-bit address")


@dataclass(frozen=True)
class PsxExeHeader:
    pc: int
    gp: int
    t_addr: int
    t_size: int
    d_addr: int
    d_size: int
    b_addr: int
    b_size: int
    sp: int
    sp_offset: int

    @classmethod
    def from_bytes(cls, data: bytes | bytearray, source: str = "image") -> "PsxExeHeader":
        require(len(data) >= HEADER_SIZE, f"{source}: shorter than the 0x800-byte header")
        require(data[:8] == PSX_EXE_MAGIC, f"{source}: missing 'PS-X EXE' magic")
        values = {
            name: struct.unpack_from("<I", data, offset)[0]
            for name, offset in HEADER_FIELDS.items()
        }
        return cls(**values)

    def fields(self) -> dict[str, int]:
        return {name: getattr(self, name) for name in HEADER_FIELDS}


def _validate_range(address: int, size: int, label: str) -> None:
    require(address + size <= UINT32_LIMIT, f"{label} range wraps the 32-bit address space")


def validate_image(
    data: bytes | bytearray,
    *,
    source: str = "image",
    expected: Mapping[str, int] | None = None,
) -> PsxExeHeader:
    """Validate the structural and address invariants of a complete PS-X EXE."""
    header = PsxExeHeader.from_bytes(data, source)

    require(header.t_size > 0, f"{source}: t_size is zero")
    require(
        header.t_size % SECTOR_SIZE == 0,
        f"{source}: t_size {header.t_size:#x} is not a multiple of 0x800",
    )
    require(
        len(data) == HEADER_SIZE + header.t_size,
        f"{source}: file size {len(data):#x} != 0x800 + t_size {header.t_size:#x}",
    )
    require(header.pc % 4 == 0, f"{source}: PC {header.pc:#x} is not word-aligned")
    require(
        header.t_addr % 4 == 0,
        f"{source}: load address {header.t_addr:#x} is not word-aligned",
    )
    _validate_range(header.t_addr, header.t_size, f"{source}: text")
    require(
        header.t_addr <= header.pc < header.t_addr + header.t_size,
        f"{source}: PC {header.pc:#x} is outside loaded payload "
        f"{header.t_addr:#x}..{header.t_addr + header.t_size:#x}",
    )

    for name, value in (
        ("GP", header.gp),
        ("data address", header.d_addr),
        ("BSS address", header.b_addr),
        ("SP", header.sp),
    ):
        require(value == 0 or value % 4 == 0, f"{source}: {name} {value:#x} is not word-aligned")
    if header.d_size:
        require(header.d_addr != 0, f"{source}: nonzero data size has a zero data address")
        _validate_range(header.d_addr, header.d_size, f"{source}: data")
    if header.b_size:
        require(header.b_addr != 0, f"{source}: nonzero BSS size has a zero BSS address")
        _validate_range(header.b_addr, header.b_size, f"{source}: BSS")

    for field, wanted in (expected or {}).items():
        require(field in HEADER_FIELDS, f"unknown PS-X EXE header field {field!r}")
        actual = getattr(header, field)
        require(
            actual == wanted,
            f"{source}: expected {field}={wanted:#x}, found {actual:#x}",
        )
    return header


def finalize_image(
    data: bytes,
    *,
    entry: int,
    load_address: int,
    source: str = "image",
    expected: Mapping[str, int] | None = None,
) -> tuple[bytes, PsxExeHeader, int]:
    """Regenerate PC/t_addr/t_size, sector-pad, then validate an EXE image."""
    # Parse before taking the payload so malformed/raw input cannot accidentally
    # be blessed with an unrelated header.
    PsxExeHeader.from_bytes(data, source)
    require(len(data) > HEADER_SIZE, f"{source}: payload is empty")
    entry = normalize_address(entry, "entry")
    load_address = normalize_address(load_address, "load address")

    payload = data[HEADER_SIZE:]
    unpadded_size = len(payload)
    _validate_range(load_address, unpadded_size, f"{source}: unpadded text")
    require(
        load_address <= entry < load_address + unpadded_size,
        f"{source}: entry {entry:#x} is outside the unpadded payload "
        f"{load_address:#x}..{load_address + unpadded_size:#x}",
    )
    padding = (-len(payload)) % SECTOR_SIZE
    if padding:
        payload += bytes(padding)
    require(len(payload) < UINT32_LIMIT, f"{source}: padded payload is too large for t_size")

    old_header = data[:HEADER_SIZE]
    new_header = bytearray(old_header)
    struct.pack_into("<I", new_header, HEADER_FIELDS["pc"], entry)
    struct.pack_into("<I", new_header, HEADER_FIELDS["t_addr"], load_address)
    struct.pack_into("<I", new_header, HEADER_FIELDS["t_size"], len(payload))

    # Make preservation executable, not merely documentary: these are the only
    # bytes finalization is permitted to alter in the template.
    mutable = set()
    for field in ("pc", "t_addr", "t_size"):
        offset = HEADER_FIELDS[field]
        mutable.update(range(offset, offset + 4))
    changed_outside_layout = [
        offset
        for offset, (before, after) in enumerate(zip(old_header, new_header))
        if before != after and offset not in mutable
    ]
    if changed_outside_layout:
        raise PsxExeError(
            f"{source}: finalizer changed non-layout header byte "
            f"{changed_outside_layout[0]:#x}"
        )

    result = bytes(new_header) + payload
    combined_expected = dict(expected or {})
    for field, value in (("pc", entry), ("t_addr", load_address), ("t_size", len(payload))):
        if field in combined_expected:
            require(
                combined_expected[field] == value,
                f"{source}: --expect {field}={combined_expected[field]:#x} conflicts "
                f"with finalized value {value:#x}",
            )
        combined_expected[field] = value
    header = validate_image(result, source=source, expected=combined_expected)
    return result, header, padding


@dataclass(frozen=True)
class ElfMetadata:
    path: Path
    entry: int
    load_addresses: tuple[int, ...]
    alloc_addresses: tuple[int, ...]
    symbols: Mapping[str, int]

    def symbol(self, name: str) -> int:
        try:
            value = self.symbols[name]
        except KeyError as error:
            raise PsxExeError(f"{self.path}: ELF has no defined symbol {name!r}") from error
        return normalize_address(value, f"ELF symbol {name}")

    def load_address(self) -> int:
        candidates = self.load_addresses or self.alloc_addresses
        require(bool(candidates), f"{self.path}: ELF has no nonempty addressed load/alloc section")
        # A linked PS-X image can contain a header PT_LOAD at vaddr zero.  Prefer
        # the normal cached/uncached MIPS segments when they exist; otherwise the
        # lowest nonzero candidate is still useful for an unusual linker script.
        psx = tuple(address for address in candidates if 0x80000000 <= address < 0xC0000000)
        return normalize_address(min(psx or candidates), "ELF load address")


def _checked_slice(data: bytes, offset: int, size: int, description: str) -> bytes:
    require(offset >= 0 and size >= 0, f"{description}: negative file range")
    require(offset + size <= len(data), f"{description}: truncated file range")
    return data[offset : offset + size]


def _cstring(data: bytes, offset: int, description: str) -> str:
    require(0 <= offset < len(data), f"{description}: string offset {offset:#x} is out of range")
    end = data.find(b"\0", offset)
    require(end >= 0, f"{description}: unterminated string")
    return data[offset:end].decode("utf-8", errors="replace")


def read_elf_metadata(path: Path) -> ElfMetadata:
    """Read enough ELF32/ELF64 metadata for address and symbol resolution."""
    data = path.read_bytes()
    require(len(data) >= 16 and data[:4] == b"\x7fELF", f"{path}: not an ELF file")
    elf_class = data[4]
    byte_order = data[5]
    require(elf_class in (1, 2), f"{path}: unsupported ELF class {elf_class}")
    require(byte_order in (1, 2), f"{path}: unsupported ELF byte order {byte_order}")
    endian = "<" if byte_order == 1 else ">"

    if elf_class == 1:
        require(len(data) >= 52, f"{path}: truncated ELF32 header")
        header = struct.unpack_from(endian + "16sHHIIIIIHHHHHH", data, 0)
        entry, phoff, shoff = header[4], header[5], header[6]
        phentsize, phnum = header[9], header[10]
        shentsize, shnum = header[11], header[12]
        expected_phentsize, expected_shentsize = 32, 40
        ph_format = endian + "IIIIIIII"
        sh_format = endian + "IIIIIIIIII"
        sym_format, expected_symentsize = endian + "IIIBBH", 16
    else:
        require(len(data) >= 64, f"{path}: truncated ELF64 header")
        header = struct.unpack_from(endian + "16sHHIQQQIHHHHHH", data, 0)
        entry, phoff, shoff = header[4], header[5], header[6]
        phentsize, phnum = header[9], header[10]
        shentsize, shnum = header[11], header[12]
        expected_phentsize, expected_shentsize = 56, 64
        ph_format = endian + "IIQQQQQQ"
        sh_format = endian + "IIQQQQIIQQ"
        sym_format, expected_symentsize = endian + "IBBHQQ", 24

    require(phnum != 0xFFFF, f"{path}: extended ELF program-header counts are unsupported")
    require(not (shoff and shnum == 0), f"{path}: extended ELF section counts are unsupported")

    load_addresses: list[int] = []
    if phnum:
        require(phentsize == expected_phentsize, f"{path}: unexpected program-header size")
        _checked_slice(data, phoff, phentsize * phnum, f"{path}: program headers")
        for index in range(phnum):
            values = struct.unpack_from(ph_format, data, phoff + index * phentsize)
            if elf_class == 1:
                p_type, _, vaddr, _, filesz = values[:5]
            else:
                p_type, _, _, vaddr, _, filesz = values[:6]
            if p_type == 1 and filesz and vaddr:
                load_addresses.append(vaddr)

    sections: list[dict[str, int]] = []
    if shnum:
        require(shentsize == expected_shentsize, f"{path}: unexpected section-header size")
        _checked_slice(data, shoff, shentsize * shnum, f"{path}: section headers")
        for index in range(shnum):
            values = struct.unpack_from(sh_format, data, shoff + index * shentsize)
            sections.append(
                {
                    "type": values[1],
                    "flags": values[2],
                    "address": values[3],
                    "offset": values[4],
                    "size": values[5],
                    "link": values[6],
                    "entry_size": values[9],
                }
            )

    alloc_addresses = [
        section["address"]
        for section in sections
        if section["flags"] & 0x2 and section["type"] != 8
        and section["size"] and section["address"]
    ]

    symbol_values: dict[str, set[int]] = {}
    for section in sections:
        if section["type"] not in (2, 11):  # SHT_SYMTAB / SHT_DYNSYM
            continue
        require(section["entry_size"] == expected_symentsize, f"{path}: unexpected symbol size")
        require(
            section["link"] < len(sections),
            f"{path}: symbol table has invalid string-table link",
        )
        raw_symbols = _checked_slice(
            data, section["offset"], section["size"], f"{path}: symbol table"
        )
        strings_section = sections[section["link"]]
        strings = _checked_slice(
            data,
            strings_section["offset"],
            strings_section["size"],
            f"{path}: symbol strings",
        )
        require(
            len(raw_symbols) % expected_symentsize == 0,
            f"{path}: truncated symbol table",
        )
        for offset in range(0, len(raw_symbols), expected_symentsize):
            values = struct.unpack_from(sym_format, raw_symbols, offset)
            if elf_class == 1:
                name_offset, value, _, _, _, section_index = values
            else:
                name_offset, _, _, section_index, value, _ = values
            if name_offset == 0 or section_index == 0:  # empty / SHN_UNDEF
                continue
            name = _cstring(strings, name_offset, f"{path}: symbol")
            if name:
                symbol_values.setdefault(name, set()).add(value)

    symbols: dict[str, int] = {}
    for name, values in symbol_values.items():
        if len(values) == 1:
            symbols[name] = next(iter(values))
        # Different definitions are intentionally omitted: asking for one then
        # produces a clear "no defined symbol" instead of choosing arbitrarily.

    return ElfMetadata(
        path=path,
        entry=entry,
        load_addresses=tuple(load_addresses),
        alloc_addresses=tuple(alloc_addresses),
        symbols=symbols,
    )


def read_map_symbol(path: Path, name: str) -> int:
    """Resolve an exact symbol from common GNU ld map spellings."""
    source = path.read_text(errors="replace")
    escaped = re.escape(name)
    number = r"(?:0[xX][0-9A-Fa-f]+|[0-9]+)"
    patterns = (
        re.compile(rf"^\s*({number})\s+{escaped}(?:\s|$|=)"),
        re.compile(rf"^\s*{escaped}\s*=\s*({number})(?:\s|$)"),
        re.compile(
            rf"^\s*({number})\s+PROVIDE(?:_HIDDEN)?\s*\(\s*{escaped}\s*="
        ),
    )
    values: set[int] = set()
    for line in source.splitlines():
        for pattern in patterns:
            match = pattern.search(line)
            if match:
                values.add(int(match.group(1), 0))
                break
    require(values, f"{path}: map has no exact symbol {name!r}")
    require(
        len(values) == 1,
        f"{path}: map gives {name!r} multiple values: "
        + ", ".join(hex(value) for value in sorted(values)),
    )
    return normalize_address(next(iter(values)), f"map symbol {name}")


def parse_expectations(values: Iterable[str]) -> dict[str, int]:
    expectations: dict[str, int] = {}
    for item in values:
        field, separator, raw_value = item.partition("=")
        require(
            bool(separator) and bool(field) and bool(raw_value),
            f"invalid --expect {item!r}; use FIELD=VALUE",
        )
        normalized = field.replace("-", "_").lower()
        field = FIELD_ALIASES.get(normalized, normalized)
        require(
            field in HEADER_FIELDS,
            f"unknown --expect field {field!r}; choose from {', '.join(HEADER_FIELDS)}",
        )
        value = parse_int(raw_value)
        if field in expectations:
            require(expectations[field] == value, f"conflicting --expect values for {field}")
        expectations[field] = value
    return expectations


@dataclass(frozen=True)
class ResolvedLayout:
    entry: int | None
    load_address: int | None


def resolve_layout(args: argparse.Namespace, *, require_complete: bool) -> ResolvedLayout:
    require(
        not (args.entry is not None and args.entry_symbol),
        "--entry and --entry-symbol are mutually exclusive",
    )
    require(
        not (args.load_address is not None and args.load_symbol),
        "--load-address and --load-symbol are mutually exclusive",
    )

    elf = read_elf_metadata(args.elf) if args.elf else None
    metadata_path = args.elf or args.map

    def symbol_value(name: str) -> int:
        require(metadata_path is not None, f"symbol {name!r} needs --elf or --map")
        return elf.symbol(name) if elf else read_map_symbol(args.map, name)

    if args.entry is not None:
        entry = parse_int(args.entry)
    elif args.entry_symbol:
        entry = symbol_value(args.entry_symbol)
    elif elf and elf.entry:
        entry = normalize_address(elf.entry, "ELF entry")
    else:
        entry = None

    if args.load_address is not None:
        load_address = parse_int(args.load_address)
    elif args.load_symbol:
        load_address = symbol_value(args.load_symbol)
    elif elf:
        load_address = elf.load_address()
    else:
        load_address = None

    if require_complete:
        require(
            entry is not None,
            "cannot determine entry: pass --entry, --entry-symbol with --elf/--map, "
            "or an ELF with nonzero e_entry",
        )
        require(
            load_address is not None,
            "cannot determine load address: pass --load-address, --load-symbol "
            "with --elf/--map, or an ELF with a loadable segment",
        )
    return ResolvedLayout(entry, load_address)


def _add_layout_options(parser: argparse.ArgumentParser) -> None:
    metadata = parser.add_mutually_exclusive_group()
    metadata.add_argument("--elf", type=Path, help="ELF used for symbols/e_entry/PT_LOAD")
    metadata.add_argument("--map", type=Path, help="GNU ld map used for named symbols")
    parser.add_argument("--entry", help="explicit initial PC (for example 0x80060268)")
    parser.add_argument("--entry-symbol", help="ELF/map symbol whose value is the initial PC")
    parser.add_argument("--load-address", help="explicit payload load address / t_addr")
    parser.add_argument("--load-symbol", help="ELF/map symbol whose value is the load address")
    parser.add_argument(
        "--expect",
        action="append",
        default=[],
        metavar="FIELD=VALUE",
        help=(
            "assert a final/preserved header field; repeatable. Fields: "
            + ", ".join(HEADER_FIELDS)
        ),
    )
    parser.add_argument("-q", "--quiet", action="store_true", help="suppress the success summary")


def build_argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Finalize or validate a sector-aligned PS-X EXE header.",
        epilog="This tool is standalone and does not modify the current Shake build graph.",
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    finalize = subparsers.add_parser(
        "finalize",
        help="set PC/t_addr/t_size and zero-pad the payload to a 0x800-byte sector",
        description=(
            "Preserve a PS-X EXE header template, regenerate PC/t_addr/t_size, "
            "zero-pad its payload, and validate the result."
        ),
    )
    finalize.add_argument(
        "input",
        type=Path,
        help="EXE-shaped input with a 0x800-byte header template",
    )
    finalize.add_argument(
        "-o",
        "--output",
        type=Path,
        required=True,
        help="distinct finalized output path",
    )
    _add_layout_options(finalize)

    validate = subparsers.add_parser(
        "validate",
        help="validate an existing PS-X EXE without changing it",
        description=(
            "Check magic, address alignment, entry containment, sector-aligned "
            "t_size, and the exact filesize=0x800+t_size invariant."
        ),
    )
    validate.add_argument("input", type=Path, help="PS-X EXE to validate")
    _add_layout_options(validate)
    return parser


def _atomic_write(path: Path, data: bytes, mode: int) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary_name: str | None = None
    try:
        with tempfile.NamedTemporaryFile(
            dir=path.parent, prefix=path.name + ".", delete=False
        ) as stream:
            temporary_name = stream.name
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.chmod(temporary_name, mode)
        os.replace(temporary_name, path)
        temporary_name = None
    finally:
        if temporary_name is not None:
            try:
                os.unlink(temporary_name)
            except FileNotFoundError:
                pass


def _summary(path: Path, header: PsxExeHeader) -> str:
    return (
        f"{path}: PC={header.pc:#010x}, load={header.t_addr:#010x}, "
        f"t_size={header.t_size:#x}, file={HEADER_SIZE + header.t_size:#x}, "
        f"GP={header.gp:#010x}, SP={header.sp:#010x}"
    )


def run(args: argparse.Namespace) -> int:
    expectations = parse_expectations(args.expect)
    layout = resolve_layout(args, require_complete=args.command == "finalize")
    data = args.input.read_bytes()

    if args.command == "validate":
        if layout.entry is not None:
            if "pc" in expectations:
                require(
                    expectations["pc"] == layout.entry,
                    f"--expect pc={expectations['pc']:#x} conflicts with resolved "
                    f"entry {layout.entry:#x}",
                )
            expectations["pc"] = layout.entry
        if layout.load_address is not None:
            if "t_addr" in expectations:
                require(
                    expectations["t_addr"] == layout.load_address,
                    f"--expect t_addr={expectations['t_addr']:#x} conflicts with "
                    f"resolved load address {layout.load_address:#x}",
                )
            expectations["t_addr"] = layout.load_address
        header = validate_image(data, source=str(args.input), expected=expectations)
        if not args.quiet:
            print("valid", _summary(args.input, header))
        return 0

    require(
        args.input.resolve() != args.output.resolve(),
        "input and output must be distinct paths",
    )
    assert layout.entry is not None and layout.load_address is not None
    result, header, padding = finalize_image(
        data,
        entry=layout.entry,
        load_address=layout.load_address,
        source=str(args.input),
        expected=expectations,
    )
    mode = stat.S_IMODE(args.input.stat().st_mode)
    _atomic_write(args.output, result, mode)
    if not args.quiet:
        print(
            "finalized",
            _summary(args.output, header),
            f"(added {padding:#x} bytes of zero padding)",
        )
    return 0


def main(argv: Sequence[str] | None = None) -> int:
    parser = build_argument_parser()
    args = parser.parse_args(argv)
    try:
        return run(args)
    except (OSError, PsxExeError) as error:
        print(f"psxexe: error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())

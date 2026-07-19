#!/usr/bin/env python3
"""Load the C/Python/linker MAIN.EXE RAM-layout constants.

The authoritative literals live in ``src/main.exe/ram_layout.h`` so matching
C sees the exact same values as the Python linker generators.  This loader
accepts only simple integer ``#define`` lines for the policy fields; derived C
macros remain derived and are recomputed here as properties.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import re
from typing import Iterable, Sequence


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_HEADER = Path("src/main.exe/ram_layout.h")

DEFINE_RE = re.compile(
    r"^\s*#define\s+(TENCHU_[A-Z0-9_]+)\s+"
    r"(-?(?:0[xX][0-9A-Fa-f]+|[0-9]+))\s*$"
)


class LayoutError(RuntimeError):
    """The central layout header is missing, malformed, or inconsistent."""


@dataclass(frozen=True)
class RamLayout:
    main_load_address: int
    persistent_state_address: int
    persistent_state_size: int
    persistent_rng_size: int
    memory_pool_floor: int
    memory_pool_end: int
    bss_alignment: int
    memory_pool_alignment: int
    memory_pool_minimum_size: int
    memory_pool_header_words: int
    executable_handoff_address: int
    executable_handoff_size: int
    initial_stack_address: int
    cached_ram_end: int
    pc_memory_handshake_address: int
    pc_memory_handshake_size: int
    scratchpad_address: int
    scratchpad_size: int
    mmio_address: int
    mmio_size: int

    @property
    def persistent_rng_address(self) -> int:
        return self.persistent_state_address + self.persistent_state_size

    @property
    def persistent_rng_end(self) -> int:
        return self.persistent_rng_address + self.persistent_rng_size

    @property
    def retail_memory_pool_capacity(self) -> int:
        return (
            (self.memory_pool_end - self.memory_pool_floor) // 4
            - self.memory_pool_header_words
        )

    @property
    def memory_pool_header_bytes(self) -> int:
        return self.memory_pool_header_words * 4

    @property
    def cached_ram_end_plus_header(self) -> int:
        return self.cached_ram_end + self.memory_pool_header_bytes

    @property
    def pc_memory_pool_address(self) -> int:
        return self.cached_ram_end

    @property
    def pc_memory_pool_size(self) -> int:
        return self.pc_memory_handshake_address - self.pc_memory_pool_address

    @property
    def pc_memory_payload_address(self) -> int:
        return self.pc_memory_pool_address + self.memory_pool_header_bytes

    @property
    def pc_memory_handshake_end(self) -> int:
        return self.pc_memory_handshake_address + self.pc_memory_handshake_size

    @property
    def executable_handoff_end(self) -> int:
        return self.executable_handoff_address + self.executable_handoff_size

    @property
    def scratchpad_end(self) -> int:
        return self.scratchpad_address + self.scratchpad_size

    @property
    def mmio_end(self) -> int:
        return self.mmio_address + self.mmio_size

    def validate(self) -> None:
        addresses = (
            self.main_load_address,
            self.persistent_state_address,
            self.memory_pool_floor,
            self.memory_pool_end,
            self.executable_handoff_address,
            self.initial_stack_address,
            self.cached_ram_end,
            self.pc_memory_handshake_address,
            self.scratchpad_address,
            self.mmio_address,
        )
        if any(address < 0 or address > 0xFFFFFFFF for address in addresses):
            raise LayoutError("RAM layout contains an invalid 32-bit address")
        for name, address in (
            ("MAIN.EXE load", self.main_load_address),
            ("persistent RNG", self.persistent_rng_address),
            ("executable handoff", self.executable_handoff_address),
            ("initial stack", self.initial_stack_address),
            ("PC-link handshake", self.pc_memory_handshake_address),
        ):
            if address % 4:
                raise LayoutError(f"{name} address is not word-aligned")
        sized_ranges = (
            ("persistent state", self.persistent_state_address, self.persistent_state_size),
            (
                "persistent RNG state",
                self.persistent_rng_address,
                self.persistent_rng_size,
            ),
            (
                "executable handoff",
                self.executable_handoff_address,
                self.executable_handoff_size,
            ),
            (
                "PC-link handshake",
                self.pc_memory_handshake_address,
                self.pc_memory_handshake_size,
            ),
            ("scratchpad", self.scratchpad_address, self.scratchpad_size),
            ("MMIO window", self.mmio_address, self.mmio_size),
        )
        for name, start, size in sized_ranges:
            if start < 0 or start > 0xFFFFFFFF or size <= 0:
                raise LayoutError(f"{name} has an invalid address or size")
            if start + size > 0x100000000:
                raise LayoutError(f"{name} wraps the 32-bit address space")
        if self.persistent_state_address % 0x20:
            raise LayoutError(
                "persistent-state base must be 0x20-aligned for matching "
                "base|offset expressions"
            )
        if self.persistent_state_size % 4:
            raise LayoutError(
                "persistent-state size must keep the following RNG word aligned"
            )
        for name, alignment in (
            ("BSS", self.bss_alignment),
            ("memory-pool", self.memory_pool_alignment),
        ):
            if alignment <= 0 or (alignment & (alignment - 1)):
                raise LayoutError(f"{name} alignment is not a power of two")
        if self.memory_pool_floor % self.memory_pool_alignment:
            raise LayoutError("memory-pool floor is not aligned")
        if self.memory_pool_end <= self.memory_pool_floor:
            raise LayoutError("memory-pool end does not follow its floor")
        if (self.memory_pool_end - self.memory_pool_floor) % 4:
            raise LayoutError("memory-pool byte size is not word-aligned")
        if (
            self.memory_pool_minimum_size <= self.memory_pool_header_bytes
            or self.memory_pool_minimum_size % 4
        ):
            raise LayoutError("memory-pool minimum size cannot hold its header")
        if self.memory_pool_header_words != 2:
            raise LayoutError(
                "memory-pool header is a fixed two-word allocator ABI"
            )
        if self.persistent_rng_size != 4:
            raise LayoutError("persistent RNG seed is a fixed 4-byte ABI")
        if self.executable_handoff_size != 0x5C:
            raise LayoutError(
                "executable handoff is a fixed 0x5c-byte BootExecRecord ABI"
            )
        if self.memory_pool_minimum_size > (
            self.memory_pool_end - self.memory_pool_floor
        ):
            raise LayoutError("memory-pool minimum exceeds the retail pool")
        if self.retail_memory_pool_capacity <= 0:
            raise LayoutError("memory-pool capacity is not positive")
        if self.main_load_address <= self.persistent_state_address:
            raise LayoutError("MAIN.EXE does not load above persistent state")
        if self.persistent_rng_end > self.main_load_address:
            raise LayoutError("persistent state overlaps MAIN.EXE load origin")
        if self.memory_pool_end > self.initial_stack_address:
            raise LayoutError("memory pool overlaps the initial stack")
        if self.initial_stack_address >= self.cached_ram_end:
            raise LayoutError("initial stack lies outside cached main RAM")
        if self.pc_memory_pool_size <= self.memory_pool_minimum_size:
            raise LayoutError("PC-link memory pool is too small")
        if self.pc_memory_handshake_size != 0x10:
            raise LayoutError(
                "PC-link handshake is a fixed 0x10-byte Buf16 ABI"
            )
        hardware_contracts = {
            "cached main-RAM end": (self.cached_ram_end, 0x80200000),
            "scratchpad address": (self.scratchpad_address, 0x1F800000),
            "scratchpad size": (self.scratchpad_size, 0x400),
            "MMIO address": (self.mmio_address, 0x1F801000),
            "MMIO window size": (self.mmio_size, 0x2000),
        }
        for name, (actual, expected) in hardware_contracts.items():
            if actual != expected:
                raise LayoutError(
                    f"{name} is fixed PS1 hardware policy "
                    f"(expected 0x{expected:x})"
                )


FIELD_DEFINES = {
    "main_load_address": "TENCHU_MAIN_LOAD_ADDRESS",
    "persistent_state_address": "TENCHU_PERSISTENT_STATE_ADDRESS",
    "persistent_state_size": "TENCHU_PERSISTENT_STATE_SIZE",
    "persistent_rng_size": "TENCHU_PERSISTENT_RNG_SIZE",
    "memory_pool_floor": "TENCHU_MEMORY_POOL_FLOOR",
    "memory_pool_end": "TENCHU_MEMORY_POOL_END",
    "bss_alignment": "TENCHU_BSS_ALIGNMENT",
    "memory_pool_alignment": "TENCHU_MEMORY_POOL_ALIGNMENT",
    "memory_pool_minimum_size": "TENCHU_MEMORY_POOL_MINIMUM_SIZE",
    "memory_pool_header_words": "TENCHU_MEMORY_POOL_HEADER_WORDS",
    "executable_handoff_address": "TENCHU_EXECUTABLE_HANDOFF_ADDRESS",
    "executable_handoff_size": "TENCHU_EXECUTABLE_HANDOFF_SIZE",
    "initial_stack_address": "TENCHU_INITIAL_STACK_ADDRESS",
    "cached_ram_end": "TENCHU_CACHED_RAM_END",
    "pc_memory_handshake_address": "TENCHU_PC_MEMORY_HANDSHAKE_ADDRESS",
    "pc_memory_handshake_size": "TENCHU_PC_MEMORY_HANDSHAKE_SIZE",
    "scratchpad_address": "TENCHU_SCRATCHPAD_ADDRESS",
    "scratchpad_size": "TENCHU_SCRATCHPAD_SIZE",
    "mmio_address": "TENCHU_MMIO_ADDRESS",
    "mmio_size": "TENCHU_MMIO_SIZE",
}

DERIVED_DEFINE_EXPRESSIONS = {
    "TENCHU_SCRATCHPAD(offset)": (
        "(TENCHU_SCRATCHPAD_ADDRESS+(offset))"
    ),
    "TENCHU_PERSISTENT_RNG_ADDRESS": (
        "(TENCHU_PERSISTENT_STATE_ADDRESS+TENCHU_PERSISTENT_STATE_SIZE)"
    ),
    "TENCHU_MEMORY_POOL_HEADER_BYTES": (
        "(TENCHU_MEMORY_POOL_HEADER_WORDS*4)"
    ),
    "TENCHU_PC_MEMORY_POOL_ADDRESS": "TENCHU_CACHED_RAM_END",
    "TENCHU_PC_MEMORY_POOL_SIZE": (
        "(TENCHU_PC_MEMORY_HANDSHAKE_ADDRESS-TENCHU_PC_MEMORY_POOL_ADDRESS)"
    ),
    "TENCHU_PC_MEMORY_PAYLOAD_ADDRESS": (
        "(TENCHU_PC_MEMORY_POOL_ADDRESS+TENCHU_MEMORY_POOL_HEADER_BYTES)"
    ),
    "TENCHU_CACHED_RAM_END_PLUS_HEADER": (
        "TENCHU_PC_MEMORY_PAYLOAD_ADDRESS"
    ),
    "TENCHU_RETAIL_MEMORY_POOL_CAPACITY": (
        "(((TENCHU_MEMORY_POOL_END-TENCHU_MEMORY_POOL_FLOOR)/4)-"
        "TENCHU_MEMORY_POOL_HEADER_WORDS)"
    ),
}


def load(path: Path | None = None) -> RamLayout:
    header = ROOT / DEFAULT_HEADER if path is None else path
    try:
        source = header.read_text()
    except OSError as error:
        raise LayoutError(f"cannot read layout header {header}: {error}") from error
    definitions: dict[str, int] = {}
    for line in source.splitlines():
        match = DEFINE_RE.match(line)
        if match is None:
            continue
        name = match.group(1)
        if name in definitions:
            raise LayoutError(f"layout header {header} defines {name} twice")
        definitions[name] = int(match.group(2), 0)
    missing = sorted(set(FIELD_DEFINES.values()) - set(definitions))
    if missing:
        raise LayoutError(
            f"layout header {header} is missing literal defines: "
            + ", ".join(missing)
        )
    logical_source = source.replace("\\\n", "")
    for name, expected in DERIVED_DEFINE_EXPRESSIONS.items():
        matches = re.findall(
            rf"^\s*#define\s+{re.escape(name)}\s+(.+?)\s*$",
            logical_source,
            re.MULTILINE,
        )
        if len(matches) != 1:
            raise LayoutError(
                f"layout header {header} must define {name} exactly once"
            )
        actual = re.sub(r"\s+", "", matches[0])
        if actual != expected:
            raise LayoutError(
                f"layout header {header} has unsupported {name} expression"
            )
    layout = RamLayout(
        **{
            field: definitions[define]
            for field, define in FIELD_DEFINES.items()
        }
    )
    layout.validate()
    return layout


LAYOUT = load()


def _without_c_comments_or_literals(source: str) -> str:
    pattern = re.compile(
        r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
        re.DOTALL,
    )

    def erase(match: re.Match[str]) -> str:
        return "".join("\n" if character == "\n" else " " for character in match.group())

    return pattern.sub(erase, source)


def _layout_literal_name(layout: RamLayout, value: int) -> str | None:
    exact = {
        layout.main_load_address: "main_load_address",
        layout.memory_pool_floor: "memory_pool_floor",
        layout.memory_pool_end: "memory_pool_end",
        layout.persistent_rng_address: "persistent_rng_address",
        layout.initial_stack_address: "initial_stack_address",
        layout.cached_ram_end: "cached_ram_end",
        layout.cached_ram_end_plus_header: "cached_ram_end_plus_header",
    }
    if value in exact:
        return exact[value]
    ranges = (
        (
            "persistent_state",
            layout.persistent_state_address,
            layout.persistent_rng_end,
        ),
        (
            "executable_handoff",
            layout.executable_handoff_address,
            layout.executable_handoff_end,
        ),
        ("scratchpad", layout.scratchpad_address, layout.scratchpad_end),
        ("mmio", layout.mmio_address, layout.mmio_end),
        (
            "pc_memory_pool",
            layout.pc_memory_pool_address,
            layout.pc_memory_handshake_address,
        ),
        (
            "pc_memory_handshake",
            layout.pc_memory_handshake_address,
            layout.pc_memory_handshake_end,
        ),
        ("memory_pool", layout.memory_pool_floor, layout.memory_pool_end),
    )
    for name, start, end in ranges:
        if start <= value < end:
            offset = value - start
            return f"{name}_address" if offset == 0 else f"{name}+0x{offset:x}"
    return None


def source_literal_findings(
    root: Path = ROOT,
    sources: Iterable[Path] | None = None,
) -> list[dict[str, object]]:
    """Find duplicated fixed-policy literals in game source outside this header."""

    layout = LAYOUT if root.resolve() == ROOT.resolve() else load(
        root / DEFAULT_HEADER
    )
    if sources is None:
        source_root = root / "src/main.exe"
        authority = (root / DEFAULT_HEADER).resolve()
        selected = sorted(
            path
            for pattern in ("*.c", "*.h")
            for path in source_root.rglob(pattern)
            if path.resolve() != authority
        )
    else:
        selected = list(sources)
    findings: list[dict[str, object]] = []
    for source_path in selected:
        source = _without_c_comments_or_literals(source_path.read_text())
        for line_number, line in enumerate(source.splitlines(), 1):
            for match in re.finditer(
                r"\b0[xX][0-9A-Fa-f]+(?:[uUlL]+)?\b", line
            ):
                raw_literal = match.group()
                value = int(re.sub(r"[uUlL]+$", "", raw_literal), 16)
                name = _layout_literal_name(layout, value)
                if name is None:
                    continue
                try:
                    display = source_path.relative_to(root).as_posix()
                except ValueError:
                    display = str(source_path)
                findings.append(
                    {
                        "file": display,
                        "line": line_number,
                        "literal": raw_literal,
                        "layout_value": name,
                    }
                )
    return findings


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Read one value from src/main.exe/ram_layout.h."
    )
    parser.add_argument("field", choices=sorted(FIELD_DEFINES))
    args = parser.parse_args(argv)
    print(f"0x{getattr(LAYOUT, args.field):08x}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

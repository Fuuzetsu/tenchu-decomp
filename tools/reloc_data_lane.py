#!/usr/bin/env python3
"""Prove reviewed MAIN.EXE data pointers are genuine ELF relocations.

``tools/reloc_data.py`` performs the source transformation.  This companion
gate applies it to a separate generated tree, assembles every touched input,
and checks the resulting objects and linked bytes:

* every reviewed pointer has an ``R_MIPS_32`` record at its exact source word;
* every exact target label, or reviewed external base used with an addend, is
  section-relative at its reviewed byte offset;
* a retail-address link reproduces every original pointer value; and
* links shifted by ``+4`` and ``+0x10004`` retarget every pointer, including a
  carry across the high 16 bits.

The ordinary matching inputs are read-only. The proof uses isolated temporary
assembly and object files under ``.shake/build/reloc-data``. The composed
``./Build relink`` lane consumes persistent copies made by the same transformer;
this tool remains its stricter per-relocation oracle, not a runnable-growth
claim by itself.
"""

from __future__ import annotations

import argparse
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile
from typing import Sequence

try:
    from tools import reloc_data
except ModuleNotFoundError:  # Direct invocation adds tools/, not the repo root.
    import reloc_data  # type: ignore[no-redef]


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = Path("config/reloc-data.main.exe.json")
DEFAULT_INPUT_DIR = Path(".shake/gen/main.exe/asm/data")
DEFAULT_WORK_DIR = Path(".shake/build/reloc-data")
PROBE_SHIFTS = (0, 4, 0x10004)

AS_FLAGS = (
    "-EL",
    f"-I{ROOT / 'include'}",
    "-march=r3000",
    "-mtune=r3000",
    "-no-pad-sections",
    "-O1",
    "-G0",
)

RELOCATION_RE = re.compile(
    r"^\s*(?P<offset>[0-9A-Fa-f]+)\s+[0-9A-Fa-f]+\s+"
    r"R_MIPS_32\s+[0-9A-Fa-f]+\s+(?P<symbol>\S+)"
)
SYMBOL_RE = re.compile(
    r"^\s*\d+:\s+(?P<value>[0-9A-Fa-f]+)\s+\d+\s+OBJECT\s+"
    r"\S+\s+\S+\s+(?P<section>\S+)\s+(?P<symbol>\S+)\s*$"
)


class LaneError(RuntimeError):
    """A transformed-object or controlled-link invariant failed."""


@dataclass(frozen=True)
class Toolchain:
    assembler: str
    linker: str
    objcopy: str
    readelf: str


@dataclass(frozen=True)
class ProofResult:
    pointers: int
    targets: int
    files: int
    shifts: int


@dataclass(frozen=True)
class ExternalTarget:
    symbol: str
    address: int


def require(condition: bool, message: str) -> None:
    if not condition:
        raise LaneError(message)


def resolve_tool(name: str) -> str:
    executable = shutil.which(name)
    if executable is None:
        raise LaneError(f"required tool {name!r} is not on PATH; run inside `nix develop`")
    return executable


def default_toolchain() -> Toolchain:
    prefix = "mipsel-unknown-linux-gnu-"
    return Toolchain(
        assembler=resolve_tool(prefix + "as"),
        linker=resolve_tool(prefix + "ld"),
        objcopy=resolve_tool(prefix + "objcopy"),
        readelf=resolve_tool(prefix + "readelf"),
    )


def run(command: Sequence[str]) -> str:
    result = subprocess.run(
        command,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if result.returncode != 0:
        detail = result.stderr.strip() or result.stdout.strip()
        raise LaneError(f"{Path(command[0]).name} failed: {detail}")
    return result.stdout


def input_base(path: Path) -> int:
    """Return the retail address represented by byte zero of one data input."""

    for line_number, line in enumerate(path.read_text().splitlines(), 1):
        match = reloc_data.ADDRESS_RE.search(line)
        if match is not None:
            return int(match.group("address"), 16)
    raise LaneError(f"{path}: no generated address comment")


def parse_relocations(source: str) -> Counter[tuple[int, str]]:
    records: Counter[tuple[int, str]] = Counter()
    for line in source.splitlines():
        match = RELOCATION_RE.match(line)
        if match is not None:
            records[(int(match.group("offset"), 16), match.group("symbol"))] += 1
    return records


def parse_symbols(source: str) -> dict[str, tuple[int, str]]:
    symbols: dict[str, tuple[int, str]] = {}
    for line in source.splitlines():
        match = SYMBOL_RE.match(line)
        if match is None:
            continue
        name = match.group("symbol")
        value = int(match.group("value"), 16)
        section = match.group("section")
        prior = symbols.setdefault(name, (value, section))
        require(prior == (value, section), f"symbol {name} has conflicting definitions")
    return symbols


def assemble(
    toolchain: Toolchain,
    sources: dict[Path, Path],
    output_dir: Path,
) -> dict[Path, Path]:
    objects: dict[Path, Path] = {}
    for file_key, source in sorted(sources.items()):
        output = output_dir / Path(str(file_key) + ".o")
        output.parent.mkdir(parents=True, exist_ok=True)
        run(
            [
                toolchain.assembler,
                *AS_FLAGS,
                "-o",
                str(output),
                str(source),
            ]
        )
        objects[file_key] = output
    return objects


def external_targets(
    entries: Sequence[reloc_data.PointerEntry],
) -> tuple[ExternalTarget, ...]:
    targets = {
        (entry.symbol, entry.symbol_address)
        for entry in entries
        if entry.target_file is None
    }
    return tuple(ExternalTarget(symbol, address) for symbol, address in sorted(targets))


def assemble_external_targets(
    toolchain: Toolchain,
    targets: Sequence[ExternalTarget],
    root: Path,
) -> Path | None:
    if not targets:
        return None
    source = root / "external-targets.s"
    output = root / "external-targets.o"
    lines: list[str] = []
    for index, target in enumerate(targets):
        lines.extend(
            [
                f'.section .reloc_data_external_{index}, "aw", @nobits\n',
                f".globl {target.symbol}\n",
                f".type {target.symbol}, @object\n",
                f"{target.symbol}:\n",
                ".space 1\n",
                f".size {target.symbol}, . - {target.symbol}\n",
            ]
        )
    source.write_text("".join(lines))
    run(
        [
            toolchain.assembler,
            *AS_FLAGS,
            "-o",
            str(output),
            str(source),
        ]
    )
    symbols = parse_symbols(run([toolchain.readelf, "-Ws", str(output)]))
    for target in targets:
        require(target.symbol in symbols, f"missing external target {target.symbol}")
        value, section = symbols[target.symbol]
        require(value == 0, f"external target {target.symbol} has nonzero object offset")
        require(
            section not in {"ABS", "UND"},
            f"external target {target.symbol} is not section-relative",
        )
    return output


def verify_objects(
    entries: Sequence[reloc_data.PointerEntry],
    bases: dict[Path, int],
    objects: dict[Path, Path],
    toolchain: Toolchain,
) -> None:
    by_source: dict[Path, list[reloc_data.PointerEntry]] = {}
    by_target: dict[Path, list[reloc_data.PointerEntry]] = {}
    for entry in entries:
        by_source.setdefault(entry.source_file, []).append(entry)
        if entry.target_file is not None:
            by_target.setdefault(entry.target_file, []).append(entry)

    for file_key, file_entries in by_source.items():
        relocations = parse_relocations(
            run([toolchain.readelf, "-Wr", str(objects[file_key])])
        )
        expected: Counter[tuple[int, str]] = Counter(
            (entry.source_address - bases[file_key], entry.symbol)
            for entry in file_entries
        )
        for record, count in expected.items():
            actual = relocations[record]
            require(
                actual == count,
                f"{file_key}:{record[0]:#x}: expected {count} R_MIPS_32 record(s) "
                f"against {record[1]}, found {actual}",
            )

    for file_key, file_entries in by_target.items():
        symbols = parse_symbols(run([toolchain.readelf, "-Ws", str(objects[file_key])]))
        reviewed = {
            (entry.symbol, entry.target_address)
            for entry in file_entries
        }
        for name, address in reviewed:
            require(name in symbols, f"{file_key}: missing exact target symbol {name}")
            value, section = symbols[name]
            expected = address - bases[file_key]
            require(
                value == expected,
                f"{file_key}: {name} offset {value:#x}, expected {expected:#x}",
            )
            require(
                section not in {"ABS", "UND"},
                f"{file_key}: {name} is not section-relative",
            )


def linker_script(
    objects: dict[Path, Path],
    bases: dict[Path, int],
    external_object: Path | None,
    external: Sequence[ExternalTarget],
    shift: int,
) -> tuple[str, dict[Path, str]]:
    lines = ["SECTIONS\n", "{\n"]
    sections: dict[Path, str] = {}
    for index, file_key in enumerate(sorted(objects)):
        section = f".reloc_data_{index}"
        sections[file_key] = section
        address = bases[file_key] + shift
        lines.append(
            f"  {section} 0x{address:08x} : SUBALIGN(4) "
            f"{{ {objects[file_key]}(.data) }}\n"
        )
    if external_object is not None:
        for index, target in enumerate(external):
            section = f".reloc_data_external_{index}"
            address = target.address + shift
            lines.append(
                f"  {section} 0x{address:08x} : "
                f"{{ {external_object}({section}) }}\n"
            )
    lines.extend(
        [
            "  /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.gnu.attributes) }\n",
            "}\n",
        ]
    )
    return "".join(lines), sections


def verify_link(
    entries: Sequence[reloc_data.PointerEntry],
    bases: dict[Path, int],
    objects: dict[Path, Path],
    external_object: Path | None,
    external: Sequence[ExternalTarget],
    toolchain: Toolchain,
    root: Path,
    shift: int,
) -> None:
    script_text, sections = linker_script(
        objects, bases, external_object, external, shift
    )
    script = root / f"shift-{shift:x}.ld"
    linked = root / f"shift-{shift:x}.elf"
    script.write_text(script_text)
    run(
        [
            toolchain.linker,
            "-EL",
            "-T",
            str(script),
            "-o",
            str(linked),
            "--unresolved-symbols=ignore-all",
            "--no-check-sections",
            "-nostdlib",
            *(str(objects[file_key]) for file_key in sorted(objects)),
            *(() if external_object is None else (str(external_object),)),
        ]
    )

    # These generated objects contain older symbolic words outside the reviewed
    # manifest, so the partial proof deliberately tolerates unrelated undefined
    # symbols.  Every reviewed target is defined above, and its final linked word
    # is checked explicitly below; an unresolved reviewed symbol therefore cannot
    # pass as zero.

    by_source: dict[Path, list[reloc_data.PointerEntry]] = {}
    for entry in entries:
        by_source.setdefault(entry.source_file, []).append(entry)
    for file_key, file_entries in by_source.items():
        binary = root / f"shift-{shift:x}-{file_key.name}.bin"
        run(
            [
                toolchain.objcopy,
                "-j",
                sections[file_key],
                "-O",
                "binary",
                str(linked),
                str(binary),
            ]
        )
        payload = binary.read_bytes()
        for entry in file_entries:
            offset = entry.source_address - bases[file_key]
            require(
                offset + 4 <= len(payload),
                f"{file_key}:{entry.source_address:#x} lies outside linked payload",
            )
            actual = struct.unpack_from("<I", payload, offset)[0]
            expected = (entry.target_address + shift) & 0xFFFFFFFF
            require(
                actual == expected,
                f"{file_key}:{entry.source_address:#x} linked to {actual:#x}, "
                f"expected {expected:#x} at shift {shift:#x}",
            )


def verify(
    manifest: Path,
    input_dir: Path,
    work_dir: Path,
    *,
    toolchain: Toolchain | None = None,
) -> ProofResult:
    entries = reloc_data.load_manifest(manifest)
    files = sorted(
        {entry.source_file for entry in entries}
        | {entry.target_file for entry in entries if entry.target_file is not None}
    )
    bases: dict[Path, int] = {}
    for file_key in files:
        path = input_dir / file_key
        require(path.is_file(), f"missing generated input {path}")
        bases[file_key] = input_base(path)

    tools = toolchain or default_toolchain()
    work_dir.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="verify-", dir=work_dir) as temporary:
        root = Path(temporary)
        rewritten_dir = root / "asm"
        object_dir = root / "obj"
        object_dir.mkdir()
        result = reloc_data.rewrite(manifest, input_dir, rewritten_dir)
        objects = assemble(
            tools,
            {file_key: rewritten_dir / file_key for file_key in files},
            object_dir,
        )
        external = external_targets(entries)
        external_object = assemble_external_targets(tools, external, root)
        verify_objects(entries, bases, objects, tools)
        for shift in PROBE_SHIFTS:
            verify_link(
                entries,
                bases,
                objects,
                external_object,
                external,
                tools,
                root,
                shift,
            )

    targets = len({(entry.target_file, entry.target_address) for entry in entries})
    require(result.pointers == len(entries), "transform pointer count changed")
    require(result.targets == targets, "transform target count changed")
    return ProofResult(len(entries), targets, len(files), len(PROBE_SHIFTS))


def parser() -> argparse.ArgumentParser:
    argument_parser = argparse.ArgumentParser(description=__doc__)
    argument_parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    argument_parser.add_argument("--input-dir", type=Path, default=DEFAULT_INPUT_DIR)
    argument_parser.add_argument("--work-dir", type=Path, default=DEFAULT_WORK_DIR)
    return argument_parser


def main(argv: Sequence[str] | None = None) -> int:
    args = parser().parse_args(argv)
    try:
        result = verify(args.manifest, args.input_dir, args.work_dir)
    except (OSError, LaneError, reloc_data.RelocDataError) as error:
        print(f"reloc-data-lane: {error}", file=sys.stderr)
        return 1
    print(
        f"reloc-data-lane: verified {result.pointers} R_MIPS_32 pointer words, "
        f"{result.targets} exact targets, and {result.shifts} link layouts "
        f"across {result.files} files"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Rewrite reviewed raw-data pointers as exact symbolic references.

The matching build's generated data assembly is byte-faithful, but a literal
``.word 0x80012345`` carries no ELF relocation.  This tool applies a small,
reviewed manifest to copies of those generated files.  Every entry:

* identifies the pointer word by file, address, and enclosing data owner;
* identifies its target by an exact address and enclosing data owner; and
* inserts a section-relative symbol at that exact target before replacing the
  literal with ``.word symbol``.  A reviewed external target may instead name
  an existing section-owned base plus an explicit addend.

The owner checks are evidence guards, not relocation bases.  In particular,
an interior target is not rewritten as a guessed top-level symbol plus an
addend.  The sole supported addend form is explicit in the manifest and is
checked at all controlled link layouts.  GNU ``as`` therefore emits a real
``R_MIPS_32`` for both forms.  The ordinary generated files are never modified
in place.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
import os
from pathlib import Path
import re
import sys
import tempfile
from typing import Mapping, Sequence


UINT32_LIMIT = 1 << 32
SYMBOL_RE = re.compile(r"^[A-Za-z_.$][A-Za-z0-9_.$]*$")
DLABEL_RE = re.compile(r"^\s*dlabel\s+(\S+)\s*$")
ENDDLABEL_RE = re.compile(r"^\s*enddlabel\s+(\S+)\s*$")
ADDRESS_RE = re.compile(
    r"/\*\s*[0-9A-Fa-f]+\s+(?P<address>[0-9A-Fa-f]{8})"
    r"(?:\s+[0-9A-Fa-f]{8})?\s*\*/"
)
WORD_RE = re.compile(r"(?P<prefix>\.word\s+)(?P<operand>[^\s#,]+)")
BYTE_RE = re.compile(r"(?P<prefix>\.byte\s+)(?P<operand>[^\s#,]+)")


class RelocDataError(RuntimeError):
    """A manifest or generated-input invariant was violated."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RelocDataError(message)


def parse_u32(value: object, description: str) -> int:
    if isinstance(value, bool):
        raise RelocDataError(f"{description} must be an integer, not a boolean")
    if isinstance(value, int):
        number = value
    elif isinstance(value, str):
        try:
            number = int(value, 0)
        except ValueError as error:
            raise RelocDataError(f"{description} has invalid integer {value!r}") from error
    else:
        raise RelocDataError(f"{description} must be an integer or integer string")
    require(0 <= number < UINT32_LIMIT, f"{description} is not a 32-bit word")
    return number


def relative_source_path(value: object, description: str) -> Path:
    require(isinstance(value, str) and bool(value), f"{description} must be a path string")
    path = Path(value)
    require(not path.is_absolute(), f"{description} must be relative")
    require(".." not in path.parts, f"{description} may not contain '..'")
    require(path.name.endswith(".data.s"), f"{description} must name a .data.s file")
    return path


def string_field(raw: Mapping[str, object], name: str, description: str) -> str:
    value = raw.get(name)
    require(isinstance(value, str) and bool(value), f"{description}.{name} must be a string")
    return value


@dataclass(frozen=True)
class PointerEntry:
    source_file: Path
    source_address: int
    source_owner: str
    target_file: Path | None
    target_address: int
    target_owner: str
    symbol: str
    target_addend: int

    @property
    def symbol_address(self) -> int:
        return self.target_address - self.target_addend

    @property
    def expression(self) -> str:
        if self.target_addend == 0:
            return self.symbol
        return f"{self.symbol} + {self.target_addend:#x}"

    @classmethod
    def from_json(cls, raw: object, index: int) -> "PointerEntry":
        description = f"entries[{index}]"
        require(isinstance(raw, dict), f"{description} must be an object")
        required = {
            "source_file",
            "source_address",
            "source_owner",
            "target_file",
            "target_address",
            "target_owner",
            "symbol",
        }
        keys = set(raw)
        allowed = required | {"target_addend"}
        require(
            required <= keys <= allowed,
            f"{description} fields differ: {sorted((required - keys) | (keys - allowed))}",
        )
        symbol = string_field(raw, "symbol", description)
        require(bool(SYMBOL_RE.fullmatch(symbol)), f"{description}.symbol is invalid: {symbol!r}")
        target_file_value = raw["target_file"]
        target_file = (
            None
            if target_file_value is None
            else relative_source_path(target_file_value, f"{description}.target_file")
        )
        target_address = parse_u32(raw["target_address"], f"{description}.target_address")
        target_addend = parse_u32(raw.get("target_addend", 0), f"{description}.target_addend")
        require(
            target_addend <= target_address,
            f"{description}.target_addend exceeds the target address",
        )
        require(
            target_file is None or target_addend == 0,
            f"{description}: generated-file targets must use an exact label",
        )
        return cls(
            source_file=relative_source_path(raw["source_file"], f"{description}.source_file"),
            source_address=parse_u32(raw["source_address"], f"{description}.source_address"),
            source_owner=string_field(raw, "source_owner", description),
            target_file=target_file,
            target_address=target_address,
            target_owner=string_field(raw, "target_owner", description),
            symbol=symbol,
            target_addend=target_addend,
        )


@dataclass(frozen=True)
class RewriteResult:
    pointers: int
    targets: int
    files: int


def load_manifest(path: Path) -> list[PointerEntry]:
    try:
        document = json.loads(path.read_text())
    except json.JSONDecodeError as error:
        raise RelocDataError(f"{path}: invalid JSON: {error}") from error
    require(isinstance(document, dict), f"{path}: manifest root must be an object")
    require(document.get("schema") == 1, f"{path}: expected schema 1")
    require(set(document) <= {"schema", "description", "entries"}, f"{path}: unknown fields")
    raw_entries = document.get("entries")
    require(
        isinstance(raw_entries, list) and bool(raw_entries),
        f"{path}: entries must be nonempty",
    )
    entries = [PointerEntry.from_json(raw, index) for index, raw in enumerate(raw_entries)]

    sources: set[tuple[Path, int]] = set()
    symbols: dict[str, tuple[Path | None, int]] = {}
    targets: dict[tuple[Path | None, int], tuple[str, int]] = {}
    for entry in entries:
        source = (entry.source_file, entry.source_address)
        require(
            source not in sources,
            f"{path}: duplicate source {entry.source_file}:{entry.source_address:#x}",
        )
        sources.add(source)

        symbol_target = (entry.target_file, entry.symbol_address)
        prior_target = symbols.setdefault(entry.symbol, symbol_target)
        require(
            prior_target == symbol_target,
            f"{path}: symbol {entry.symbol} names multiple targets",
        )
        target = (entry.target_file, entry.target_address)
        expression = (entry.symbol, entry.target_addend)
        prior_symbol = targets.setdefault(target, expression)
        require(
            prior_symbol == expression,
            f"{path}: target {entry.target_address:#x} has multiple expressions",
        )
    return entries


def _active_owner(line: str, owner: str | None) -> tuple[str | None, str | None]:
    start = DLABEL_RE.fullmatch(line.rstrip("\r\n"))
    if start is not None:
        require(owner is None, f"nested dlabel {start.group(1)} inside {owner}")
        return start.group(1), None
    end = ENDDLABEL_RE.fullmatch(line.rstrip("\r\n"))
    if end is not None:
        require(owner == end.group(1), f"enddlabel {end.group(1)} does not close {owner}")
        return owner, end.group(1)
    return owner, None


def _anchor(symbol: str, address: int, owner: str) -> str:
    return (
        f"/* reloc-data: exact interior {address:#010x} within {owner} */\n"
        f".globl {symbol}\n"
        f".type {symbol}, @object\n"
        f"{symbol}:\n"
    )


def rewrite_file(path: Path, file_key: Path, entries: Sequence[PointerEntry]) -> str:
    source = path.read_text()
    lines = source.splitlines(keepends=True)
    source_entries = {
        entry.source_address: entry for entry in entries if entry.source_file == file_key
    }
    target_entries = {
        entry.target_address: entry for entry in entries if entry.target_file == file_key
    }
    expected_symbols = {entry.symbol for entry in target_entries.values()}
    for symbol in expected_symbols:
        definition = re.compile(rf"^\s*{re.escape(symbol)}:\s*$", re.MULTILINE)
        require(definition.search(source) is None, f"{path}: symbol {symbol} is already defined")

    found_sources: set[int] = set()
    found_targets: set[int] = set()
    output: list[str] = []
    owner: str | None = None
    index = 0
    while index < len(lines):
        line_number = index + 1
        line = lines[index]
        pointer: PointerEntry | None = None
        encoded_lines = 1
        owner, closing = _active_owner(line, owner)
        address_match = ADDRESS_RE.search(line)
        if address_match is not None:
            address = int(address_match.group("address"), 16)
            target = target_entries.get(address)
            if target is not None:
                require(
                    address not in found_targets,
                    f"{path}:{line_number}: duplicate target address",
                )
                require(
                    owner == target.target_owner,
                    f"{path}:{line_number}: target owner is {owner}, "
                    f"expected {target.target_owner}",
                )
                output.append(_anchor(target.symbol, target.target_address, target.target_owner))
                found_targets.add(address)

            pointer = source_entries.get(address)
            if pointer is not None:
                require(
                    address not in found_sources,
                    f"{path}:{line_number}: duplicate source address",
                )
                require(
                    owner == pointer.source_owner,
                    f"{path}:{line_number}: source owner is {owner}, "
                    f"expected {pointer.source_owner}",
                )
                word = WORD_RE.search(line)
                byte = BYTE_RE.search(line)
                if word is not None:
                    try:
                        literal = int(word.group("operand"), 0)
                    except ValueError as error:
                        raise RelocDataError(
                            f"{path}:{line_number}: source operand is not a literal"
                        ) from error
                    start, end = word.span("operand")
                    line = line[:start] + pointer.expression + line[end:]
                elif byte is not None:
                    require(
                        closing is None,
                        f"{path}:{line_number}: byte-encoded source closes its owner",
                    )
                    octets: list[int] = []
                    for byte_index in range(4):
                        candidate_index = index + byte_index
                        require(
                            candidate_index < len(lines),
                            f"{path}:{line_number}: truncated four-byte pointer",
                        )
                        candidate = lines[candidate_index]
                        candidate_address_match = ADDRESS_RE.search(candidate)
                        require(
                            candidate_address_match is not None,
                            f"{path}:{candidate_index + 1}: pointer byte has no address",
                        )
                        candidate_address = int(
                            candidate_address_match.group("address"), 16
                        )
                        require(
                            candidate_address == address + byte_index,
                            f"{path}:{candidate_index + 1}: pointer bytes are not contiguous",
                        )
                        candidate_byte = BYTE_RE.search(candidate)
                        require(
                            candidate_byte is not None,
                            f"{path}:{candidate_index + 1}: expected a .byte directive",
                        )
                        try:
                            octet = int(candidate_byte.group("operand"), 0)
                        except ValueError as error:
                            raise RelocDataError(
                                f"{path}:{candidate_index + 1}: source byte is not a literal"
                            ) from error
                        require(
                            0 <= octet <= 0xFF,
                            f"{path}:{candidate_index + 1}: source byte is out of range",
                        )
                        require(
                            address + byte_index not in target_entries,
                            f"{path}:{candidate_index + 1}: pointer byte overlaps a target",
                        )
                        if byte_index != 0:
                            require(
                                address + byte_index not in source_entries,
                                f"{path}:{candidate_index + 1}: pointer encodings overlap",
                            )
                        octets.append(octet)
                    literal = sum(
                        octet << (8 * position)
                        for position, octet in enumerate(octets)
                    )
                    start, end = byte.span()
                    line = line[:start] + f".word {pointer.expression}" + line[end:]
                    encoded_lines = 4
                else:
                    raise RelocDataError(
                        f"{path}:{line_number}: source is neither a .word nor four .byte directives"
                    )
                require(
                    literal == pointer.target_address,
                    f"{path}:{line_number}: literal {literal:#x} != target "
                    f"{pointer.target_address:#x}",
                )
                found_sources.add(address)
        output.append(line)
        if closing is not None:
            owner = None
        index += encoded_lines if pointer is not None else 1

    missing_sources = sorted(set(source_entries) - found_sources)
    missing_targets = sorted(set(target_entries) - found_targets)
    if missing_sources:
        raise RelocDataError(f"{path}: missing source address {missing_sources[0]:#x}")
    if missing_targets:
        raise RelocDataError(f"{path}: missing target address {missing_targets[0]:#x}")
    require(owner is None, f"{path}: unterminated dlabel {owner}")
    return "".join(output)


def atomic_write(path: Path, content: str, mode: int) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary = tempfile.mkstemp(prefix=f".{path.name}.", dir=path.parent)
    try:
        with os.fdopen(descriptor, "w", newline="") as stream:
            stream.write(content)
        os.chmod(temporary, mode)
        os.replace(temporary, path)
    except BaseException:
        try:
            os.unlink(temporary)
        except FileNotFoundError:
            pass
        raise


def rewrite(
    manifest: Path,
    input_dir: Path,
    output_dir: Path | None,
) -> RewriteResult:
    entries = load_manifest(manifest)
    files = sorted(
        {entry.source_file for entry in entries}
        | {entry.target_file for entry in entries if entry.target_file is not None}
    )
    rewritten: dict[Path, str] = {}
    for relative in files:
        source_path = input_dir / relative
        require(source_path.is_file(), f"missing generated input {source_path}")
        rewritten[relative] = rewrite_file(source_path, relative, entries)

    if output_dir is not None:
        require(
            output_dir.resolve() != input_dir.resolve(),
            "output directory must differ from generated input directory",
        )
        for relative, content in rewritten.items():
            source_path = input_dir / relative
            mode = source_path.stat().st_mode & 0o777
            atomic_write(output_dir / relative, content, mode)

    target_count = len({(entry.target_file, entry.target_address) for entry in entries})
    return RewriteResult(len(entries), target_count, len(files))


def parser() -> argparse.ArgumentParser:
    argument_parser = argparse.ArgumentParser(description=__doc__)
    argument_parser.add_argument("--manifest", type=Path, required=True)
    argument_parser.add_argument("--input-dir", type=Path, required=True)
    argument_parser.add_argument("--output-dir", type=Path)
    argument_parser.add_argument(
        "--check",
        action="store_true",
        help="validate the manifest against generated inputs without writing copies",
    )
    return argument_parser


def main(argv: Sequence[str] | None = None) -> int:
    args = parser().parse_args(argv)
    if args.check and args.output_dir is not None:
        print("reloc-data: --check and --output-dir are mutually exclusive", file=sys.stderr)
        return 2
    if not args.check and args.output_dir is None:
        print("reloc-data: --output-dir is required unless --check is used", file=sys.stderr)
        return 2
    try:
        result = rewrite(args.manifest, args.input_dir, args.output_dir)
    except (OSError, RelocDataError) as error:
        print(f"reloc-data: {error}", file=sys.stderr)
        return 1
    action = "validated" if args.check else "rewrote"
    print(
        f"reloc-data: {action} {result.pointers} pointer words and "
        f"{result.targets} exact targets across {result.files} files"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

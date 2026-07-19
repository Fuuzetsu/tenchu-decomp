#!/usr/bin/env python3
"""Inventory fixed-address blockers in the current ``main.exe`` relink.

The report is intentionally read-only: it inspects the final ELF, generated
linker script, and only the generated raw inputs that the linker script names.
It does not alter the normal matching build.  JSON output contains every
finding; the default text output is a stable summary suitable for build logs.
"""

from __future__ import annotations

import argparse
from collections import Counter
from dataclasses import asdict, dataclass
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
from typing import Iterable


ROOT = Path(__file__).resolve().parents[1]

# Retail MAIN.EXE layout.  These boundaries classify the audit; they are not a
# proposed future linker layout.  The movable interval ends where the current
# BSS meets HEAP_START.
MAIN_START = 0x80011000
LEADING_DATA_END = 0x80016134
GAME_CODE_END = 0x80060268
CODE_END = 0x80086764
LOADED_END = 0x80098000
MOVABLE_END = 0x800CDBAC

DEFAULT_ELF = Path(".shake/build/tenchu/main.exe.elf")
DEFAULT_LINKER = Path(".shake/gen/main.exe/linker/main.exe.ld")
DEFAULT_RAW_DIR = Path(".shake/gen/main.exe/asm/data")
DEFAULT_NM = "mipsel-unknown-linux-gnu-nm"

LINKED_RAW_RE = re.compile(r"data/([0-9A-Fa-f]+\.data\.s)\.o")
NM_RE = re.compile(r"^([0-9A-Fa-f]+)\s+([Aa])\s+(.+)$")
WORD_RE = re.compile(
    r"/\*\s*([0-9A-Fa-f]+)\s+([0-9A-Fa-f]+)"
    r"(?:\s+[0-9A-Fa-f]{8})?\s*\*/\s*\.word\s+([^\s#,]+)"
)
DLABEL_RE = re.compile(r"^\s*dlabel\s+(\S+)")


class AuditError(RuntimeError):
    """The requested audit could not be performed."""


@dataclass(frozen=True)
class AbsSymbol:
    name: str
    value: int
    binding: str
    region: str
    movable: bool


@dataclass(frozen=True)
class WordRecord:
    source: int
    file: str
    line: int
    owner: str | None
    operand: str
    value: int | None
    source_region: str


def hex32(value: int) -> str:
    return f"0x{value & 0xFFFFFFFF:08x}"


def address_region(value: int) -> str:
    value &= 0xFFFFFFFF
    if value < MAIN_START:
        return "below_main"
    if value < LEADING_DATA_END:
        return "leading_data"
    if value < GAME_CODE_END:
        return "game_code"
    if value < CODE_END:
        return "crt_sdk_code"
    if value < LOADED_END:
        return "trailing_loaded"
    if value < MOVABLE_END:
        return "bss"
    if value < 0x80200000:
        return "heap_or_later"
    return "outside_main"


def source_region(value: int) -> str:
    if MAIN_START <= value < LEADING_DATA_END:
        return "leading_data"
    if LEADING_DATA_END <= value < CODE_END:
        return "raw_code"
    if CODE_END <= value < LOADED_END:
        return "trailing_data"
    return "outside_main"


def is_movable(value: int) -> bool:
    value &= 0xFFFFFFFF
    return MAIN_START <= value < MOVABLE_END


def parse_nm(text: str) -> list[AbsSymbol]:
    """Parse and de-duplicate absolute symbols from GNU ``nm`` output."""
    found: dict[tuple[str, int, str], AbsSymbol] = {}
    for raw in text.splitlines():
        match = NM_RE.match(raw.strip())
        if match is None:
            continue
        value = int(match.group(1), 16) & 0xFFFFFFFF
        symbol_type = match.group(2)
        binding = "global" if symbol_type == "A" else "local"
        name = match.group(3)
        symbol = AbsSymbol(
            name=name,
            value=value,
            binding=binding,
            region=address_region(value),
            movable=is_movable(value),
        )
        found[(name, value, binding)] = symbol
    return sorted(found.values(), key=lambda item: (item.value, item.name, item.binding))


def run_nm(elf: Path, command: str = DEFAULT_NM) -> str:
    executable = shutil.which(command)
    if executable is None:
        raise AuditError(
            f"required tool {command!r} is not on PATH; run inside `nix develop`"
        )
    result = subprocess.run(
        [executable, "-n", str(elf)],
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    if result.returncode != 0:
        detail = result.stderr.strip() or result.stdout.strip()
        raise AuditError(f"{command} failed for {elf}: {detail}")
    return result.stdout


def linked_raw_inputs(linker_text: str) -> list[str]:
    """Return only raw generated inputs actually named by the linker script."""
    return sorted(
        set(LINKED_RAW_RE.findall(linker_text)),
        key=lambda name: int(name.split(".", 1)[0], 16),
    )


def parse_word_source(text: str, filename: str) -> list[WordRecord]:
    records: list[WordRecord] = []
    owner: str | None = None
    for line_number, raw in enumerate(text.splitlines(), 1):
        label = DLABEL_RE.match(raw)
        if label is not None:
            owner = label.group(1)
        match = WORD_RE.search(raw)
        if match is None:
            continue
        source = int(match.group(2), 16)
        operand = match.group(3).rstrip(",")
        try:
            value: int | None = int(operand, 0) & 0xFFFFFFFF
        except ValueError:
            value = None
        records.append(
            WordRecord(
                source=source,
                file=filename,
                line=line_number,
                owner=owner,
                operand=operand,
                value=value,
                source_region=source_region(source),
            )
        )
    return records


def _candidate(record: WordRecord, **extra: object) -> dict[str, object]:
    result: dict[str, object] = {
        "source": hex32(record.source),
        "source_region": record.source_region,
        "file": record.file,
        "line": record.line,
        "owner": record.owner,
    }
    result.update(extra)
    return result


LOW_ADDRESS_OPS = {
    8: "addi",
    9: "addiu",
    13: "ori",
    32: "lb",
    33: "lh",
    34: "lwl",
    35: "lw",
    36: "lbu",
    37: "lhu",
    38: "lwr",
    39: "lwu",
    40: "sb",
    41: "sh",
    42: "swl",
    43: "sw",
    44: "sdl_or_swc1",
    45: "sdr_or_sdc1",
    46: "swr",
}


def analyse_words(records: Iterable[WordRecord]) -> dict[str, object]:
    words = sorted(records, key=lambda item: (item.file, item.source, item.line))
    by_location = {(item.file, item.source): item for item in words}

    raw_counts: dict[str, Counter[str]] = {}
    for record in words:
        counts = raw_counts.setdefault(record.source_region, Counter())
        counts["words"] += 1
        counts["literal" if record.value is not None else "symbolic"] += 1

    jumps: list[dict[str, object]] = []
    hi_lo: list[dict[str, object]] = []
    pointers: list[dict[str, object]] = []

    for record in words:
        value = record.value
        if value is None:
            continue

        if record.source_region == "raw_code":
            opcode = value >> 26
            if opcode in (2, 3):
                target = (
                    ((record.source + 4) & 0xF0000000)
                    | ((value & 0x03FFFFFF) << 2)
                ) & 0xFFFFFFFF
                jumps.append(
                    _candidate(
                        record,
                        kind="j" if opcode == 2 else "jal",
                        instruction=hex32(value),
                        target=hex32(target),
                        target_region=address_region(target),
                        target_movable=is_movable(target),
                    )
                )

            # Conservative floor: only an immediately adjacent LUI/low use of
            # the same base register, with a reconstructed movable target.
            if opcode == 15:
                next_record = by_location.get((record.file, record.source + 4))
                if next_record is not None and next_record.value is not None:
                    low_word = next_record.value
                    low_opcode = low_word >> 26
                    high_register = (value >> 16) & 31
                    low_base = (low_word >> 21) & 31
                    if low_opcode in LOW_ADDRESS_OPS and low_base == high_register:
                        high = value & 0xFFFF
                        low = low_word & 0xFFFF
                        if low_opcode == 13:
                            target = ((high << 16) | low) & 0xFFFFFFFF
                        else:
                            signed_low = low - 0x10000 if low & 0x8000 else low
                            target = ((high << 16) + signed_low) & 0xFFFFFFFF
                        if is_movable(target):
                            hi_lo.append(
                                _candidate(
                                    record,
                                    kind="hi_lo",
                                    low_source=hex32(next_record.source),
                                    low_operation=LOW_ADDRESS_OPS[low_opcode],
                                    high_instruction=hex32(value),
                                    low_instruction=hex32(low_word),
                                    target=hex32(target),
                                    target_region=address_region(target),
                                )
                            )

        if (
            record.source_region in {"leading_data", "trailing_data"}
            and value % 4 == 0
            and is_movable(value)
        ):
            pointers.append(
                _candidate(
                    record,
                    kind="literal_pointer",
                    value=hex32(value),
                    target_region=address_region(value),
                )
            )

    summary = {
        region: dict(sorted(counts.items()))
        for region, counts in sorted(raw_counts.items())
    }
    return {
        "raw_word_summary": summary,
        "literal_jumps": sorted(jumps, key=lambda item: (item["source"], item["file"])),
        "conservative_hi_lo": sorted(
            hi_lo, key=lambda item: (item["source"], item["file"])
        ),
        "literal_pointers": sorted(
            pointers, key=lambda item: (item["source"], item["file"])
        ),
    }


def _counter_dict(values: Iterable[str]) -> dict[str, int]:
    return dict(sorted(Counter(values).items()))


def build_report(
    abs_symbols: Iterable[AbsSymbol],
    raw_inputs: Iterable[str],
    words: Iterable[WordRecord],
) -> dict[str, object]:
    symbols = list(abs_symbols)
    inputs = list(raw_inputs)
    analysis = analyse_words(words)
    movable_symbols = [symbol for symbol in symbols if symbol.movable]

    serial_symbols = []
    for symbol in symbols:
        item = asdict(symbol)
        item["value"] = hex32(symbol.value)
        serial_symbols.append(item)

    jumps = analysis["literal_jumps"]
    hi_lo = analysis["conservative_hi_lo"]
    pointers = analysis["literal_pointers"]
    assert isinstance(jumps, list) and isinstance(hi_lo, list) and isinstance(pointers, list)

    summary = {
        "linked_raw_inputs": len(inputs),
        "absolute_symbols": len(symbols),
        "movable_absolute_symbols": len(movable_symbols),
        "absolute_symbols_by_region": _counter_dict(symbol.region for symbol in symbols),
        "movable_absolute_symbols_by_region": _counter_dict(
            symbol.region for symbol in movable_symbols
        ),
        "literal_jumps": len(jumps),
        "literal_jumps_by_kind": _counter_dict(str(item["kind"]) for item in jumps),
        "literal_jumps_by_target_region": _counter_dict(
            str(item["target_region"]) for item in jumps
        ),
        "conservative_hi_lo": len(hi_lo),
        "conservative_hi_lo_by_target_region": _counter_dict(
            str(item["target_region"]) for item in hi_lo
        ),
        "literal_pointers": len(pointers),
        "literal_pointers_by_target_region": _counter_dict(
            str(item["target_region"]) for item in pointers
        ),
    }
    summary["finding_count"] = (
        summary["movable_absolute_symbols"]
        + summary["literal_jumps"]
        + summary["conservative_hi_lo"]
        + summary["literal_pointers"]
    )

    return {
        "schema": 1,
        "layout": {
            "main_start": hex32(MAIN_START),
            "leading_data_end": hex32(LEADING_DATA_END),
            "game_code_end": hex32(GAME_CODE_END),
            "code_end": hex32(CODE_END),
            "loaded_end": hex32(LOADED_END),
            "movable_end": hex32(MOVABLE_END),
        },
        "summary": summary,
        "raw_word_summary": analysis["raw_word_summary"],
        "linked_raw_inputs": inputs,
        "absolute_symbols": serial_symbols,
        "literal_jumps": jumps,
        "conservative_hi_lo": hi_lo,
        "literal_pointers": pointers,
    }


def _format_counts(counts: dict[str, int]) -> str:
    if not counts:
        return "none"
    return ", ".join(f"{name}={counts[name]}" for name in sorted(counts))


def render_text(report: dict[str, object], *, details: bool = False) -> str:
    summary = report["summary"]
    raw = report["raw_word_summary"]
    assert isinstance(summary, dict) and isinstance(raw, dict)
    lines = ["reloc-audit main.exe"]
    lines.append(f"  linked raw inputs: {summary['linked_raw_inputs']}")
    lines.append(
        "  final ABS symbols: "
        f"{summary['absolute_symbols']} total; "
        f"{summary['movable_absolute_symbols']} in movable MAIN"
    )
    lines.append(
        "    movable regions: "
        + _format_counts(summary["movable_absolute_symbols_by_region"])
    )
    lines.append("  raw word directives:")
    for region in sorted(raw):
        counts = raw[region]
        lines.append(
            f"    {region}: words={counts.get('words', 0)}, "
            f"literal={counts.get('literal', 0)}, "
            f"symbolic={counts.get('symbolic', 0)}"
        )
    lines.append(
        f"  literal J/JAL candidates: {summary['literal_jumps']} "
        f"({_format_counts(summary['literal_jumps_by_kind'])})"
    )
    lines.append(
        "    targets: " + _format_counts(summary["literal_jumps_by_target_region"])
    )
    lines.append(
        f"  conservative adjacent HI/LO candidates: {summary['conservative_hi_lo']}"
    )
    lines.append(
        "    targets: "
        + _format_counts(summary["conservative_hi_lo_by_target_region"])
    )
    lines.append(
        f"  literal aligned data pointers: {summary['literal_pointers']}"
    )
    lines.append(
        "    targets: "
        + _format_counts(summary["literal_pointers_by_target_region"])
    )
    lines.append(
        f"  findings: {summary['finding_count']} (audit-only unless "
        "--fail-on-findings is passed)"
    )

    if details:
        lines.append("  details:")
        for symbol in report["absolute_symbols"]:
            if symbol["movable"]:
                lines.append(
                    f"    ABS {symbol['value']} {symbol['region']} "
                    f"{symbol['binding']} {symbol['name']}"
                )
        for key, label in (
            ("literal_jumps", "JUMP"),
            ("conservative_hi_lo", "HILO"),
            ("literal_pointers", "PTR"),
        ):
            for item in report[key]:
                target = item.get("target", item.get("value", "?"))
                lines.append(
                    f"    {label} {item['source']} -> {target} "
                    f"{item['file']}:{item['line']} {item.get('owner') or '-'}"
                )
    return "\n".join(lines) + "\n"


def resolve(root: Path, path: Path) -> Path:
    return path if path.is_absolute() else root / path


def collect(root: Path, elf: Path, linker: Path, raw_dir: Path, nm: str) -> dict[str, object]:
    elf = resolve(root, elf)
    linker = resolve(root, linker)
    raw_dir = resolve(root, raw_dir)
    for path, description in ((elf, "ELF"), (linker, "linker script"), (raw_dir, "raw input directory")):
        if not path.exists():
            raise AuditError(
                f"missing {description}: {path}; run `./Build` before the audit"
            )

    inputs = linked_raw_inputs(linker.read_text())
    if not inputs:
        raise AuditError(f"{linker}: no linked generated data inputs found")
    words: list[WordRecord] = []
    for name in inputs:
        source = raw_dir / name
        if not source.is_file():
            raise AuditError(f"linked raw input is missing: {source}")
        words.extend(parse_word_source(source.read_text(), name))
    return build_report(parse_nm(run_nm(elf, nm)), inputs, words)


def parser() -> argparse.ArgumentParser:
    argument_parser = argparse.ArgumentParser(
        description=(
            "Inventory final absolute symbols and unrelocated address-bearing "
            "words in the current MAIN.EXE link."
        ),
        epilog=(
            "The initial mode is deliberately an audit: findings exit zero unless "
            "--fail-on-findings is used. Before enabling that switch in CI, split "
            "fixed hardware/ABI symbols into a reviewed allowlist and make every "
            "movable code/data symbol linker-owned. The eventual acceptance gate "
            "should pair this static audit with controlled +4 and +0x10004 relinks "
            "so HI16 carry and header/BSS updates are tested, not merely inferred."
        ),
    )
    argument_parser.add_argument("--root", type=Path, default=ROOT)
    argument_parser.add_argument("--elf", type=Path, default=DEFAULT_ELF)
    argument_parser.add_argument("--linker", type=Path, default=DEFAULT_LINKER)
    argument_parser.add_argument("--raw-dir", type=Path, default=DEFAULT_RAW_DIR)
    argument_parser.add_argument("--nm", default=DEFAULT_NM)
    argument_parser.add_argument("--json", action="store_true", help="emit the complete machine-readable report")
    argument_parser.add_argument("--details", action="store_true", help="list every movable ABS symbol and raw candidate")
    argument_parser.add_argument(
        "--fail-on-findings",
        action="store_true",
        help="exit 1 when the report has findings (intended for the future allowlisted gate)",
    )
    return argument_parser


def main(argv: list[str] | None = None) -> int:
    args = parser().parse_args(argv)
    try:
        report = collect(args.root, args.elf, args.linker, args.raw_dir, args.nm)
    except (AuditError, OSError) as error:
        print(f"reloc-audit: {error}", file=sys.stderr)
        return 2
    if args.json:
        print(json.dumps(report, indent=2, sort_keys=True))
    else:
        print(render_text(report, details=args.details), end="")
    finding_count = int(report["summary"]["finding_count"])
    return 1 if args.fail_on_findings and finding_count else 0


if __name__ == "__main__":
    raise SystemExit(main())

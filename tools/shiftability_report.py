#!/usr/bin/env python3
"""Render one authoritative MAIN.EXE shiftability dashboard.

The matching report answers "how much source matches?".  This report answers a
different question: "what still prevents a normal GNU-ld layout from moving?"
It composes the existing relocation audits and growth proof instead of adding a
second set of address heuristics.

Findings are divided into three deliberately different classes:

* BLOCKER: a linked input/final image lacks a required relocation or a proof
  failed;
* DEBT: exact and normal builds still need different human-source shapes, even
  though the normal-link object is relocation-safe; and
* CONTRACT/BOUNDED: an intentional PS1/game memory boundary or a stated proof
  limit, neither of which is a stale internal pointer.

``./Build shiftability-report`` builds fresh normal-link inputs and runs the
complete dashboard.  ``--json`` is intended for agents and other tooling.
"""

from __future__ import annotations

import argparse
from collections import Counter
from dataclasses import asdict
import json
from pathlib import Path
import re
import sys
from typing import Callable

try:
    from tools import (
        ram_layout,
        reloc_audit,
        reloc_c_literals,
        reloc_growth_probe,
        reloc_input_audit,
    )
except ModuleNotFoundError:  # Direct invocation adds tools/, not the repo root.
    import ram_layout  # type: ignore[no-redef]
    import reloc_audit  # type: ignore[no-redef]
    import reloc_c_literals  # type: ignore[no-redef]
    import reloc_growth_probe  # type: ignore[no-redef]
    import reloc_input_audit  # type: ignore[no-redef]


ROOT = Path(__file__).resolve().parents[1]

NORMAL_ELF = Path(".shake/build/tenchu/main_relink.exe.elf")
NORMAL_MAP = Path(".shake/build/tenchu/main_relink.exe.map")
NORMAL_LINKER = Path(".shake/build/relink/layout/main.exe.ld")
NORMAL_C_LINKER = Path(".shake/build/relink/c/main.exe.ld")
RAW_DATA_DIR = Path(".shake/gen/main.exe/asm/data")
RETAIL_LAYOUT_ELF = Path(".shake/build/tenchu/main_reloc_bss.exe.elf")
GROWTH_OUTPUT_DIR = Path(".shake/build/shiftability-report/growth")

RAM_LAYOUT = ram_layout.LAYOUT
POOL_FLOOR = RAM_LAYOUT.memory_pool_floor
POOL_END = RAM_LAYOUT.memory_pool_end
RAM_END = RAM_LAYOUT.cached_ram_end
INITIAL_STACK = RAM_LAYOUT.initial_stack_address

CONDITIONAL_RE = re.compile(
    r"^\s*#\s*(?:if|ifdef|ifndef|elif)\b.*\bTENCHU_RELOCATABLE\b"
)
VMEM_ACCESS_RE = re.compile(r"\bVMEM_DEFAULT_(?:POOL|CAPACITY)\b")
INITIAL_SOURCE_VARIANT_INVENTORY = 6


class ReportError(RuntimeError):
    """The dashboard inventory is inconsistent or incomplete."""


def resolve(root: Path, path: Path) -> Path:
    return path if path.is_absolute() else root / path


def hex32(value: int) -> str:
    return f"0x{value & 0xFFFFFFFF:08x}"


def _capture(callback: Callable[[], dict[str, object]]) -> dict[str, object]:
    try:
        return {"status": "pass", "data": callback()}
    except Exception as error:  # Keep rendering the other independent sections.
        return {
            "status": "fail",
            "error_type": type(error).__name__,
            "error": str(error),
        }


def _symbol_value(elf: reloc_c_literals.ElfObject, name: str) -> int:
    return elf.symbol(name).value


def collect_layout(root: Path, elf_path: Path = NORMAL_ELF) -> dict[str, object]:
    elf = reloc_c_literals.ElfObject(resolve(root, elf_path))
    values = {
        name: _symbol_value(elf, name)
        for name in (
            "__load_start",
            "__SN_ENTRY_POINT",
            "main_exe_INITIALIZED_END",
            "__bss_start",
            "__bss_end",
            "HEAP_START",
            "MemoryPool",
            "MemoryPoolEnd",
            "MemoryPoolCapacity",
            "__tenchu_handoff_start",
            "__tenchu_handoff_end",
            "PersistentState",
            "STARTING_RNG_SEED",
        )
    }
    fixed_expectations = {
        "__load_start": RAM_LAYOUT.main_load_address,
        "__tenchu_handoff_start": RAM_LAYOUT.executable_handoff_address,
        "__tenchu_handoff_end": RAM_LAYOUT.executable_handoff_end,
        "PersistentState": RAM_LAYOUT.persistent_state_address,
        "STARTING_RNG_SEED": RAM_LAYOUT.persistent_rng_address,
    }
    for name, expected in fixed_expectations.items():
        if values[name] != expected:
            raise ReportError(
                f"{name} is {hex32(values[name])}, but the central RAM "
                f"layout requires {hex32(expected)}"
            )
    pool_alignment = RAM_LAYOUT.memory_pool_alignment
    if values["MemoryPool"] != max(
        POOL_FLOOR,
        (values["__bss_end"] + pool_alignment - 1)
        & ~(pool_alignment - 1),
    ):
        raise ReportError("MemoryPool is not the preferred floor/aligned BSS end")
    if values["MemoryPoolEnd"] != POOL_END:
        raise ReportError(
            f"MemoryPoolEnd is {hex32(values['MemoryPoolEnd'])}, expected "
            f"current budget ceiling {hex32(POOL_END)}"
        )
    expected_capacity = (
        values["MemoryPoolEnd"] - values["MemoryPool"]
    ) // 4 - RAM_LAYOUT.memory_pool_header_words
    if values["MemoryPoolCapacity"] != expected_capacity:
        raise ReportError("MemoryPoolCapacity does not follow the linked bounds")

    return {
        "symbols": {name: hex32(value) for name, value in values.items()},
        "load_bytes": values["main_exe_INITIALIZED_END"] - values["__load_start"],
        "bss_bytes": values["__bss_end"] - values["__bss_start"],
        "pool_bytes": values["MemoryPoolEnd"] - values["MemoryPool"],
        "pool_capacity_words": values["MemoryPoolCapacity"],
        "headroom_until_pool_moves": max(0, POOL_FLOOR - values["__bss_end"]),
        "headroom_until_bss_handoff_limit": (
            values["__tenchu_handoff_start"] - values["__bss_end"]
        ),
        "pool_policy": (
            f"MemoryPool=max({hex32(POOL_FLOOR)}, "
            f"align{RAM_LAYOUT.memory_pool_alignment}(BSS_END)); the base "
            "moves upward after retail headroom is consumed"
        ),
    }


def _object_paths(root: Path) -> tuple[dict[str, Path], dict[str, Path]]:
    objects = {
        name: root / ".shake/reloc-c-literals" / f"{name}.o"
        for name in reloc_c_literals.REPLACEMENT_OBJECT_SPECS
    }
    objects.update(
        {
            name: root / ".shake/build/main.exe" / f"{name}.c.o"
            for name in reloc_c_literals.ORDINARY_OBJECT_SPECS
        }
    )
    references = {
        name: root / ".shake/build/main.exe" / f"{name}.c.o"
        for name in reloc_c_literals.REPLACEMENT_OBJECT_SPECS
    }
    return objects, references


def _unique_sdk_symbols(elf: reloc_c_literals.ElfObject) -> int:
    candidates = [
        symbol
        for symbol in elf.symbols
        if reloc_c_literals.SDK_TEXT_START
        <= symbol.value
        < reloc_c_literals.SDK_TEXT_END
        and symbol.section_index
        not in (reloc_c_literals.SHN_UNDEF, reloc_c_literals.SHN_ABS)
    ]
    counts = Counter(symbol.name for symbol in candidates)
    return sum(count == 1 for count in counts.values())


def collect_compiler_references(root: Path) -> dict[str, object]:
    objects, references = _object_paths(root)
    for name, path in sorted(objects.items()):
        reloc_c_literals.verify_contract(
            reloc_c_literals.ElfObject(path),
            reloc_c_literals.OBJECT_SPECS[name],
            name,
        )

    retail = reloc_c_literals.ElfObject(resolve(root, RETAIL_LAYOUT_ELF))
    normal = reloc_c_literals.ElfObject(resolve(root, NORMAL_ELF))
    linker_source = resolve(root, NORMAL_C_LINKER).read_text()
    # The generated linker records repo-relative object names.  The verifier
    # also opens those objects, so make both views absolute when --root points
    # at a repository other than the caller's current directory.
    for name, object_path in objects.items():
        relative = object_path.relative_to(root)
        linker_source = linker_source.replace(str(relative), str(object_path))
    checks = reloc_c_literals.verify_normal_link(
        retail,
        normal,
        references,
        objects,
        linker_source,
    )
    linked_pairs = reloc_c_literals.verify_linked_relocations(normal, objects)
    shrink = reloc_c_literals.text_shrink(references, objects)
    relocation_records = sum(
        sum(expected.values())
        for spec in reloc_c_literals.OBJECT_SPECS.values()
        for expected in spec.targets.values()
    )
    return {
        "audited_functions": len(reloc_c_literals.OBJECT_SPECS),
        "target_pairs": len(linked_pairs),
        "relocation_records": relocation_records,
        "normal_text_delta": -shrink,
        "sdk_symbols_following_layout": _unique_sdk_symbols(retail),
        "ordinary_exact_symbolic_sources": sorted(
            reloc_c_literals.ORDINARY_OBJECT_SPECS
        ),
        "alternate_normal_link_objects": sorted(
            reloc_c_literals.REPLACEMENT_OBJECT_SPECS
        ),
        "linked_targets": sorted(
            {
                target
                for spec in reloc_c_literals.OBJECT_SPECS.values()
                for target in spec.targets
            }
        ),
        "checks": checks,
    }


def collect_source_debt(root: Path) -> dict[str, object]:
    source_root = root / "src/main.exe"
    discovered: dict[str, list[int]] = {}
    for source in sorted(source_root.rglob("*.c")):
        lines = [
            line_number
            for line_number, line in enumerate(source.read_text().splitlines(), 1)
            if CONDITIONAL_RE.search(line)
        ]
        if lines:
            discovered[source.relative_to(root).as_posix()] = lines

    expected_conditional_paths = {
        f"src/main.exe/{name}.c"
        for name, debt in reloc_c_literals.SOURCE_VARIANT_DEBT.items()
        if debt.category == reloc_c_literals.SOURCE_RECONSTRUCTION
    }
    actual_paths = set(discovered)
    if actual_paths != expected_conditional_paths:
        missing = ", ".join(sorted(expected_conditional_paths - actual_paths)) or "none"
        extra = ", ".join(sorted(actual_paths - expected_conditional_paths)) or "none"
        raise ReportError(
            "per-source TENCHU_RELOCATABLE inventory differs from reviewed debt: "
            f"missing {missing}; extra {extra}"
        )

    entries = []
    for name, debt in sorted(reloc_c_literals.SOURCE_VARIANT_DEBT.items()):
        path = f"src/main.exe/{name}.c"
        if debt.category == reloc_c_literals.SOURCE_RECONSTRUCTION:
            sites = discovered[path]
            interface = "per_source_conditional"
        else:
            sites = [
                line_number
                for line_number, line in enumerate(
                    (root / path).read_text().splitlines(), 1
                )
                if VMEM_ACCESS_RE.search(line)
                and not line.lstrip().startswith(("*", "/*", "//"))
            ]
            if not sites:
                raise ReportError(
                    f"{path} has no centralized VMEM_DEFAULT_* variant access"
                )
            interface = "central_vmemory_accessor"
        entries.append(
            {
                "name": name,
                "path": path,
                "line": sites[0],
                "variant_sites": len(sites),
                "interface": interface,
                "category": debt.category,
                "detail": debt.detail,
                "normal_relink_blocker": False,
            }
        )
    categories = Counter(entry["category"] for entry in entries)
    return {
        "count": len(entries),
        "resolved_from_initial_inventory": (
            INITIAL_SOURCE_VARIANT_INVENTORY - len(entries)
        ),
        "initial_inventory": INITIAL_SOURCE_VARIANT_INVENTORY,
        "categories": dict(sorted(categories.items())),
        "entries": entries,
    }


def collect_central_layout_usage(root: Path) -> dict[str, object]:
    findings = ram_layout.source_literal_findings(root)
    if findings:
        rendered = ", ".join(
            f"{item['file']}:{item['line']}={item['literal']}"
            for item in findings
        )
        raise ReportError(
            "fixed RAM-layout literals bypass src/main.exe/ram_layout.h: "
            + rendered
        )
    return {
        "authority": "src/main.exe/ram_layout.h",
        "python_loader": "tools/ram_layout.py",
        "duplicated_game_source_literals": 0,
    }


def collect_input_audit(root: Path, linker: Path, elf: Path, link_map: Path) -> dict[str, object]:
    return reloc_input_audit.collect(root, linker, elf, link_map)


def collect_final_audit(
    root: Path, elf: Path, linker: Path, raw_dir: Path, nm: str
) -> dict[str, object]:
    return reloc_audit.collect(root, elf, linker, raw_dir, nm)


def collect_growth(root: Path, nm: str) -> dict[str, object]:
    args = reloc_growth_probe.arguments([])
    args.root = root
    args.nm = nm
    # check-relink runs the same proof directly. Keep the dashboard's mutable
    # intermediates separate so parallel Shake targets cannot race each other.
    args.output_dir = GROWTH_OUTPUT_DIR
    return asdict(reloc_growth_probe.execute(args))


def intentional_contracts() -> list[dict[str, str]]:
    persistent_end = RAM_LAYOUT.persistent_rng_address
    persistent_rng_end = RAM_LAYOUT.persistent_rng_end
    handoff_end = RAM_LAYOUT.executable_handoff_end
    return [
        {
            "name": "MAIN.EXE load origin",
            "range": hex32(RAM_LAYOUT.main_load_address),
            "reason": (
                "central build-time load contract consumed by the output "
                "section and PS-X header"
            ),
        },
        {
            "name": "persistent launcher state",
            "range": (
                f"{hex32(RAM_LAYOUT.persistent_state_address)}.."
                f"{hex32(persistent_end)}"
            ),
            "reason": (
                "cross-executable ABI; MAIN aliases rebase, but peer "
                "executables must move in concert"
            ),
        },
        {
            "name": "persistent RNG seed",
            "range": (
                f"{hex32(RAM_LAYOUT.persistent_rng_address)}.."
                f"{hex32(persistent_rng_end)}"
            ),
            "reason": "RNG follows the configured persistent-state size",
        },
        {
            "name": "executable handoff record",
            "range": (
                f"{hex32(RAM_LAYOUT.executable_handoff_address)}.."
                f"{hex32(handoff_end)}"
            ),
            "reason": "cross-executable ABI with reviewed lifetime overlap",
        },
        {
            "name": "initial stack",
            "range": hex32(INITIAL_STACK),
            "reason": "PS-X EXE startup contract",
        },
        {
            "name": "cached main-RAM end",
            "range": hex32(RAM_END),
            "reason": "PS1 hardware boundary",
        },
        {
            "name": "PC-link memory-disk pool",
            "range": (
                f"{hex32(RAM_LAYOUT.pc_memory_pool_address)}.."
                f"{hex32(RAM_LAYOUT.pc_memory_handshake_address)}"
            ),
            "reason": "development PC-link memory contract outside retail 2 MiB RAM",
        },
        {
            "name": "PC-link memory handshake",
            "range": (
                f"{hex32(RAM_LAYOUT.pc_memory_handshake_address)}.."
                f"{hex32(RAM_LAYOUT.pc_memory_handshake_end)}"
            ),
            "reason": "development host-interface sentinel",
        },
        {
            "name": "scratchpad",
            "range": (
                f"{hex32(RAM_LAYOUT.scratchpad_address)}.."
                f"{hex32(RAM_LAYOUT.scratchpad_end)}"
            ),
            "reason": "PS1 hardware address",
        },
        {
            "name": "MMIO window",
            "range": (
                f"{hex32(RAM_LAYOUT.mmio_address)}.."
                f"{hex32(RAM_LAYOUT.mmio_end)}"
            ),
            "reason": "PS1 hardware addresses",
        },
        {
            "name": "virtual-memory block header",
            "range": f"{RAM_LAYOUT.memory_pool_header_words} words",
            "reason": "fixed allocator ABI used by all valloc-family functions",
        },
    ]


def configurable_layout_policy() -> list[dict[str, str]]:
    return [
        {
            "name": "BSS alignment",
            "value": str(RAM_LAYOUT.bss_alignment),
            "effect": "applied after the initialized image before linked BSS",
        },
        {
            "name": "virtual-memory pool preferred floor",
            "value": hex32(POOL_FLOOR),
            "effect": "the linked base may move upward after BSS reaches this floor",
        },
        {
            "name": "virtual-memory pool ceiling",
            "value": hex32(POOL_END),
            "effect": "fixed for the current RAM budget; moving it changes available capacity",
        },
        {
            "name": "virtual-memory pool alignment",
            "value": str(RAM_LAYOUT.memory_pool_alignment),
            "effect": "applied to the linked BSS end before choosing a moved pool base",
        },
        {
            "name": "virtual-memory pool minimum size",
            "value": hex32(RAM_LAYOUT.memory_pool_minimum_size),
            "effect": "linker assertion keeps a usable allocator arena",
        },
    ]


PROOF_LIMITS = [
    "The input HI16/LO16 scan is bounded and linear; arbitrary register arithmetic is not proved.",
    "Opaque packed assembly relies on the reviewed pointer manifest and shifted growth proof.",
    "The hermetic dashboard proves structure and one large growth, not broad gameplay or every media path.",
    "Growth remains bounded by the fixed handoff, pool ceiling, stack, RAM, and MIPS relocation ranges.",
]


def _data(section: dict[str, object]) -> dict[str, object] | None:
    value = section.get("data")
    return value if isinstance(value, dict) else None


def _mark_finding_failure(
    section: dict[str, object], finding_count: int
) -> None:
    """Make an audit finding visible as FAIL without inventing an exception."""

    if section.get("status") == "pass" and finding_count:
        section["status"] = "fail"


def active_blockers(sections: dict[str, dict[str, object]]) -> list[dict[str, object]]:
    blockers: list[dict[str, object]] = []
    for name, section in sections.items():
        if section.get("status") == "fail" and "error" in section:
            blockers.append(
                {
                    "category": name,
                    "kind": "proof_error",
                    "detail": section["error"],
                }
            )

    input_data = _data(sections.get("input_audit", {}))
    if input_data is not None:
        for finding in input_data.get("findings", []):
            blockers.append({"category": "input_audit", **finding})

    final_data = _data(sections.get("final_audit", {}))
    if final_data is not None:
        for key in (
            "absolute_symbols",
            "literal_jumps",
            "conservative_hi_lo",
            "literal_pointers",
        ):
            for finding in final_data.get(key, []):
                if key == "absolute_symbols" and not finding.get("movable"):
                    continue
                blockers.append(
                    {"category": "final_audit", "kind": key, **finding}
                )
    return blockers


def collect_report(
    *,
    root: Path,
    elf: Path = NORMAL_ELF,
    link_map: Path = NORMAL_MAP,
    linker: Path = NORMAL_LINKER,
    raw_dir: Path = RAW_DATA_DIR,
    nm: str = reloc_audit.DEFAULT_NM,
    run_growth: bool = True,
) -> dict[str, object]:
    root = root.resolve()
    if root != ROOT.resolve():
        raise ReportError(
            "alternate repository roots are unsupported because the composed "
            "audit modules load this checkout's RAM policy"
        )
    sections = {
        "layout": _capture(lambda: collect_layout(root, elf)),
        "compiler_references": _capture(lambda: collect_compiler_references(root)),
        "input_audit": _capture(
            lambda: collect_input_audit(root, linker, elf, link_map)
        ),
        "final_audit": _capture(
            lambda: collect_final_audit(root, elf, linker, raw_dir, nm)
        ),
        "source_debt": _capture(lambda: collect_source_debt(root)),
        "central_layout_usage": _capture(
            lambda: collect_central_layout_usage(root)
        ),
    }
    input_data = _data(sections["input_audit"])
    if input_data is not None:
        _mark_finding_failure(
            sections["input_audit"], int(input_data["summary"]["findings"])
        )
    final_data = _data(sections["final_audit"])
    if final_data is not None:
        _mark_finding_failure(
            sections["final_audit"],
            int(final_data["summary"]["finding_count"]),
        )
    sections["growth_proof"] = (
        _capture(lambda: collect_growth(root, nm))
        if run_growth
        else {"status": "skipped", "reason": "--no-growth"}
    )
    blockers = active_blockers(sections)
    status = "fail" if blockers else ("pass" if run_growth else "partial")
    return {
        "schema": 1,
        "artifact": "MAIN.EXE normal relink",
        "status": status,
        "active_blocker_count": len(blockers),
        "runtime_confidence": "bounded",
        "sections": sections,
        "active_blockers": blockers,
        "intentional_fixed_contracts": intentional_contracts(),
        "configurable_layout_policy": configurable_layout_policy(),
        "proof_limits": PROOF_LIMITS,
    }


def _section_status(section: dict[str, object]) -> str:
    return str(section.get("status", "unknown")).upper()


def _render_error(lines: list[str], section: dict[str, object]) -> bool:
    if section.get("status") != "fail" or "error" not in section:
        return False
    lines.append(f"  ERROR: {section['error']}")
    return True


def render_text(report: dict[str, object]) -> str:
    sections = report["sections"]
    assert isinstance(sections, dict)
    blockers = report["active_blockers"]
    assert isinstance(blockers, list)
    lines = [
        "shiftability-report MAIN.EXE",
        f"status: {str(report['status']).upper()} — {len(blockers)} active blocker(s)",
        f"runtime confidence: {str(report['runtime_confidence']).upper()}",
        "",
    ]

    layout_section = sections["layout"]
    assert isinstance(layout_section, dict)
    lines.append(f"{_section_status(layout_section)}  Linked memory layout")
    if not _render_error(lines, layout_section):
        layout = _data(layout_section)
        assert layout is not None
        symbols = layout["symbols"]
        assert isinstance(symbols, dict)
        lines.extend(
            [
                f"  initialized: {symbols['__load_start']}..{symbols['main_exe_INITIALIZED_END']}",
                f"  BSS: {symbols['__bss_start']}..{symbols['__bss_end']} ({layout['bss_bytes']} bytes)",
                f"  HEAP_START: {symbols['HEAP_START']} "
                "(BSS+4 boundary; not the valloc pool)",
                "  MemoryPool: "
                f"{symbols['MemoryPool']}..{symbols['MemoryPoolEnd']} "
                f"({layout['pool_bytes']} bytes; capacity "
                f"0x{int(layout['pool_capacity_words']):x} words)",
                "  preferred-floor headroom: "
                f"0x{int(layout['headroom_until_pool_moves']):x} bytes; "
                "then the pool base moves and capacity shrinks",
                "  BSS-to-handoff limit: "
                f"0x{int(layout['headroom_until_bss_handoff_limit']):x} bytes",
            ]
        )

    compiler_section = sections["compiler_references"]
    assert isinstance(compiler_section, dict)
    lines.extend(["", f"{_section_status(compiler_section)}  Compiler-produced references"])
    if not _render_error(lines, compiler_section):
        compiler = _data(compiler_section)
        assert compiler is not None
        lines.extend(
            [
                f"  normal C text delta: {int(compiler['normal_text_delta']):+d} bytes; boundary pad: none",
                f"  unique SDK symbols following layout: {compiler['sdk_symbols_following_layout']}",
                f"  audited functions: {compiler['audited_functions']}; target pairs: "
                f"{compiler['target_pairs']}; relocation records: {compiler['relocation_records']}",
                "  ordinary exact symbolic source: "
                + ", ".join(compiler["ordinary_exact_symbolic_sources"]),
                f"  alternate normal-link objects: {len(compiler['alternate_normal_link_objects'])}",
            ]
        )

    central_section = sections["central_layout_usage"]
    assert isinstance(central_section, dict)
    lines.extend(["", f"{_section_status(central_section)}  Central RAM-layout policy"])
    if not _render_error(lines, central_section):
        central = _data(central_section)
        assert central is not None
        lines.extend(
            [
                f"  authority: {central['authority']}",
                f"  linker/audit loader: {central['python_loader']}",
                "  duplicated fixed-address literals in game source: "
                f"{central['duplicated_game_source_literals']}",
            ]
        )

    input_section = sections["input_audit"]
    assert isinstance(input_section, dict)
    lines.extend(["", f"{_section_status(input_section)}  Input ownership and relocation coverage"])
    if not _render_error(lines, input_section):
        input_report = _data(input_section)
        assert input_report is not None
        counts = input_report["counts"]
        assert isinstance(counts, dict)
        finding_count = input_report["summary"]["findings"]
        lines.extend(
            [
                f"  linked objects: {counts['objects']}; selected alloc sections: "
                f"{counts['selected_alloc_sections']} "
                f"(exec={counts['executable_sections']}, data={counts['data_sections']})",
                f"  reviewed MIPS metadata sections: {counts['reviewed_alloc_metadata_sections']}",
                f"  direct J/JAL: {counts['relocated_direct_jumps']}/{counts['direct_jumps']} R_MIPS_26-backed",
                "  PC-relative branches: "
                f"{counts['local_pc_relative_branches']} same-section; "
                f"{counts['relocated_pc_relative_branches']} R_MIPS_PC16-backed",
                f"  gp address uses: {counts['relocated_gp_address_uses']}/{counts['gp_address_uses']} relocation-backed",
                f"  symbolic HI16 records: {counts['symbolic_hi16']}",
                f"  alloc-data windows: {counts['data_words']}; R_MIPS_32-backed: {counts['symbolic_data_words']}",
                f"  findings: {finding_count}",
            ]
        )

    final_section = sections["final_audit"]
    assert isinstance(final_section, dict)
    lines.extend(["", f"{_section_status(final_section)}  Final fixed-address scan"])
    if not _render_error(lines, final_section):
        final_report = _data(final_section)
        assert final_report is not None
        summary = final_report["summary"]
        assert isinstance(summary, dict)
        lines.extend(
            [
                f"  movable ABS definitions: {summary['movable_absolute_symbols']}",
                f"  literal J/JAL: {summary['literal_jumps']}",
                f"  adjacent literal HI/LO: {summary['conservative_hi_lo']}",
                f"  literal loaded pointers: {summary['literal_pointers']}",
                f"  ABS definitions: {summary['absolute_symbols']} total; "
                f"{int(summary['absolute_symbols']) - int(summary['movable_absolute_symbols'])} "
                "non-movable",
                f"  findings: {summary['finding_count']}",
            ]
        )

    growth_section = sections["growth_proof"]
    assert isinstance(growth_section, dict)
    lines.extend(["", f"{_section_status(growth_section)}  +0x10004 growth proof"])
    if growth_section.get("status") == "skipped":
        lines.append("  not run (--no-growth)")
    elif not _render_error(lines, growth_section):
        growth = _data(growth_section)
        assert growth is not None
        lines.extend(
            [
                f"  downstream representatives: {growth['shifted_symbols']} moved",
                f"  section-owned symbols: {growth['owned_symbols']} checked",
                f"  compiler pairs: {growth['compiler_relocations']} linked; "
                f"HI16 carries: {growth['hi16_carries']}",
                f"  loaded pointers: {growth['manifest_pointers']} moved",
                f"  BSS delta: +0x{int(growth['bss_delta']):x}",
                "  MemoryPool: "
                f"{hex32(int(growth['pool_start_before']))}->"
                f"{hex32(int(growth['pool_start_after']))}; capacity "
                f"0x{int(growth['pool_capacity_before']):x}->"
                f"0x{int(growth['pool_capacity_after']):x} words",
                "  PS-X header: PC "
                f"{hex32(int(growth['pc_before']))}->"
                f"{hex32(int(growth['pc_after']))}; t_size "
                f"0x{int(growth['load_size_before']):x}->"
                f"0x{int(growth['load_size_after']):x}",
                f"  movable findings: {growth['audit_findings']}",
            ]
        )

    source_section = sections["source_debt"]
    assert isinstance(source_section, dict)
    lines.extend(["", f"DEBT  Exact-vs-normal source variants ({_section_status(source_section)})"])
    if not _render_error(lines, source_section):
        source = _data(source_section)
        assert source is not None
        lines.append(
            f"  remaining: {source['count']}/{source['initial_inventory']} "
            "(normal relink already uses safe symbolic objects)"
        )
        for entry in source["entries"]:
            lines.append(
                f"  {entry['category']}: {entry['path']}:{entry['line']} "
                f"({entry['variant_sites']} {entry['interface']} site(s))"
            )

    if blockers:
        lines.extend(["", "BLOCKERS  Agent work queue"])
        for blocker in blockers:
            location = blocker.get("object") or blocker.get("file") or "-"
            offset = blocker.get("offset") or blocker.get("source") or ""
            detail = blocker.get("detail") or blocker.get("name") or blocker.get("kind")
            lines.append(
                f"  {blocker['category']}: {location}{('+' + str(offset)) if offset else ''}: {detail}"
            )
    else:
        growth_word = (
            "static proofs only; growth skipped"
            if growth_section.get("status") == "skipped"
            else "the current static/growth proofs"
        )
        lines.extend(["", f"BLOCKERS  none in {growth_word}"])

    lines.extend(["", "CONTRACT  Intentional fixed contracts (not relocation blockers)"])
    for contract in report["intentional_fixed_contracts"]:
        lines.append(
            f"  {contract['range']} {contract['name']}: {contract['reason']}"
        )

    lines.extend(["", "POLICY  Configurable linker/RAM budget (not hardware pins)"])
    for policy in report["configurable_layout_policy"]:
        lines.append(
            f"  {policy['value']} {policy['name']}: {policy['effect']}"
        )

    lines.extend(["", "BOUNDED  Proof limits"])
    for limit in report["proof_limits"]:
        lines.append(f"  {limit}")
    return "\n".join(lines) + "\n"


def parser() -> argparse.ArgumentParser:
    argument_parser = argparse.ArgumentParser(description=__doc__)
    argument_parser.add_argument("--elf", type=Path, default=NORMAL_ELF)
    argument_parser.add_argument("--map", type=Path, default=NORMAL_MAP)
    argument_parser.add_argument("--linker", type=Path, default=NORMAL_LINKER)
    argument_parser.add_argument("--raw-dir", type=Path, default=RAW_DATA_DIR)
    argument_parser.add_argument("--nm", default=reloc_audit.DEFAULT_NM)
    argument_parser.add_argument("--json", action="store_true")
    argument_parser.add_argument(
        "--no-growth",
        action="store_true",
        help="skip the +0x10004 link (static audits still run)",
    )
    return argument_parser


def main(argv: list[str] | None = None) -> int:
    args = parser().parse_args(argv)
    report = collect_report(
        root=ROOT,
        elf=args.elf,
        link_map=args.map,
        linker=args.linker,
        raw_dir=args.raw_dir,
        nm=args.nm,
        run_growth=not args.no_growth,
    )
    if args.json:
        print(json.dumps(report, indent=2, sort_keys=True))
    else:
        print(render_text(report), end="")
    return 1 if report["status"] == "fail" else 0


if __name__ == "__main__":
    raise SystemExit(main())

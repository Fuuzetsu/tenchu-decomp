#!/usr/bin/env python3
"""Resolve functions to reconstructed original translation units.

Most of the tree still has one artificial ``<function>.c`` file per function.
As those fragments are reunited into original, upper-case source files, tools
must keep accepting a function name while opening and building its containing
translation unit.  ``config/translation-units.main.exe.json`` records only the
units which have actually been reconstructed; unlisted functions retain the
one-file fallback.
"""
from __future__ import annotations

from dataclasses import dataclass
import json
from pathlib import Path
import re
from typing import Iterator


ROOT = Path(__file__).resolve().parent.parent
DEFAULT_MANIFEST = ROOT / "config/translation-units.main.exe.json"
DEFAULT_SOURCE_DIR = Path("src/main.exe")
IDENTIFIER = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
INCLUDE_ASM = re.compile(r"\bINCLUDE_ASM\s*\(")


@dataclass(frozen=True)
class SourceUnit:
    source: str
    functions: tuple[str, ...]
    debug_symbol_order: tuple[str, ...] = ()
    definition_order: tuple[str, ...] = ()

    @property
    def stem(self) -> str:
        return Path(self.source).stem

    @property
    def combined(self) -> bool:
        return len(self.functions) > 1


def load_units(manifest: str | Path = DEFAULT_MANIFEST) -> tuple[SourceUnit, ...]:
    """Load and validate the reconstructed-unit manifest.

    The debug order may contain demo-only functions which are absent from the
    retail unit.  That difference is evidence, so it is deliberately retained
    rather than forced to be a permutation of ``functions``.
    """
    path = Path(manifest)
    if not path.exists():
        return ()
    raw = json.loads(path.read_text())
    if raw.get("schema") != 1 or not isinstance(raw.get("units"), list):
        raise ValueError(f"{path}: expected schema 1 with a units list")

    units: list[SourceUnit] = []
    seen_sources: set[str] = set()
    seen_stems: set[str] = set()
    seen_functions: set[str] = set()
    for index, entry in enumerate(raw["units"]):
        where = f"{path}: units[{index}]"
        if not isinstance(entry, dict):
            raise ValueError(f"{where}: expected an object")
        source = entry.get("source")
        functions = entry.get("functions")
        debug_order = entry.get("debug_symbol_order", [])
        definition_order = entry.get("definition_order", functions)
        if (
            not isinstance(source, str)
            or Path(source).name != source
            or Path(source).suffix.lower() != ".c"
        ):
            raise ValueError(f"{where}: source must be one C filename")
        if not isinstance(functions, list) or not functions:
            raise ValueError(f"{where}: functions must be a non-empty list")
        if not isinstance(debug_order, list):
            raise ValueError(f"{where}: debug_symbol_order must be a list")
        if not isinstance(definition_order, list):
            raise ValueError(f"{where}: definition_order must be a list")
        if any(not isinstance(name, str) or not IDENTIFIER.match(name)
               for name in functions + debug_order + definition_order):
            raise ValueError(f"{where}: invalid function name")
        if len(functions) != len(set(functions)):
            raise ValueError(f"{where}: duplicate function in retail order")
        if len(debug_order) != len(set(debug_order)):
            raise ValueError(f"{where}: duplicate function in debug-symbol order")
        if len(definition_order) != len(set(definition_order)):
            raise ValueError(f"{where}: duplicate function in definition order")
        if set(definition_order) != set(functions):
            raise ValueError(
                f"{where}: definition_order must be a permutation of functions"
            )

        stem = Path(source).stem
        duplicate_functions = seen_functions.intersection(functions)
        if source in seen_sources or stem in seen_stems or duplicate_functions:
            detail = ", ".join(sorted(duplicate_functions)) or source
            raise ValueError(f"{where}: duplicate source, stem, or member: {detail}")
        seen_sources.add(source)
        seen_stems.add(stem)
        seen_functions.update(functions)
        units.append(SourceUnit(
            source,
            tuple(functions),
            tuple(debug_order),
            tuple(definition_order),
        ))
    return tuple(units)


def explicit_unit_for_function(
    name: str, manifest: str | Path = DEFAULT_MANIFEST
) -> SourceUnit | None:
    for unit in load_units(manifest):
        if name in unit.functions:
            return unit
    return None


def unit_for_function(
    name: str, manifest: str | Path = DEFAULT_MANIFEST
) -> SourceUnit:
    return explicit_unit_for_function(name, manifest) or SourceUnit(
        f"{name}.c", (name,)
    )


def explicit_unit_for_stem(
    stem: str, manifest: str | Path = DEFAULT_MANIFEST
) -> SourceUnit | None:
    for unit in load_units(manifest):
        if unit.stem == stem:
            return unit
    return None


def source_for_function(
    name: str,
    source_dir: str | Path = DEFAULT_SOURCE_DIR,
    manifest: str | Path = DEFAULT_MANIFEST,
) -> Path:
    """Return the checked-in source path containing ``name``.

    Explicit manifest spelling wins.  The two fallback probes let stand-alone
    original-style files use ``.C`` without requiring a manifest entry.
    """
    directory = Path(source_dir)
    unit = explicit_unit_for_function(name, manifest)
    if unit is not None:
        return directory / unit.source
    lower = directory / f"{name}.c"
    upper = directory / f"{name}.C"
    return lower if lower.exists() or not upper.exists() else upper


def iter_source_files(
    source_dir: str | Path = DEFAULT_SOURCE_DIR,
) -> Iterator[Path]:
    directory = Path(source_dir)
    if not directory.is_dir():
        return
    yield from sorted(
        (path for path in directory.iterdir()
         if path.is_file() and path.suffix.lower() == ".c"),
        key=lambda path: path.name,
    )


def iter_function_sources(
    source_dir: str | Path = DEFAULT_SOURCE_DIR,
    manifest: str | Path = DEFAULT_MANIFEST,
) -> Iterator[tuple[str, Path, SourceUnit]]:
    """Yield every logical function and the physical source which owns it."""
    directory = Path(source_dir)
    units = load_units(manifest)
    mapped_sources = {unit.source for unit in units}
    for unit in units:
        path = directory / unit.source
        if path.exists():
            for name in unit.functions:
                yield name, path, unit
    for path in iter_source_files(directory):
        if path.name in mapped_sources:
            continue
        unit = SourceUnit(path.name, (path.stem,))
        yield path.stem, path, unit


def source_has_asm_fallback(path: str | Path, name: str | None = None) -> bool:
    """Whether a source (or one named member) still delegates to INCLUDE_ASM."""
    text = Path(path).read_text(errors="replace")
    if name is None:
        return bool(INCLUDE_ASM.search(text))
    invocation = re.compile(
        rf"\bINCLUDE_ASM\s*\([^;]*,\s*{re.escape(name)}\s*\)", re.S
    )
    return bool(invocation.search(text))

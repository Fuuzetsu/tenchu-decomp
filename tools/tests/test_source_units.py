from __future__ import annotations

import json
from pathlib import Path
import re
import tempfile
import unittest

from tools import source_units as su
from tools import rtldump


class SourceUnitTests(unittest.TestCase):
    def manifest(self, directory: str) -> Path:
        path = Path(directory) / "units.json"
        path.write_text(json.dumps({
            "schema": 1,
            "units": [{
                "source": "WORLD.C",
                "functions": ["First", "Second"],
                "definition_order": ["Second", "First"],
                "debug_symbol_order": ["Second", "DemoOnly", "First"],
            }],
        }))
        return path

    def test_resolves_a_function_to_its_uppercase_combined_source(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            manifest = self.manifest(temporary)
            source_dir = Path(temporary) / "src"
            source_dir.mkdir()
            (source_dir / "WORLD.C").write_text("void First(void) {}\n")
            unit = su.unit_for_function("Second", manifest)
            self.assertEqual(unit.source, "WORLD.C")
            self.assertEqual(unit.functions, ("First", "Second"))
            self.assertEqual(unit.definition_order, ("Second", "First"))
            self.assertEqual(
                unit.debug_symbol_order, ("Second", "DemoOnly", "First")
            )
            self.assertEqual(
                su.source_for_function("Second", source_dir, manifest),
                source_dir / "WORLD.C",
            )

    def test_unlisted_functions_keep_the_one_file_fallback(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            manifest = self.manifest(temporary)
            source_dir = Path(temporary) / "src"
            source_dir.mkdir()
            (source_dir / "Solo.C").write_text("void Solo(void) {}\n")
            self.assertEqual(
                su.source_for_function("Solo", source_dir, manifest),
                source_dir / "Solo.C",
            )
            self.assertEqual(
                su.source_for_function("Missing", source_dir, manifest),
                source_dir / "Missing.c",
            )

    def test_function_iteration_expands_combined_units_once(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            manifest = self.manifest(temporary)
            source_dir = Path(temporary) / "src"
            source_dir.mkdir()
            (source_dir / "WORLD.C").write_text("void First(void) {}\n")
            (source_dir / "Solo.c").write_text("void Solo(void) {}\n")
            rows = list(su.iter_function_sources(source_dir, manifest))
            self.assertEqual(
                [(name, path.name) for name, path, _unit in rows],
                [("First", "WORLD.C"), ("Second", "WORLD.C"),
                 ("Solo", "Solo.c")],
            )

    def test_member_specific_asm_detection(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "MIXED.C"
            path.write_text(
                'INCLUDE_ASM("x", First);\nvoid Second(void) {}\n'
            )
            self.assertTrue(su.source_has_asm_fallback(path))
            self.assertTrue(su.source_has_asm_fallback(path, "First"))
            self.assertFalse(su.source_has_asm_fallback(path, "Second"))

    def test_rejects_duplicate_members(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "units.json"
            path.write_text(json.dumps({
                "schema": 1,
                "units": [
                    {"source": "A.C", "functions": ["Same"]},
                    {"source": "B.C", "functions": ["Same"]},
                ],
            }))
            with self.assertRaisesRegex(ValueError, "duplicate"):
                su.load_units(path)

    def test_rejects_definition_order_that_is_not_a_permutation(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "units.json"
            path.write_text(json.dumps({
                "schema": 1,
                "units": [{
                    "source": "A.C",
                    "functions": ["First", "Second"],
                    "definition_order": ["First"],
                }],
            }))
            with self.assertRaisesRegex(ValueError, "permutation"):
                su.load_units(path)

    def test_live_units_retain_debug_order_and_follow_retail_order(self) -> None:
        debug_by_unit: dict[str, list[tuple[int, str]]] = {}
        for line in (su.ROOT / "reference/psxsym-tu-map.tsv").read_text().splitlines():
            fields = line.split("\t")
            if len(fields) == 10 and not line.startswith("#"):
                debug_by_unit.setdefault(fields[2], []).append(
                    (int(fields[3]), fields[9])
                )
        retail = {}
        for line in (su.ROOT / "config/functions.main.exe.tsv").read_text().splitlines():
            fields = line.split("\t")
            if len(fields) >= 3 and not line.startswith("#"):
                retail[fields[2]] = (int(fields[0], 16), int(fields[1]))

        for unit in su.load_units():
            self.assertEqual(
                list(unit.debug_symbol_order),
                [name for _line, name in sorted(debug_by_unit[unit.source])],
            )
            self.assertEqual(
                list(unit.functions),
                sorted(unit.functions, key=lambda name: retail[name][0]),
            )
            for left, right in zip(unit.functions, unit.functions[1:]):
                self.assertEqual(
                    retail[left][0] + retail[left][1], retail[right][0],
                    f"{unit.source}: gap between {left} and {right}",
                )
            source = (su.ROOT / "src/main.exe" / unit.source).read_text()
            positions = []
            for name in unit.definition_order:
                definition = re.search(
                    rf"^[^#\n;{{}}]*\b{re.escape(name)}\s*"
                    rf"\([^;{{}}]*\)\s*\{{",
                    source,
                    re.M,
                )
                self.assertIsNotNone(definition, f"missing {name} in {unit.source}")
                positions.append(definition.start())
            self.assertEqual(positions, sorted(positions))

    def test_rtl_dump_is_focused_to_one_combined_unit_member(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            raw = Path(temporary) / "unit.i.greg"
            raw.write_text(
                ";; Function First\nfirst facts\n\n"
                ";; Function Second\nsecond facts\n"
            )
            focused = rtldump.isolate_function_dump(
                str(raw), "Second", temporary
            )
            self.assertEqual(
                Path(focused).read_text(), ";; Function Second\nsecond facts\n"
            )


if __name__ == "__main__":
    unittest.main()

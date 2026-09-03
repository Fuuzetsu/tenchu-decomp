from __future__ import annotations

import json
from pathlib import Path
import tempfile
import unittest

from tools import source_units as su


class SourceUnitTests(unittest.TestCase):
    def manifest(self, directory: str) -> Path:
        path = Path(directory) / "units.json"
        path.write_text(json.dumps({
            "schema": 1,
            "units": [{
                "source": "WORLD.C",
                "functions": ["First", "Second"],
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


if __name__ == "__main__":
    unittest.main()

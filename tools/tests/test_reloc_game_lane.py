"""Tests for the linker-owned game-code lane generator.

Run: nix develop -c python3 -m unittest tools.tests.test_reloc_game_lane
"""

from __future__ import annotations

import tempfile
from pathlib import Path
import unittest

from tools import reloc_game_lane as lane


class SymbolFilterTests(unittest.TestCase):
    def test_filters_only_half_open_game_range(self) -> None:
        source = """\
before = 0x80016130;
first = 0x80016134;
inside = 0x80020000;
last = 0x800601d0;
sdk = 0x800601D4;
relative = game_base + 4;
"""
        output, removed = lane.filter_symbols(source)
        self.assertEqual(
            output,
            "before = 0x80016130;\nsdk = 0x800601D4;\nrelative = game_base + 4;\n",
        )
        self.assertEqual(
            removed,
            {
                "first": 0x80016134,
                "inside": 0x80020000,
                "last": 0x800601D0,
            },
        )

    def test_rejects_script_without_game_symbols(self) -> None:
        with self.assertRaisesRegex(lane.LaneError, "contained no assignments"):
            lane.filter_symbols("sdk = 0x800601d4;\n")


class LinkerRewriteTests(unittest.TestCase):
    LINKER = """\
SECTIONS
{
  main_exe_TEXT_START = .;
  .shake/build/main.exe/First.c.o(.text);
  .shake/build/main.exe/StaticLeaf.c.o(.text);
  .shake/build/main.exe/LIBAPI_4F9D4.s.o(.text);
  .shake/build/main.exe/Sdk.c.o(.text);
}
"""

    def test_anchors_game_inputs_only(self) -> None:
        output, anchors = lane.rewrite_linker(self.LINKER, expected_inputs=2)
        self.assertEqual(anchors, ["First", "StaticLeaf"])
        self.assertIn("  First = .;\n  .shake/build/main.exe/First.c.o(.text);", output)
        self.assertIn(
            "  StaticLeaf = .;\n  .shake/build/main.exe/StaticLeaf.c.o(.text);",
            output,
        )
        self.assertNotIn("Sdk = .;", output)

    def test_requires_expected_complete_inventory(self) -> None:
        with self.assertRaisesRegex(lane.LaneError, "expected 3 game inputs"):
            lane.rewrite_linker(self.LINKER, expected_inputs=3)

    def test_requires_sdk_boundary(self) -> None:
        source = self.LINKER.replace(lane.FIRST_SDK_OWNER, "/OTHER.s.o(.text);")
        with self.assertRaisesRegex(lane.LaneError, "missing first SDK owner"):
            lane.rewrite_linker(source, expected_inputs=2)


class GenerateTests(unittest.TestCase):
    def test_writes_both_outputs_after_cross_check(self) -> None:
        linker = LinkerRewriteTests.LINKER
        symbols = """\
First = 0x80016134;
StaticLeaf = 0x80016138;
Sdk = 0x800601d4;
"""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            linker_in = root / "input.ld"
            symbols_in = root / "symbols.ld"
            linker_out = root / "generated" / "output.ld"
            symbols_out = root / "generated" / "symbols.ld"
            linker_in.write_text(linker)
            symbols_in.write_text(symbols)

            anchors, removed = lane.generate(
                linker_in,
                symbols_in,
                linker_out,
                symbols_out,
                expected_inputs=2,
            )

            self.assertEqual((anchors, removed), (2, 2))
            self.assertTrue(linker_out.exists())
            self.assertTrue(symbols_out.exists())
            self.assertIn("First = .;", linker_out.read_text())
            self.assertNotIn("First = 0x", symbols_out.read_text())
            self.assertIn("Sdk = 0x800601d4;", symbols_out.read_text())

    def test_rejects_anchor_without_removed_absolute_symbol(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            linker_in = root / "input.ld"
            symbols_in = root / "symbols.ld"
            linker_out = root / "output.ld"
            symbols_out = root / "output-symbols.ld"
            linker_in.write_text(LinkerRewriteTests.LINKER)
            symbols_in.write_text(
                "First = 0x80016134;\nSdk = 0x800601d4;\n"
            )

            with self.assertRaisesRegex(
                lane.LaneError, "lack retail-range absolute assignments: StaticLeaf"
            ):
                lane.generate(
                    linker_in,
                    symbols_in,
                    linker_out,
                    symbols_out,
                    expected_inputs=2,
                )
            self.assertFalse(linker_out.exists())
            self.assertFalse(symbols_out.exists())


if __name__ == "__main__":
    unittest.main()

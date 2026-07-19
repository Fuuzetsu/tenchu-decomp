"""Tests for the bounded canonical-assembly SDK relocation lane."""

from __future__ import annotations

import tempfile
from pathlib import Path
import unittest

from tools import reloc_sdk_lane as lane


class ProbeTests(unittest.TestCase):
    LINKER = """\
SECTIONS
{
  before.o(.text);
  sdk/LIBAPI_4F9D4.s.o(.text);
  after.o(.text);
}
"""

    def test_zero_probe_preserves_linker_verbatim(self) -> None:
        self.assertEqual(lane.add_probe(self.LINKER, 0), self.LINKER)

    def test_four_byte_probe_is_immediately_before_sdk(self) -> None:
        output = lane.add_probe(self.LINKER, 4)
        self.assertIn(
            "  LONG(0x00000000);\n  sdk/LIBAPI_4F9D4.s.o(.text);", output
        )

    def test_hi_carry_probe_adds_file_backed_word_and_gap(self) -> None:
        output = lane.add_probe(self.LINKER, 0x10004)
        self.assertIn(
            "  LONG(0x00000000);\n"
            "  . = . + 0x10000;\n"
            "  sdk/LIBAPI_4F9D4.s.o(.text);",
            output,
        )

    def test_rejects_missing_or_ambiguous_marker(self) -> None:
        with self.assertRaisesRegex(lane.LaneError, "found 0"):
            lane.add_probe("other.o(.text);\n", 4)
        with self.assertRaisesRegex(lane.LaneError, "found 0"):
            lane.add_probe("other.o(.text);\n", 0)
        with self.assertRaisesRegex(lane.LaneError, "found 2"):
            lane.add_probe(self.LINKER + self.LINKER, 4)

    def test_rejects_probe_other_than_controlled_sizes(self) -> None:
        with self.assertRaisesRegex(lane.LaneError, "must be 0, 4, or 0x10004"):
            lane.add_probe(self.LINKER, 8)


class GenerateTests(unittest.TestCase):
    def test_filters_only_canonical_sdk_prefix_and_writes_probe(self) -> None:
        symbols = """\
game = 0x800601D0;
Exec = 0x800601D4;
middle = 0x80062000;
last = 0x800650FC;
GsSetLsMatrix = 0x80065100;
"""
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            linker_in = root / "input.ld"
            symbols_in = root / "symbols.ld"
            linker_out = root / "generated" / "output.ld"
            symbols_out = root / "generated" / "symbols.ld"
            linker_in.write_text(ProbeTests.LINKER)
            symbols_in.write_text(symbols)

            removed = lane.generate(
                linker_in, symbols_in, linker_out, symbols_out, pad=4
            )

            self.assertEqual(removed, 3)
            self.assertIn("LONG(0x00000000);", linker_out.read_text())
            filtered = symbols_out.read_text()
            self.assertIn("game = 0x800601D0;", filtered)
            self.assertNotIn("Exec =", filtered)
            self.assertNotIn("middle =", filtered)
            self.assertNotIn("last =", filtered)
            self.assertIn("GsSetLsMatrix = 0x80065100;", filtered)

    def test_rejects_changed_retail_symbol_inventory_before_writing(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            linker_in = root / "input.ld"
            symbols_in = root / "symbols.ld"
            linker_out = root / "output.ld"
            symbols_out = root / "output-symbols.ld"
            linker_in.write_text(ProbeTests.LINKER)
            symbols_in.write_text("Exec = 0x800601D4;\n")

            with self.assertRaisesRegex(lane.LaneError, "expected 2.*found 1"):
                lane.generate(
                    linker_in,
                    symbols_in,
                    linker_out,
                    symbols_out,
                    expected_removed=2,
                )
            self.assertFalse(linker_out.exists())
            self.assertFalse(symbols_out.exists())


class InstructionDecodeTests(unittest.TestCase):
    def test_jump_decode_uses_pc_region(self) -> None:
        # Retail `jal InitHeap` from __SN_ENTRY_POINT.
        self.assertEqual(lane.jump_target(0x0C0180FA, 0x800602F0), 0x800603E8)

    def test_hi_lo_decode_sign_extends_low_half(self) -> None:
        self.assertEqual(lane.hi_lo_target(0x3C028009, 0x2442F204), 0x8008F204)


class ElfReaderTests(unittest.TestCase):
    def test_rejects_non_elf_input(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "not.elf"
            path.write_bytes(b"not an elf")
            with self.assertRaisesRegex(lane.LaneError, "truncated ELF header"):
                lane.Elf32(path)


if __name__ == "__main__":
    unittest.main()

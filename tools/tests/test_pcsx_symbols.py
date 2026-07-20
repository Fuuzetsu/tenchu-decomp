from __future__ import annotations

from pathlib import Path
import tempfile
import unittest

from tools import pcsx_symbols
from tools.tests.test_psxexe import ENTRY, LOAD, make_elf32


class PcsxSymbolsTests(unittest.TestCase):
    def test_collects_elf_symbols_and_emits_insertSymbol_lines(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            elf = Path(temporary) / "fixture.elf"
            elf.write_bytes(make_elf32())
            names = pcsx_symbols.collect(elf, [])
            self.assertEqual(names[ENTRY], "_start")
            self.assertEqual(names[LOAD], "__load_start")
            text = pcsx_symbols.emit(names, "fixture.elf")
            self.assertIn(f'PCSX.insertSymbol({ENTRY:#010x}, "_start")', text)
            self.assertIn("do not edit", text)

    def test_preferred_ranks_real_names_over_auto_labels(self) -> None:
        self.assertEqual(pcsx_symbols.preferred("D_80011000", "PadProc"), "PadProc")
        self.assertEqual(pcsx_symbols.preferred("PadProc", "FUN_80011000"), "PadProc")
        self.assertEqual(pcsx_symbols.preferred("__internal", "_gp"), "_gp")
        self.assertEqual(pcsx_symbols.preferred(None, "D_80011000"), "D_80011000")

    def test_tsv_rows_merge_with_elf_names(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            tsv = Path(temporary) / "functions.tsv"
            tsv.write_text("#addr\tsize\tname\n0x80014400\t328\tInitGraphicsSystem\n")
            names = pcsx_symbols.collect(None, [tsv])
            self.assertEqual(names[0x80014400], "InitGraphicsSystem")


if __name__ == "__main__":
    unittest.main()

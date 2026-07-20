from __future__ import annotations

from pathlib import Path
import unittest

from tools import gen_debug_gdb as g


class ParseNmTests(unittest.TestCase):
    def test_masks_sign_extended_addresses_and_accepts_abs_symbols(self) -> None:
        # nm sign-extends to 64-bit; config-defined function symbols are 'A'.
        text = (
            "ffffffff80040500 A ProcItemKusuri\n"
            "ffffffff8001ada4 A PadProc\n"
            "80016000 T main\n"
            "         U undefined_extern\n"
            "80097000 B someBss\n"
        )
        syms = g.parse_nm(text)
        self.assertEqual(syms["ProcItemKusuri"], 0x80040500)
        self.assertEqual(syms["PadProc"], 0x8001ADA4)
        self.assertEqual(syms["main"], 0x80016000)
        self.assertNotIn("undefined_extern", syms)  # no address column

    def test_first_definition_wins(self) -> None:
        syms = g.parse_nm("80040500 A F\n80050000 T F\n")
        self.assertEqual(syms["F"], 0x80040500)


class BuildScriptTests(unittest.TestCase):
    def test_emits_add_symbol_file_per_resolved_object(self) -> None:
        symbols = {"ProcItemKusuri": 0x80040500, "PadProc": 0x8001ADA4}
        objs = [Path("d/ProcItemKusuri.c.o"), Path("d/PadProc.c.o"),
                Path("d/Unresolved.c.o")]
        script, count = g.build_script(symbols, objs, ".shake/main.exe-dbg")
        self.assertEqual(count, 2)  # Unresolved has no symbol
        self.assertTrue(script.startswith("set architecture mips:3000\n"))
        self.assertIn(
            "add-symbol-file .shake/main.exe-dbg/ProcItemKusuri.c.o "
            "-s .text 0x80040500",
            script,
        )
        self.assertNotIn("Unresolved", script)

    def test_addresses_outside_main_ram_are_skipped(self) -> None:
        symbols = {"Sdk": 0x1F800000, "Game": 0x80020000}
        objs = [Path("d/Sdk.c.o"), Path("d/Game.c.o")]
        script, count = g.build_script(symbols, objs, "d")
        self.assertEqual(count, 1)
        self.assertIn("Game.c.o", script)
        self.assertNotIn("Sdk.c.o", script)


if __name__ == "__main__":
    unittest.main()

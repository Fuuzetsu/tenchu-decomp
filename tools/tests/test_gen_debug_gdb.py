from __future__ import annotations

from pathlib import Path
import tempfile
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


class ObjectInventoryTests(unittest.TestCase):
    def test_source_union_normalises_uppercase_c_and_excludes_stale_objects(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            user = root / "src"
            generated = root / "gen"
            objects = root / "objects"
            user.mkdir()
            generated.mkdir()
            objects.mkdir()
            (user / "WORLD.C").write_text("void CreateStage(void) {}\n")
            (generated / "WORLD.c").write_text("generated\n")
            (generated / "Other.c").write_text("generated\n")
            (objects / "FormerMember.c.o").touch()

            self.assertEqual(
                g.objects_for_source_roots(objects, [user, generated]),
                [objects / "Other.c.o", objects / "WORLD.c.o"],
            )


class BuildScriptTests(unittest.TestCase):
    def test_emits_absolute_paths_and_a_source_directory(self) -> None:
        symbols = {"ProcItemKusuri": 0x80040500, "PadProc": 0x8001ADA4}
        objs = [Path("d/ProcItemKusuri.c.o"), Path("d/PadProc.c.o"),
                Path("d/Unresolved.c.o")]
        script, count = g.build_script(symbols, objs, "d", "/repo")
        self.assertEqual(count, 2)  # Unresolved has no symbol
        self.assertTrue(script.startswith("set architecture mips:3000\n"))
        self.assertIn("directory /repo\n", script)
        # Object paths are absolute so gdb's cwd does not matter.
        abs_dir = str(Path("d").resolve())
        self.assertIn(
            f"add-symbol-file {abs_dir}/ProcItemKusuri.c.o -s .text 0x80040500",
            script,
        )
        self.assertNotIn("Unresolved", script)

    def test_addresses_outside_main_ram_are_skipped(self) -> None:
        symbols = {"Sdk": 0x1F800000, "Game": 0x80020000}
        objs = [Path("d/Sdk.c.o"), Path("d/Game.c.o")]
        script, count = g.build_script(symbols, objs, "d", "/repo")
        self.assertEqual(count, 1)
        self.assertIn("Game.c.o", script)
        self.assertNotIn("Sdk.c.o", script)

    def test_combined_object_uses_its_first_retail_function_as_base(self) -> None:
        symbols = {"CreateStage": 0x8003A3A0}
        objs = [Path("d/WORLD.c.o")]
        script, count = g.build_script(symbols, objs, "d", "/repo")
        self.assertEqual(count, 1)
        self.assertIn("WORLD.c.o -s .text 0x8003a3a0", script)


if __name__ == "__main__":
    unittest.main()

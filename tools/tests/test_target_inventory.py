import json
from pathlib import Path
import tempfile
import unittest

from tools.target_inventory import load_target_units


class TargetInventoryTests(unittest.TestCase):
    def test_combined_menu_source_uses_its_own_manifest(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            config = root / "config"
            source = root / "src/menu.exe"
            config.mkdir()
            source.mkdir(parents=True)
            (config / "functions.menu.exe.tsv").write_text(
                "#addr\tsize\tname\n"
                "0x80011000\t8\tFirst\n"
                "0x80011008\t8\tSecond\n"
                "0x80011010\t8\tUncarved\n"
            )
            (config / "symbols.menu.exe.txt").write_text("")
            (config / "splat.menu.exe.yaml").write_text(
                "      - [0x800, c, MODULE]\n"
                "      - [0x810, textbin]\n"
            )
            (config / "translation-units.menu.exe.json").write_text(
                json.dumps({"schema": 1, "units": [{
                    "source": "MODULE.C", "functions": ["First", "Second"]
                }]})
            )
            (source / "MODULE.C").write_text('#include "implementation.inc"\n')
            (source / "implementation.inc").write_text(
                'void First(void) {}\n#ifndef NON_MATCHING\n'
                'INCLUDE_ASM("x", Second);\n#else\n'
                'void Second(void) {}\n#endif\n'
            )
            (source / "Uncarved.c").write_text("void Uncarved(void) {}\n")
            functions, matched, sources = load_target_units("menu.exe", root)
            self.assertEqual(len(functions), 3)
            self.assertEqual(matched, {0x80011000})
            self.assertEqual(sources[0x80011000], str(source / "MODULE.C"))

    def test_live_menu_modules_have_contiguous_members(self):
        from tools import source_units as su

        functions, _, _ = load_target_units("menu.exe", su.ROOT)
        by_name = {n: (a, s) for a, s, n in functions}
        units = su.load_units(su.ROOT / "config/translation-units.menu.exe.json")
        for unit in units:
            self.assertTrue((su.ROOT / "src/menu.exe" / unit.source).is_file())
            for left, right in zip(unit.functions, unit.functions[1:]):
                self.assertEqual(
                    sum(by_name[left]), by_name[right][0],
                    f"{unit.source}: gap between {left} and {right}",
                )


if __name__ == "__main__":
    unittest.main()

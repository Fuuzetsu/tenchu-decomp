import json
from pathlib import Path
import tempfile
import unittest

from tools.target_inventory import SDK_STARTS, code_buckets, load_target_units, textbin_bytes


class TargetInventoryTests(unittest.TestCase):
    def test_sdk_boundary_preserves_unmatched_functions_in_each_bucket(self):
        boundary = SDK_STARTS["menu.exe"]
        functions = [
            (boundary - 16, 8, "Complete"),
            (boundary - 8, 8, "Draft"),
            (boundary, 16, "Sdk"),
        ]
        buckets = code_buckets("menu.exe", functions, {boundary - 16})
        self.assertEqual(buckets["game"], {
            "fns": 2, "fns_matched": 1, "bytes": 16, "bytes_matched": 8,
        })
        self.assertEqual(buckets["sdk"], {
            "fns": 1, "fns_matched": 0, "bytes": 16, "bytes_matched": 0,
        })
        self.assertEqual(code_buckets("trial.exe", functions, set()), {})

    def test_inventory_cannot_straddle_reviewed_sdk_boundary(self):
        boundary = SDK_STARTS["menu.exe"]
        with self.assertRaisesRegex(ValueError, "crosses the game/SDK boundary"):
            code_buckets("menu.exe", [(boundary - 4, 8, "Unsplit")], set())

    def test_compiler_jump_tables_end_raw_textbin_spans(self):
        config = (
            "      - [0x800, textbin]\n"
            "      - { start: 0x900, type: .rodata, name: Switch, "
            "linker_section_order: .text }\n"
            "      - [0x920, textbin]\n"
            "      - [0x980, c, Switch]\n"
            "      - [0xa00]\n"
        )
        self.assertEqual(textbin_bytes(config), 0x160)

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

    def test_live_sibling_modules_have_contiguous_members(self):
        from tools import source_units as su

        for target in ("menu.exe", "ending.exe"):
            with self.subTest(target=target):
                functions, _, _ = load_target_units(target, su.ROOT)
                by_name = {n: (a, s) for a, s, n in functions}
                units = su.load_units(su.ROOT / f"config/translation-units.{target}.json")
                for unit in units:
                    self.assertTrue((su.ROOT / "src" / target / unit.source).is_file())
                    for left, right in zip(unit.functions, unit.functions[1:]):
                        self.assertEqual(
                            sum(by_name[left]), by_name[right][0],
                            f"{unit.source}: gap between {left} and {right}",
                        )

    def test_ending_game_span_stops_before_bios_and_crt(self):
        from tools import source_units as su

        functions, matched, _ = load_target_units("ending.exe", su.ROOT)
        boundary = SDK_STARTS["ending.exe"]
        adt = next((a, s) for a, s, n in functions if n == "AdtSelect")
        self.assertEqual(sum(adt), boundary)
        buckets = code_buckets("ending.exe", functions, matched)
        self.assertEqual(buckets["game"]["bytes"], boundary - 0x800138C0)
        self.assertEqual(buckets["sdk"]["fns_matched"], 0)


if __name__ == "__main__":
    unittest.main()

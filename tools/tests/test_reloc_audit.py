"""Regression tests for the read-only MAIN.EXE relocation audit.

Run: nix develop -c python -m unittest tools.tests.test_reloc_audit
"""

from __future__ import annotations

import json
from pathlib import Path
import tempfile
import unittest

from tools import reloc_audit as audit


class SymbolTests(unittest.TestCase):
    def test_nm_parser_masks_sign_extension_and_deduplicates(self) -> None:
        symbols = audit.parse_nm(
            """\
ffffffff800162a4 A main
00000001 a _MACRO_INC_GUARD
00000001 a _MACRO_INC_GUARD
80097f00 A MutableData
800cdbac A HEAP_START
800162a4 T ignored_text_symbol
"""
        )
        self.assertEqual([symbol.name for symbol in symbols], [
            "_MACRO_INC_GUARD", "main", "MutableData", "HEAP_START"
        ])
        by_name = {symbol.name: symbol for symbol in symbols}
        self.assertEqual(by_name["main"].value, 0x800162A4)
        self.assertEqual(by_name["main"].region, "game_code")
        self.assertTrue(by_name["MutableData"].movable)
        self.assertFalse(by_name["HEAP_START"].movable)


class RawInputTests(unittest.TestCase):
    def test_linker_inventory_ignores_unlinked_stale_files(self) -> None:
        linker = """\
foo/data/4FA48.data.s.o(.data);
foo/data/800.data.s.o(.data);
foo/data/4FA48.data.s.o(.data);
foo/data/not-a-generated-name.s.o(.data);
"""
        self.assertEqual(
            audit.linked_raw_inputs(linker),
            ["800.data.s", "4FA48.data.s"],
        )

    def test_word_parser_retains_source_owner_and_symbolic_words(self) -> None:
        records = audit.parse_word_source(
            """\
dlabel Start
    /* 800 80011000 CC1F0380 */ .word FunctionName
    /* 804 80011004 441A0180 */ .word 0x80011A44
""",
            "800.data.s",
        )
        self.assertEqual(len(records), 2)
        self.assertEqual(records[0].owner, "Start")
        self.assertIsNone(records[0].value)
        self.assertEqual(records[1].value, 0x80011A44)
        self.assertEqual(records[1].source_region, "leading_data")

    def test_analysis_finds_jump_hi_lo_and_only_aligned_data_pointer(self) -> None:
        text = """\
dlabel Lead
    /* 800 80011000 441A0180 */ .word 0x80011A44
dlabel Entry
    /* 4FA68 80060268 0980023C */ .word 0x3C028009
    /* 4FA6C 8006026C B07E4224 */ .word 0x24427EB0
    /* 4FB04 80060304 A958000C */ .word 0x0C0058A9
dlabel Tail
    /* 79640 80089E40 441A0180 */ .word 0x80011A44
    /* 79644 80089E44 B22E0C80 */ .word 0x800C2EB2
    /* 79648 80089E48 00800E80 */ .word 0x800E8000
"""
        result = audit.analyse_words(audit.parse_word_source(text, "fixture.data.s"))
        self.assertEqual(len(result["literal_jumps"]), 1)
        self.assertEqual(result["literal_jumps"][0]["kind"], "jal")
        self.assertEqual(result["literal_jumps"][0]["target"], "0x800162a4")
        self.assertEqual(len(result["conservative_hi_lo"]), 1)
        self.assertEqual(
            result["conservative_hi_lo"][0]["target"], "0x80097eb0"
        )
        self.assertEqual(
            [item["source"] for item in result["literal_pointers"]],
            ["0x80011000", "0x80089e40"],
        )


class ReportTests(unittest.TestCase):
    def fixture_report(self) -> dict[str, object]:
        symbols = audit.parse_nm("800162a4 A main\n800cdbac A HEAP_START\n")
        words = audit.parse_word_source(
            """\
dlabel Entry
    /* 4FB04 80060304 A958000C */ .word 0x0C0058A9
dlabel Tail
    /* 79640 80089E40 441A0180 */ .word 0x80011A44
""",
            "fixture.data.s",
        )
        return audit.build_report(symbols, ["fixture.data.s"], words)

    def test_json_schema_has_full_machine_readable_findings(self) -> None:
        report = self.fixture_report()
        encoded = json.loads(json.dumps(report))
        self.assertEqual(encoded["schema"], 1)
        self.assertEqual(encoded["summary"]["movable_absolute_symbols"], 1)
        self.assertEqual(encoded["summary"]["literal_jumps"], 1)
        self.assertEqual(encoded["summary"]["literal_pointers"], 1)
        self.assertEqual(encoded["absolute_symbols"][0]["value"], "0x800162a4")

    def test_text_output_is_stable_and_calls_out_audit_only_mode(self) -> None:
        rendered = audit.render_text(self.fixture_report())
        self.assertIn("reloc-audit main.exe\n", rendered)
        self.assertIn("final ABS symbols: 2 total; 1 in movable MAIN", rendered)
        self.assertIn("literal J/JAL candidates: 1 (jal=1)", rendered)
        self.assertIn("findings: 3 (audit-only", rendered)

    def test_help_describes_allowlist_and_controlled_shift_direction(self) -> None:
        help_text = audit.parser().format_help()
        self.assertIn("allowlist", help_text)
        self.assertIn("+0x10004", help_text)
        self.assertIn("--fail-on-findings", help_text)

    def test_missing_artifacts_fail_without_writing(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            with self.assertRaisesRegex(audit.AuditError, "run `./Build`"):
                audit.collect(
                    root,
                    audit.DEFAULT_ELF,
                    audit.DEFAULT_LINKER,
                    audit.DEFAULT_RAW_DIR,
                    audit.DEFAULT_NM,
                )


if __name__ == "__main__":
    unittest.main()

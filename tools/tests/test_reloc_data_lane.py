"""Tests for the controlled loaded-data relocation proof lane."""

from __future__ import annotations

import json
from pathlib import Path
import shutil
import tempfile
import unittest

from tools import reloc_data_lane as lane


MIPS_PREFIX = "mipsel-unknown-linux-gnu-"
HAVE_MIPS_TOOLS = all(
    shutil.which(MIPS_PREFIX + tool)
    for tool in ("as", "ld", "objcopy", "readelf")
)


TARGET_BASE = 0x8001FFF8
SOURCE_BASE = 0x80030000

TARGET_ASM = """\
.include "macro.inc"
.section .data, "wa"

dlabel ExactStrings
    /* 0000 8001FFF8 41414100 */ .word 0x00414141
    /* 0004 8001FFFC 42424200 */ .word 0x00424242
enddlabel ExactStrings
"""

SOURCE_ASM = """\
.include "macro.inc"
.section .data, "wa"

dlabel PointerTable
    /* 1000 80030000 FCFF0180 */ .word 0x8001FFFC
    /* 1004 80030004 F8FF0180 */ .word 0x8001FFF8
    /* 1008 80030008 FAFF0180 */ .word 0x8001FFFA
enddlabel PointerTable
"""


class ParseTests(unittest.TestCase):
    def test_parses_only_r_mips_32_records(self) -> None:
        output = """\
00000004  00000302 R_MIPS_32              00000000   exact_target
00000008  00000405 R_MIPS_HI16            00000000   ignored_target
"""
        self.assertEqual(
            lane.parse_relocations(output),
            {(4, "exact_target"): 1},
        )

    def test_symbol_parser_retains_section_index(self) -> None:
        output = "  7: 00000004     0 OBJECT  GLOBAL DEFAULT    2 exact_target\n"
        self.assertEqual(lane.parse_symbols(output), {"exact_target": (4, "2")})


@unittest.skipUnless(HAVE_MIPS_TOOLS, "MIPS binutils are not on PATH")
class BinutilsProofTests(unittest.TestCase):
    def test_exact_and_base_addend_targets_follow_all_controlled_links(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            inputs = root / "input"
            inputs.mkdir()
            (inputs / "target.data.s").write_text(TARGET_ASM)
            (inputs / "source.data.s").write_text(SOURCE_ASM)
            manifest = root / "manifest.json"
            manifest.write_text(
                json.dumps(
                    {
                        "schema": 1,
                        "description": "lane fixture",
                        "entries": [
                            {
                                "source_file": "source.data.s",
                                "source_address": "0x80030000",
                                "source_owner": "PointerTable",
                                "target_file": "target.data.s",
                                "target_address": "0x8001fffc",
                                "target_owner": "ExactStrings",
                                "symbol": "exact_string_b",
                            },
                            {
                                "source_file": "source.data.s",
                                "source_address": "0x80030004",
                                "source_owner": "PointerTable",
                                "target_file": "target.data.s",
                                "target_address": "0x8001fff8",
                                "target_owner": "ExactStrings",
                                "symbol": "exact_string_a",
                            },
                            {
                                "source_file": "source.data.s",
                                "source_address": "0x80030008",
                                "source_owner": "PointerTable",
                                "target_file": None,
                                "target_address": "0x8001fffa",
                                "target_owner": "ExternalBuffer",
                                "symbol": "ExternalBuffer",
                                "target_addend": "0x2",
                            },
                        ],
                    }
                )
            )

            result = lane.verify(manifest, inputs, root / "work")

            self.assertEqual(result, lane.ProofResult(3, 3, 2, 3))


if __name__ == "__main__":
    unittest.main()

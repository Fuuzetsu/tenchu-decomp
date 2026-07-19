"""Tests for the retail-exact linker-owned BSS layout proof.

Run: nix develop -c python3 -m unittest tools.tests.test_reloc_bss_lane
"""

from __future__ import annotations

import subprocess
import tempfile
from pathlib import Path
import unittest
from unittest import mock

from tools import reloc_bss_lane as lane


class SymbolScriptTests(unittest.TestCase):
    def test_filters_loaded_image_bss_and_owned_boundaries(self) -> None:
        source = """\
BeforeImage = 0x80010ffc;
LeadingData = 0x80012000;
GameText = 0x80020000;
TrailingData = 0x80090000;
_gp = 0x80097698;
BeforeBss = 0x80097eac;
OTablePt = 0x80097eb0;
LastBssWord = 0x800cdba4;
AtBssEnd = 0x800cdba8;
D_800CDBA8 = 0x800cdba8;
HEAP_START = 0x800cdbac;
MemoryPool = 0x800dc000;
Handoff = 0x80100000;
"""
        output, removed = lane.filter_symbol_script(source)
        self.assertEqual(
            output,
            "BeforeImage = 0x80010ffc;\n"
            "AtBssEnd = 0x800cdba8;\n"
            "Handoff = 0x80100000;\n",
        )
        self.assertEqual(
            removed,
            {
                "LeadingData": 0x80012000,
                "GameText": 0x80020000,
                "TrailingData": 0x80090000,
                "_gp": lane.GP_ADDRESS,
                "BeforeBss": lane.BSS_START - 4,
                "OTablePt": lane.BSS_START,
                "LastBssWord": lane.BSS_END - 4,
                "D_800CDBA8": lane.BSS_END,
                "HEAP_START": lane.HEAP_START,
                "MemoryPool": lane.MEMORY_POOL_START,
            },
        )

    def test_rejects_changed_boundary_address(self) -> None:
        with self.assertRaisesRegex(lane.LaneError, "_gp moved"):
            lane.filter_symbol_script("_gp = 0x8009769c;\n")


class TailTransformTests(unittest.TestCase):
    SOURCE = """\
.include "macro.inc"
.section .data, "wa"
dlabel Initialized
    .word 1
enddlabel Initialized
nonmatching OTablePt
dlabel OTablePt
    .word 0
enddlabel OTablePt
nonmatching World
dlabel World
    .word 0
enddlabel World
"""

    def test_moves_zero_tail_to_nobits_without_rewriting_data(self) -> None:
        output, labels = lane.transform_tail_source(self.SOURCE)
        self.assertIn(
            '.word 1\nenddlabel Initialized\n.section .bss, "aw", @nobits\n\n'
            "nonmatching OTablePt",
            output,
        )
        self.assertEqual(labels, {"OTablePt", "World"})

    def test_requires_one_known_split_marker(self) -> None:
        with self.assertRaisesRegex(lane.LaneError, "expected one"):
            lane.transform_tail_source(self.SOURCE.replace("nonmatching OTablePt", ""))


class LinkerRewriteTests(unittest.TestCase):
    LINKER = """\
SECTIONS
{
    _gp = 0x80097698;
    .main_exe 0x80011000 :
    {
        old/72CD0.data.s.o(.data);
        main_exe_RODATA_END = .;
        main_exe_BSS_START = .;
        object/One.c.o(.bss);
        . = ALIGN(., 4);
        main_exe_BSS_END = .;
        main_exe_BSS_SIZE = ABSOLUTE(main_exe_BSS_END - main_exe_BSS_START);
    }
    __romPos += SIZEOF(.main_exe);
    __romPos = ALIGN(__romPos, 4);
    . = ALIGN(., 4);
    main_exe_ROM_END = __romPos;
    main_exe_VRAM_END = .;
    /DISCARD/ : { *(*); }
}
"""

    def test_builds_following_noload_bss_and_fixed_pool_reservation(self) -> None:
        output = lane.rewrite_linker(
            self.LINKER,
            old_tail_object="old/72CD0.data.s.o",
            new_tail_object="new/72CD0.reloc.s.o",
            extension_object_glob="build/reloc/*.c.o",
            aliases=[lane.Symbol("OTable", 0x80098018)],
        )
        self.assertNotIn("_gp = 0x80097698", output)
        self.assertIn("__load_start = .;", output)
        self.assertIn("PutMapMode = D_80097BA0 + 0x11;", output)
        self.assertIn("new/72CD0.reloc.s.o(.data);", output)
        self.assertIn(".main_exe_bss (NOLOAD)", output)
        self.assertIn("new/72CD0.reloc.s.o(.bss);", output)
        self.assertIn("object/One.c.o(.bss);", output)
        self.assertIn(". += 0x18;\n        OTable = .;", output)
        self.assertIn("HEAP_START = . + 4;", output)
        self.assertIn("D_800CDBA8 = .;", output)
        self.assertIn(".main_exe_memory_pool 0x800dc000 (NOLOAD)", output)
        self.assertIn("MemoryPool = .;\n        . += 0x120000;", output)
        self.assertIn(".main_exe_extension : AT(__romPos)", output)
        self.assertIn("build/reloc/*.c.o(.text .text.*", output)
        self.assertIn("build/reloc/*.c.o(.sbss .sbss.* .scommon);", output)
        self.assertIn("build/reloc/*.c.o(.bss .bss.* COMMON);", output)

    def test_growth_uses_relative_layout_and_collision_asserts(self) -> None:
        output = lane.rewrite_linker(
            self.LINKER,
            old_tail_object="old/72CD0.data.s.o",
            new_tail_object="new/72CD0.reloc.s.o",
            extension_object_glob="build/reloc/*.c.o",
            aliases=[],
        )
        self.assertNotIn("retail BSS start changed", output)
        self.assertNotIn("retail _gp changed", output)
        self.assertIn(
            'ASSERT(main_exe_BSS_START == ALIGN(main_exe_INITIALIZED_END, 16), '
            '"BSS does not follow aligned initialized data")',
            output,
        )
        self.assertIn(
            'ASSERT(main_exe_BSS_END <= MemoryPool, '
            '"BSS overlaps the fixed virtual-memory pool")',
            output,
        )

    def test_substitutes_reviewed_data_objects_exactly_once(self) -> None:
        source = self.LINKER.replace(
            "old/72CD0.data.s.o(.data);",
            "old/207C.data.s.o(.data);\n        old/72CD0.data.s.o(.data);",
        )
        output = lane.rewrite_linker(
            source,
            old_tail_object="old/72CD0.data.s.o",
            new_tail_object="new/72CD0.reloc.s.o",
            extension_object_glob="build/reloc/*.c.o",
            aliases=[],
            object_replacements=[("old/207C.data.s.o", "new/207C.reloc.s.o")],
        )
        self.assertNotIn("old/207C.data.s.o", output)
        self.assertIn("new/207C.reloc.s.o(.data);", output)

        with self.assertRaisesRegex(lane.LaneError, "expected one linker reference"):
            lane.rewrite_linker(
                self.LINKER,
                old_tail_object="old/72CD0.data.s.o",
                new_tail_object="new/72CD0.reloc.s.o",
                extension_object_glob="build/reloc/*.c.o",
                aliases=[],
                object_replacements=[("missing.data.s.o", "new.data.s.o")],
            )

    def test_pool_and_transient_handoff_intentionally_overlap(self) -> None:
        self.assertLess(lane.MEMORY_POOL_START, lane.HANDOFF_START)
        self.assertLess(lane.HANDOFF_END, lane.MEMORY_POOL_END)


class ValidationTests(unittest.TestCase):
    def test_reference_is_logical_prefix_plus_zero_sector_padding(self) -> None:
        logical = bytes(lane.LOGICAL_FILE_SIZE)
        reference = logical + bytes(lane.REFERENCE_PADDING_SIZE)
        lane.validate_reference(logical, reference)

        bad = bytearray(reference)
        bad[-1] = 1
        with self.assertRaisesRegex(lane.LaneError, "overlap is not all zero"):
            lane.validate_reference(logical, bytes(bad))

    def test_nm_parser_masks_mips_sign_extension(self) -> None:
        parsed = lane.parse_nm_posix("HEAP_START B ffffffff800cdbac \n")
        self.assertEqual(parsed["HEAP_START"], (lane.HEAP_START, "B"))

    @staticmethod
    def nm_output(heap_type: str) -> str:
        return f"""\
OTablePt B ffffffff80097eb0 8
__bss_start B ffffffff80097eb0
__bss_end B ffffffff800cdba8
D_800CDBA8 B ffffffff800cdba8
HEAP_START {heap_type} ffffffff800cdbac
MemoryPool B ffffffff800dc000
_gp T ffffffff80097698 8
__load_start T ffffffff80011000
main T ffffffff800162a4
Exec T ffffffff800601d4
GsInitCoord2param T ffffffff800650d4
"""

    READELF_OUTPUT = """\
  [ 3] .main_exe_bss     NOBITS          80097eb0 097eb0 035cf8 00  WA  0   0 16
  [ 4] .main_exe_memory_pool NOBITS       800dc000 097eb0 120000 00  WA  0   0  1
"""

    @mock.patch("tools.reloc_bss_lane.subprocess.run")
    def test_elf_gate_requires_heap_start_to_be_section_relative(
        self, run: mock.Mock
    ) -> None:
        run.return_value = subprocess.CompletedProcess(
            [], 0, stdout=self.nm_output("A")
        )
        with self.assertRaisesRegex(lane.LaneError, "HEAP_START is .* expected"):
            lane.validate_elf(
                elf=Path("main.elf"),
                nm="nm",
                readelf="readelf",
                expected_bss_symbols={"OTablePt": lane.BSS_START},
            )

    @mock.patch("tools.reloc_bss_lane.subprocess.run")
    def test_elf_gate_requires_real_nobits_sections(self, run: mock.Mock) -> None:
        run.side_effect = [
            subprocess.CompletedProcess([], 0, stdout=self.nm_output("B")),
            subprocess.CompletedProcess([], 0, stdout=self.READELF_OUTPUT),
        ]
        lane.validate_elf(
            elf=Path("main.elf"),
            nm="nm",
            readelf="readelf",
            expected_bss_symbols={"OTablePt": lane.BSS_START},
        )

    @mock.patch("tools.reloc_bss_lane.subprocess.run")
    def test_elf_gate_rejects_absolute_initialized_symbol(
        self, run: mock.Mock
    ) -> None:
        run.return_value = subprocess.CompletedProcess(
            [], 0, stdout=self.nm_output("B") + "StageChar A ffffffff8008e6dc\n"
        )
        with self.assertRaisesRegex(lane.LaneError, "section-owned"):
            lane.validate_elf(
                elf=Path("main.elf"),
                nm="nm",
                readelf="readelf",
                expected_bss_symbols={"OTablePt": lane.BSS_START},
                expected_initialized_symbols={"StageChar": 0x8008E6DC},
            )

    @mock.patch("tools.reloc_bss_lane.subprocess.run")
    def test_elf_gate_allows_obsolete_unreferenced_initialized_alias(
        self, run: mock.Mock
    ) -> None:
        run.side_effect = [
            subprocess.CompletedProcess([], 0, stdout=self.nm_output("B")),
            subprocess.CompletedProcess([], 0, stdout=self.READELF_OUTPUT),
        ]
        lane.validate_elf(
            elf=Path("main.elf"),
            nm="nm",
            readelf="readelf",
            expected_bss_symbols={"OTablePt": lane.BSS_START},
            expected_initialized_symbols={
                "switchD_8001cd44__switchdataD_80011240": 0x80011240
            },
        )


if __name__ == "__main__":
    unittest.main()

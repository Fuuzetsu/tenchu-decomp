"""Tests for the retail-exact linker-owned BSS layout proof.

Run: nix develop -c python3 -m unittest tools.tests.test_reloc_bss_lane
"""

from __future__ import annotations

import re
import shutil
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
            ordinary_c_object_glob="build/main.exe/*.c.o",
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
        self.assertIn("MemoryPoolEnd = .;", output)
        self.assertIn(
            "MemoryPoolCapacity = ABSOLUTE("
            "(MemoryPoolEnd - MemoryPool) / 4 - 2);",
            output,
        )
        self.assertIn(".main_exe_extension : AT(__romPos)", output)
        self.assertIn("build/reloc/*.c.o(.text .text.*", output)
        self.assertIn("build/main.exe/*.c.o(.sdata .sdata.*);", output)
        self.assertIn("build/reloc/*.c.o(.sbss .sbss.* .scommon);", output)
        self.assertIn(
            "build/main.exe/*.c.o(.sbss .sbss.* .scommon);", output
        )
        self.assertIn("build/reloc/*.c.o(.bss .bss.* COMMON);", output)
        self.assertIn("build/main.exe/*.c.o(.bss .bss.* COMMON);", output)

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
            '"BSS overlaps the virtual-memory pool")',
            output,
        )

    def test_dynamic_pool_follows_aligned_bss_and_keeps_fixed_upper_bound(self) -> None:
        output = lane.rewrite_linker(
            self.LINKER,
            old_tail_object="old/72CD0.data.s.o",
            new_tail_object="new/72CD0.reloc.s.o",
            extension_object_glob="build/reloc/*.c.o",
            aliases=[],
            dynamic_pool=True,
        )
        self.assertNotIn(".main_exe_memory_pool 0x800dc000", output)
        self.assertIn(
            ".main_exe_memory_pool MAX(0x800dc000, "
            "ALIGN(ABSOLUTE(main_exe_BSS_END), 16)) (NOLOAD)",
            output,
        )
        self.assertIn(
            "__tenchu_memory_pool_size = ABSOLUTE(0x801fc000 "
            "- MAX(0x800dc000, "
            "ALIGN(ABSOLUTE(main_exe_BSS_END), 16)));",
            output,
        )
        self.assertIn(". += __tenchu_memory_pool_size;", output)
        self.assertIn(
            'ASSERT(MemoryPool == MAX(0x800dc000, '
            'ALIGN(ABSOLUTE(main_exe_BSS_END), 16)), '
            '"dynamic MemoryPool does not follow its minimum/aligned BSS")',
            output,
        )
        self.assertIn(
            'ASSERT(MemoryPoolEnd - MemoryPool >= 0x10, '
            '"virtual-memory pool is too small")',
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


class OrdinaryGlobalIntegrationTests(unittest.TestCase):
    TOOLS = (
        "cc1-281",
        "maspsx",
        "mipsel-unknown-linux-gnu-as",
        "mipsel-unknown-linux-gnu-ld",
        "mipsel-unknown-linux-gnu-nm",
        "mipsel-unknown-linux-gnu-readelf",
    )

    CC_FLAGS = (
        "-mcpu=3000",
        "-quiet",
        "-fno-builtin",
        "-G8",
        "-w",
        "-O2",
        "-funsigned-char",
        "-fpeephole",
        "-ffunction-cse",
        "-fpcc-struct-return",
        "-fcommon",
        "-fverbose-asm",
        "-fgnu-linker",
        "-mgas",
        "-msoft-float",
    )

    AS_FLAGS = (
        "-EL",
        "-march=r3000",
        "-mtune=r3000",
        "-no-pad-sections",
        "-O1",
        "-G0",
    )

    SOURCE = """\
int ordinary_small_common;
char ordinary_large_common[16];
static int ordinary_static_small;
static char ordinary_static_large[16];

int ordinary_small_init = 7;
char ordinary_large_init[16] = { 9 };
static const int ordinary_small_const = 2;
static const char ordinary_short_const[] = "hello";
static const char ordinary_long_const[] = "this string is definitely long";
static const float ordinary_small_float = 1.25f;
static const double ordinary_small_double = 2.5;

int ordinary_probe(void)
{
    return ordinary_small_common + ordinary_large_common[0]
        + ordinary_static_small + ordinary_static_large[0]
        + ordinary_small_init + ordinary_large_init[0]
        + ordinary_small_const + ordinary_short_const[0]
        + ordinary_long_const[0];
}
"""

    @classmethod
    def setUpClass(cls) -> None:
        missing = [tool for tool in cls.TOOLS if shutil.which(tool) is None]
        if missing:
            raise unittest.SkipTest("missing pinned MIPS tools: " + ", ".join(missing))

    @classmethod
    def compile_c(cls, output: Path) -> tuple[str, str]:
        cc = subprocess.run(
            ["cc1-281", *cls.CC_FLAGS],
            input=cls.SOURCE,
            text=True,
            check=True,
            stdout=subprocess.PIPE,
        ).stdout
        assembly = subprocess.run(
            ["maspsx", "--aspsx-version=2.77", "-G8"],
            input=cc,
            text=True,
            check=True,
            stdout=subprocess.PIPE,
        ).stdout
        subprocess.run(
            [
                "mipsel-unknown-linux-gnu-as",
                *cls.AS_FLAGS,
                "-o",
                str(output),
            ],
            input=assembly,
            text=True,
            check=True,
        )
        return cc, assembly

    @classmethod
    def assemble(cls, source: str, output: Path) -> None:
        subprocess.run(
            [
                "mipsel-unknown-linux-gnu-as",
                *cls.AS_FLAGS,
                "-o",
                str(output),
            ],
            input=source,
            text=True,
            check=True,
        )

    def test_pinned_c_globals_are_retained_in_owned_gp_near_outputs(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            ordinary = root / "ordinary"
            ordinary.mkdir()
            probe = ordinary / "probe.c.o"
            extra = ordinary / "common.s.o"
            tail = root / "tail.reloc.s.o"
            elf = root / "probe.elf"
            linker = root / "probe.ld"

            cc_assembly, maspsx_assembly = self.compile_c(probe)
            self.assertRegex(cc_assembly, r"(?m)^\s*\.comm\s+ordinary_small_common,4$")
            self.assertRegex(cc_assembly, r"(?m)^\s*\.comm\s+ordinary_large_common,16$")
            self.assertRegex(cc_assembly, r"(?m)^\s*\.lcomm\s+ordinary_static_small,4$")
            self.assertRegex(cc_assembly, r"(?m)^\s*\.lcomm\s+ordinary_static_large,16$")
            self.assertIn(".section .sdata", maspsx_assembly)
            self.assertIn(".section .sbss", maspsx_assembly)
            self.assertIn(".section .bss", maspsx_assembly)
            self.assertIn(".section .rodata", maspsx_assembly)
            self.assertNotIn(".rdata", maspsx_assembly)
            self.assertNotRegex(maspsx_assembly, r"(?m)^\s*\.lit[48]\b")

            sections = subprocess.run(
                ["mipsel-unknown-linux-gnu-readelf", "-SW", str(probe)],
                text=True,
                check=True,
                stdout=subprocess.PIPE,
            ).stdout
            emitted = set(re.findall(r"^\s*\[\s*\d+\]\s+(\S+)", sections, re.MULTILINE))
            runtime_sections = {
                name
                for name in emitted
                if name
                in {".text", ".data", ".bss", ".sdata", ".rodata", ".sbss"}
            }
            self.assertEqual(
                runtime_sections,
                {".text", ".data", ".bss", ".sdata", ".rodata", ".sbss"},
            )
            self.assertFalse({".rdata", ".lit4", ".lit8"} & emitted)

            self.assemble(
                """\
.section .scommon,"aw",@nobits
.globl explicit_scommon
.align 2
explicit_scommon:
.space 4
.comm explicit_gnu_common,16,4
""",
                extra,
            )
            self.assemble(
                f"""\
.section .data,"wa"
.space 0x{lane.GP_ADDRESS - lane.MAIN_LOAD_ADDRESS:x}
.globl _gp
_gp:
.word 0
.space 0x{0x80097BA0 - lane.GP_ADDRESS - 4:x}
.globl D_80097BA0
D_80097BA0:
.space 0x{lane.INITIALIZED_END - 0x80097BA0:x}
.section .bss,"aw",@nobits
.space 0x{lane.BSS_PAD_END - lane.BSS_START:x}
""",
                tail,
            )

            ordinary_glob = str(ordinary / "*.o")
            old_tail = str(root / "tail.original.s.o")
            base_linker = f"""\
SECTIONS
{{
    __romPos = 0;
    _gp = 0x80097698;
    .main_exe 0x80011000 : AT(__romPos) SUBALIGN(4)
    {{
        {old_tail}(.data);
        {ordinary_glob}(.text .rodata .data);
        main_exe_RODATA_END = .;
        main_exe_BSS_START = .;
        {ordinary_glob}(.bss);
        . = ALIGN(., 4);
        main_exe_BSS_END = .;
        main_exe_BSS_SIZE = ABSOLUTE(main_exe_BSS_END - main_exe_BSS_START);
    }}
    __romPos += SIZEOF(.main_exe);
    __romPos = ALIGN(__romPos, 4);
    . = ALIGN(., 4);
    main_exe_ROM_END = __romPos;
    main_exe_VRAM_END = .;
    /DISCARD/ : {{ *(*); }}
}}
"""
            linker.write_text(
                lane.rewrite_linker(
                    base_linker,
                    old_tail_object=old_tail,
                    new_tail_object=str(tail),
                    extension_object_glob=str(root / "reloc" / "*.c.o"),
                    ordinary_c_object_glob=ordinary_glob,
                    aliases=[],
                )
            )
            subprocess.run(
                [
                    "mipsel-unknown-linux-gnu-ld",
                    "-EL",
                    "-o",
                    str(elf),
                    "-T",
                    str(linker),
                    "--no-check-sections",
                    "-nostdlib",
                    str(probe),
                    str(extra),
                ],
                check=True,
            )

            nm = subprocess.run(
                ["mipsel-unknown-linux-gnu-nm", "-P", "-n", str(elf)],
                text=True,
                check=True,
                stdout=subprocess.PIPE,
            ).stdout
            symbols = lane.parse_nm_posix(nm)
            retained = {
                "ordinary_probe",
                "ordinary_small_common",
                "ordinary_large_common",
                "ordinary_static_small",
                "ordinary_static_large",
                "ordinary_small_init",
                "ordinary_large_init",
                "ordinary_small_const",
                "ordinary_short_const",
                "ordinary_long_const",
                "ordinary_small_float",
                "ordinary_small_double",
                "explicit_scommon",
                "explicit_gnu_common",
            }
            self.assertEqual(retained - symbols.keys(), set())
            gp = symbols["_gp"][0]
            for name in {
                "ordinary_small_common",
                "ordinary_static_small",
                "ordinary_small_init",
                "ordinary_small_const",
                "ordinary_short_const",
                "ordinary_small_float",
                "ordinary_small_double",
                "explicit_scommon",
            }:
                displacement = symbols[name][0] - gp
                self.assertGreaterEqual(displacement, -0x8000, name)
                self.assertLessEqual(displacement, 0x7FFF, name)
            for name in {
                "ordinary_large_common",
                "ordinary_static_large",
                "explicit_gnu_common",
            }:
                self.assertIn(symbols[name][1], {"B", "b"}, name)


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
MemoryPoolEnd B ffffffff801fc000
MemoryPoolCapacity A 00047ffe
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

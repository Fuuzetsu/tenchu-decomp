"""Tests for exact symbolic pointers in generated loaded data."""

from __future__ import annotations

import contextlib
import io
import json
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import unittest

from tools import reloc_data


ROOT = Path(__file__).resolve().parents[2]
BASE = 0x80011000
TARGET = BASE + 4
SOURCE = 0x80012000
SYMBOL = "reviewed_interior_string"


TARGET_ASM = """\
.include "macro.inc"
.section .data, "wa"

dlabel StringPool
    /* 0000 80011000 EFBEADDE */ .word 0xDEADBEEF
    /* 0004 80011004 44332211 */ .word 0x11223344
enddlabel StringPool
"""

SOURCE_ASM = """\
.include "macro.inc"
.section .data, "wa"

dlabel PointerTable
    /* 1000 80012000 04100180 */ .word 0x80011004
enddlabel PointerTable
"""


def manifest_document(**updates: object) -> dict[str, object]:
    entry: dict[str, object] = {
        "source_file": "source.data.s",
        "source_address": hex(SOURCE),
        "source_owner": "PointerTable",
        "target_file": "target.data.s",
        "target_address": hex(TARGET),
        "target_owner": "StringPool",
        "symbol": SYMBOL,
    }
    entry.update(updates)
    return {"schema": 1, "description": "fixture", "entries": [entry]}


class Fixture:
    def __init__(self, root: Path, document: dict[str, object] | None = None) -> None:
        self.root = root
        self.inputs = root / "input"
        self.outputs = root / "output"
        self.manifest = root / "manifest.json"
        self.inputs.mkdir()
        (self.inputs / "target.data.s").write_text(TARGET_ASM)
        (self.inputs / "source.data.s").write_text(SOURCE_ASM)
        self.manifest.write_text(json.dumps(document or manifest_document()))

    def rewrite(self) -> reloc_data.RewriteResult:
        return reloc_data.rewrite(self.manifest, self.inputs, self.outputs)


class RewriteTests(unittest.TestCase):
    def test_repository_manifest_contains_only_reviewed_named_tables(self) -> None:
        entries = reloc_data.load_manifest(ROOT / "config/reloc-data.main.exe.json")
        self.assertEqual(len(entries), 108)
        self.assertEqual(
            {entry.source_owner for entry in entries},
            {
                "STAGE_SOUND_PREFICES",
                "STAGE_ANIMATION_PREFICES",
                "D_8008EA90",
                "ITEM_SEL_SPRITE_PTRS",
                "RS_ARCHIVE_PTRS",
                "RANK_ARCHIVE_PTRS",
                "RANKS_ARCHIVE_PTRS",
                "TRN_SPRITE_PTRS",
                "GOV_ARCHIVE_PTRS",
            },
        )
        self.assertEqual(
            {entry.target_owner for entry in entries},
            {
                "D_80013500",
                "D_8001359C",
                "D_800136B0",
                "D_800137A0",
                "ITEM_HELP_TIM_PATHS",
                "D_80013AC0",
            },
        )
        demo_sources = {
            address
            for language in range(4)
            for stage in range(11)
            for field, address in enumerate(
                (
                    0x8008EA90 + language * 0x84 + stage * 0xC,
                    0x8008EA94 + language * 0x84 + stage * 0xC,
                )
            )
            # Stage 9 has no assets; mojo11 already uses the existing exact
            # D_800136B0 symbol in Splat's generated input.
            if stage != 8 and not (stage == 10 and field == 1)
        }
        self.assertEqual(
            {entry.source_address for entry in entries},
            set(range(0x8008EA58, 0x8008EA78, 4))
            | demo_sources
            | set(range(0x8008EF28, 0x8008EF78, 4))
            | set(range(0x8008EF88, 0x8008EF98, 4)),
        )
        self.assertEqual(
            len({(entry.target_file, entry.target_address) for entry in entries}),
            47,
        )
        demo_entries = [
            entry for entry in entries if entry.source_owner == "D_8008EA90"
        ]
        self.assertEqual(len(demo_entries), 76)
        self.assertEqual(
            {entry.source_address for entry in demo_entries}, demo_sources
        )
        expected_demo_targets = {
            "demo_background_st011_tim_name": 0x800136BC,
            "demo_foreground_mojo10_tim_name": 0x800136C8,
            "demo_background_st010_tim_name": 0x800136D4,
            "demo_foreground_mojo8_tim_name": 0x800136E0,
            "demo_background_st008_tim_name": 0x800136EC,
            "demo_foreground_mojo7_tim_name": 0x800136F8,
            "demo_background_st007_tim_name": 0x80013704,
            "demo_foreground_mojo6_tim_name": 0x80013710,
            "demo_background_st006_tim_name": 0x8001371C,
            "demo_foreground_mojo5_tim_name": 0x80013728,
            "demo_background_st005_tim_name": 0x80013734,
            "demo_foreground_mojo4_tim_name": 0x80013740,
            "demo_background_st004_tim_name": 0x8001374C,
            "demo_foreground_mojo3_tim_name": 0x80013758,
            "demo_background_st003_tim_name": 0x80013764,
            "demo_foreground_mojo2_tim_name": 0x80013770,
            "demo_background_st002_tim_name": 0x8001377C,
            "demo_foreground_mojo1_tim_name": 0x80013788,
            "demo_background_st001_tim_name": 0x80013794,
        }
        self.assertEqual(
            {entry.symbol: entry.target_address for entry in demo_entries},
            expected_demo_targets,
        )
        for symbol in expected_demo_targets:
            self.assertEqual(
                sum(entry.symbol == symbol for entry in demo_entries),
                4,
            )
        new_symbols = {
            entry.symbol for entry in entries if entry.source_address >= 0x8008EF28
        }
        self.assertEqual(
            new_symbols,
            {
                *(f"item_selection_sprite_path_{language}" for language in
                  ("english", "french", "italian", "japanese")),
                *(f"rs_tim_path_{language}" for language in
                  ("english", "french", "italian", "japanese")),
                *(f"rank_archive_path_{language}" for language in
                  ("english", "french", "italian", "japanese")),
                *(f"trn_sprite_path_{language}" for language in
                  ("english", "french", "italian", "japanese")),
                *(f"gov_archive_name_{language}" for language in
                  ("english", "french", "italian", "japanese")),
            },
        )
        for language in ("english", "french", "italian", "japanese"):
            self.assertEqual(
                sum(
                    entry.symbol == f"rank_archive_path_{language}"
                    for entry in entries
                ),
                2,
            )

    def test_rewrite_inserts_exact_label_and_symbolic_word(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            fixture = Fixture(Path(temporary))
            result = fixture.rewrite()

            self.assertEqual(result, reloc_data.RewriteResult(1, 1, 2))
            target = (fixture.outputs / "target.data.s").read_text()
            source = (fixture.outputs / "source.data.s").read_text()
            anchor = (
                "/* reloc-data: exact interior 0x80011004 within StringPool */\n"
                f".globl {SYMBOL}\n"
                f".type {SYMBOL}, @object\n"
                f"{SYMBOL}:\n"
            )
            self.assertIn(anchor + "    /* 0004 80011004", target)
            self.assertIn(f".word {SYMBOL}", source)
            self.assertNotIn(".word 0x80011004", source)
            self.assertEqual(target.count(SYMBOL), 3)

    def test_check_mode_validates_without_writing(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            fixture = Fixture(Path(temporary))
            stdout = io.StringIO()
            with contextlib.redirect_stdout(stdout):
                status = reloc_data.main(
                    [
                        "--manifest",
                        str(fixture.manifest),
                        "--input-dir",
                        str(fixture.inputs),
                        "--check",
                    ]
                )
            self.assertEqual(status, 0)
            self.assertFalse(fixture.outputs.exists())
            self.assertIn("validated 1 pointer words", stdout.getvalue())

    def test_wrong_source_owner_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            fixture = Fixture(
                Path(temporary), manifest_document(source_owner="GuessedOwner")
            )
            with self.assertRaisesRegex(reloc_data.RelocDataError, "source owner"):
                fixture.rewrite()

    def test_wrong_literal_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            fixture = Fixture(
                Path(temporary), manifest_document(target_address="0x80011000")
            )
            with self.assertRaisesRegex(reloc_data.RelocDataError, "literal.*!= target"):
                fixture.rewrite()

    def test_duplicate_source_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            document = manifest_document()
            entries = document["entries"]
            assert isinstance(entries, list)
            entries.append(dict(entries[0]))
            fixture = Fixture(root, document)
            with self.assertRaisesRegex(reloc_data.RelocDataError, "duplicate source"):
                fixture.rewrite()

    def test_refuses_to_modify_generated_input_directory(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            fixture = Fixture(Path(temporary))
            with self.assertRaisesRegex(reloc_data.RelocDataError, "must differ"):
                reloc_data.rewrite(fixture.manifest, fixture.inputs, fixture.inputs)


MIPS_AS = shutil.which("mipsel-unknown-linux-gnu-as")
MIPS_LD = shutil.which("mipsel-unknown-linux-gnu-ld")
MIPS_OBJCOPY = shutil.which("mipsel-unknown-linux-gnu-objcopy")
MIPS_READELF = shutil.which("mipsel-unknown-linux-gnu-readelf")
HAVE_MIPS_TOOLS = all((MIPS_AS, MIPS_LD, MIPS_OBJCOPY, MIPS_READELF))


@unittest.skipUnless(HAVE_MIPS_TOOLS, "MIPS binutils are not on PATH")
class BinutilsProofTests(unittest.TestCase):
    def test_object_has_r_mips_32_and_pointer_tracks_large_carry_shift(self) -> None:
        assert MIPS_AS and MIPS_LD and MIPS_OBJCOPY and MIPS_READELF
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            fixture = Fixture(root)
            fixture.rewrite()
            target_object = root / "target.o"
            source_object = root / "source.o"
            for source, output in (
                (fixture.outputs / "target.data.s", target_object),
                (fixture.outputs / "source.data.s", source_object),
            ):
                subprocess.run(
                    [
                        MIPS_AS,
                        "-EL",
                        "-I",
                        str(ROOT / "include"),
                        "-march=r3000",
                        "-no-pad-sections",
                        "-G0",
                        "-o",
                        str(output),
                        str(source),
                    ],
                    check=True,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True,
                )

            relocations = subprocess.run(
                [MIPS_READELF, "-Wr", str(source_object)],
                check=True,
                stdout=subprocess.PIPE,
                text=True,
            ).stdout
            self.assertIn("R_MIPS_32", relocations)
            self.assertIn(SYMBOL, relocations)

            symbols = subprocess.run(
                [MIPS_READELF, "-Ws", str(target_object)],
                check=True,
                stdout=subprocess.PIPE,
                text=True,
            ).stdout
            symbol_line = next(line for line in symbols.splitlines() if SYMBOL in line)
            self.assertIn("00000004", symbol_line)
            self.assertNotIn(" ABS ", symbol_line)

            for shift in (0, 4, 0x10004):
                pointer = self._link_and_read_pointer(
                    root, target_object, source_object, shift
                )
                self.assertEqual(pointer, TARGET + shift)

    def _link_and_read_pointer(
        self,
        root: Path,
        target_object: Path,
        source_object: Path,
        shift: int,
    ) -> int:
        assert MIPS_LD and MIPS_OBJCOPY
        script = root / f"shift-{shift:x}.ld"
        linked = root / f"shift-{shift:x}.elf"
        binary = root / f"shift-{shift:x}.bin"
        script.write_text(
            "SECTIONS\n"
            "{\n"
            f"  .payload {BASE + shift:#x} : SUBALIGN(4) {{ *(.data) }}\n"
            "  /DISCARD/ : { *(.reginfo) *(.MIPS.abiflags) *(.gnu.attributes) }\n"
            "}\n"
        )
        subprocess.run(
            [
                MIPS_LD,
                "-EL",
                "-T",
                str(script),
                "-o",
                str(linked),
                str(target_object),
                str(source_object),
            ],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        subprocess.run(
            [
                MIPS_OBJCOPY,
                "-j",
                ".payload",
                "-O",
                "binary",
                str(linked),
                str(binary),
            ],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        payload = binary.read_bytes()
        self.assertEqual(len(payload), 12)
        return struct.unpack_from("<I", payload, 8)[0]


if __name__ == "__main__":
    unittest.main()

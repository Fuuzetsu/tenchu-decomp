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

BYTE_SOURCE_ASM = """\
.include "macro.inc"
.section .data, "wa"

dlabel PointerTable
    /* 1000 80012000 */ .byte 0x04
    /* 1001 80012001 */ .byte 0x10
    /* 1002 80012002 */ .byte 0x01
    /* 1003 80012003 */ .byte 0x80
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
    def __init__(
        self,
        root: Path,
        document: dict[str, object] | None = None,
        *,
        source: str = SOURCE_ASM,
    ) -> None:
        self.root = root
        self.inputs = root / "input"
        self.outputs = root / "output"
        self.manifest = root / "manifest.json"
        self.inputs.mkdir()
        (self.inputs / "target.data.s").write_text(TARGET_ASM)
        (self.inputs / "source.data.s").write_text(source)
        self.manifest.write_text(json.dumps(document or manifest_document()))

    def rewrite(self) -> reloc_data.RewriteResult:
        return reloc_data.rewrite(self.manifest, self.inputs, self.outputs)


class RewriteTests(unittest.TestCase):
    def test_repository_manifest_contains_only_reviewed_pointer_tables(self) -> None:
        entries = reloc_data.load_manifest(ROOT / "config/reloc-data.main.exe.json")
        self.assertEqual(len(entries), 208)
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
                "HumanData",
                "WeaponModel",
                "ThinkDB",
                "CD_comstr",
                "CD_intstr",
                "D_8008F580",
                "D_800967B8",
                "D_800973F4",
                "D_80097400",
                "D_80097C98",
                "D_80097C9C",
                "VoiceXaName",
                "VoiceXaNameF",
                "VoiceXaNameI",
                "VoiceXaNameJ",
                "TENCHU_ID",
                "CID",
                "StageConfig",
                "D_80097E98",
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
                "D_800116B8",
                "D_80011960",
                "D_80012C68",
                "D_80012CBC",
                "D_80012EB4",
                "D_800130AC",
                "D_80013BC4",
                "D_800140E0",
                "D_80014B34",
                "D_80014D34",
                "D_80015C04",
                "D_80015CA0",
                "switchD_8007d9a8__switchdataD_80015ccc",
                "D_80011C9C",
                "D_80011CB8",
                "D_80011CD8",
                "D_80011CFC",
                "D_80011D1C",
                "D_80011D38",
                "D_80011D54",
                "D_80011D70",
                "D_80011D90",
                "D_80011DB4",
                "D_80011DD4",
                "D_80011DE8",
                "D_80011E08",
                "D_80011E1C",
                "D_80011E3C",
                "D_80011E50",
                "D_80011E70",
                "D_80011E7C",
                "D_80011E9C",
                "D_80011EB4",
                "D_80011ED4",
                "D_80011EF8",
                "D_800C2EB0",
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
        old_sources = (
            set(range(0x8008EA58, 0x8008EA78, 4))
            | demo_sources
            | set(range(0x8008EF28, 0x8008EF78, 4))
            | set(range(0x8008EF88, 0x8008EF98, 4))
        )
        stage_config_sources = {
            0x80011F18 + record * 0x1C + field
            for record in range(11)
            for field in (4, 8)
        }
        final_sources = (
            {
                0x80088A9C,
                0x80088ACC,
                0x80088DFC,
                0x80088EBC,
                0x800899C8,
                0x800899D4,
                0x80089A7C,
                0x8008F594,
                0x800967B8,
                0x800973F4,
                0x80097400,
                0x80097D04,
                0x80097D8C,
                0x80097E98,
            }
            | set(range(0x80089E40, 0x80089ED0, 8))
            | set(range(0x8008F2C4, 0x8008F364, 4))
            | set(range(0x80097C98, 0x80097CB0, 4))
        )
        self.assertEqual(
            {entry.source_address for entry in entries},
            stage_config_sources | old_sources | final_sources,
        )
        self.assertEqual(
            len({(entry.target_file, entry.target_address) for entry in entries}),
            135,
        )
        self.assertEqual(
            {str(entry.source_file) for entry in entries}
            | {
                str(entry.target_file)
                for entry in entries
                if entry.target_file is not None
            },
            {
                "1490.data.s",
                "E58.data.s",
                "1160.data.s",
                "207C.data.s",
                "2EB0.data.s",
                "33C4.data.s",
                "37A8.data.s",
                "400C.data.s",
                "4900.data.s",
                "75F64.data.s",
            },
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
        archive_owners = {
            "ITEM_SEL_SPRITE_PTRS",
            "RS_ARCHIVE_PTRS",
            "RANK_ARCHIVE_PTRS",
            "RANKS_ARCHIVE_PTRS",
            "TRN_SPRITE_PTRS",
            "GOV_ARCHIVE_PTRS",
        }
        new_symbols = {
            entry.symbol for entry in entries if entry.source_owner in archive_owners
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

        owner_counts = {
            "HumanData": 4,
            "WeaponModel": 3,
            "ThinkDB": 18,
            "CD_comstr": 32,
            "CD_intstr": 8,
            "D_8008F580": 1,
            "D_800967B8": 1,
            "D_800973F4": 1,
            "D_80097400": 1,
            "D_80097C98": 1,
            "D_80097C9C": 1,
            "VoiceXaName": 1,
            "VoiceXaNameF": 1,
            "VoiceXaNameI": 1,
            "VoiceXaNameJ": 1,
            "TENCHU_ID": 1,
            "CID": 1,
            "StageConfig": 22,
            "D_80097E98": 1,
        }
        for owner, count in owner_counts.items():
            self.assertEqual(
                sum(entry.source_owner == owner for entry in entries), count
            )

        final_entries = [
            entry for entry in entries if entry.source_address in final_sources
        ]
        self.assertEqual(len(final_entries), 78)
        self.assertEqual(
            sum(entry.symbol == "human_name_rikimaru" for entry in final_entries),
            2,
        )
        self.assertEqual(
            sum(entry.symbol == "cd_unknown_name" for entry in final_entries),
            12,
        )
        self.assertEqual(
            {entry.symbol for entry in final_entries if entry.source_owner == "ThinkDB"},
            {
                "think_db_name_1_pad_1",
                "think_db_name_1_pad_2",
                *(f"think_db_name_{name}" for name in (
                    "1a_trace", "1b_watch", "1c_random", "1d_ninja",
                    "1e_sleep", "1f_chase", "2a_confirm", "2b_contact",
                    "3a_callaid", "3b_atk_chase", "3c_atk_point",
                    "3d_escape", "3e_atk_area", "3f_atk_hitaway",
                    "4a_abandon", "4b_contact",
                )),
            },
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

    def test_rewrite_coalesces_four_little_endian_bytes_into_symbolic_word(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            fixture = Fixture(Path(temporary), source=BYTE_SOURCE_ASM)
            result = fixture.rewrite()

            self.assertEqual(result, reloc_data.RewriteResult(1, 1, 2))
            source = (fixture.outputs / "source.data.s").read_text()
            self.assertIn(f"/* 1000 80012000 */ .word {SYMBOL}", source)
            self.assertNotIn("80012001", source)
            self.assertNotIn(".byte", source)

    def test_rewrite_preserves_reviewed_external_base_addend(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            document = manifest_document(
                target_file=None,
                target_address="0x80011006",
                target_owner="ExternalBuffer",
                symbol="ExternalBuffer",
                target_addend="0x2",
            )
            source = SOURCE_ASM.replace("0x80011004", "0x80011006")
            fixture = Fixture(root, document, source=source)
            result = fixture.rewrite()

            self.assertEqual(result, reloc_data.RewriteResult(1, 1, 1))
            rewritten = (fixture.outputs / "source.data.s").read_text()
            self.assertIn(".word ExternalBuffer + 0x2", rewritten)

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

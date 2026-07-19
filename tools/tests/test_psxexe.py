"""Focused tests for the standalone PS-X EXE finalizer/validator.

Run: python -m unittest tools.tests.test_psxexe
"""

from __future__ import annotations

import argparse
import contextlib
import io
from pathlib import Path
import struct
import tempfile
import unittest

from tools import psxexe


LOAD = 0x80011000
ENTRY = 0x80011234
PAYLOAD_CONTAINING_ENTRY = b"A" * (ENTRY - LOAD + 4)


def make_exe(
    payload: bytes,
    *,
    pc: int = LOAD,
    load: int = LOAD,
    declared_size: int | None = None,
) -> bytes:
    """Build a synthetic EXE-shaped input with visible preserved header bytes."""
    header = bytearray(psxexe.HEADER_SIZE)
    header[:8] = psxexe.PSX_EXE_MAGIC
    # The marker/license area is intentionally nonzero: preservation tests must
    # prove the finalizer does not replace the whole header with a canned one.
    header[0x4C:0x70] = b"synthetic header template\0".ljust(0x24, b"!")
    fields = {
        "pc": pc,
        "gp": 0x80018000,
        "t_addr": load,
        "t_size": len(payload) if declared_size is None else declared_size,
        "d_addr": 0,
        "d_size": 0,
        "b_addr": 0x80030000,
        "b_size": 0x1000,
        "sp": 0x801FFFF0,
        "sp_offset": 0,
    }
    for name, value in fields.items():
        struct.pack_into("<I", header, psxexe.HEADER_FIELDS[name], value)
    return bytes(header) + payload


def make_elf32() -> bytes:
    """Hermetic ELF32 fixture with PT_LOAD entries and two defined symbols."""
    strings = b"\0_start\0__load_start\0"
    start_name = strings.index(b"_start")
    load_name = strings.index(b"__load_start")
    str_offset = 0x100
    sym_offset = 0x140
    section_offset = 0x200
    section_count = 3
    size = section_offset + section_count * 40
    data = bytearray(size)

    ident = bytearray(16)
    ident[:4] = b"\x7fELF"
    ident[4] = 1  # ELFCLASS32
    ident[5] = 1  # ELFDATA2LSB
    ident[6] = 1  # EV_CURRENT
    struct.pack_into(
        "<16sHHIIIIIHHHHHH",
        data,
        0,
        bytes(ident),
        2,  # ET_EXEC
        8,  # EM_MIPS
        1,
        ENTRY,
        52,
        section_offset,
        0,
        52,
        32,
        2,
        40,
        section_count,
        0,
    )
    # A vaddr-zero header segment must not be mistaken for the PS-X payload.
    struct.pack_into("<IIIIIIII", data, 52, 1, 0x80, 0, 0, 0x40, 0x40, 4, 0x10)
    struct.pack_into(
        "<IIIIIIII", data, 84, 1, 0x800, LOAD, LOAD, 0x900, 0xA00, 5, 0x800
    )

    data[str_offset : str_offset + len(strings)] = strings
    symbols = bytearray(48)
    struct.pack_into("<IIIBBH", symbols, 16, start_name, ENTRY, 0, 0x12, 0, 1)
    struct.pack_into("<IIIBBH", symbols, 32, load_name, LOAD, 0, 0x10, 0, 1)
    data[sym_offset : sym_offset + len(symbols)] = symbols

    # Section zero, then SHT_SYMTAB linked to section 2, then SHT_STRTAB.
    struct.pack_into(
        "<IIIIIIIIII",
        data,
        section_offset + 40,
        0,
        2,
        0,
        0,
        sym_offset,
        len(symbols),
        2,
        1,
        4,
        16,
    )
    struct.pack_into(
        "<IIIIIIIIII",
        data,
        section_offset + 80,
        0,
        3,
        0,
        0,
        str_offset,
        len(strings),
        0,
        0,
        1,
        0,
    )
    return bytes(data)


class FinalizeTests(unittest.TestCase):
    def test_finalizer_pads_and_only_changes_three_header_words(self) -> None:
        source = make_exe(b"A" * 0x801, pc=0xDEADBEE0, load=0x80020000)
        result, header, padding = psxexe.finalize_image(
            source,
            entry=ENTRY,
            load_address=LOAD,
            expected={"gp": 0x80018000, "sp": 0x801FFFF0},
        )

        self.assertEqual(padding, 0x7FF)
        self.assertEqual(len(result), 0x1800)
        self.assertEqual(header.pc, ENTRY)
        self.assertEqual(header.t_addr, LOAD)
        self.assertEqual(header.t_size, 0x1000)
        self.assertEqual(result[-0x7FF:], bytes(0x7FF))

        mutable = set()
        for field in ("pc", "t_addr", "t_size"):
            offset = psxexe.HEADER_FIELDS[field]
            mutable.update(range(offset, offset + 4))
        for offset, (before, after) in enumerate(
            zip(source[: psxexe.HEADER_SIZE], result[: psxexe.HEADER_SIZE])
        ):
            if offset not in mutable:
                self.assertEqual(before, after, f"header byte {offset:#x} changed")

    def test_finalization_is_idempotent_after_sector_padding(self) -> None:
        first, _, first_padding = psxexe.finalize_image(
            make_exe(PAYLOAD_CONTAINING_ENTRY), entry=ENTRY, load_address=LOAD
        )
        second, _, second_padding = psxexe.finalize_image(
            first, entry=ENTRY, load_address=LOAD
        )
        self.assertEqual(
            first_padding, psxexe.SECTOR_SIZE - len(PAYLOAD_CONTAINING_ENTRY)
        )
        self.assertEqual(second_padding, 0)
        self.assertEqual(second, first)

    def test_conflicting_expected_layout_is_rejected(self) -> None:
        with self.assertRaisesRegex(psxexe.PsxExeError, "conflicts with finalized"):
            psxexe.finalize_image(
                make_exe(PAYLOAD_CONTAINING_ENTRY),
                entry=ENTRY,
                load_address=LOAD,
                expected={"pc": ENTRY + 4},
            )

    def test_entry_cannot_land_only_in_new_padding(self) -> None:
        with self.assertRaisesRegex(psxexe.PsxExeError, "outside the unpadded payload"):
            psxexe.finalize_image(
                make_exe(b"payload"),
                entry=LOAD + 0x100,
                load_address=LOAD,
            )


class ValidateTests(unittest.TestCase):
    def valid_image(self) -> bytes:
        image, _, _ = psxexe.finalize_image(
            make_exe(PAYLOAD_CONTAINING_ENTRY), entry=ENTRY, load_address=LOAD
        )
        return image

    def test_validator_asserts_exact_file_size_contract(self) -> None:
        image = self.valid_image()
        with self.assertRaisesRegex(psxexe.PsxExeError, r"file size.*0x800 \+ t_size"):
            psxexe.validate_image(image + b"x")

    def test_validator_rejects_missing_magic(self) -> None:
        image = bytearray(self.valid_image())
        image[:8] = b"NOT EXE!"
        with self.assertRaisesRegex(psxexe.PsxExeError, "missing 'PS-X EXE' magic"):
            psxexe.validate_image(image)

    def test_validator_rejects_non_sector_size_even_when_file_matches(self) -> None:
        image = bytearray(make_exe(b"A" * 0x804, pc=ENTRY, declared_size=0x804))
        with self.assertRaisesRegex(psxexe.PsxExeError, "not a multiple of 0x800"):
            psxexe.validate_image(image)

    def test_validator_rejects_entry_outside_loaded_payload(self) -> None:
        image = bytearray(self.valid_image())
        struct.pack_into("<I", image, psxexe.HEADER_FIELDS["pc"], LOAD + 0x800)
        with self.assertRaisesRegex(psxexe.PsxExeError, "outside loaded payload"):
            psxexe.validate_image(image)

    def test_validator_checks_preserved_fields_on_request(self) -> None:
        image = self.valid_image()
        psxexe.validate_image(
            image,
            expected={"gp": 0x80018000, "b_size": 0x1000, "sp": 0x801FFFF0},
        )
        with self.assertRaisesRegex(psxexe.PsxExeError, "expected gp=0x0"):
            psxexe.validate_image(image, expected={"gp": 0})

    def test_expectation_aliases_are_supported(self) -> None:
        self.assertEqual(
            psxexe.parse_expectations(
                ["entry=0x80011234", "load-address=0x80011000", "stack=0x801ffff0"]
            ),
            {"pc": ENTRY, "t_addr": LOAD, "sp": 0x801FFFF0},
        )


class MetadataTests(unittest.TestCase):
    def test_elf_resolves_symbols_entry_and_nonzero_load_segment(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "fixture.elf"
            path.write_bytes(make_elf32())
            metadata = psxexe.read_elf_metadata(path)
            self.assertEqual(metadata.entry, ENTRY)
            self.assertEqual(metadata.symbol("_start"), ENTRY)
            self.assertEqual(metadata.symbol("__load_start"), LOAD)
            self.assertEqual(metadata.load_address(), LOAD)

    def test_map_accepts_both_common_symbol_spellings(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "fixture.map"
            path.write_text(
                "  0x80011234                __entry = .\n"
                "__load_start = 0x80011000\n"
            )
            self.assertEqual(psxexe.read_map_symbol(path, "__entry"), ENTRY)
            self.assertEqual(psxexe.read_map_symbol(path, "__load_start"), LOAD)

    def test_map_accepts_gnu_provide_spelling(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "fixture.map"
            path.write_text(
                "0x80011234 PROVIDE (__entry = .)\n"
                "0x80011000 PROVIDE_HIDDEN (__load_start = .)\n"
            )
            self.assertEqual(psxexe.read_map_symbol(path, "__entry"), ENTRY)
            self.assertEqual(psxexe.read_map_symbol(path, "__load_start"), LOAD)

    def test_resolve_layout_derives_elf_entry_and_load_segment(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "fixture.elf"
            path.write_bytes(make_elf32())
            args = argparse.Namespace(
                elf=path,
                map=None,
                entry=None,
                entry_symbol=None,
                load_address=None,
                load_symbol=None,
            )
            self.assertEqual(
                psxexe.resolve_layout(args, require_complete=True),
                psxexe.ResolvedLayout(ENTRY, LOAD),
            )

    def test_sign_extended_map_address_is_normalized(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "fixture.map"
            path.write_text("0xffffffff80011234 __entry\n")
            self.assertEqual(psxexe.read_map_symbol(path, "__entry"), ENTRY)


class CliTests(unittest.TestCase):
    def test_map_driven_finalize_then_validate(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / "linked.exe"
            output = root / "final.exe"
            map_path = root / "game.map"
            original = make_exe(b"B" * 0x901, pc=0, load=0)
            source.write_bytes(original)
            map_path.write_text(
                "0x80011234 __entry = .\n"
                "0x80011000 __load_start = .\n"
            )

            with contextlib.redirect_stdout(io.StringIO()):
                status = psxexe.main(
                    [
                        "finalize",
                        str(source),
                        "-o",
                        str(output),
                        "--map",
                        str(map_path),
                        "--entry-symbol",
                        "__entry",
                        "--load-symbol",
                        "__load_start",
                        "--expect",
                        "gp=0x80018000",
                    ]
                )
            self.assertEqual(status, 0)
            self.assertEqual(source.read_bytes(), original, "finalize modified its input")
            header = psxexe.validate_image(output.read_bytes())
            self.assertEqual((header.pc, header.t_addr, header.t_size), (ENTRY, LOAD, 0x1000))

            args = argparse.Namespace(
                command="validate",
                input=output,
                elf=None,
                map=map_path,
                entry=None,
                entry_symbol="__entry",
                load_address=None,
                load_symbol="__load_start",
                expect=[],
                quiet=True,
            )
            self.assertEqual(psxexe.run(args), 0)

    def test_finalize_refuses_to_overwrite_its_input(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "game.exe"
            path.write_bytes(make_exe(b"payload"))
            with contextlib.redirect_stderr(io.StringIO()):
                status = psxexe.main(
                    [
                        "finalize",
                        str(path),
                        "-o",
                        str(path),
                        "--entry",
                        hex(ENTRY),
                        "--load-address",
                        hex(LOAD),
                    ]
                )
            self.assertEqual(status, 2)

    def test_validate_rejects_conflicting_explicit_and_expected_entry(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "game.exe"
            image, _, _ = psxexe.finalize_image(
                make_exe(PAYLOAD_CONTAINING_ENTRY), entry=ENTRY, load_address=LOAD
            )
            path.write_bytes(image)
            stderr = io.StringIO()
            with contextlib.redirect_stderr(stderr):
                status = psxexe.main(
                    [
                        "validate",
                        str(path),
                        "--entry",
                        hex(ENTRY + 4),
                        "--expect",
                        f"pc={ENTRY:#x}",
                    ]
                )
            self.assertEqual(status, 2)
            self.assertIn("conflicts with resolved entry", stderr.getvalue())


if __name__ == "__main__":
    unittest.main()

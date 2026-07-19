"""Tests for the C/Python/linker RAM-layout authority."""

from __future__ import annotations

import contextlib
import io
from pathlib import Path
import tempfile
import unittest

from tools import ram_layout


class LayoutTests(unittest.TestCase):
    def test_checked_in_layout_derives_retail_allocator_values(self) -> None:
        layout = ram_layout.LAYOUT
        self.assertEqual(layout.memory_pool_end - layout.memory_pool_floor, 0x120000)
        self.assertEqual(layout.retail_memory_pool_capacity, 0x47FFE)
        self.assertEqual(layout.persistent_rng_address, 0x80010E70)
        self.assertEqual(layout.persistent_rng_end, 0x80010E74)
        self.assertEqual(layout.executable_handoff_end, 0x8010005C)
        self.assertEqual(layout.pc_memory_pool_size, 0x5F0000)
        self.assertEqual(layout.pc_memory_payload_address, 0x80200008)

    def test_rejects_missing_or_inconsistent_policy(self) -> None:
        source = (ram_layout.ROOT / ram_layout.DEFAULT_HEADER).read_text()
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "ram_layout.h"
            path.write_text(
                source.replace(
                    "#define TENCHU_MEMORY_POOL_END                0x801fc000",
                    "#define TENCHU_MEMORY_POOL_END                0x800dc000",
                )
            )
            with self.assertRaisesRegex(
                ram_layout.LayoutError, "end does not follow its floor"
            ):
                ram_layout.load(path)

    def test_rejects_duplicate_or_divergent_derived_defines(self) -> None:
        source = (ram_layout.ROOT / ram_layout.DEFAULT_HEADER).read_text()
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "ram_layout.h"
            path.write_text(
                source.replace(
                    "#define TENCHU_MAIN_LOAD_ADDRESS              0x80011000",
                    "#define TENCHU_MAIN_LOAD_ADDRESS              0x80011000\n"
                    "#define TENCHU_MAIN_LOAD_ADDRESS              0x80011000",
                )
            )
            with self.assertRaisesRegex(ram_layout.LayoutError, "defines.*twice"):
                ram_layout.load(path)

            path.write_text(
                source.replace(
                    "(TENCHU_MEMORY_POOL_HEADER_WORDS * 4)",
                    "(TENCHU_MEMORY_POOL_HEADER_WORDS * 8)",
                )
            )
            with self.assertRaisesRegex(
                ram_layout.LayoutError, "unsupported.*HEADER_BYTES"
            ):
                ram_layout.load(path)

    def test_nondefault_policy_values_are_derived_together(self) -> None:
        source = (ram_layout.ROOT / ram_layout.DEFAULT_HEADER).read_text()
        source = source.replace(
            "#define TENCHU_BSS_ALIGNMENT                  16",
            "#define TENCHU_BSS_ALIGNMENT                  32",
        ).replace(
            "#define TENCHU_MEMORY_POOL_ALIGNMENT          16",
            "#define TENCHU_MEMORY_POOL_ALIGNMENT          32",
        )
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "ram_layout.h"
            path.write_text(source)
            layout = ram_layout.load(path)
        self.assertEqual(layout.bss_alignment, 32)
        self.assertEqual(layout.memory_pool_alignment, 32)
        self.assertEqual(layout.memory_pool_header_bytes, 8)
        self.assertEqual(layout.retail_memory_pool_capacity, 0x47FFE)
        self.assertEqual(layout.pc_memory_payload_address, 0x80200008)

    def test_allocator_header_words_are_a_fixed_abi(self) -> None:
        source = (ram_layout.ROOT / ram_layout.DEFAULT_HEADER).read_text().replace(
            "#define TENCHU_MEMORY_POOL_HEADER_WORDS       2",
            "#define TENCHU_MEMORY_POOL_HEADER_WORDS       3",
        )
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "ram_layout.h"
            path.write_text(source)
            with self.assertRaisesRegex(ram_layout.LayoutError, "two-word"):
                ram_layout.load(path)

    def test_rejects_persistent_base_that_breaks_base_or_offsets(self) -> None:
        source = (ram_layout.ROOT / ram_layout.DEFAULT_HEADER).read_text().replace(
            "#define TENCHU_PERSISTENT_STATE_ADDRESS       0x80010000",
            "#define TENCHU_PERSISTENT_STATE_ADDRESS       0x80010004",
        )
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "ram_layout.h"
            path.write_text(source)
            with self.assertRaisesRegex(ram_layout.LayoutError, "0x20-aligned"):
                ram_layout.load(path)

    def test_rejects_mutable_pc_handshake_aggregate_size(self) -> None:
        source = (ram_layout.ROOT / ram_layout.DEFAULT_HEADER).read_text().replace(
            "#define TENCHU_PC_MEMORY_HANDSHAKE_SIZE       0x00000010",
            "#define TENCHU_PC_MEMORY_HANDSHAKE_SIZE       0x00000008",
        )
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "ram_layout.h"
            path.write_text(source)
            with self.assertRaisesRegex(ram_layout.LayoutError, "Buf16 ABI"):
                ram_layout.load(path)

    def test_rejects_mutable_hardware_addresses(self) -> None:
        source = (ram_layout.ROOT / ram_layout.DEFAULT_HEADER).read_text().replace(
            "#define TENCHU_SCRATCHPAD_ADDRESS             0x1f800000",
            "#define TENCHU_SCRATCHPAD_ADDRESS             0x1f900000",
        )
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "ram_layout.h"
            path.write_text(source)
            with self.assertRaisesRegex(ram_layout.LayoutError, "PS1 hardware"):
                ram_layout.load(path)

    def test_accepts_uppercase_hex_literal_defines(self) -> None:
        source = (ram_layout.ROOT / ram_layout.DEFAULT_HEADER).read_text().replace(
            "#define TENCHU_MAIN_LOAD_ADDRESS              0x80011000",
            "#define TENCHU_MAIN_LOAD_ADDRESS              0X80011000",
        )
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "ram_layout.h"
            path.write_text(source)
            layout = ram_layout.load(path)
        self.assertEqual(layout.main_load_address, 0x80011000)

    def test_rejects_empty_fixed_contract_ranges(self) -> None:
        source = (ram_layout.ROOT / ram_layout.DEFAULT_HEADER).read_text().replace(
            "#define TENCHU_EXECUTABLE_HANDOFF_SIZE        0x0000005c",
            "#define TENCHU_EXECUTABLE_HANDOFF_SIZE        0",
        )
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "ram_layout.h"
            path.write_text(source)
            with self.assertRaisesRegex(ram_layout.LayoutError, "invalid address or size"):
                ram_layout.load(path)

    def test_game_source_uses_the_central_policy_names(self) -> None:
        self.assertEqual(ram_layout.source_literal_findings(), [])

    def test_default_literal_scan_includes_headers_but_not_authority(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source_root = root / "src/main.exe"
            source_root.mkdir(parents=True)
            (source_root / "ram_layout.h").write_text(
                (ram_layout.ROOT / ram_layout.DEFAULT_HEADER).read_text()
            )
            (source_root / "duplicate.h").write_text(
                "#define BAD_POOL ((void *)0x800dc000)\n"
            )
            findings = ram_layout.source_literal_findings(root)
        self.assertEqual(len(findings), 1)
        self.assertEqual(findings[0]["file"], "src/main.exe/duplicate.h")
        self.assertEqual(findings[0]["layout_value"], "memory_pool_floor")

    def test_cli_exposes_values_to_the_build_driver(self) -> None:
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            self.assertEqual(ram_layout.main(["initial_stack_address"]), 0)
        self.assertEqual(output.getvalue(), "0x801ffff0\n")

    def test_literal_scan_ignores_comments_but_finds_code(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            source = Path(temporary) / "fixture.c"
            source.write_text(
                "/* 0x80010000 is documentation. */\n"
                'char *s = "0x1f800000 is text";\n'
                "void *p = (void *)0X80010000UL;\n"
                "void *q = (void *)0x1f800020;\n"
            )
            findings = ram_layout.source_literal_findings(
                ram_layout.ROOT, [source]
            )
        self.assertEqual(len(findings), 2)
        self.assertEqual(findings[0]["line"], 3)
        self.assertEqual(findings[0]["layout_value"], "persistent_state_address")
        self.assertEqual(findings[1]["line"], 4)
        self.assertEqual(findings[1]["layout_value"], "scratchpad+0x20")


if __name__ == "__main__":
    unittest.main()

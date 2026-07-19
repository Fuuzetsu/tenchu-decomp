from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import tempfile
import unittest

from tools import reloc_c_literals, reloc_data, reloc_growth_probe


@dataclass(frozen=True)
class FakeSymbol:
    name: str
    value: int
    section_index: int = 1


class FakeElf:
    def __init__(
        self,
        values: dict[str, int | tuple[int, int]],
        words: dict[int, int] | None = None,
    ) -> None:
        self.path = Path("fixture.elf")
        self._values = values
        self._words = words or {}

    def symbol(self, name: str) -> FakeSymbol:
        raw = self._values[name]
        if isinstance(raw, tuple):
            value, section = raw
        else:
            value, section = raw, 1
        return FakeSymbol(name, value, section)

    def word_at_address(self, address: int) -> int:
        return self._words[address]


class RelocGrowthProbeTests(unittest.TestCase):
    def test_c_object_inventory_uses_ordinary_proc_item_shinsoku(self) -> None:
        self.assertEqual(
            reloc_growth_probe.C_OBJECTS["ProcItemShinsoku"],
            Path(".shake/build/main.exe/ProcItemShinsoku.c.o"),
        )
        self.assertEqual(
            reloc_growth_probe.C_OBJECTS["FileOption"],
            Path(".shake/reloc-c-literals/FileOption.o"),
        )

    def test_extension_inputs_mirror_recursive_user_and_generated_union(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            user_dir = root / "src/main.exe/reloc"
            generated_dir = root / ".shake/gen/main.exe/src/reloc"
            object_dir = root / ".shake/build/main.exe/reloc"
            user_dir.mkdir(parents=True)
            generated_dir.mkdir(parents=True)
            object_dir.mkdir(parents=True)

            user_sources = ("live.c", "nested/user.c", "shared.c")
            generated_sources = ("generated.c", "nested/generated.c", "shared.c")
            for relative in user_sources:
                source = user_dir / relative
                source.parent.mkdir(parents=True, exist_ok=True)
                source.write_text("int user_source(void) { return 1; }\n")
            for relative in generated_sources:
                source = generated_dir / relative
                source.parent.mkdir(parents=True, exist_ok=True)
                source.write_text("int generated_source(void) { return 1; }\n")

            expected_sources = sorted(set(user_sources) | set(generated_sources))
            for relative in expected_sources:
                obj = object_dir / f"{relative}.o"
                obj.parent.mkdir(parents=True, exist_ok=True)
                obj.touch()
            (object_dir / "stale.c.o").touch()

            self.assertEqual(
                reloc_growth_probe.extension_object_inputs(root),
                [
                    Path(f".shake/build/main.exe/reloc/{relative}.o")
                    for relative in expected_sources
                ],
            )

    def test_extension_inputs_require_each_source_backed_object(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            source = root / ".shake/gen/main.exe/src/reloc/nested/missing.c"
            source.parent.mkdir(parents=True)
            source.touch()

            with self.assertRaisesRegex(
                reloc_growth_probe.ProbeError,
                r"extension object for reloc/nested/missing\.c",
            ):
                reloc_growth_probe.extension_object_inputs(root)

    def test_injects_one_ordinary_input_after_main(self) -> None:
        source = """\
SECTIONS
{
    main = .;
    .shake/build/main.exe/main.c.o(.text);
    next = .;
}
"""
        result = reloc_growth_probe.inject_probe(source, Path("/tmp/probe.o"))
        main_index = result.index("main.c.o(.text);")
        probe_index = result.index("/tmp/probe.o(.text);")
        next_index = result.index("next = .;")
        self.assertLess(main_index, probe_index)
        self.assertLess(probe_index, next_index)
        self.assertNotIn("0x800", result)

    def test_injection_rejects_missing_or_ambiguous_anchor(self) -> None:
        with self.assertRaisesRegex(reloc_growth_probe.ProbeError, "0 .*anchors"):
            reloc_growth_probe.inject_probe("SECTIONS {}\n", Path("probe.o"))
        duplicate = (
            f"{reloc_growth_probe.ANCHOR_INPUT}\n"
            f"{reloc_growth_probe.ANCHOR_INPUT}\n"
        )
        with self.assertRaisesRegex(reloc_growth_probe.ProbeError, "2 .*anchors"):
            reloc_growth_probe.inject_probe(duplicate, Path("probe.o"))

    def test_probe_assembly_has_exact_requested_extent(self) -> None:
        source = reloc_growth_probe.probe_assembly(0x10004)
        self.assertIn(".space 0x10004, 0", source)
        self.assertIn(".size __tenchu_growth_probe", source)
        self.assertNotIn(".org", source)

    def test_manifest_words_are_checked_at_moved_source_addresses(self) -> None:
        growth = 0x10004
        base_tail = 0x80086758
        grown_tail = base_tail + growth
        source_retail = 0x80086764 + 0x20
        base_source = base_tail + 0x20
        grown_source = grown_tail + 0x20
        entries = [
            reloc_data.PointerEntry(
                source_file=Path("75F64.data.s"),
                source_address=source_retail,
                source_owner="tail_owner",
                target_file=Path("207C.data.s"),
                target_address=0x80013524,
                target_owner="target_owner",
                symbol="exact_target",
                target_addend=0,
            )
        ]
        base = FakeElf(
            {"tail_owner": base_tail, "exact_target": 0x80013524},
            {base_source: 0x80013524},
        )
        grown = FakeElf(
            {"tail_owner": grown_tail, "exact_target": 0x80013524},
            {grown_source: 0x80013524},
        )
        self.assertEqual(
            reloc_growth_probe.verify_manifest_pointers(
                base,
                grown,
                entries,
                {(Path("75F64.data.s"), "tail_owner"): source_retail - 0x20},
                growth,
                growth,
            ),
            1,
        )

        grown._words[grown_source] = 0x80013528
        with self.assertRaisesRegex(
            reloc_growth_probe.ProbeError, "grown pointer"
        ):
            reloc_growth_probe.verify_manifest_pointers(
                base,
                grown,
                entries,
                {(Path("75F64.data.s"), "tail_owner"): source_retail - 0x20},
                growth,
                growth,
            )

    def test_owner_start_comes_from_generated_address_comment(self) -> None:
        source = """\
dlabel Before
    /* 1000 80011000 */ .word 0
enddlabel Before
dlabel StageConfig
    /* 171C 80011F1C */ .byte 0x9C
enddlabel StageConfig
"""
        self.assertEqual(
            reloc_growth_probe.owner_retail_start(
                source, "StageConfig", Path("1490.data.s")
            ),
            0x80011F1C,
        )

    def test_manifest_distinguishes_unchanged_stage_and_moved_bss_cursor(self) -> None:
        growth = 0x10004
        bss_delta = 0x10000
        stage_source = 0x80011F1C
        tail_source = 0x80097E98
        base_tail_owner = 0x80097E98
        grown_tail_owner = base_tail_owner + growth
        base_bss_target = 0x800C2EB2
        grown_bss_target = base_bss_target + bss_delta
        entries = [
            reloc_data.PointerEntry(
                Path("1490.data.s"),
                stage_source,
                "StageConfig",
                Path("1490.data.s"),
                0x80011C9C,
                "stage_name_pool",
                "stage_name_exact",
                0,
            ),
            reloc_data.PointerEntry(
                Path("75F64.data.s"),
                tail_source,
                "D_80097E98",
                None,
                base_bss_target,
                "D_800C2EB0",
                "allocator_cursor_base",
                0x2,
            ),
        ]
        base = FakeElf(
            {
                "StageConfig": stage_source,
                "stage_name_exact": 0x80011C9C,
                "D_80097E98": base_tail_owner,
                "allocator_cursor_base": base_bss_target - 0x2,
            },
            {
                stage_source: 0x80011C9C,
                tail_source: base_bss_target,
            },
        )
        grown = FakeElf(
            {
                "StageConfig": stage_source,
                "stage_name_exact": 0x80011C9C,
                "D_80097E98": grown_tail_owner,
                "allocator_cursor_base": grown_bss_target - 0x2,
            },
            {
                stage_source: 0x80011C9C,
                tail_source + growth: grown_bss_target,
            },
        )
        starts = {
            (Path("1490.data.s"), "StageConfig"): stage_source,
            (Path("75F64.data.s"), "D_80097E98"): tail_source,
        }
        self.assertEqual(
            reloc_growth_probe.verify_manifest_pointers(
                base, grown, entries, starts, growth, bss_delta  # type: ignore[arg-type]
            ),
            2,
        )

    def test_bss_growth_moves_and_shrinks_dynamic_pool(self) -> None:
        base = FakeElf(
            {
                "main_exe_INITIALIZED_END": 0x80097EA4,
                "__bss_start": 0x80097EB0,
                "__bss_end": 0x800CDBA8,
                "D_800CDBA8": 0x800CDBA8,
                "HEAP_START": 0x800CDBAC,
                "MemoryPool": 0x800DC000,
                "MemoryPoolEnd": 0x801FC000,
                "main_exe_MEMORY_POOL_END": 0x801FC000,
                "MemoryPoolCapacity": (
                    (0x801FC000 - 0x800DC000) // 4
                    - reloc_growth_probe.POOL_HEADER_WORDS,
                    reloc_c_literals.SHN_ABS,
                ),
            }
        )
        grown = FakeElf(
            {
                "main_exe_INITIALIZED_END": 0x800A7EA8,
                "__bss_start": 0x800A7EB0,
                "__bss_end": 0x800DDBA8,
                "D_800CDBA8": 0x800DDBA8,
                "HEAP_START": 0x800DDBAC,
                "MemoryPool": 0x800DDBB0,
                "MemoryPoolEnd": 0x801FC000,
                "main_exe_MEMORY_POOL_END": 0x801FC000,
                "MemoryPoolCapacity": (
                    (0x801FC000 - 0x800DDBB0) // 4
                    - reloc_growth_probe.POOL_HEADER_WORDS,
                    reloc_c_literals.SHN_ABS,
                ),
            }
        )
        result = reloc_growth_probe.verify_bss_and_pool(
            base, grown  # type: ignore[arg-type]
        )
        self.assertEqual(result[0], 0x10000)
        self.assertEqual(result[2] - result[1], 0x1BB0)
        self.assertLess(result[5], result[4])


if __name__ == "__main__":
    unittest.main()

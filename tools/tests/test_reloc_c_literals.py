"""Tests for the first symbolic-C address relocation gate."""

from __future__ import annotations

from pathlib import Path
import struct
import tempfile
import unittest

from tools import reloc_c_literals as audit


class FakeElf:
    def __init__(
        self,
        text: bytes,
        relocations: list[audit.Relocation],
        symbols: dict[str, audit.Symbol],
    ):
        self.text = text
        self.relocation_list = relocations
        self.symbols = symbols

    def section_data(self, name: str) -> bytes:
        if name != ".text":
            raise AssertionError(name)
        return self.text

    def relocations(self, name: str) -> list[audit.Relocation]:
        if name != ".rel.text":
            raise AssertionError(name)
        return self.relocation_list

    def symbol(self, name: str) -> audit.Symbol:
        return self.symbols[name]


def instruction(opcode: int, immediate: int = 0) -> bytes:
    return struct.pack("<I", opcode << 26 | immediate)


class ContractTests(unittest.TestCase):
    SPEC = audit.ObjectSpec(
        {"Target": {audit.R_MIPS_HI16: 1, audit.R_MIPS_LO16: 1}}, (0x8009,)
    )

    def elf(self, *, raw_literal: bool = False) -> FakeElf:
        text = instruction(0x0F) + instruction(0x09)
        if raw_literal:
            text += instruction(0x0F, 0x8009)
        relocations = [
            audit.Relocation(0, audit.R_MIPS_HI16, "Target"),
            audit.Relocation(4, audit.R_MIPS_LO16, "Target"),
        ]
        symbols = {"Target": audit.Symbol("Target", 0, audit.SHN_UNDEF)}
        return FakeElf(text, relocations, symbols)

    def test_accepts_symbolic_hi_lo_pair(self) -> None:
        report = audit.verify_contract(self.elf(), self.SPEC, "Example")
        self.assertIn("Target HI16=1 LO16=1", report)

    def test_rejects_missing_low_relocation(self) -> None:
        elf = self.elf()
        elf.relocation_list.pop()
        with self.assertRaisesRegex(audit.AuditError, "relocations.*expected"):
            audit.verify_contract(elf, self.SPEC, "Example")

    def test_rejects_absolute_target_symbol(self) -> None:
        elf = self.elf()
        elf.symbols["Target"] = audit.Symbol("Target", 0, 0xFFF1)
        with self.assertRaisesRegex(audit.AuditError, "not an undefined"):
            audit.verify_contract(elf, self.SPEC, "Example")

    def test_rejects_raw_matching_high_half(self) -> None:
        with self.assertRaisesRegex(audit.AuditError, "unrelocated LUI 0x8009"):
            audit.verify_contract(
                self.elf(raw_literal=True), self.SPEC, "Example"
            )

    def test_rejects_relocation_on_wrong_instruction(self) -> None:
        elf = self.elf()
        elf.text = instruction(0x09) + instruction(0x09)
        with self.assertRaisesRegex(audit.AuditError, "HI16.*not LUI"):
            audit.verify_contract(elf, self.SPEC, "Example")

    def test_rejects_relocation_at_wrong_fixed_offset(self) -> None:
        spec = audit.ObjectSpec(
            self.SPEC.targets,
            self.SPEC.literal_high_halves,
            {
                "Target": {
                    audit.R_MIPS_HI16: (4,),
                    audit.R_MIPS_LO16: (0,),
                }
            },
        )
        with self.assertRaisesRegex(audit.AuditError, "offsets.*expected"):
            audit.verify_contract(self.elf(), spec, "Example")

    def test_finds_matching_only_pool_capacity_materialisation(self) -> None:
        text = struct.pack("<II", 0x3C020004, 0x34427FFE)
        self.assertEqual(audit.find_literal_pool_capacity(text), [0])

        symbolic = struct.pack("<II", 0x3C020000, 0x24420000)
        self.assertEqual(audit.find_literal_pool_capacity(symbolic), [])


class InventoryTests(unittest.TestCase):
    def test_classifies_every_replacement_as_exact_source_debt(self) -> None:
        self.assertEqual(
            set(audit.SOURCE_VARIANT_DEBT),
            set(audit.REPLACEMENT_OBJECT_SPECS),
        )
        self.assertEqual(
            {
                name
                for name, debt in audit.SOURCE_VARIANT_DEBT.items()
                if debt.category == audit.SOURCE_RECONSTRUCTION
            },
            {"SelectCameraOwnerOption", "FileOption", "ActivateHumans"},
        )
        self.assertEqual(
            {
                name
                for name, debt in audit.SOURCE_VARIANT_DEBT.items()
                if debt.category == audit.EXACT_OPCODE_CONFLICT
            },
            {"vinit", "valloc"},
        )
        self.assertTrue(
            all(debt.detail for debt in audit.SOURCE_VARIANT_DEBT.values())
        )

    def test_rejects_source_debt_inventory_drift(self) -> None:
        incomplete = dict(audit.SOURCE_VARIANT_DEBT)
        del incomplete["FileOption"]
        with self.assertRaisesRegex(
            audit.AuditError,
            "debt inventory differs.*missing FileOption",
        ):
            audit.validate_source_variant_debt_inventory(incomplete)

        extra = dict(audit.SOURCE_VARIANT_DEBT)
        extra["ProcItemShinsoku"] = audit.SourceVariantDebt(
            audit.SOURCE_RECONSTRUCTION,
            "already resolved",
        )
        with self.assertRaisesRegex(
            audit.AuditError,
            "debt inventory differs.*extra ProcItemShinsoku",
        ):
            audit.validate_source_variant_debt_inventory(extra)

    def test_separates_replacement_and_ordinary_object_inventories(self) -> None:
        self.assertNotIn("ProcItemShinsoku", audit.REPLACEMENT_OBJECT_SPECS)
        self.assertEqual(
            set(audit.ORDINARY_OBJECT_SPECS), {"ProcItemShinsoku"}
        )
        self.assertEqual(
            set(audit.OBJECT_SPECS),
            set(audit.REPLACEMENT_OBJECT_SPECS)
            | set(audit.ORDINARY_OBJECT_SPECS),
        )

    def test_requires_exact_fixed_inventory(self) -> None:
        with self.assertRaisesRegex(audit.AuditError, "object inventory differs"):
            audit.parse_objects(["FileOption=somewhere.o"])

    def test_replacement_parser_rejects_ordinary_object(self) -> None:
        mappings = [
            f"{name}=somewhere/{name}.o"
            for name in audit.REPLACEMENT_OBJECT_SPECS
        ]
        parsed = audit.parse_objects(
            mappings, expected_specs=audit.REPLACEMENT_OBJECT_SPECS
        )
        self.assertEqual(set(parsed), set(audit.REPLACEMENT_OBJECT_SPECS))
        with self.assertRaisesRegex(audit.AuditError, "extra ProcItemShinsoku"):
            audit.parse_objects(
                mappings + ["ProcItemShinsoku=ordinary.o"],
                expected_specs=audit.REPLACEMENT_OBJECT_SPECS,
            )

    def test_rejects_malformed_mapping(self) -> None:
        with self.assertRaisesRegex(audit.AuditError, "expected NAME=PATH"):
            audit.parse_objects(["FileOption"])


class MemoryPoolLayoutTests(unittest.TestCase):
    def test_capacity_policy_comes_from_the_central_layout(self) -> None:
        self.assertEqual(
            audit.MEMORY_POOL_HEADER_WORDS,
            audit.ram_layout.LAYOUT.memory_pool_header_words,
        )
        self.assertEqual(
            audit.MINIMUM_MEMORY_POOL_SIZE,
            audit.ram_layout.LAYOUT.memory_pool_minimum_size,
        )

    def test_retail_gap_preserves_original_pool_capacity(self) -> None:
        start = audit.memory_pool_start_for_bss(audit.MEMORY_POOL_START - 4)
        self.assertEqual(start, audit.MEMORY_POOL_START)
        self.assertEqual(
            audit.memory_pool_capacity_from_bounds(start, audit.MEMORY_POOL_END),
            audit.RETAIL_MEMORY_POOL_CAPACITY,
        )

    def test_growth_past_retail_base_moves_and_shrinks_pool(self) -> None:
        retail_capacity = audit.memory_pool_capacity_from_bounds(
            audit.MEMORY_POOL_START, audit.MEMORY_POOL_END
        )
        start = audit.memory_pool_start_for_bss(audit.MEMORY_POOL_START + 0x11)
        capacity = audit.memory_pool_capacity_from_bounds(
            start, audit.MEMORY_POOL_END
        )
        self.assertEqual(start, audit.MEMORY_POOL_START + 0x20)
        self.assertEqual(capacity, retail_capacity - 8)

    def test_rejects_exhausted_pool(self) -> None:
        with self.assertRaisesRegex(audit.AuditError, "invalid size"):
            audit.memory_pool_capacity_from_bounds(
                audit.MEMORY_POOL_END - 8, audit.MEMORY_POOL_END
            )


class LinkerRewriteTests(unittest.TestCase):
    def mappings(self, prefix: str) -> dict[str, Path]:
        return {
            name: Path(prefix) / f"{name}.o"
            for name in audit.REPLACEMENT_OBJECT_SPECS
        }

    def linker(self, references: dict[str, Path]) -> str:
        lines = ["SECTIONS\n", "{\n"]
        for section in (".data", ".text", ".rodata", ".bss"):
            for name in audit.REPLACEMENT_OBJECT_SPECS:
                lines.append(f"  {references[name]}({section});\n")
        lines.append(f"  {audit.FIRST_SDK_TEXT_INPUT}\n")
        lines.append("}\n")
        return "".join(lines)

    def test_substitutes_every_section_and_adds_file_backed_padding(self) -> None:
        references = self.mappings("old")
        variants = self.mappings("new")
        output = audit.rewrite_linker(
            self.linker(references),
            references,
            variants,
            padding=audit.EXPECTED_TEXT_SHRINK,
        )
        self.assertTrue(output.startswith(audit.POOL_CAPACITY_PROVISION))
        for name in audit.REPLACEMENT_OBJECT_SPECS:
            self.assertNotIn(str(references[name]), output)
            self.assertEqual(output.count(str(variants[name])), 4)
        self.assertEqual(
            output.count("LONG(0x00000000);"),
            audit.EXPECTED_TEXT_SHRINK // 4,
        )
        self.assertIn(
            "LONG(0x00000000);\n  " + audit.FIRST_SDK_TEXT_INPUT,
            output,
        )

    def test_substitutes_every_section_without_normal_link_padding(self) -> None:
        references = self.mappings("old")
        variants = self.mappings("new")
        output = audit.rewrite_linker(
            self.linker(references), references, variants, padding=0
        )
        for name in audit.REPLACEMENT_OBJECT_SPECS:
            self.assertNotIn(str(references[name]), output)
            self.assertEqual(output.count(str(variants[name])), 4)
        self.assertNotIn("LONG(0x00000000);", output)
        self.assertIn(audit.FIRST_SDK_TEXT_INPUT, output)

    def test_rejects_incomplete_object_inventory_in_linker(self) -> None:
        references = self.mappings("old")
        variants = self.mappings("new")
        source = self.linker(references).replace(str(references["vinit"]), "missing")
        with self.assertRaisesRegex(audit.AuditError, "references.*expected 4"):
            audit.rewrite_linker(
                source,
                references,
                variants,
                padding=audit.EXPECTED_TEXT_SHRINK,
            )

    def test_rejects_unexpected_net_text_change(self) -> None:
        references = self.mappings("old")
        variants = self.mappings("new")
        with self.assertRaisesRegex(audit.AuditError, "padding is 8, expected 0 or 12"):
            audit.rewrite_linker(
                self.linker(references), references, variants, padding=8
            )

    def test_rejects_preexisting_capacity_definition(self) -> None:
        references = self.mappings("old")
        variants = self.mappings("new")
        with self.assertRaisesRegex(
            audit.AuditError, "already defines MemoryPoolCapacity"
        ):
            audit.rewrite_linker(
                "MemoryPoolCapacity = 1;\n" + self.linker(references),
                references,
                variants,
                padding=audit.EXPECTED_TEXT_SHRINK,
            )


class ElfReaderTests(unittest.TestCase):
    def test_rejects_non_elf_input(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "not.o"
            path.write_bytes(b"not an elf")
            with self.assertRaisesRegex(audit.AuditError, "truncated ELF header"):
                audit.ElfObject(path)


if __name__ == "__main__":
    unittest.main()

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

    def test_allocator_pair_requires_addiu_on_the_lui_register(self) -> None:
        elf = self.elf()
        elf.text = struct.pack("<II", 0x3C020000, 0x24620000)
        with self.assertRaisesRegex(audit.AuditError, "not self-contained"):
            audit.verify_contract(
                elf, self.SPEC, "vinit", strict_layout=False
            )
        elf.text = struct.pack("<II", 0x3C020000, 0x34420000)
        with self.assertRaisesRegex(audit.AuditError, "is not ADDIU"):
            audit.verify_contract(
                elf, self.SPEC, "vinit", strict_layout=False
            )

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
        report = audit.verify_contract(
            self.elf(), spec, "Example", strict_layout=False
        )
        self.assertIn("Target HI16=1 LO16=1", report)

    def test_finds_matching_only_pool_capacity_materialisation(self) -> None:
        text = struct.pack("<II", 0x3C020004, 0x34427FFE)
        self.assertEqual(audit.find_literal_pool_capacity(text), [0])

        symbolic = struct.pack("<II", 0x3C020000, 0x24420000)
        self.assertEqual(audit.find_literal_pool_capacity(symbolic), [])

    def test_slices_a_function_out_of_a_combined_object(self) -> None:
        prefix = instruction(0x0F, 0x8009)
        body = instruction(0x0F) + instruction(0x09)
        elf = FakeElf(
            prefix + body,
            [
                audit.Relocation(4, audit.R_MIPS_HI16, "Target"),
                audit.Relocation(8, audit.R_MIPS_LO16, "Target"),
            ],
            {
                "Example": audit.Symbol("Example", 4, 1, len(body)),
                "Target": audit.Symbol("Target", 0, audit.SHN_UNDEF),
            },
        )
        report = audit.verify_contract(elf, self.SPEC, "Example")
        self.assertIn("Target HI16=1 LO16=1", report)


class AllocatorAssemblyTransformTests(unittest.TestCase):
    def test_abi_pair_reconstructs_values_across_low_half_boundaries(self) -> None:
        for value in (
            0x800D7FFF,
            0x800D8000,
            0x800DFFFF,
            0x800E0000,
            audit.MEMORY_POOL_START,
            audit.RETAIL_MEMORY_POOL_CAPACITY,
        ):
            high = audit.relocated_hi16(value)
            low = value & 0xFFFF
            self.assertEqual(audit.reconstruct_lui_addiu(high, low), value)

    def test_relocates_exact_lui_ori_pairs_without_changing_line_count(self) -> None:
        source = """\
li\t$2,-2146631680\t\t\t# 0x800d0000
ori\t$2,$2,0xc000
li\t$3,262144\t\t\t# 0x00040000
ori\t$3,$3,0x7ffe
"""
        transformed = audit.relocate_allocator_literals(source, "vinit")
        self.assertEqual(len(transformed.splitlines()), 4)
        self.assertIn(
            "lui\t$2,%hi(MemoryPool)", transformed
        )
        self.assertIn(
            "addiu\t$2,$2,%lo(MemoryPool)", transformed
        )
        self.assertIn(
            "lui\t$3,%hi(MemoryPoolCapacity)", transformed
        )
        self.assertIn(
            "addiu\t$3,$3,%lo(MemoryPoolCapacity)", transformed
        )

    def test_expands_a_single_instruction_literal_when_policy_shape_changes(self) -> None:
        old_symbols = audit.ALLOCATOR_RELOCATION_SYMBOLS
        old_counts = audit.ALLOCATOR_LITERAL_COUNTS
        try:
            audit.ALLOCATOR_RELOCATION_SYMBOLS = {
                0x1234: "Pool",
                0x5678: "Capacity",
            }
            audit.ALLOCATOR_LITERAL_COUNTS = {
                "vinit": {"Pool": 1, "Capacity": 1}
            }
            transformed = audit.relocate_allocator_literals(
                "li $2,0x1234\nli $3,0x5678\n", "vinit"
            )
        finally:
            audit.ALLOCATOR_RELOCATION_SYMBOLS = old_symbols
            audit.ALLOCATOR_LITERAL_COUNTS = old_counts
        self.assertEqual(len(transformed.splitlines()), 4)
        self.assertIn("%hi(Pool)", transformed)
        self.assertIn("%lo(Capacity)", transformed)

    def test_rejects_partial_or_unreviewed_transform(self) -> None:
        with self.assertRaisesRegex(audit.AuditError, "inventory differs"):
            audit.relocate_allocator_literals(
                "li $2,-2146631680\nori $2,$2,0xc000\n", "vinit"
            )
        with self.assertRaisesRegex(audit.AuditError, "not a reviewed"):
            audit.relocate_allocator_literals("", "other")

    def test_rejects_duplicate_reviewed_materialisation(self) -> None:
        source = """\
li $2,-2146631680
ori $2,$2,0xc000
li $3,-2146631680
ori $3,$3,0xc000
li $4,262144
ori $4,$4,0x7ffe
"""
        with self.assertRaisesRegex(audit.AuditError, "MemoryPool=2/1"):
            audit.relocate_allocator_literals(source, "valloc")

    def test_rejects_trailing_assembly_instead_of_deleting_it(self) -> None:
        source = """\
li $2,-2146631680; addu $7,$7,$7
ori $2,$2,0xc000
li $3,262144
ori $3,$3,0x7ffe
"""
        with self.assertRaisesRegex(audit.AuditError, "MemoryPool=0/1"):
            audit.relocate_allocator_literals(source, "vinit")


class InventoryTests(unittest.TestCase):
    def test_source_debt_is_zero_and_replacements_are_bounded_transforms(self) -> None:
        self.assertEqual(audit.SOURCE_VARIANT_DEBT, {})
        self.assertEqual(
            set(audit.ASSEMBLY_RELOCATION_TRANSFORMS),
            set(audit.REPLACEMENT_OBJECT_SPECS),
        )
        self.assertEqual(
            set(audit.REPLACEMENT_OBJECT_SPECS), {"vinit", "valloc"}
        )

    def test_rejects_replacement_reason_inventory_drift(self) -> None:
        unreviewed = dict(audit.REPLACEMENT_OBJECT_SPECS)
        unreviewed["Unreviewed"] = next(
            iter(audit.REPLACEMENT_OBJECT_SPECS.values())
        )
        with self.assertRaisesRegex(
            audit.AuditError,
            "reason inventory differs.*missing Unreviewed",
        ):
            audit.validate_source_variant_debt_inventory(
                replacement_specs=unreviewed
            )

        debt = {
            "ProcItemShinsoku": audit.SourceVariantDebt(
                audit.SOURCE_RECONSTRUCTION,
                "already resolved",
            )
        }
        with self.assertRaisesRegex(
            audit.AuditError,
            "reason inventory differs.*extra ProcItemShinsoku",
        ):
            audit.validate_source_variant_debt_inventory(debt=debt)

        overlapping = {
            "vinit": audit.SourceVariantDebt(
                audit.SOURCE_RECONSTRUCTION, "must not have two reasons"
            )
        }
        with self.assertRaisesRegex(audit.AuditError, "overlap vinit"):
            audit.validate_source_variant_debt_inventory(debt=overlapping)

    def test_separates_replacement_and_ordinary_object_inventories(self) -> None:
        self.assertNotIn("ProcItemShinsoku", audit.REPLACEMENT_OBJECT_SPECS)
        self.assertEqual(
            set(audit.ORDINARY_OBJECT_SPECS),
            {
                "ActivateHumans",
                "SelectCameraOwnerOption",
                "FileOption",
                "ProcItemShinsoku",
            },
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

    def test_focused_substitution_needs_no_file_backed_padding(self) -> None:
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
        self.assertNotIn("LONG(0x00000000);", output)
        self.assertIn(audit.FIRST_SDK_TEXT_INPUT, output)

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

    def test_substitutes_reconstructed_unit_small_data_section(self) -> None:
        references = self.mappings("old")
        variants = self.mappings("new")
        source = self.linker(references).replace(
            audit.FIRST_SDK_TEXT_INPUT,
            f"{references['vinit']}(.sdata);\n  {audit.FIRST_SDK_TEXT_INPUT}",
        )
        output = audit.rewrite_linker(
            source, references, variants, padding=0
        )
        self.assertEqual(output.count(str(variants["vinit"])), 5)
        self.assertEqual(output.count(str(variants["valloc"])), 4)

    def test_rejects_incomplete_object_inventory_in_linker(self) -> None:
        references = self.mappings("old")
        variants = self.mappings("new")
        source = self.linker(references).replace(str(references["vinit"]), "missing")
        with self.assertRaisesRegex(audit.AuditError, "section inventory.*missing"):
            audit.rewrite_linker(
                source,
                references,
                variants,
                padding=audit.EXPECTED_TEXT_SHRINK,
            )

    def test_rejects_duplicate_object_section_in_linker(self) -> None:
        references = self.mappings("old")
        variants = self.mappings("new")
        source = self.linker(references).replace(
            audit.FIRST_SDK_TEXT_INPUT,
            f"{references['vinit']}(.text);\n  {audit.FIRST_SDK_TEXT_INPUT}",
        )
        with self.assertRaisesRegex(audit.AuditError, "repeated \\.text"):
            audit.rewrite_linker(source, references, variants, padding=0)

    def test_rejects_unexpected_net_text_change(self) -> None:
        references = self.mappings("old")
        variants = self.mappings("new")
        with self.assertRaisesRegex(audit.AuditError, "padding is 8, expected 0"):
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

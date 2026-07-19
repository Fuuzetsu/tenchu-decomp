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


class InventoryTests(unittest.TestCase):
    def test_requires_exact_fixed_inventory(self) -> None:
        with self.assertRaisesRegex(audit.AuditError, "object inventory differs"):
            audit.parse_objects(["FileOption=somewhere.o"])

    def test_rejects_malformed_mapping(self) -> None:
        with self.assertRaisesRegex(audit.AuditError, "expected NAME=PATH"):
            audit.parse_objects(["FileOption"])


class LinkerRewriteTests(unittest.TestCase):
    def mappings(self, prefix: str) -> dict[str, Path]:
        return {name: Path(prefix) / f"{name}.o" for name in audit.OBJECT_SPECS}

    def linker(self, references: dict[str, Path]) -> str:
        lines = ["SECTIONS\n", "{\n"]
        for section in (".data", ".text", ".rodata", ".bss"):
            for name in audit.OBJECT_SPECS:
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
        for name in audit.OBJECT_SPECS:
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
        for name in audit.OBJECT_SPECS:
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


class ElfReaderTests(unittest.TestCase):
    def test_rejects_non_elf_input(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "not.o"
            path.write_bytes(b"not an elf")
            with self.assertRaisesRegex(audit.AuditError, "truncated ELF header"):
                audit.ElfObject(path)


if __name__ == "__main__":
    unittest.main()

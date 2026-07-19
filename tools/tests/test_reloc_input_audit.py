"""Regression tests for the normal-link input relocation gate.

Run: nix develop -c python -m unittest tools.tests.test_reloc_input_audit
"""

from __future__ import annotations

from pathlib import Path
import struct
import tempfile
import unittest

from tools import reloc_input_audit as audit


def instruction_lui(register: int, high: int) -> int:
    return (0x0F << 26) | (register << 16) | high


def instruction_addiu(target: int, source: int, low: int) -> int:
    return (0x09 << 26) | (source << 21) | (target << 16) | (low & 0xFFFF)


def instruction_ori(target: int, source: int, low: int) -> int:
    return (0x0D << 26) | (source << 21) | (target << 16) | (low & 0xFFFF)


def instruction_lw(target: int, base: int, low: int) -> int:
    return (0x23 << 26) | (base << 21) | (target << 16) | (low & 0xFFFF)


def instruction_beq(left: int, right: int, displacement: int) -> int:
    return (
        (0x04 << 26)
        | (left << 21)
        | (right << 16)
        | (displacement & 0xFFFF)
    )


def instruction_lwc2(target: int, base: int, low: int) -> int:
    return (0x32 << 26) | (base << 21) | (target << 16) | (low & 0xFFFF)


def words(*values: int) -> bytes:
    return struct.pack(f"<{len(values)}I", *values)


def model() -> audit.MovableModel:
    return audit.MovableModel(
        ranges=(
            (0x80011000, 0x80097EB0, ".main_exe"),
            (0x80097EB0, 0x800CDBA8, ".main_exe_bss"),
        ),
        boundary_values=frozenset({0x800CDBA8, 0x800CDBAC, 0x800DC000}),
    )


def _align(value: int, alignment: int = 4) -> int:
    return (value + alignment - 1) & -alignment


def make_relocatable_elf(
    path: Path,
    text: bytes,
    relocations: list[tuple[int, int]],
    extra_alloc_section: tuple[str, bytes] | None = None,
) -> None:
    """Write one strict ELF32/MIPS REL fixture with a real .rel.text table."""

    extra_name = extra_alloc_section[0] if extra_alloc_section else None
    shstrtab = b"\0.text\0.rel.text\0.symtab\0.strtab\0"
    if extra_name is not None:
        shstrtab += extra_name.encode() + b"\0"
    shstrtab += b".shstrtab\0"
    sh_names = {
        name: shstrtab.index(name.encode())
        for name in (".text", ".rel.text", ".symtab", ".strtab", ".shstrtab")
    }
    if extra_name is not None:
        sh_names[extra_name] = shstrtab.index(extra_name.encode())
    strtab = b"\0target\0"
    symtab = b"\0" * audit.SYMBOL_ENTRY.size + audit.SYMBOL_ENTRY.pack(
        1, 0, 0, 0x10, 0, audit.SHN_UNDEF
    )
    rel_text = b"".join(
        audit.REL_ENTRY.pack(offset, (1 << 8) | relocation_type)
        for offset, relocation_type in relocations
    )

    payload = bytearray(b"\0" * audit.ELF_HEADER.size)

    def append(raw: bytes, alignment: int = 4) -> int:
        offset = _align(len(payload), alignment)
        payload.extend(b"\0" * (offset - len(payload)))
        payload.extend(raw)
        return offset

    text_offset = append(text)
    rel_offset = append(rel_text)
    sym_offset = append(symtab)
    str_offset = append(strtab, 1)
    extra_offset = (
        append(extra_alloc_section[1]) if extra_alloc_section is not None else 0
    )
    shstr_offset = append(shstrtab, 1)
    section_offset = _align(len(payload))
    payload.extend(b"\0" * (section_offset - len(payload)))

    sections = [
        audit.SECTION_HEADER.pack(0, 0, 0, 0, 0, 0, 0, 0, 0, 0),
        audit.SECTION_HEADER.pack(
            sh_names[".text"], audit.SHT_PROGBITS, 0x6, 0, text_offset,
            len(text), 0, 0, 4, 0,
        ),
        audit.SECTION_HEADER.pack(
            sh_names[".rel.text"], audit.SHT_REL, 0, 0, rel_offset,
            len(rel_text), 3, 1, 4, audit.REL_ENTRY.size,
        ),
        audit.SECTION_HEADER.pack(
            sh_names[".symtab"], audit.SHT_SYMTAB, 0, 0, sym_offset,
            len(symtab), 4, 1, 4, audit.SYMBOL_ENTRY.size,
        ),
        audit.SECTION_HEADER.pack(
            sh_names[".strtab"], 3, 0, 0, str_offset, len(strtab), 0, 0, 1, 0
        ),
    ]
    if extra_alloc_section is not None:
        sections.append(
            audit.SECTION_HEADER.pack(
                sh_names[extra_alloc_section[0]],
                audit.SHT_PROGBITS,
                audit.SHF_ALLOC,
                0,
                extra_offset,
                len(extra_alloc_section[1]),
                0,
                0,
                4,
                0,
            )
        )
    sections.append(
        audit.SECTION_HEADER.pack(
            sh_names[".shstrtab"], 3, 0, 0, shstr_offset, len(shstrtab),
            0, 0, 1, 0,
        )
    )
    for section in sections:
        payload.extend(section)

    ident = b"\x7fELF" + bytes((1, 1, 1)) + b"\0" * 9
    header = audit.ELF_HEADER.pack(
        ident,
        audit.ET_REL,
        audit.EM_MIPS,
        1,
        0,
        0,
        section_offset,
        0,
        audit.ELF_HEADER.size,
        0,
        0,
        audit.SECTION_HEADER.size,
        len(sections),
        len(sections) - 1,
    )
    payload[: len(header)] = header
    path.write_bytes(payload)


class ElfRelocationTruthTests(unittest.TestCase):
    def test_symbolic_machine_words_pass_via_real_elf_relocations(self) -> None:
        # The zero immediates are exactly what a REL object carries before ld
        # resolves the symbolic direct call and symbolic pointer.
        text = words(
            0x0C000000,
            instruction_lui(2, 0),
            instruction_lw(2, 2, 0),
            instruction_beq(2, 0, 0),
            instruction_lw(2, 28, 0),
        )
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "symbolic.c.o"
            make_relocatable_elf(
                path,
                text,
                [
                    (0, audit.R_MIPS_26),
                    (4, audit.R_MIPS_HI16),
                    (8, audit.R_MIPS_LO16),
                    (12, audit.R_MIPS_PC16),
                    (16, audit.R_MIPS_GPREL16),
                ],
            )
            elf = audit.Elf32(path)
            section = next(item for item in elf.sections if item.name == ".text")
            relocation_types = audit.relocation_map(elf.relocations_for(section))
            findings, _reviewed, evidence = audit.analyse_executable_section(
                elf.section_data(section),
                relocation_types,
                model(),
                object_name="symbolic.c.o",
                section_name=".text",
            )
        self.assertEqual(findings, [])
        self.assertEqual(evidence["relocated_direct_jumps"], 1)
        self.assertEqual(evidence["symbolic_hi16"], 1)
        self.assertEqual(evidence["relocated_pc_relative_branches"], 1)
        self.assertEqual(evidence["relocated_gp_address_uses"], 1)

    def test_stripping_direct_call_relocation_fails(self) -> None:
        findings, _reviewed, _evidence = audit.analyse_executable_section(
            words(0x0C0058A9),
            {},
            model(),
            object_name="fault.c.o",
            section_name=".text",
        )
        self.assertEqual(
            [finding.kind for finding in findings],
            ["direct_jump_without_r_mips_26"],
        )

    def test_stripping_external_branch_and_gp_relocations_fails(self) -> None:
        text = words(
            instruction_beq(2, 0, 0x100),
            instruction_lw(2, 28, 0x1234),
        )
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "stripped.c.o"
            make_relocatable_elf(path, text, [])
            elf = audit.Elf32(path)
            section = next(item for item in elf.sections if item.name == ".text")
            findings, _reviewed, _evidence = audit.analyse_executable_section(
                elf.section_data(section),
                audit.relocation_map(elf.relocations_for(section)),
                model(),
                object_name="stripped.c.o",
                section_name=".text",
            )
        self.assertEqual(
            {finding.kind for finding in findings},
            {
                "pc_relative_branch_without_r_mips_pc16",
                "gp_address_use_without_relocation",
            },
        )


class LiteralInstructionTests(unittest.TestCase):
    def analyse(self, text: bytes, object_name: str = "fault.c.o"):
        return audit.analyse_executable_section(
            text,
            {},
            model(),
            object_name=object_name,
            section_name=".text",
        )

    def test_numeric_void_pointer_cast_fails(self) -> None:
        findings, _reviewed, _evidence = self.analyse(
            words(
                instruction_lui(2, 0x8001),
                instruction_ori(2, 2, 0x62A4),
            )
        )
        self.assertEqual(len(findings), 1)
        self.assertEqual(findings[0].kind, "literal_lui_low_main_address")
        self.assertEqual(findings[0].target, "0x800162a4")

    def test_numeric_pointer_load_fails(self) -> None:
        findings, _reviewed, _evidence = self.analyse(
            words(
                instruction_lui(2, 0x8001),
                instruction_lw(2, 2, 0x62A4),
            )
        )
        self.assertEqual(len(findings), 1)
        self.assertEqual(findings[0].target, "0x800162a4")

    def test_numeric_function_pointer_call_fails(self) -> None:
        findings, _reviewed, _evidence = self.analyse(
            words(
                instruction_lui(25, 0x8002),
                instruction_addiu(25, 25, 0x3456),
                0x0320F809,  # jalr t9
                0,
            )
        )
        self.assertEqual(len(findings), 1)
        self.assertEqual(findings[0].target, "0x80023456")

    def test_call_delay_slot_low_half_is_not_missed(self) -> None:
        # gcc may schedule an argument's low half into a jal delay slot.
        text = words(
            instruction_lui(4, 0x8001),
            0x0C000000,
            instruction_addiu(4, 4, 0x62A4),
        )
        findings, _reviewed, _evidence = audit.analyse_executable_section(
            text,
            {4: {audit.R_MIPS_26}},
            model(),
            object_name="fault.c.o",
            section_name=".text",
        )
        self.assertEqual(len(findings), 1)
        self.assertEqual(findings[0].target, "0x800162a4")

    def test_same_section_unrelocated_branch_is_shift_safe(self) -> None:
        findings, _reviewed, evidence = self.analyse(
            words(instruction_beq(2, 0, 0), 0)
        )
        self.assertEqual(findings, [])
        self.assertEqual(evidence["local_pc_relative_branches"], 1)

    def test_gp_addiu_needs_an_address_relocation(self) -> None:
        instruction = instruction_addiu(2, 28, 0x1234)
        findings, _reviewed, _evidence = self.analyse(words(instruction))
        self.assertEqual(
            [finding.kind for finding in findings],
            ["gp_address_use_without_relocation"],
        )

        findings, _reviewed, evidence = audit.analyse_executable_section(
            words(instruction),
            {0: {audit.R_MIPS_GPREL16}},
            model(),
            object_name="symbolic.s.o",
            section_name=".text",
        )
        self.assertEqual(findings, [])
        self.assertEqual(evidence["relocated_gp_address_uses"], 1)

        findings, _reviewed, _evidence = audit.analyse_executable_section(
            words(instruction),
            {0: {audit.R_MIPS_LO16}},
            model(),
            object_name="wrong-relocation.s.o",
            section_name=".text",
        )
        self.assertEqual(len(findings), 1)

    def test_crt_gp_initialization_accepts_its_symbolic_lo16(self) -> None:
        instruction = instruction_addiu(28, 28, 0)
        findings, _reviewed, evidence = audit.analyse_executable_section(
            bytes(0xA0) + words(instruction),
            {0xA0: {audit.R_MIPS_LO16}},
            model(),
            object_name=".shake/build/main.exe/CRT_SDK_4FA48.s.o",
            section_name=".text",
        )
        self.assertEqual(findings, [])
        self.assertEqual(evidence["relocated_gp_address_uses"], 1)

        findings, _reviewed, _evidence = audit.analyse_executable_section(
            words(instruction),
            {0: {audit.R_MIPS_LO16}},
            model(),
            object_name="elsewhere/CRT_SDK_4FA48.s.o",
            section_name=".text",
        )
        self.assertEqual(len(findings), 1)

    def test_coprocessor_load_does_not_overwrite_same_numbered_gpr(self) -> None:
        findings, _reviewed, _evidence = self.analyse(
            words(
                instruction_lui(8, 0x8001),
                instruction_lwc2(8, 9, 0),
                instruction_ori(8, 8, 0x62A4),
            )
        )
        self.assertEqual(len(findings), 1)
        self.assertEqual(findings[0].target, "0x800162a4")

    def test_mfc2_does_overwrite_its_gpr_target(self) -> None:
        mfc2_t0 = (0x12 << 26) | (8 << 16)
        findings, _reviewed, _evidence = self.analyse(
            words(
                instruction_lui(8, 0x8001),
                mfc2_t0,
                instruction_ori(8, 8, 0x62A4),
            )
        )
        self.assertEqual(findings, [])

    def test_reviewed_fixed_ps1_contracts_pass(self) -> None:
        fixtures = {
            "persistent_state": words(
                instruction_lui(2, 0x8001), instruction_addiu(2, 2, 0x0E70)
            ),
            "executable_handoff": words(
                instruction_lui(2, 0x8010), instruction_ori(2, 2, 0x54)
            ),
            "initial_stack": words(
                instruction_lui(2, 0x801F), instruction_ori(2, 2, 0xFFF0)
            ),
            "scratchpad": words(
                instruction_lui(2, 0x1F80), instruction_ori(2, 2, 0x80)
            ),
            "ram_end": words(
                instruction_lui(2, 0x8020), instruction_ori(2, 2, 0x08)
            ),
            "pc_link_memory": words(
                instruction_lui(2, 0x807F), instruction_ori(2, 2, 0)
            ),
        }
        for expected, text in fixtures.items():
            with self.subTest(expected):
                findings, reviewed, _evidence = self.analyse(text)
                self.assertEqual(findings, [])
                self.assertEqual(reviewed[expected], 1)

    def test_fixed_contract_ranges_are_half_open(self) -> None:
        self.assertEqual(
            audit.fixed_contract(audit.PERSISTENT_END - 1),
            "persistent_state",
        )
        self.assertIsNone(audit.fixed_contract(audit.PERSISTENT_END))
        self.assertEqual(
            audit.fixed_contract(audit.HANDOFF_END - 1),
            "executable_handoff",
        )
        self.assertIsNone(audit.fixed_contract(audit.HANDOFF_END))

    def test_changed_persistent_policy_rejects_retail_literal_code(self) -> None:
        persistent = audit.PersistentOracle(
            retail_base=0x80010000,
            retail_rng=0x80010E70,
            target_base=0x80008000,
            target_rng=0x80008F00,
        )
        text = words(
            instruction_lui(2, 0x8001), instruction_ori(2, 2, 0x0058)
        )
        findings, _reviewed, _evidence = audit.analyse_executable_section(
            text,
            {},
            model(),
            object_name="stale.s.o",
            section_name=".text",
            persistent_addresses=persistent,
        )
        self.assertEqual(len(findings), 1)
        self.assertEqual(findings[0].kind, "stale_retail_persistent_address")
        self.assertIn("0x80008058", findings[0].detail)

        findings, _reviewed, _evidence = audit.analyse_executable_section(
            text,
            {0: {audit.R_MIPS_HI16}, 4: {audit.R_MIPS_LO16}},
            model(),
            object_name="symbolic.s.o",
            section_name=".text",
            persistent_addresses=persistent,
        )
        self.assertEqual(findings, [])

    def test_known_sdk_numeric_mask_is_narrowly_scoped(self) -> None:
        instructions = words(
            instruction_lui(20, 0x8002),
            instruction_ori(20, 20, 0x0009),
        )
        text = bytes(0x9F80) + instructions
        findings, reviewed, _evidence = self.analyse(
            text, ".shake/build/main.exe/SDK_TEXT_58164.s.o"
        )
        self.assertEqual(findings, [])
        self.assertEqual(reviewed["numeric_constant"], 1)

        for object_name, candidate in (
            ("elsewhere/SDK_TEXT_58164.s.o", text),
            (".shake/build/main.exe/SDK_TEXT_58164.s.o", instructions),
            ("new_game_code.c.o", instructions),
        ):
            with self.subTest(object_name=object_name, size=len(candidate)):
                findings, _reviewed, _evidence = self.analyse(
                    candidate, object_name
                )
                self.assertEqual(len(findings), 1)


class LiteralDataTests(unittest.TestCase):
    def test_numeric_pointer_word_fails_but_r_mips_32_passes(self) -> None:
        data = words(0x800162A4)
        findings, _reviewed, _evidence = audit.analyse_data_section(
            data,
            {},
            model(),
            object_name="globals.c.o",
            section_name=".data",
        )
        self.assertEqual(len(findings), 1)
        self.assertEqual(findings[0].kind, "literal_alloc_data_main_address")

        findings, _reviewed, evidence = audit.analyse_data_section(
            data,
            {0: {audit.R_MIPS_32}},
            model(),
            object_name="globals.c.o",
            section_name=".data",
        )
        self.assertEqual(findings, [])
        self.assertEqual(evidence["symbolic_data_words"], 1)

    def test_packed_compiled_pointer_is_scanned_at_its_exact_offset(self) -> None:
        data = b"\x7f" + words(0x800162A4)
        findings, _reviewed, _evidence = audit.analyse_data_section(
            data,
            {},
            model(),
            object_name="packed_globals.c.o",
            section_name=".data",
        )
        self.assertEqual(len(findings), 1)
        self.assertEqual(findings[0].offset, "0x1")

        findings, _reviewed, evidence = audit.analyse_data_section(
            data,
            {1: {audit.R_MIPS_32}},
            model(),
            object_name="packed_globals.c.o",
            section_name=".data",
        )
        self.assertEqual(findings, [])
        self.assertEqual(evidence["symbolic_data_words"], 1)

    def test_compiled_data_covers_pool_but_packed_asm_avoids_noise(self) -> None:
        data = words(0x800E8000)
        compiled_findings, _reviewed, _evidence = audit.analyse_data_section(
            data,
            {},
            model(),
            object_name="new_global.c.o",
            section_name=".data",
        )
        raw_findings, _reviewed, _evidence = audit.analyse_data_section(
            data,
            {},
            model(),
            object_name="packed.data.s.o",
            section_name=".data",
        )
        self.assertEqual(len(compiled_findings), 1)
        self.assertEqual(raw_findings, [])

    def test_only_two_psx_header_words_are_exempt(self) -> None:
        data = bytearray(0x20)
        struct.pack_into("<I", data, 0x10, 0x80060268)
        struct.pack_into("<I", data, 0x18, 0x80011000)
        findings, reviewed, _evidence = audit.analyse_data_section(
            bytes(data),
            {},
            model(),
            object_name=".shake/build/main.exe/header.s.o",
            section_name=".data",
        )
        self.assertEqual(findings, [])
        self.assertEqual(reviewed["psx_header_placeholder"], 2)

        findings, _reviewed, _evidence = audit.analyse_data_section(
            bytes(data),
            {},
            model(),
            object_name="elsewhere/header.s.o",
            section_name=".data",
        )
        self.assertEqual(len(findings), 2)

        struct.pack_into("<I", data, 0x10, 0x8006026C)
        findings, _reviewed, _evidence = audit.analyse_data_section(
            bytes(data),
            {},
            model(),
            object_name=".shake/build/main.exe/header.s.o",
            section_name=".data",
        )
        self.assertEqual(len(findings), 1)
        self.assertEqual(findings[0].offset, "0x10")

    def test_changed_persistent_policy_rejects_retail_literal_data(self) -> None:
        persistent = audit.PersistentOracle(
            retail_base=0x80010000,
            retail_rng=0x80010E70,
            target_base=0x80008000,
            target_rng=0x80008F00,
        )
        findings, _reviewed, _evidence = audit.analyse_data_section(
            words(0x8001046C),
            {},
            model(),
            object_name="stale.data.s.o",
            section_name=".data",
            persistent_addresses=persistent,
        )
        self.assertEqual(len(findings), 1)
        self.assertEqual(findings[0].kind, "stale_retail_persistent_address")
        self.assertIn("0x8000846c", findings[0].detail)

        findings, _reviewed, _evidence = audit.analyse_data_section(
            words(0x8001046C),
            {0: {audit.R_MIPS_32}},
            model(),
            object_name="symbolic.data.s.o",
            section_name=".data",
            persistent_addresses=persistent,
        )
        self.assertEqual(findings, [])


class LinkerInventoryTests(unittest.TestCase):
    def test_persistent_oracle_comes_from_retail_symbol_input(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            config = root / "config"
            config.mkdir()
            (config / "symbols.main.exe.txt").write_text(
                "PersistentState = 0x80010000;\n"
                "STARTING_RNG_SEED = 0x80010e70;\n"
            )
            persistent = audit.persistent_oracle(root)
        self.assertEqual(persistent.retail_base, 0x80010000)
        self.assertEqual(persistent.retail_rng, 0x80010E70)
        self.assertEqual(persistent.target_base, audit.PERSISTENT_START)

    def test_exact_and_glob_selectors_union_and_comments_do_not_link(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            build = root / "build"
            build.mkdir()
            (build / "one.c.o").touch()
            (build / "two.c.o").touch()
            linker = """\
/* build/stale.c.o(.text); */
build/one.c.o(.text);
build/*.c.o(.sdata .sdata.*);
"""
            selections = audit.linked_selections(linker, root)
        by_name = {Path(item.display).name: item.patterns for item in selections}
        self.assertEqual(set(by_name), {"one.c.o", "two.c.o"})
        self.assertEqual(
            set(by_name["one.c.o"]), {".text", ".sdata", ".sdata.*"}
        )
        self.assertEqual(set(by_name["two.c.o"]), {".sdata", ".sdata.*"})

    def test_map_load_inventory_filters_stale_wildcard_matches(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            build = root / "build"
            build.mkdir()
            (build / "live.c.o").touch()
            (build / "stale.c.o").touch()
            selections = audit.linked_selections(
                "build/*.c.o(.sdata .sdata.*);", root
            )
            loaded = audit.loaded_object_paths("LOAD build/live.c.o\n", root)
            filtered = audit.filter_loaded_selections(selections, loaded, root)
        self.assertEqual([Path(item.display).name for item in filtered], ["live.c.o"])

    def test_ld_object_wildcard_matches_nested_paths(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            top = root / "build/reloc/top.c.o"
            nested = root / "build/reloc/nested/live.c.o"
            top.parent.mkdir(parents=True)
            nested.parent.mkdir(parents=True)
            top.touch()
            nested.touch()

            selections = audit.linked_selections(
                "build/reloc/*.c.o(.text);", root
            )

        self.assertEqual(
            [Path(item.display) for item in selections],
            [
                Path("build/reloc/nested/live.c.o"),
                Path("build/reloc/top.c.o"),
            ],
        )

    def test_map_object_absent_from_linker_inventory_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            linked = root / "linked.o"
            linked.touch()
            other = root / "other.o"
            other.touch()
            selections = audit.linked_selections("linked.o(.text);", root)
            loaded = audit.loaded_object_paths("LOAD other.o\n", root)
            with self.assertRaisesRegex(audit.AuditError, "absent"):
                audit.filter_loaded_selections(selections, loaded, root)

    def test_help_states_the_gate_limits(self) -> None:
        help_text = audit.parser().format_help()
        self.assertIn("R_MIPS", help_text)
        self.assertIn("unaligned", help_text)
        self.assertIn("+0x10004", help_text)


class AllocSectionOwnershipTests(unittest.TestCase):
    @staticmethod
    def section(
        index: int,
        name: str,
        *,
        section_type: int = audit.SHT_PROGBITS,
        flags: int = audit.SHF_ALLOC,
        size: int = 4,
        alignment: int = 4,
        entry_size: int = 0,
    ) -> audit.Section:
        return audit.Section(
            index=index,
            name=name,
            type=section_type,
            flags=flags,
            address=0,
            offset=0,
            size=size,
            link=0,
            info=0,
            alignment=alignment,
            entry_size=entry_size,
        )

    def test_alloc_debug_payload_outside_runtime_ownership_fails(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "fault.c.o"
            make_relocatable_elf(
                path,
                b"",
                [],
                extra_alloc_section=(".debug_payload", words(0x12345678)),
            )
            elf = audit.Elf32(path)
            text_index = next(
                section.index for section in elf.sections if section.name == ".text"
            )
            findings, reviewed = audit.audit_alloc_section_ownership(
                elf.sections, {text_index}, object_name="fault.c.o"
            )
        self.assertEqual(reviewed, {})
        self.assertEqual(len(findings), 1)
        self.assertEqual(findings[0].kind, "unowned_alloc_section")
        self.assertEqual(findings[0].section, ".debug_payload")

    def test_selected_alloc_section_with_unsupported_type_fails(self) -> None:
        init_array = self.section(
            1,
            ".data",
            section_type=audit.SHT_INIT_ARRAY,
            size=4,
        )
        findings, reviewed = audit.audit_alloc_section_ownership(
            [init_array],
            {init_array.index},
            object_name="constructors.c.o",
        )
        self.assertEqual(reviewed, {})
        self.assertEqual(len(findings), 1)
        self.assertEqual(
            findings[0].kind,
            "unsupported_owned_alloc_section",
        )

    def test_only_structurally_exact_mips_alloc_metadata_passes(self) -> None:
        valid_reginfo = self.section(
            1,
            ".reginfo",
            section_type=audit.SHT_MIPS_REGINFO,
            size=24,
            alignment=4,
            entry_size=24,
        )
        valid_abiflags = self.section(
            2,
            ".MIPS.abiflags",
            section_type=audit.SHT_MIPS_ABIFLAGS,
            size=24,
            alignment=8,
            entry_size=24,
        )
        malformed_reginfo = self.section(
            3,
            ".reginfo",
            section_type=audit.SHT_MIPS_REGINFO,
            size=28,
            alignment=4,
            entry_size=24,
        )
        findings, reviewed = audit.audit_alloc_section_ownership(
            [valid_reginfo, valid_abiflags, malformed_reginfo],
            set(),
            object_name="metadata.o",
        )
        self.assertEqual(
            reviewed,
            {".reginfo": 1, ".MIPS.abiflags": 1},
        )
        self.assertEqual(len(findings), 1)
        self.assertEqual(findings[0].section, ".reginfo")

    def test_only_expected_empty_alloc_companions_are_zero_size_owned(self) -> None:
        expected_empty = self.section(1, ".text", size=0)
        unknown_empty = self.section(2, ".debug_payload", size=0)
        findings, reviewed = audit.audit_alloc_section_ownership(
            [expected_empty, unknown_empty],
            set(),
            object_name="empty.o",
        )
        self.assertEqual(reviewed, {})
        self.assertEqual(len(findings), 1)
        self.assertEqual(findings[0].section, ".debug_payload")


class CurrentRelinkIntegrationTests(unittest.TestCase):
    def test_current_loaded_inputs_have_no_unowned_alloc_runtime_bytes(self) -> None:
        required = (
            audit.ROOT / audit.DEFAULT_LINKER,
            audit.ROOT / audit.DEFAULT_ELF,
            audit.ROOT / audit.DEFAULT_MAP,
        )
        if not all(path.is_file() for path in required):
            self.skipTest("normal relink artifacts have not been built")

        report = audit.collect(
            audit.ROOT,
            audit.DEFAULT_LINKER,
            audit.DEFAULT_ELF,
            audit.DEFAULT_MAP,
        )
        findings = report["findings"]
        self.assertIsInstance(findings, list)
        self.assertEqual(
            [item for item in findings if item["kind"] == "unowned_alloc_section"],
            [],
        )
        counts = report["counts"]
        self.assertIsInstance(counts, dict)
        self.assertGreater(counts["reviewed_alloc_metadata_sections"], 0)


if __name__ == "__main__":
    unittest.main()

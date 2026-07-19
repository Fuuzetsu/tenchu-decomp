#!/usr/bin/env python3
"""Hash-gate and link one user-supplied PsyQ library member.

This is deliberately an opt-in audit lane.  It never downloads an SDK and it
never changes the normal Splat/Shake build graph.  Generated proprietary and
converted files stay below ``.shake/psyq`` (which is gitignored).
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import tempfile
from typing import Any, Iterable


ROOT = Path(__file__).resolve().parents[1]
RELOCATION_NAMES = {
    4: "R_MIPS_26",
    5: "R_MIPS_HI16",
    6: "R_MIPS_LO16",
}
BINDING_NAMES = {
    0: "LOCAL",
    1: "GLOBAL",
    2: "WEAK",
}
SHT_NOBITS = 8
SHN_UNDEF = 0
SHN_ABS = 0xFFF1
SHN_COMMON = 0xFFF2


class LaneError(RuntimeError):
    """An audit invariant failed."""


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def parse_int(value: int | str) -> int:
    if isinstance(value, int):
        return value
    return int(value, 0)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise LaneError(message)


def require_tool(name: str) -> str:
    path = shutil.which(name)
    if path is None:
        raise LaneError(
            f"required tool {name!r} is not on PATH; run inside `nix develop`"
        )
    return path


def run(command: Iterable[str], *, cwd: Path = ROOT) -> None:
    argv = [str(part) for part in command]
    print("+", " ".join(argv), flush=True)
    result = subprocess.run(
        argv,
        cwd=cwd,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if result.stdout:
        print(result.stdout, end="" if result.stdout.endswith("\n") else "\n")
    if result.returncode != 0:
        raise LaneError(
            f"command exited with status {result.returncode}: {' '.join(argv)}"
        )


class ElfFile:
    """Small, strict ELF32 little-endian reader for converted PsyQ objects."""

    def __init__(self, path: Path):
        self.path = path
        self.data = path.read_bytes()
        require(self.data[:4] == b"\x7fELF", f"{path}: not an ELF file")
        require(self.data[4] == 1, f"{path}: expected ELF32")
        require(self.data[5] == 1, f"{path}: expected little-endian ELF")
        require(len(self.data) >= 52, f"{path}: truncated ELF header")

        header = struct.unpack_from("<16sHHIIIIIHHHHHH", self.data, 0)
        self.elf_type = header[1]
        self.machine = header[2]
        section_offset = header[6]
        section_entry_size = header[11]
        section_count = header[12]
        shstr_index = header[13]
        require(section_entry_size == 40, f"{path}: unexpected section entry size")
        require(section_count > 0, f"{path}: ELF has no section table")
        require(shstr_index < section_count, f"{path}: invalid shstrtab index")
        require(
            section_offset + section_count * section_entry_size <= len(self.data),
            f"{path}: truncated section table",
        )

        self.sections: list[dict[str, Any]] = []
        for index in range(section_count):
            values = struct.unpack_from(
                "<IIIIIIIIII", self.data, section_offset + index * 40
            )
            self.sections.append(
                {
                    "index": index,
                    "name_offset": values[0],
                    "type": values[1],
                    "flags": values[2],
                    "address": values[3],
                    "offset": values[4],
                    "size": values[5],
                    "link": values[6],
                    "info": values[7],
                    "alignment": values[8],
                    "entry_size": values[9],
                }
            )

        shstr = self.sections[shstr_index]
        names = self._section_bytes(shstr)
        for section in self.sections:
            section["name"] = self._cstring(names, section["name_offset"])
        self.sections_by_name = {section["name"]: section for section in self.sections}

        self.symbols: list[dict[str, Any]] = []
        symtab = self.sections_by_name.get(".symtab")
        if symtab is not None:
            require(symtab["entry_size"] == 16, f"{path}: unexpected symbol size")
            require(symtab["link"] < len(self.sections), f"{path}: bad symtab strtab")
            strings = self._section_bytes(self.sections[symtab["link"]])
            raw_symbols = self._section_bytes(symtab)
            require(
                len(raw_symbols) % 16 == 0, f"{path}: truncated symbol table"
            )
            for index in range(len(raw_symbols) // 16):
                name_offset, value, size, info, other, shndx = struct.unpack_from(
                    "<IIIBBH", raw_symbols, index * 16
                )
                section_name = self._symbol_section_name(shndx)
                self.symbols.append(
                    {
                        "index": index,
                        "name": self._cstring(strings, name_offset),
                        "value": value,
                        "size": size,
                        "binding": BINDING_NAMES.get(info >> 4, str(info >> 4)),
                        "type": info & 0xF,
                        "other": other,
                        "section_index": shndx,
                        "section": section_name,
                    }
                )

            first_nonlocal = next(
                (
                    symbol["index"]
                    for symbol in self.symbols
                    if symbol["binding"] != "LOCAL"
                ),
                len(self.symbols),
            )
            require(
                symtab["info"] == first_nonlocal,
                f"{path}: malformed symtab sh_info={symtab['info']}, "
                f"expected first non-local index {first_nonlocal}",
            )
            require(
                all(
                    symbol["binding"] != "LOCAL"
                    for symbol in self.symbols[first_nonlocal:]
                ),
                f"{path}: local symbols occur after global symbols",
            )

    @staticmethod
    def _cstring(data: bytes, offset: int) -> str:
        require(offset <= len(data), "string-table offset is out of range")
        end = data.find(b"\0", offset)
        require(end != -1, "unterminated ELF string")
        return data[offset:end].decode("ascii")

    def _section_bytes(self, section: dict[str, Any]) -> bytes:
        if section["type"] == SHT_NOBITS:
            return b""
        start = section["offset"]
        end = start + section["size"]
        require(end <= len(self.data), f"{self.path}: truncated {section.get('name', 'section')}")
        return self.data[start:end]

    def section_bytes(self, name: str) -> bytes:
        section = self.sections_by_name.get(name)
        require(section is not None, f"{self.path}: missing {name} section")
        return self._section_bytes(section)

    def _symbol_section_name(self, index: int) -> str:
        if index == SHN_UNDEF:
            return "UND"
        if index == SHN_ABS:
            return "ABS"
        if index == SHN_COMMON:
            return "COMMON"
        require(index < len(self.sections), f"{self.path}: bad symbol section {index}")
        return str(self.sections[index].get("name", index))

    def named_symbol(self, name: str) -> dict[str, Any]:
        matches = [symbol for symbol in self.symbols if symbol["name"] == name]
        require(len(matches) == 1, f"{self.path}: expected one symbol {name}, got {len(matches)}")
        return matches[0]

    def relocations(self, section_name: str = ".rel.text") -> list[dict[str, Any]]:
        section = self.sections_by_name.get(section_name)
        require(section is not None, f"{self.path}: missing {section_name}")
        require(section["entry_size"] == 8, f"{self.path}: unexpected relocation size")
        raw = self._section_bytes(section)
        require(len(raw) % 8 == 0, f"{self.path}: truncated relocations")
        relocations = []
        for entry in range(len(raw) // 8):
            offset, info = struct.unpack_from("<II", raw, entry * 8)
            symbol_index = info >> 8
            reloc_type = info & 0xFF
            require(symbol_index < len(self.symbols), f"{self.path}: bad relocation symbol")
            symbol = self.symbols[symbol_index]
            target = symbol["name"]
            if not target:
                target = f"{symbol['section']}+0x{symbol['value']:x}"
            relocations.append(
                {
                    "offset": offset,
                    "type": RELOCATION_NAMES.get(reloc_type, str(reloc_type)),
                    "target": target,
                }
            )
        return relocations


def load_manifest(path: Path) -> dict[str, Any]:
    try:
        manifest = json.loads(path.read_text())
    except (OSError, json.JSONDecodeError) as error:
        raise LaneError(f"cannot read manifest {path}: {error}") from error
    require(manifest.get("schema") == 1, f"{path}: unsupported manifest schema")
    require(manifest.get("object") == "GS_107", f"{path}: this lane only accepts GS_107")
    return manifest


def validate_archive(archive: Path, manifest: dict[str, Any]) -> str:
    require(archive.is_file(), f"PsyQ archive does not exist: {archive}")
    actual = sha256(archive)
    accepted = manifest["archive"]["accepted_sha256"]
    versions = [name for name, expected in accepted.items() if expected == actual]
    require(
        len(versions) == 1,
        f"{archive}: sha256 {actual} is not an accepted PsyQ 4.4/4.5/4.6 LIBGS.LIB",
    )
    return versions[0]


def validate_member(member: Path, manifest: dict[str, Any]) -> None:
    expected = manifest["archive"]
    actual_size = member.stat().st_size
    require(
        actual_size == expected["member_size"],
        f"{member}: size {actual_size}, expected {expected['member_size']}",
    )
    actual_hash = sha256(member)
    require(
        actual_hash == expected["member_sha256"],
        f"{member}: sha256 {actual_hash}, expected {expected['member_sha256']}",
    )


def expected_relocations(entries: list[dict[str, Any]]) -> list[dict[str, Any]]:
    return [
        {
            "offset": parse_int(entry["offset"]),
            "type": entry["type"],
            "target": entry["target"],
        }
        for entry in entries
    ]


def validate_object(elf: ElfFile, manifest: dict[str, Any]) -> None:
    expected = manifest["elf"]
    require(elf.elf_type == 1, f"{elf.path}: expected relocatable ELF")
    require(elf.machine == expected["machine"], f"{elf.path}: wrong ELF machine")

    for name in (".text", ".bss"):
        section = elf.sections_by_name.get(name)
        require(section is not None, f"{elf.path}: missing {name}")
        section_expected = expected[name.removeprefix(".")]
        require(
            section["size"] == parse_int(section_expected["size"]),
            f"{elf.path}: {name} size 0x{section['size']:x} is not "
            f"{section_expected['size']}",
        )
        require(
            section["alignment"] == section_expected["alignment"],
            f"{elf.path}: {name} alignment {section['alignment']} is not "
            f"{section_expected['alignment']}",
        )

    text_hash = hashlib.sha256(elf.section_bytes(".text")).hexdigest()
    require(
        text_hash == expected["text"]["unlinked_sha256"],
        f"{elf.path}: unlinked .text sha256 {text_hash} is not the manifest value",
    )

    for name, wanted in expected["symbols"].items():
        actual = elf.named_symbol(name)
        for key in ("section", "size", "binding"):
            require(
                actual[key] == wanted[key],
                f"{elf.path}: {name} {key}={actual[key]!r}, expected {wanted[key]!r}",
            )
        require(
            actual["value"] == parse_int(wanted["value"]),
            f"{elf.path}: {name} value 0x{actual['value']:x}, expected {wanted['value']}",
        )

    local_values = sorted(
        symbol["value"]
        for symbol in elf.symbols
        if symbol["binding"] == "LOCAL"
        and symbol["section"] == ".text"
        and symbol["type"] == 0
        and not symbol["name"]
    )
    wanted_locals = sorted(parse_int(value) for value in expected["local_text_symbols"])
    require(
        local_values == wanted_locals,
        f"{elf.path}: local text symbols {local_values!r}, expected {wanted_locals!r}",
    )

    actual_relocs = elf.relocations()
    wanted_relocs = expected_relocations(expected["relocations"])
    require(
        actual_relocs == wanted_relocs,
        f"{elf.path}: relocation table differs from the manifest\n"
        f"actual:   {actual_relocs!r}\nexpected: {wanted_relocs!r}",
    )


def replace_once(text: str, old: str, new: str, *, description: str) -> str:
    count = text.count(old)
    require(count == 1, f"expected one {description}, found {count}: {old}")
    return text.replace(old, new)


def linker_path(path: Path, repo_root: Path) -> str:
    try:
        shown = path.resolve().relative_to(repo_root.resolve()).as_posix()
    except ValueError:
        shown = path.resolve().as_posix()
    require(not re.search(r"\s", shown), f"linker input path contains whitespace: {shown}")
    return shown


def rewrite_linker_script(
    source: str,
    link: dict[str, Any],
    object_path: str,
    tail_object_path: str,
) -> str:
    require(
        ".main_exe 0x80011000 : AT(main_exe_ROM_START) SUBALIGN(4)" in source,
        "generated linker script no longer gives .main_exe SUBALIGN(4)",
    )
    indent = "        "
    source = replace_once(
        source,
        indent + link["text_owner"],
        indent + f"{object_path}(.text);",
        description="GS_107 text owner",
    )
    for owner, sections in link["remove_objects"].items():
        for section in sections:
            source = replace_once(
                source,
                indent + f"{owner}({section});",
                "",
                description=f"obsolete {owner} {section} owner",
            )
    source = replace_once(
        source,
        indent + link["tail_owner"],
        indent + f"{tail_object_path}(.data);",
        description="post-GS_107 raw owner",
    )

    discard = "    /DISCARD/ :\n"
    bss = (
        f"    .psyq_gs107_bss 0x{parse_int(link['bss_vram']):08x} "
        f"(NOLOAD) : SUBALIGN(4)\n"
        "    {\n"
        f"        {object_path}(.bss COMMON);\n"
        "    }\n\n"
    )
    source = replace_once(
        source, discard, bss + discard, description="/DISCARD/ section"
    )
    return source


def strip_assignments(
    source: str, symbols: Iterable[str], *, required: Iterable[str] = ()
) -> str:
    required_set = set(required)
    lines = source.splitlines(keepends=True)
    for symbol in symbols:
        pattern = re.compile(rf"^\s*{re.escape(symbol)}\s*=.*;\s*(?:#.*)?$")
        matches = [index for index, line in enumerate(lines) if pattern.match(line.rstrip("\n"))]
        if symbol in required_set:
            require(len(matches) == 1, f"expected one assignment for {symbol}, found {len(matches)}")
        else:
            require(len(matches) <= 1, f"expected at most one assignment for {symbol}")
        for index in matches:
            lines[index] = ""
    return "".join(lines)


def validate_input_image(path: Path, link: dict[str, Any], description: str) -> None:
    require(path.is_file(), f"missing {description}: {path}")
    require(
        path.stat().st_size == link["expected_exe_size"],
        f"{description} has wrong size: {path}",
    )
    actual = sha256(path)
    require(
        actual == link["expected_exe_sha256"],
        f"{description} {path} has sha256 {actual}, expected {link['expected_exe_sha256']}",
    )


def first_difference(left: bytes, right: bytes) -> int | None:
    for offset, (a, b) in enumerate(zip(left, right)):
        if a != b:
            return offset
    if len(left) != len(right):
        return min(len(left), len(right))
    return None


def audit_output_name(output_dir: Path, repo_root: Path) -> str:
    ignored_root = (repo_root / ".shake/psyq").resolve()
    try:
        relative = output_dir.resolve().relative_to(ignored_root)
    except ValueError as error:
        raise LaneError(
            f"audit output must stay below the ignored {ignored_root}: {output_dir}"
        ) from error
    return (Path(".shake/psyq") / relative).as_posix()


def validate_linked_elf(elf: ElfFile, manifest: dict[str, Any]) -> None:
    link = manifest["link"]
    text = elf.sections_by_name.get(".main_exe")
    bss = elf.sections_by_name.get(".psyq_gs107_bss")
    require(text is not None, f"{elf.path}: missing .main_exe")
    require(bss is not None, f"{elf.path}: missing .psyq_gs107_bss")
    require(bss["type"] == SHT_NOBITS, f"{elf.path}: GS_107 BSS is not NOLOAD/NOBITS")
    require(bss["address"] == parse_int(link["bss_vram"]), f"{elf.path}: wrong BSS address")
    require(bss["size"] == parse_int(manifest["elf"]["bss"]["size"]), f"{elf.path}: wrong BSS size")

    expected_symbols = {
        "GsSetFlatLight": (".main_exe", parse_int(link["text_vram"])),
        "GsLIGHTWSMATRIX": (".psyq_gs107_bss", parse_int(link["bss_vram"])),
        "_LC": (".psyq_gs107_bss", parse_int(link["bss_vram"]) + 0x20),
    }
    for name, (section, value) in expected_symbols.items():
        symbol = elf.named_symbol(name)
        require(symbol["section"] == section, f"{elf.path}: {name} is not owned by {section}")
        require(symbol["value"] == value, f"{elf.path}: {name} has wrong linked address")


def build_lane(
    archive: Path,
    manifest_path: Path,
    output_dir: Path,
    repo_root: Path = ROOT,
) -> None:
    manifest = load_manifest(manifest_path)
    archive_version = validate_archive(archive, manifest)
    link = manifest["link"]
    output_name = audit_output_name(output_dir, repo_root)

    target = repo_root / link["target"]
    baseline = repo_root / link["baseline"]
    validate_input_image(target, link, "reference executable")
    validate_input_image(baseline, link, "normal matching build")

    psyqdump = require_tool("psyqdump")
    psyq2elf = require_tool("psyq2elf")
    objcopy = require_tool("mipsel-unknown-linux-gnu-objcopy")
    ld = require_tool("mipsel-unknown-linux-gnu-ld")

    output_dir.mkdir(parents=True, exist_ok=True)
    member_name = manifest["archive"]["member"]
    member = output_dir / member_name
    raw_object = output_dir / "GS_107.raw.o"
    fixed_object = output_dir / "GS_107.o"
    with tempfile.TemporaryDirectory(prefix="extract-", dir=output_dir) as temporary:
        extracted = Path(temporary) / "members"
        extracted.mkdir()
        run([psyqdump, archive.resolve(), extracted.resolve()], cwd=repo_root)
        extracted_member = extracted / member_name
        require(extracted_member.is_file(), f"{archive}: does not contain {member_name}")
        validate_member(extracted_member, manifest)
        shutil.copyfile(extracted_member, member)

    run([psyq2elf, member, raw_object], cwd=repo_root)
    # Upstream psyq2elf emits local symbols after globals and writes a bad
    # .symtab sh_info.  A no-op GNU objcopy canonicalises the table; ElfFile
    # explicitly verifies the repaired local/global boundary below.
    run([objcopy, raw_object, fixed_object], cwd=repo_root)
    validate_object(ElfFile(fixed_object), manifest)

    target_bytes = target.read_bytes()
    tail_start = parse_int(link["tail_source_start"])
    tail_end = parse_int(link["tail_source_end"])
    require(0 <= tail_start < tail_end <= len(target_bytes), "invalid tail range")
    tail_bin = output_dir / "post-GS_107.bin"
    tail_object = output_dir / "post-GS_107.o"
    tail_bin.write_bytes(target_bytes[tail_start:tail_end])
    run([ld, "-EL", "-r", "-b", "binary", "-o", tail_object, tail_bin], cwd=repo_root)

    linker_source_path = repo_root / link["generated_linker_script"]
    symbols_source_path = repo_root / link["symbols_script"]
    undefined_source_path = repo_root / link["undefined_symbols_script"]
    functions_source_path = repo_root / link["undefined_functions_script"]
    for path in (
        linker_source_path,
        symbols_source_path,
        undefined_source_path,
        functions_source_path,
    ):
        require(path.is_file(), f"missing generated link input: {path}")

    object_link_path = linker_path(fixed_object, repo_root)
    tail_link_path = linker_path(tail_object, repo_root)
    alternate_linker = output_dir / "main.exe.ld"
    alternate_symbols = output_dir / "symbols.main.exe.txt"
    alternate_undefined = output_dir / "undefined_symbols_auto.main.exe.txt"
    alternate_linker.write_text(
        rewrite_linker_script(
            linker_source_path.read_text(), link, object_link_path, tail_link_path
        )
    )
    object_symbols = link["object_defined_symbols"]
    alternate_symbols.write_text(
        strip_assignments(
            symbols_source_path.read_text(), object_symbols, required=object_symbols
        )
    )
    alternate_undefined.write_text(
        strip_assignments(undefined_source_path.read_text(), object_symbols)
    )

    output_elf = output_dir / "main.exe.elf"
    output_map = output_dir / "main.exe.map"
    output_exe = output_dir / "main.exe"
    run(
        [
            ld,
            "-EL",
            "-o",
            output_elf,
            "-Map",
            output_map,
            "-T",
            alternate_linker,
            "-T",
            alternate_symbols,
            "-T",
            alternate_undefined,
            "-T",
            functions_source_path,
            "--no-check-sections",
            "-nostdlib",
        ],
        cwd=repo_root,
    )
    validate_linked_elf(ElfFile(output_elf), manifest)
    run([objcopy, "-O", "binary", output_elf, output_exe], cwd=repo_root)

    linked_bytes = output_exe.read_bytes()
    difference = first_difference(target_bytes, linked_bytes)
    if difference is not None:
        raise LaneError(
            "alternate object link is not byte-identical: first difference at "
            f"file offset 0x{difference:x}, sizes {len(target_bytes)} and "
            f"{len(linked_bytes)}"
        )
    text_start = parse_int(link["text_file_offset"])
    text_size = parse_int(manifest["elf"]["text"]["size"])
    linked_text_hash = hashlib.sha256(
        linked_bytes[text_start : text_start + text_size]
    ).hexdigest()
    require(
        linked_text_hash == link["linked_text_sha256"],
        f"linked GS_107 text sha256 {linked_text_hash} is not the manifest value",
    )

    print(
        f"check-psyq-gs107: {archive_version} {member_name} is hash-gated, "
        "relocatable, BSS-preserving, and main.exe byte-identical"
    )
    print(f"generated audit artifacts: {output_name}")


def arguments(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--archive",
        type=Path,
        default=os.environ.get("TENCHU_PSYQ_LIBGS"),
        required="TENCHU_PSYQ_LIBGS" not in os.environ,
        help="user-supplied PsyQ LIBGS.LIB (or set TENCHU_PSYQ_LIBGS)",
    )
    parser.add_argument(
        "--manifest",
        type=Path,
        default=ROOT / "config/psyq-objects.main.exe.json",
    )
    parser.add_argument(
        "--output-dir", type=Path, default=ROOT / ".shake/psyq/GS_107"
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = arguments(argv)
    try:
        build_lane(
            args.archive.resolve(),
            args.manifest.resolve(),
            args.output_dir.resolve(),
        )
    except LaneError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

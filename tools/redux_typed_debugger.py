#!/usr/bin/env python3
"""Emit PCSX-Redux Typed Debugger import files from recovered types.

The Typed Debugger reads two semicolon-delimited files (its own format,
separate from the address->name symbol map that names disassembly):

  data types:  Name;type,field,size;type,field,size;...
  functions:   addr;name;type,arg,size;...        (up to 4 args)

This turns the committed recovered types into those files:

  - data types come from reference/psxsym-types.h (struct field offsets give
    exact per-field sizes as consecutive deltas);
  - functions come from reference/psxsym-protos.h joined to the launched
    artifact's ELF, so addresses match whatever layout is running (exact,
    mod, relink).

Compatibility with the widget's resolver (src/gui/widgets/typed_debugger.cc):
field/argument type strings use the BARE struct name so nesting matches
m_structs, pointers are "Base *" (the widget dereferences by stripping the
trailing " *"), and multi-word C primitives are normalised to the single
tokens isPrimitive() accepts.

    tools/redux_typed_debugger.py --elf main.exe.elf \
        --types-out main.exe.redux_data_types.txt \
        --funcs-out main.exe.redux_funcs.txt
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import re
import sys

try:
    from tools import psxexe
except ImportError:  # Direct execution.
    import psxexe  # type: ignore[no-redef]


ROOT = Path(__file__).resolve().parent.parent
TYPES_H = ROOT / "reference" / "psxsym-types.h"
PROTOS_H = ROOT / "reference" / "psxsym-protos.h"

# Multi-word C spellings -> the single tokens typed_debugger's isPrimitive()
# accepts (it never sees "unsigned char"). char stays char (handled as char*).
PRIMITIVE_ALIASES = {
    "unsigned char": "u_char",
    "unsigned short": "u_short",
    "unsigned int": "u_int",
    "unsigned long": "u_long",
    "signed char": "char",
    "u8": "u_char", "s8": "char",
    "u16": "u_short", "s16": "short",
    "u32": "u_long", "s32": "long",
}
PRIMITIVE_SIZES = {
    "char": 1, "u_char": 1, "uchar": 1,
    "short": 2, "u_short": 2, "ushort": 2,
    "int": 4, "u_int": 4, "uint": 4,
    "long": 4, "u_long": 4, "ulong": 4,
    "float": 4, "double": 8, "void": 0,
}


@dataclass
class Field:
    type: str
    name: str
    size: int


@dataclass
class Struct:
    name: str
    size: int
    fields: list[Field]
    is_union: bool


def normalize_base(ctype: str) -> str:
    """Bare, resolver-friendly spelling of a non-pointer C type."""
    ctype = ctype.strip()
    ctype = re.sub(r"^(struct|union|enum)\s+", "", ctype)
    return PRIMITIVE_ALIASES.get(ctype, ctype)


def parse_declaration(decl: str) -> tuple[str, str] | None:
    """Split a C declaration 'type name' into (type_string, name).

    Pointers become 'Base *'; arrays fold [N] into the type string as the
    widget's array regex expects; the bare identifier is the field/arg name.
    """
    decl = decl.strip().rstrip(";").strip()
    if not decl or decl == "void":
        return None
    array = ""
    array_match = re.search(r"(\[[0-9\]\[]*\])\s*$", decl)
    if array_match:
        array = array_match.group(1)
        decl = decl[: array_match.start()].strip()
    name_match = re.search(r"([A-Za-z_]\w*)$", decl)
    if not name_match:
        return None
    name = name_match.group(1)
    base = decl[: name_match.start()].strip()
    stars = ""
    while base.endswith("*"):
        stars += "*"
        base = base[:-1].strip()
    base = normalize_base(base)
    if stars:
        type_string = f"{base} " + " ".join("*" * len(stars))
        type_string = f"{base} {stars}"
    else:
        type_string = base
    return type_string + array, name


def parse_types(text: str) -> dict[str, Struct]:
    structs: dict[str, Struct] = {}
    header = re.compile(r"^(struct|union)\s+(\w+)\s*\{\s*/\*\s*size (\d+)")
    field = re.compile(r"^\s*(.+?);\s*/\*\s*\+0x([0-9a-fA-F]+)")
    current: Struct | None = None
    pending: list[tuple[str, str, int]] = []  # (type, name, offset)
    for line in text.splitlines():
        head = header.match(line)
        if head:
            kind, name, size = head.group(1), head.group(2), int(head.group(3))
            current = Struct(name, size, [], kind == "union")
            pending = []
            continue
        if current is None:
            continue
        if line.startswith("}"):
            _finish_struct(current, pending, structs)
            current = None
            continue
        fmatch = field.match(line)
        if fmatch:
            decl = parse_declaration(fmatch.group(1))
            if decl:
                pending.append((decl[0], decl[1], int(fmatch.group(2), 16)))
    return structs


def _finish_struct(struct: Struct, pending: list[tuple[str, str, int]],
                   structs: dict[str, Struct]) -> None:
    for index, (type_string, name, offset) in enumerate(pending):
        if struct.is_union:
            # Union fields all sit at offset 0, so a delta is meaningless;
            # each field's size is its own type's size.
            size = _sizeof(type_string, structs)
        elif index + 1 < len(pending):
            size = pending[index + 1][2] - offset
        else:
            size = struct.size - offset
        if size <= 0:
            size = _sizeof(type_string, structs)
        struct.fields.append(Field(type_string, name, size))
    structs[struct.name] = struct


def _sizeof(type_string: str, structs: dict[str, Struct]) -> int:
    array = re.search(r"\[(\d+)\]", type_string)
    count = int(array.group(1)) if array else 1
    base = re.sub(r"\[[0-9\]\[]*\]", "", type_string).strip()
    if base.endswith("*"):
        return 4 * count
    base = normalize_base(base)
    if base in PRIMITIVE_SIZES:
        return PRIMITIVE_SIZES[base] * count
    if base in structs:
        return structs[base].size * count
    return 4 * count  # register-width default keeps display sane


def emit_types(structs: dict[str, Struct]) -> str:
    lines = []
    for name in sorted(structs):
        struct = structs[name]
        if not struct.fields:
            continue
        parts = [name] + [
            f"{f.type},{f.name},{f.size}" for f in struct.fields
        ]
        lines.append(";".join(parts) + ";")
    return "\n".join(lines) + "\n"


def split_args(param_string: str) -> list[str]:
    """Split a parameter list on top-level commas (pointers have none)."""
    args, depth, current = [], 0, ""
    for char in param_string:
        if char in "([":
            depth += 1
        elif char in ")]":
            depth -= 1
        if char == "," and depth == 0:
            args.append(current)
            current = ""
        else:
            current += char
    if current.strip():
        args.append(current)
    return args


def parse_protos(text: str) -> dict[str, list[Field]]:
    protos: dict[str, list[Field]] = {}
    pattern = re.compile(r"^[\w\s\*]+?\b(\w+)\s*\((.*)\)\s*;")
    for line in text.splitlines():
        line = re.sub(r"/\*.*?\*/", "", line).strip()
        match = pattern.match(line)
        if not match:
            continue
        name, params = match.group(1), match.group(2).strip()
        args: list[Field] = []
        if params and params != "void":
            for raw in split_args(params):
                decl = parse_declaration(raw)
                if decl:
                    args.append(Field(decl[0], decl[1], 0))
        protos[name] = args
    return protos


def emit_funcs(protos: dict[str, list[Field]], addresses: dict[str, int],
               structs: dict[str, Struct]) -> tuple[str, int]:
    lines = []
    for name in sorted(protos):
        if name not in addresses:
            continue
        parts = [f"{addresses[name]:08x}", name]
        for arg in protos[name][:4]:  # widget reads at most 4 args
            parts.append(f"{arg.type},{arg.name},{_sizeof(arg.type, structs)}")
        lines.append(";".join(parts) + ";")
    return "\n".join(lines) + "\n", len(lines)


def elf_addresses(elf: Path) -> dict[str, int]:
    metadata = psxexe.read_elf_metadata(elf)
    return {
        name: address
        for name, address in metadata.symbols.items()
        if "." not in name and 0x80000000 <= address < 0x80800000
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--elf", type=Path, help="artifact ELF for function addresses")
    parser.add_argument("--types-out", type=Path)
    parser.add_argument("--funcs-out", type=Path)
    args = parser.parse_args(argv)
    if not args.types_out and not args.funcs_out:
        parser.error("need --types-out and/or --funcs-out")

    structs = parse_types(TYPES_H.read_text())
    if args.types_out:
        args.types_out.write_text(emit_types(structs))
        named = sum(1 for s in structs.values() if s.fields)
        print(f"redux-typed: {named} data types -> {args.types_out}")
    if args.funcs_out:
        if not args.elf:
            parser.error("--funcs-out needs --elf for addresses")
        try:
            addresses = elf_addresses(args.elf)
        except (OSError, psxexe.PsxExeError) as error:
            print(f"redux-typed: {error}", file=sys.stderr)
            return 1
        protos = parse_protos(PROTOS_H.read_text())
        text, count = emit_funcs(protos, addresses, structs)
        args.funcs_out.write_text(text)
        print(f"redux-typed: {count} functions -> {args.funcs_out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

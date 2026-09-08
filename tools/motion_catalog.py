#!/usr/bin/env python3
"""Catalog retail animation clips and annotate unregistered motion states.

    python3 tools/motion_catalog.py --write
    python3 tools/motion_catalog.py --check

MOT_* state IDs stay in C; AMD clip IDs live in reference/motion-clips.tsv.
C attack constants take precedence over catalog labels. Other reviewed labels
are retained; new unknown clips get ANIM_* placeholders in the TSV only.
Inputs are main.exe, DATA.VOL (including trial assets), and the current C.
Only evidenced IDs are named: gaps in either numeric space are not motions.
Usage comments are limited to entries without MAIN registrations.
"""

from __future__ import annotations

import argparse
from collections import defaultdict
import csv
import io
import mmap
from pathlib import Path
import re
import struct
import sys
import textwrap

try:
    from . import gamedata, voldump
except ImportError:
    import gamedata
    import voldump


ROOT = Path(__file__).resolve().parent.parent
TYPES = ROOT / "src/main.exe/game_types.h"
CATALOG = ROOT / "reference/motion-clips.tsv"
HUMANOID = ROOT / "src/main.exe/humanoid.h"
INTEGER = r"-?(?:0[xX][0-9a-fA-F]+|[0-9]+)"
DEFINITION = re.compile(rf"\b(MOT_\w+)\s*=\s*({INTEGER})\b")
GENERATED = re.compile(r"    /\* (?:Usage|Unregistered) \(motion_catalog.py\):.*?\*/\n", re.S)
HANDLERS = (
    "ActNORMAL", "ActACTION", "ActMOVE", "ActSWIM", "ActKAGI", "ActENGAGE",
    "ActCHASE", "ActATTACK", "ActSTATE", "ActJUMP", "ActHANG", "ActSQUAT",
    "ActSTICKON", "ActCEILHANG", "ActSYURI", "ActITEM", "ActDAMAGE", "ActDEAD",
)


def c_code(source):
    """Blank comments and literals, retaining offsets for C reference ownership."""
    pattern = r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\''
    return re.sub(pattern, lambda m: re.sub(r"[^\n]", " ", m[0]), source,
                  flags=re.S)


def state_names(source):
    names = {}
    for match in DEFINITION.finditer(c_code(source)):
        value = int(match[2], 0)
        if value in names:
            raise ValueError(f"duplicate motion ID {value:#x}: {names[value]}, {match[1]}")
        names[value] = match[1]
    return names


def character_names(source):
    body = re.search(r"enum character_kind\s*\{(.*?)\};", c_code(source), re.S)
    if body is None:
        raise ValueError("character_kind enum not found")
    return {int(value, 0): name for name, value in
            re.findall(rf"(\w+)\s*=\s*({INTEGER})", body[1])}


def clip_names(source):
    """Read reviewed asset labels without making them C types/constants."""
    rows = csv.DictReader(io.StringIO(source), delimiter="\t")
    if rows.fieldnames != ["id", "name", "note"]:
        raise ValueError("clip catalog must have id, name, note columns")
    names = {}
    for row in rows:
        if None in row or any(value is None for value in row.values()):
            raise ValueError("invalid clip catalog row")
        name, value = row["name"], row["id"]
        if not re.fullmatch(r"[A-Za-z_]\w*", name):
            raise ValueError(f"invalid clip name: {name}")
        clip = int(value, 0)
        if not 0 <= clip <= 0x7fff:
            raise ValueError(f"invalid clip ID: {value}")
        if clip in names or name in names.values():
            raise ValueError(f"duplicate animation clip name/ID: {name} = {value}")
        names[clip] = name
    return names


def attack_clip_names(source):
    """The constants actually used by C remain the authority for their names."""
    names = {}
    for name, value in re.findall(
            rf"(?m)^#define[ \t]+(ATTACK_MOTID_\w+)[ \t]+({INTEGER})[ \t]*$",
            c_code(source)):
        clip = int(value, 0)
        if clip in names or name in names.values():
            raise ValueError(f"duplicate attack clip name/ID: {name} = {value}")
        names[clip] = name
    return names


def volume_files(data):
    """Resolve AFS parent indices, keeping HUMAN and TRIAL assets distinct."""
    paths = []
    elements = voldump.read_elements(data)
    for index, (flag, pos, packed, size, raw_name) in enumerate(elements):
        parent = re.fullmatch(r"@(\d+)_(.*)", raw_name)
        if parent:
            parent_index = int(parent[1])
            if parent_index >= index or elements[parent_index][0] != 2:
                raise ValueError(f"invalid AFS parent: {raw_name}")
            path = paths[parent_index] + "/" + parent[2]
        else:
            path = raw_name
        paths.append(path)
        if flag != 1:
            continue
        if not path.upper().endswith((".AMD", ".CAD", ".ESD")):
            continue
        if packed != size or pos + size > len(data):
            raise ValueError(f"unsupported packed or truncated asset: {path}")
        path = path.removeprefix("K:/WORK/CDIMAGE/")
        yield path, data[pos:pos + size]


def cad_motions(data):
    """Yield effective (mid, actor) references; next_motion==0 selects stance."""
    for offset in range(0, len(data) - 1, 12):
        mode, = struct.unpack_from("<h", data, offset)
        if mode == -1:
            return
        if mode not in range(9) or offset + 12 > len(data):
            raise ValueError(f"invalid CAD command at {offset:#x}")
        _, actor, motion, _, next_motion, _ = struct.unpack_from("<6h", data, offset)
        if mode == 3 and motion != -1:
            if motion < 0 or next_motion < -1:
                raise ValueError(f"invalid CAD motion at {offset:#x}")
            yield motion, actor
            if next_motion != -1:
                yield (0x501 if next_motion == 0 else next_motion), actor
    raise ValueError("CAD has no END sentinel")


def esd_motions(data):
    for offset in range(0, len(data) - 3, 20):
        header, = struct.unpack_from("<i", data, offset)
        if header == -1:
            return
        if offset + 20 > len(data) or data[offset + 5] not in range(9):
            raise ValueError(f"invalid ESD event at {offset:#x}")
        if data[offset + 5] == 4:
            yield struct.unpack_from("<h", data, offset + 6)[0], data[offset + 4]
    raise ValueError("ESD has no END sentinel")


def amd_clips(data):
    if len(data) < 4:
        raise ValueError("truncated AMD header")
    count, = struct.unpack_from("<i", data)
    if count < 0 or 4 + count * 4 > len(data):
        raise ValueError("invalid AMD motion count")
    for index in range(count):
        offset, = struct.unpack_from("<I", data, 4 + index * 4)
        if offset < 4 + count * 4 or offset + 8 > len(data):
            raise ValueError(f"invalid AMD clip offset {offset:#x}")
        clip, = struct.unpack_from("<h", data, offset + 6)
        if clip < 0:
            raise ValueError(f"negative AMD clip ID {clip}")
        yield clip


def symbol_address(name):
    source = (ROOT / "config/symbols.main.exe.txt").read_text()
    match = re.search(rf"(?m)^{re.escape(name)}\s*=\s*(0x[0-9a-fA-F]+);", source)
    if match is None:
        raise ValueError(f"missing retail symbol: {name}")
    return int(match[1], 0)


def source_uses(tokens):
    """Find literal named references by function/macro, excluding definitions."""
    uses = defaultdict(set)
    function = re.compile(r"(?m)^[A-Za-z_][\w \t*]*?\b(\w+)\s*\([^;{}]*\)\s*\{")
    for path in sorted((ROOT / "src/main.exe").rglob("*")):
        if path.suffix.lower() not in (".c", ".h") or path == TYPES:
            continue
        code = c_code(path.read_text())
        regions = []
        for match in function.finditer(code):
            depth = 1
            end = match.end()
            while depth and end < len(code):
                depth += (code[end] == "{") - (code[end] == "}")
                end += 1
            regions.append((match.start(), end, match[1]))
        offset = 0
        definitions = set()
        lines = code.splitlines(keepends=True)
        for index, line in enumerate(lines):
            match = re.match(r"#define\s+(\w+)", line)
            if match:
                definitions.add(offset + match.start(1))
                end = offset + len(line)
                following = index + 1
                while lines[following - 1].rstrip().endswith("\\") and following < len(lines):
                    end += len(lines[following])
                    following += 1
                regions.append((offset + match.end(), end, match[1]))
            offset += len(line)
        relative = str(path.relative_to(ROOT / "src/main.exe"))
        for match in re.finditer(r"\b[A-Za-z_]\w*\b", code):
            if match[0] not in tokens or match.start() in definitions:
                continue
            owner = next((name for start, end, name in regions
                          if start <= match.start() < end), "file scope")
            # Report actual users instead of a constant's own definition.
            if owner in tokens:
                continue
            uses[tokens[match[0]]].add((relative, owner))
    return uses


def collect(source):
    names = state_names(source)
    characters = character_names(source)
    img = gamedata.Image()
    states = {mid: dict(reg=set(), scripts=set(), events=set()) for mid in names}
    clips = defaultdict(lambda: dict(reg=defaultdict(set), assets=set(), battle=set()))

    def state(mid):
        if mid not in states:
            raise ValueError(f"unnamed motion state {mid:#06x}; add a MOT_* constant")
        return states[mid]

    tables = [("MOTcommon", symbol_address("MOTcommon"))]
    tables += [(characters[row["type"]], row["mtbl"])
               for _, row in img.rows("HumanData")]
    for owner, address in tables:
        for mid, clip in img.motion_rows(address):
            state(mid)["reg"].add(owner)
            clips[clip]["reg"][names[mid]].add(owner)

    file_counts = defaultdict(int)
    with voldump.VOL.open("rb") as stream:
        with mmap.mmap(stream.fileno(), 0, access=mmap.ACCESS_READ) as data:
            for path, blob in volume_files(data):
                suffix = Path(path).suffix.upper()
                file_counts[suffix] += 1
                try:
                    if suffix == ".AMD":
                        for clip in amd_clips(blob):
                            clips[clip]["assets"].add(path)
                    else:
                        parser = cad_motions if suffix == ".CAD" else esd_motions
                        field = "scripts" if suffix == ".CAD" else "events"
                        for mid, actor in parser(blob):
                            actor_name = characters.get(actor, f"actor {actor:#x}")
                            state(mid)[field].add((path, actor_name))
                except ValueError as error:
                    raise ValueError(f"{path}: {error}") from error

    address = img.off(symbol_address("BattleDB"))
    for index in range(256):
        clip, = struct.unpack_from("<h", img.exe, address + 16 * index)
        if clip == -1:
            break
        clips[clip]["battle"].add(index)
    else:
        raise ValueError("BattleDB has no end sentinel")

    reviewed = clip_names(CATALOG.read_text()) if CATALOG.exists() else {}
    known = attack_clip_names(HUMANOID.read_text())
    if (reviewed.keys() | known.keys()) - clips.keys():
        raise ValueError("clip catalog or C constants contain IDs without resource evidence")
    for clip, row in clips.items():
        row["name"] = known.get(clip, reviewed.get(clip, f"ANIM_{clip:04X}"))
    if len({row["name"] for row in clips.values()}) != len(clips):
        raise ValueError("catalog labels conflict with C attack clip names")
    tokens = {name: ("state", mid) for mid, name in names.items()}
    tokens.update({row["name"]: ("clip", clip) for clip, row in clips.items()})
    uses = source_uses(tokens)
    return names, states, clips, uses, file_counts


def grouped(pairs):
    groups = defaultdict(set)
    for key, value in pairs:
        groups[key].add(value)
    return "; ".join(f"{key} ({', '.join(sorted(values))})"
                     for key, values in sorted(groups.items()))


def asset_list(paths):
    return grouped((str(Path(path).parent), Path(path).name) for path in paths)


def comment(lines, marker="Unregistered (motion_catalog.py):"):
    out = ["    /* " + marker]
    for line in lines:
        out.extend(textwrap.wrap(line, width=79, initial_indent="     * ",
                                 subsequent_indent="     * ", break_long_words=False,
                                 break_on_hyphens=False))
    return "\n".join(out + ["     */"]) + "\n"


def render_states(source, names, states, uses):
    source = GENERATED.sub("", source)
    by_name = {name: mid for mid, name in names.items()}

    def annotate(match):
        mid = by_name[match[1]]
        row = states[mid]
        if row["reg"]:
            return match[0]
        lines = ["No MAIN registration."]
        c_refs = uses.get(("state", mid), set())
        if c_refs:
            lines.append("C references: " + grouped(c_refs) + ".")
        for field, label in (("scripts", "CAD actors"), ("events", "ESD triggers")):
            if row[field]:
                lines.append(label + ": " + grouped(row[field]) + ".")
        if not row["scripts"] and not row["events"]:
            lines[0] = "No MAIN registration or CAD/ESD requests."
            if not c_refs:
                lines.append(f"Family base for {HANDLERS[mid >> 8]} only.")
        return comment(lines) + match[0]

    return re.sub(r"(?m)^    (MOT_\w+)\s*=.*$", annotate, source)


def render_clips(clips, uses):
    out = io.StringIO()
    writer = csv.writer(out, delimiter="\t", lineterminator="\n")
    writer.writerow(("id", "name", "note"))
    for clip, row in sorted(clips.items()):
        notes = []
        if not row["reg"]:
            notes.append("No MAIN registration.")
            if any(path.startswith("TRIAL/") for path in row["assets"]):
                notes.append("Trial use not audited.")
            notes.append("Assets: " + (asset_list(row["assets"]) or "none found") + ".")
            if row["battle"]:
                notes.append("BattleDB rows: " + ", ".join(map(str, sorted(row["battle"]))) + ".")
            if uses.get(("clip", clip)):
                notes.append("C references: " + grouped(uses["clip", clip]) + ".")
        writer.writerow((f"0x{clip:04x}", row["name"], " ".join(notes) or "-"))
    return out.getvalue()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--write", action="store_true", help="regenerate names and unregistered-entry comments")
    mode.add_argument("--check", action="store_true", help="fail on missing names or stale comments")
    args = parser.parse_args()
    source = TYPES.read_text()
    names, states, clips, uses, counts = collect(source)
    outputs = {TYPES: render_states(source, names, states, uses),
               CATALOG: render_clips(clips, uses)}
    stale = []
    for path, wanted in outputs.items():
        if not path.exists() or path.read_text() != wanted:
            if args.write:
                path.write_text(wanted)
            else:
                stale.append(str(path.relative_to(ROOT)))
    if stale:
        raise ValueError("stale motion catalog: " + ", ".join(stale) + "; run --write")
    print(f"{'Wrote' if args.write else 'Checked'} {len(names)} motion states and {len(clips)} animation clips; "
          f"{counts['.AMD']} AMD, {counts['.CAD']} CAD, {counts['.ESD']} ESD files.")


if __name__ == "__main__":
    try:
        main()
    except (ValueError, OSError, struct.error, csv.Error) as error:
        sys.exit(f"motion_catalog: {error}")

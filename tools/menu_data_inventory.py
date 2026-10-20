#!/usr/bin/env python3
"""Audit MENU's leading data, including the raw textbin and owned switch tables.

The report describes bytes, not C coverage. Run --check to verify the committed
inventory, or --write after reviewing changes. No generated assembly is needed.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parents[1]
START = 0x80011000
END = 0x80013AC0
FILE_BIAS = START - 0x800
REPORT = ROOT / "reference/menu-leading-data.tsv"

# Table start, end, and the consumer's LUI instruction. Some SDK inventory rows
# contain several functions; use the actual consumer labels where known.
SWITCH_TABLES = [
    (0x80011F84, 0x80011FB8, 0x80019770, "RunTitleMenu"),
    (0x80012180, 0x800121A4, 0x8001D75C, "UpdateChoiceOptions"),
    (0x800121A8, 0x800121CC, 0x8001E204, "UpdateChoiceOptions"),
    (0x800122DC, 0x80012368, 0x80021E28, "UpdateStartupCardCheck"),
    (0x8001236C, 0x800124B8, 0x800221C4, "UpdateLoadCardScreen"),
    (0x800124BC, 0x800125B8, 0x800224E0, "update_card_screen_"),
    (0x800125BC, 0x80012730, 0x800228DC, "update_card_message_"),
    (0x80012E00, 0x80012E14, 0x80028778, "FUN_80028534"),
    (0x800131A0, 0x8001322C, 0x8002E428, "Gssub_make_matrix"),
    (0x80013250, 0x800132C8, 0x8002F104, "GsLinkObject4"),
    (0x800132F0, 0x80013368, 0x8002F3F4, "OBJT2_OBJ_E0"),
    (0x80013370, 0x80013388, 0x80032CE0, "SSTICK_OBJ_4C"),
    (0x800133E0, 0x80013400, 0x80037EB4, "SpuSetCommonAttr"),
    (0x80013400, 0x80013420, 0x80037F74, "S_SCA_OBJ_98"),
    (0x80013420, 0x8001343C, 0x80038318, "SpuSetVoiceAttr"),
    (0x80013440, 0x8001345C, 0x800383F8, "S_SVA_OBJ_1A0"),
    (0x80013488, 0x8001353C, 0x80038D90, "SPRINTF_OBJ_2A0"),
    (0x80013608, 0x8001368C, 0x80046B48, "FntPrint"),
    (0x80013928, 0x800139A8, 0x8004ADBC, "FUN_8004ad94"),
    (0x80013AA0, 0x80013AB4, 0x8004D990, "PadInfoAct"),
]

# Six-byte TVoiceTable rows, including the final id=0xff sentinel.
VOICE_TABLES = [
    (0x80011618, 80, "English/CommonVoiceTable"),
    (0x80011810, 80, "French"),
    (0x80011A08, 80, "Italian"),
    (0x80011C00, 84, "Japanese"),
]


def require(condition, message):
    if not condition:
        raise ValueError(message)


def audit(image):
    require(image[:8] == b"PS-X EXE", "not a PS-X EXE")
    require(struct.unpack_from("<I", image, 0x18)[0] == START,
            "unexpected MENU load address")
    require(len(image) >= END - FILE_BIAS, "truncated leading data")
    functions = []
    for line in (ROOT / "config/functions.menu.exe.tsv").read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        address, size, name = line.split("\t")
        functions.append((int(address, 16), int(size), name))
    symbols = {
        int(address, 16): name
        for name, address in re.findall(
            r"^(\w+) = (0x[0-9a-fA-F]+);",
            (ROOT / "config/symbols.menu.exe.txt").read_text(), re.M
        )
    }
    data = image[START - FILE_BIAS:END - FILE_BIAS]
    covered = bytearray(len(data))
    rows = []

    def span(a, z):
        return image[a - FILE_BIAS:z - FILE_BIAS]

    def add(a, z, kind, label, detail=""):
        require(START <= a < z <= END, f"invalid data span {a:#x}..{z:#x}")
        require(not any(covered[a - START:z - START]),
                f"overlapping data span at {a:#x}")
        covered[a - START:z - START] = b"\1" * (z - a)
        rows.append((a, z, kind, label, detail))

    def function_at(address):
        return next((row for row in functions
                     if row[0] <= address < row[0] + row[1]), None)

    for a, z, ref, name in SWITCH_TABLES:
        # Check the address construction at the consumer, not just plausible
        # pointer values in the table. LUI is followed by ADDIU or indexed LW.
        insns = struct.unpack("<5I", span(ref, ref + 20))
        lui = insns[0]
        require(lui >> 26 == 15 and (lui & 0xFFFF) == (a + 0x8000) >> 16,
                f"{name}: missing table LUI at {ref:#x}")
        base = (lui >> 16) & 31
        require(any(w >> 26 in (9, 35) and (w >> 21) & 31 == base
                    and w & 0xFFFF == a & 0xFFFF for w in insns[1:]),
                f"{name}: missing table low-half reference")
        owner = function_at(ref)
        require(owner is not None, f"{name}: consumer outside inventory")
        for (target,) in struct.iter_unpack("<I", span(a, z)):
            require(target % 4 == 0 and function_at(target) == owner,
                    f"{name}: table target {target:#x} outside consumer's span")
        add(a, z, "switch-table", name, f"LUI reference {ref:#010x}")

    # The shared IMAGES implementation indexes 19 twelve-byte TMusicTable rows.
    # The following 504 zero bytes are deliberately not called extra music rows.
    music = span(0x800112DC, 0x800113C0)
    for filename, channel, minute, second, end_minute, end_second in \
            struct.iter_unpack("<I5B3x", music):
        require(filename in (0x800115B8, 0x800115C4) and channel < 8,
                "invalid music row")
        require(all(v >> 4 < 10 and v & 15 < 10
                    for v in (minute, second, end_minute, end_second)),
                "invalid music BCD time")
    require(all(music[i:i + 3] == b"\0\0\0"
                for i in range(9, len(music), 12)), "nonzero music row padding")
    add(0x800112DC, 0x800113C0, "music-table", "MusicTable",
        "19 TMusicTable rows; src/main.exe/music.h")

    for a, count, language in VOICE_TABLES:
        z = a + count * 6
        cues = list(struct.iter_unpack("6B", span(a, z)))
        require(cues[-1] == (255, 0, 0, 0, 0, 0),
                f"{language}: missing final voice sentinel")
        for cue in cues[:-1]:
            require(cue[0] != 255 and
                    (cue[1] < 16 or cue[1:] == (255, 0, 0, 0, 0)),
                    f"{language}: invalid voice cue")
            require(all(v >> 4 < 10 and v & 15 < 10 for v in cue[2:]),
                    f"{language}: invalid voice BCD time")
        add(a, z, "voice-table", language,
            f"{count} TVoiceTable rows including sentinel; src/main.exe/music.h")
    require(struct.unpack("<4I", span(0x80011E3C, 0x80011E4C)) ==
            tuple(a for a, _, _ in VOICE_TABLES), "voice pointer table differs")
    add(0x80011E3C, 0x80011E4C, "data-pointers", "EventVoiceTables",
        "English, French, Italian, Japanese")

    title = span(0x8001220C, 0x80012221)
    require(title[-1] == 0 and b"\0" not in title[:-1], "invalid save title")
    add(0x8001220C, 0x80012221, "shift-jis-string", "str_sjis_title",
        json.dumps(title[:-1].decode("shift_jis"), ensure_ascii=False))

    # Retail literals begin at four-byte boundaries. Keep the terminating NUL
    # with each string, and account for following zero bytes separately.
    for a in range(START, END, 4):
        p = a - START
        if covered[p] or not data[p]:
            continue
        z = data.find(b"\0", p)
        if z < 0 or any(covered[p:z + 1]):
            continue
        text = data[p:z]
        if all(v in (9, 10, 13) or 32 <= v <= 126 for v in text):
            add(a, START + z + 1, "ascii-string", symbols.get(a, ""),
                json.dumps(text.decode("ascii")))

    p = 0
    while p < len(data):
        if covered[p]:
            p += 1
            continue
        require(data[p] == 0, f"unclassified nonzero byte at {START + p:#x}")
        z = p + 1
        while z < len(data) and not covered[z] and data[z] == 0:
            z += 1
        # Zero fill is observable. Padding vs a reserved object is not always
        # recoverable; do not invent a type for these bytes.
        add(START + p, START + z, "zero-fill", "", "purpose unspecified")
        p = z
    rows.sort()
    require(sum(z - a for a, z, *_ in rows) == END - START,
            "incomplete leading-data coverage")
    return rows


def render(rows, image):
    digest = hashlib.sha256(image[START - FILE_BIAS:END - FILE_BIAS]).hexdigest()
    lines = [
        "# MENU leading-data inventory; generated by tools/menu_data_inventory.py.",
        "# Half-open address spans. Classification is not a C-ownership claim.",
        f"# Leading-data SHA-256: {digest}",
        "# start\tend\tbytes\tkind\tlabel\tdetail",
    ]
    for a, z, kind, label, detail in rows:
        lines.append(f"{a:#010x}\t{z:#010x}\t{z-a}\t{kind}\t{label}\t{detail}")
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--write", action="store_true")
    mode.add_argument("--check", action="store_true")
    parser.add_argument("--image", type=Path, default=ROOT / "disks/tenchu/menu.exe")
    args = parser.parse_args()
    try:
        image = args.image.read_bytes()
        rows = audit(image)
        report = render(rows, image)
        if args.write:
            REPORT.write_text(report)
        else:
            require(REPORT.read_text() == report,
                    "leading-data inventory differs; review before --write")
    except (ValueError, OSError, struct.error, UnicodeError) as error:
        parser.exit(1, f"menu_data_inventory: {error}\n")
    counts = Counter()
    for a, z, kind, *_ in rows:
        counts[kind] += z - a
    print(f"MENU leading data: {sum(counts.values()):,} bytes, no unclassified bytes")
    for kind, size in sorted(counts.items()):
        print(f"  {kind}: {size:,}")


if __name__ == "__main__":
    main()

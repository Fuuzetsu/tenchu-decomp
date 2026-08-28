#!/usr/bin/env python3
"""Read the retail AFS volume (data.vol) and its stage area maps.

The volume format is the one the matched AFS layer documents:
``AfsGetHeader`` reads the "AFS_VOL_200" header (big-endian element
count at +0xC, element-table position at +0x10), and ``AfsGetEntry``
decodes 36-byte big-endian elements with an "IX" marker: flag@2 (2 =
directory, 1 = file), pos@4, psize@8, size@12, name@16 (19 chars).
Directory entries carry an ``@N_`` prefix; files belong to the most
recent directory row.

STAGE.ACM files are ``LoadAreaMap``'s input: 16-byte ``NodeIndexType``
rows (index==0 terminates; ``index`` is the byte offset of the row's
``AreaNodeType[n]`` array, or an ``IndexArrayType`` when n < 0) whose
16-byte nodes hold the terrain attribute word at +0xC (MAP_WATER 4,
MAP_WOOD 8, the slope bits 0x4000/0x8000 — see game_types.h).

This parse identified MAP_WOOD: bit 8 marks only the training stage's
pond-deck node and CAVE2's mine walkways, matching ActCHASE's hollow
footstep.

    python3 tools/voldump.py list [PATTERN]
    python3 tools/voldump.py extract NAME OUT
    python3 tools/voldump.py acm-histogram
"""

from __future__ import annotations

import struct
import sys
from collections import Counter
from pathlib import Path

VOL = Path(__file__).resolve().parent.parent / "disks/tenchu/data.vol"

STAGE_DIRS = (
    "YAMIJOU", "TEMPLE", "TANREN", "TANREN_MV", "SEKISHO", "JOUKA2",
    "JOUKA", "CAVE2", "CAVE", "CASTLE", "BUKEYA", "AKINDO",
)


def read_elements(data):
    if data[:11] != b"AFS_VOL_200":
        raise SystemExit("not an AFS_VOL_200 volume")
    count = struct.unpack_from(">I", data, 0xC)[0]
    pos = struct.unpack_from(">I", data, 0x10)[0]
    out = []
    off = pos
    for _ in range(count):
        rec = data[off:off + 36]
        if rec[0:2] != b"IX":
            raise SystemExit(f"bad IX marker at {off:#x}")
        flag = (rec[2] << 8) | rec[3]
        fpos = int.from_bytes(rec[4:8], "big")
        psize = int.from_bytes(rec[8:12], "big")
        size = int.from_bytes(rec[12:16], "big")
        name = rec[16:35].split(b"\0")[0].decode(errors="replace")
        out.append((flag, fpos, psize, size, name))
        off += 36
    return out


def with_dirs(elements):
    current = ""
    for flag, fpos, psize, size, name in elements:
        if flag == 2:
            current = name.split("_", 1)[-1] if "_" in name else name
        else:
            yield current, fpos, psize, size, name


def acm_attribute_histogram(buf):
    rows = []
    off = 0
    while True:
        n, = struct.unpack_from("<h", buf, off + 2)
        idx, = struct.unpack_from("<i", buf, off + 4)
        if idx == 0:
            break
        rows.append((n, idx))
        off += 16
    attrs = Counter()
    for n, idx in rows:
        if n < 0:  # IndexArrayType indirection: layered rows, skipped here
            continue
        for k in range(n):
            a, = struct.unpack_from("<H", buf, idx + k * 16 + 0xC)
            attrs[a] += 1
    return attrs


def main(argv):
    data = VOL.read_bytes()
    elements = read_elements(data)
    cmd = argv[1] if len(argv) > 1 else "list"
    if cmd == "list":
        pattern = argv[2].upper() if len(argv) > 2 else ""
        for dirname, fpos, psize, size, name in with_dirs(elements):
            if pattern in name.upper() or pattern in dirname.upper():
                print(f"{dirname:12} {name:20} pos={fpos:#010x} size={size}")
    elif cmd == "extract":
        target, out = argv[2], Path(argv[3])
        for dirname, fpos, psize, size, name in with_dirs(elements):
            if name == target or f"{dirname}/{name}" == target:
                out.write_bytes(data[fpos:fpos + size])
                print(f"wrote {out} ({size} bytes)")
                return 0
        raise SystemExit(f"{target} not found")
    elif cmd == "acm-histogram":
        for dirname, fpos, psize, size, name in with_dirs(elements):
            if dirname in STAGE_DIRS and name.endswith("STAGE.ACM"):
                attrs = acm_attribute_histogram(data[fpos:fpos + size])
                total = sum(attrs.values())
                wood = sum(c for a, c in attrs.items() if a & 8)
                water = sum(c for a, c in attrs.items() if a & 4)
                top = ", ".join(f"{a:#x}:{c}" for a, c in attrs.most_common(5))
                print(f"{dirname:10} nodes={total:5} wood={wood:3} "
                      f"water={water:3}  top: {top}")
    else:
        raise SystemExit(__doc__)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))

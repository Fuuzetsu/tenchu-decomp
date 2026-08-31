#!/usr/bin/env python3
"""Read the retail data tables, so constants get the game's own names.

Several tables carry a `char *name` beside the id they describe -- the
engine builds .TMD paths and debug-menu rows out of them -- so the original
names for characters, weapons and AI think types are sitting in
disks/tenchu/main.exe. Prefer one of those over a name we invent
(docs/matching-cookbook.md, "Before inventing a name ... look in the DATA").

    tools/gamedata.py --list
    tools/gamedata.py HumanData
    tools/gamedata.py --whatis 0xf1        # what could this constant be?
    tools/gamedata.py --attack-ids         # MOT_ATTACK id -> characters/weapon
"""
import argparse
import pathlib
import struct
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
EXE = ROOT / "disks" / "tenchu" / "main.exe"

# addr, stride, and the fields to show. `name` is a char* to resolve.
TABLES = {
    "HumanData":   (0x80088A8C, 24, "the character roster",
                    [("type", "<h", 0), ("wepid", "<h", 2), ("turn", "<h", 4),
                     ("life", "<h", 6), ("width", "<h", 8), ("height", "<h", 10),
                     ("mtbl", "<I", 12), ("name", "name", 16)]),
    "WeaponModel": (0x8008986C, 12, "weapon kinds and their .TMD stems",
                    [("name", "name", 0), ("wid", "<h", 4), ("model", "<I", 8)]),
    "WeaponDB":    (0x80089AAC, 24, "per-weapon conflict box (wid is in ilup1.pad)",
                    [("confp_x", "<h", 0), ("confp_y", "<h", 2), ("confp_z", "<h", 4),
                     ("size", "<h", 6), ("wid", "<h", 22)]),
    "ThinkDB":     (0x80089E40,  8, "AI think types; the name encodes table+index",
                    [("name", "name", 0), ("value", "<h", 4)]),
    "MOTcommon":   (0x80086B94,  8, "the shared motion registration table",
                    [("mid", "<h", 0), ("id", "<h", 2)]),
}


def load():
    if not EXE.exists():
        sys.exit(f"{EXE} not found (the retail disc image is not in the repo)")
    exe = EXE.read_bytes()
    return exe, struct.unpack_from("<I", exe, 0x18)[0]


class Image:
    def __init__(self):
        self.exe, self.load = load()

    def off(self, addr):
        return addr - self.load + 0x800

    def ok(self, addr):
        return 0 <= self.off(addr) < len(self.exe) - 4

    def cstr(self, addr):
        if not addr or not self.ok(addr):
            return ""
        o = self.off(addr)
        end = self.exe.find(b"\0", o)
        return self.exe[o:end].decode("shift_jis", "replace")

    def rows(self, table, limit=256):
        addr, stride, _, fields = TABLES[table]
        out = []
        for i in range(limit):
            o = self.off(addr) + i * stride
            row = {}
            for nm, kind, at in fields:
                v = struct.unpack_from("<I" if kind == "name" else kind,
                                       self.exe, o + at)[0]
                row[nm] = self.cstr(v) if kind == "name" else v
            # every one of these tables ends on an all-ones or null row
            if all(v in (0, 0xFFFF, -1, "") for v in row.values()):
                break
            if row.get("name") == "" and "name" in row:
                break
            out.append((i, row))
        return out

    def motion_rows(self, mtbl):
        """(mid, id) pairs of one MotionRegistType table."""
        out, p = [], self.off(mtbl)
        while True:
            mid, id_ = struct.unpack_from("<hh", self.exe, p)
            if mid == -1:
                return out
            out.append((mid, id_))
            p += 8


def attack_ids(img):
    """MOT_ATTACK id -> the characters that use it, and their weapons."""
    weapons = {r["wid"]: r["name"] for _, r in img.rows("WeaponModel")}
    by_id = {}
    for _, r in img.rows("HumanData"):
        if not r["mtbl"] or not img.ok(r["mtbl"]):
            continue
        for mid, id_ in img.motion_rows(r["mtbl"]):
            if mid == 0x700:
                by_id.setdefault(id_, []).append(
                    (r["type"], r["name"], weapons.get(r["wepid"], hex(r["wepid"]))))
    return by_id


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("table", nargs="?", help="table to dump")
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--whatis", help="find a value across every table")
    ap.add_argument("--attack-ids", action="store_true",
                    help="MOT_ATTACK id -> characters and their weapon")
    args = ap.parse_args()

    if args.list or not (args.table or args.whatis or args.attack_ids):
        for name, (addr, stride, doc, _) in TABLES.items():
            print(f"  {name:13} 0x{addr:08x}  stride {stride:2}  {doc}")
        return

    img = Image()

    if args.attack_ids:
        for id_, hits in sorted(attack_ids(img).items()):
            weps = sorted({w for _, _, w in hits})
            print(f"0x{id_:03x}  weapon{'s' if len(weps) > 1 else ''}: {', '.join(weps)}")
            print(f"       {', '.join(f'{n}(0x{t:02x})' for t, n, _ in hits)}")
        return

    if args.whatis:
        want = int(args.whatis, 0)
        for name in TABLES:
            for i, row in img.rows(name):
                for k, v in row.items():
                    if v == want and k != "name":
                        label = row.get("name") or ""
                        print(f"  {name}[{i}].{k} == {args.whatis}"
                              + (f"   {label}" if label else "")
                              + f"   ({', '.join(fmt(a, b) for a, b in row.items() if a != k)})")
        for id_, hits in attack_ids(img).items():
            if id_ == want:
                print(f"  a MOT_ATTACK animation id, used by: "
                      f"{', '.join(n for _, n, _ in hits)}")
        return

    if args.table not in TABLES:
        sys.exit(f"unknown table {args.table}; try --list")
    for i, row in img.rows(args.table):
        print(f"  [{i:2}] " + "  ".join(fmt(k, v) for k, v in row.items()))


# ids and addresses read as hex; measurements read as decimal.
HEX = {"type", "wepid", "wid", "value", "mid", "id"}
ADDR = {"mtbl", "model"}


def fmt(k, v):
    if isinstance(v, str):
        return f"{k}={v}"
    if k in ADDR:
        return f"{k}=0x{v & 0xFFFFFFFF:08x}"
    if k in HEX:
        return f"{k}=0x{v & 0xFFFF:02x}"
    return f"{k}={v}"


if __name__ == "__main__":
    sys.exit(main())

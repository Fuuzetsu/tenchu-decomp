#!/usr/bin/env python3
"""Rank matched C sources by machine-decomp artifact density.

This is the work queue for the humanising pass: rewriting already
byte-matched functions into the C a human plausibly wrote (official
recovered names, struct fields instead of offset casts, natural control
flow) while `./Build check` stays byte-identical.

Artifacts are only counted in CODE — comments and strings are stripped
first, so PSX.SYM fact blocks and prose headers don't rank a file.
Files still containing INCLUDE_ASM are listed separately: they ship
original bytes, so their reference dumps must stay until matched.

Usage: tools/humanscan.py [N]        show top N (default 40)
       tools/humanscan.py --dumps    list matched files still carrying
                                     stale Ghidra/m2c/triage dumps
"""
import pathlib
import re
import sys

SRC = pathlib.Path(__file__).resolve().parent.parent / "src/main.exe"

PATTERNS = [
    # in_/unaff_ only match Ghidra register-artifact suffixes (in_a0, unaff_s2)
    # so semantic identifiers like an `in_range:` label do not false-positive.
    ("ghidra_local", re.compile(r"\b(?:[a-z]{1,3}Var\d+(?:_[a-z0-9]+)?|p[A-Z][A-Za-z]*Var\d+|local_[0-9a-f]+|param_\d+|(?:in|unaff)_(?:[atsv][0-9]|k[01]|ra|gp|sp|fp|zero|lo|hi|f[0-9]+)|uStack_?[0-9a-f]+)\b")),
    ("generic_temp", re.compile(r"\b(?:temp_[a-z0-9_]+|var_[a-z0-9_]+|phi_[a-z0-9_]+)\b")),
    ("fun_sym", re.compile(r"\bFUN_800[0-9a-fA-F]{5}\b")),
    ("data_sym", re.compile(r"\bD_800[0-9a-fA-F]{5}\b")),
    ("lab", re.compile(r"\b(?:LAB_|block_|joined_r0x)[0-9a-fA-F_]+\b")),
    ("offset_cast", re.compile(r"\*\s*\(\s*(?:u8|u16|u32|s8|s16|s32|short|int|char|long|undefined\d?)\s*\*\s*\)\s*\(")),
    ("byte_arith", re.compile(r"\(\s*(?:u8|char|s8)\s*\*\s*\)\s*[A-Za-z_][A-Za-z0-9_]*\s*\+")),
]

DUMP_MARKERS = (
    "// Ghidra decompilation",
    "// m2c (",
    "// triage:",
)


def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    text = re.sub(r"//[^\n]*", " ", text)
    text = re.sub(r'"(?:[^"\\]|\\.)*"', '""', text)
    return text


def main():
    dumps_mode = "--dumps" in sys.argv
    top = 40
    for a in sys.argv[1:]:
        if a.isdigit():
            top = int(a)

    rows = []
    dump_files = []
    for p in sorted(SRC.glob("*.c")):
        raw = p.read_text(errors="replace")
        guarded = re.search(r"^\s*INCLUDE_ASM\(", raw, re.M) is not None
        if not guarded and any(m in raw for m in DUMP_MARKERS):
            dump_files.append(p.name)
        code = strip_comments(raw)
        counts = {}
        total = 0
        for key, rx in PATTERNS:
            if key in ("offset_cast", "byte_arith"):
                n = len(rx.findall(code))
            else:
                n = len(set(rx.findall(code)))
            counts[key] = n
            total += n
        if total:
            lines = raw.count("\n") + 1
            rows.append((total, lines, p.name, guarded, counts))

    if dumps_mode:
        print(f"# {len(dump_files)} matched files still carry stale machine dumps")
        for name in dump_files:
            print(name)
        return

    rows.sort(key=lambda r: -r[0])
    print(f"# {len(rows)} files with code-level artifacts"
          f" ('G' = still INCLUDE_ASM-guarded, ships original bytes)")
    print(f"{'score':>5} {'lines':>6}    file                                  detail")
    for total, lines, name, guarded, c in rows[:top]:
        flag = "G" if guarded else " "
        detail = " ".join(f"{k}={v}" for k, v in c.items() if v)
        print(f"{total:5} {lines:6}  {flag} {name:36}  {detail}")


if __name__ == "__main__":
    main()

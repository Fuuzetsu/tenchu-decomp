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
Quoted C and .inc includes are followed so shared implementations are audited.

Usage: tools/humanscan.py [N]        show top N (default 40)
       tools/humanscan.py --target trial.exe --headers
                                     audit another executable and its headers
       tools/humanscan.py --dumps    list matched files still carrying
                                     stale Ghidra/m2c/triage dumps
"""
import argparse
import os
import pathlib
import re

try:
    from . import source_units as SU
except ImportError:
    import source_units as SU

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
    # Compiler-allocation camouflage, not a meaningful arithmetic operation.
    ("folded_identity", re.compile(
        r"\(\s*(?:\([^()]*\)\s*)?([A-Za-z_]\w*)\s*\+\s*"
        r"(?:\([^()]*\)\s*)?\1\s*\)\s*-\s*"
        r"(?:\([^()]*\)\s*)?\1\b"
    )),
    ("empty_loop", re.compile(r"\bdo\s*\{\s*\}\s*while\s*\(\s*0\s*\)")),
    ("discarded_volatile", re.compile(r"\(\s*void\s*\)\s*\*\s*\([^;]*\bvolatile\b[^;]*;")),
    # Review object qualifiers separately from legitimate asm volatile (...).
    ("volatile_object", re.compile(r"\bvolatile\b(?!\s*\()")),
    ("cancelled_update", re.compile(r"\b([A-Za-z_]\w*)\s*\+=\s*[A-Za-z_]\w*\s*-\s*\1\s*;")),
    ("identical_arms", re.compile(r"\bif\s*\([^;{}]*\)\s*\{\s*([^{}]+?)\s*\}\s*else\s*\{\s*\1\s*\}")),
    ("unknown_field", re.compile(r"\bunknown[0-9a-fA-F]+\b")),
]

DUMP_MARKERS = (
    "// Ghidra decompilation",
    "// m2c (",
    "// triage:",
)


def strip_comments(text):
    # Match literals together with comments so comment delimiters inside a
    # string, or braces inside character literals, cannot change the scan.
    pattern = (
        r'/\*.*?\*/|//[^\n]*'
        r'|"(?:[^"\\]|\\.)*"'
        r"|'(?:[^'\\]|\\.)*'"
    )
    return re.sub(pattern,
                  lambda match: "\n" * match[0].count("\n") + " ",
                  text, flags=re.S)


def without_directives(code):
    """Keep ordinary source sites separate from conventional macro wrappers."""
    lines = []
    continued = False
    for line in code.splitlines():
        directive = continued or line.lstrip().startswith("#")
        continued = directive and line.rstrip().endswith("\\")
        lines.append("" if directive else line)
    return "\n".join(lines)


def nonempty_one_shot_loops(code):
    """Find ordinary do/while(0) bodies, including nested wrappers.

    These are review candidates: a body with an early break may be intentional.
    Empty loops have their own counter, and statement macros are excluded by
    the caller. Balance braces instead of letting a regex cross loop bodies.
    """
    bodies = []
    for start in re.finditer(r"\bdo\s*\{", code):
        depth = 1
        end = start.end()
        while end < len(code) and depth:
            depth += (code[end] == "{") - (code[end] == "}")
            end += 1
        if not depth and re.match(r"\s*while\s*\(\s*0\s*\)", code[end:]):
            body = code[start.end():end - 1].strip()
            if body:
                bodies.append(body)
    return bodies


def with_shared_sources(paths):
    """Visit each implementation once, including wrappers' quoted C includes."""
    seen = set()

    def visit(path):
        path = path.resolve()
        if path in seen:
            return
        seen.add(path)
        yield path
        for include in re.findall(r'^\s*#\s*include\s+"([^"]+)"',
                                  path.read_text(errors="replace"), re.M):
            if pathlib.Path(include).suffix.lower() in (".c", ".inc"):
                yield from visit(path.parent / include)

    for path in paths:
        yield from visit(path)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("top", nargs="?", type=int, default=40)
    parser.add_argument("--target", default="main.exe",
                        choices=("main.exe", "menu.exe", "ending.exe", "trial.exe",
                                 "run.exe", "slps_019.01"))
    parser.add_argument("--headers", action="store_true")
    parser.add_argument("--dumps", action="store_true")
    args = parser.parse_args()
    source_dir = SRC.parent / args.target
    sources = list(SU.iter_source_files(source_dir))
    if args.headers:
        sources += sorted(source_dir.glob("*.h"))

    rows = []
    dump_files = []
    for p in with_shared_sources(sources):
        raw = p.read_text(errors="replace")
        name = os.path.relpath(p, source_dir)
        guarded = re.search(r"^\s*INCLUDE_ASM\(", raw, re.M) is not None
        if not guarded and any(m in raw for m in DUMP_MARKERS):
            dump_files.append(name)
        code = strip_comments(raw)
        ordinary_code = without_directives(code)
        counts = {}
        total = 0
        for key, rx in PATTERNS:
            matches = rx.findall(ordinary_code if key == "empty_loop" else code)
            if key in ("offset_cast", "byte_arith", "empty_loop", "discarded_volatile"):
                n = len(matches)
            else:
                n = len(set(matches))
            counts[key] = n
            total += n
        counts["one_shot_loop"] = len(nonempty_one_shot_loops(ordinary_code))
        total += counts["one_shot_loop"]
        if total:
            lines = raw.count("\n") + 1
            rows.append((total, lines, name, guarded, counts))

    if args.dumps:
        print(f"# {len(dump_files)} matched files still carry stale machine dumps")
        for name in dump_files:
            print(name)
        return

    rows.sort(key=lambda r: -r[0])
    print(f"# {len(rows)} files with potential artifacts in {args.target}"
          f" ('G' = still INCLUDE_ASM-guarded, ships original bytes)")
    print(f"{'score':>5} {'lines':>6}    file                                  detail")
    for total, lines, name, guarded, c in rows[:args.top]:
        flag = "G" if guarded else " "
        detail = " ".join(f"{k}={v}" for k, v in c.items() if v)
        print(f"{total:5} {lines:6}  {flag} {name:36}  {detail}")


if __name__ == "__main__":
    main()

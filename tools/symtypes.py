#!/usr/bin/env python3
"""Compare our declared types against the ones PSX.SYM recorded.

The demo's debug symbols carry the original build's own spelling for every
global it knew.  Where our decomp disagrees the difference is usually ours,
and it is not cosmetic: an array declared as a pointer forces byte-walk or
integer-sum address arithmetic at every use, because `arr[i]` and `ptr[i]`
do not compile to the same instruction (see docs/matching-cookbook.md,
"Array symbol vs pointer variable").

Retail did change some declarations, so a mismatch is a lead, not a verdict.
Rank is by how much codegen the difference can move.

    tools/symtypes.py            # ranked mismatches
    tools/symtypes.py --kind array-vs-pointer
    tools/symtypes.py --uses Name    # where the disagreement is spent
"""
import argparse
import pathlib
import re
import subprocess
import sys

try:
    from tools import source_units as SU
except ModuleNotFoundError:
    import source_units as SU  # type: ignore[no-redef]

ROOT = pathlib.Path(__file__).resolve().parent.parent
SRC = ROOT / "src" / "main.exe"
REF = ROOT / "reference" / "psxsym-globals.h"

# PSX.SYM writes C's own spelling; our headers use the short aliases.
ALIAS = {
    "u8": "unsigned char", "s8": "char", "u16": "unsigned short",
    "s16": "short", "u32": "unsigned long", "s32": "long",
    "int": "long", "unsigned int": "unsigned long", "unsigned": "unsigned long",
    "u_long": "unsigned long", "u_short": "unsigned short", "u_char": "unsigned char",
}

DECL = re.compile(
    r"^\s*extern\s+(?P<type>.+?)\s*(?P<stars>\**)\s*"
    r"(?P<name>\w+)\s*(?P<dims>(?:\[[^\]]*\])*)\s*;")


def norm(type_: str) -> str:
    """Collapse spelling differences that carry no meaning."""
    t = " ".join(type_.replace("struct ", "").replace("enum ", "").split())
    t = re.sub(r"\bconst\b|\bvolatile\b", "", t).strip()
    # Longest first, and one pass only: substituting piecewise turns
    # "unsigned short" into "unsigned long short".
    pat = "|".join(re.escape(k) for k in
                   sorted(ALIAS, key=len, reverse=True))
    t = re.sub(rf"\b(?:{pat})\b", lambda m: ALIAS[m.group(0)], t)
    # Our typedefs drop the tag_ prefix the original used.
    t = re.sub(r"\btag_(\w+)", r"\1", t)
    t = re.sub(r"^T(?=[A-Z])", "", t)
    return " ".join(t.split())


def parse(path: pathlib.Path):
    out = {}
    for line in path.read_text(errors="replace").splitlines():
        if "(" in line.split(";")[0] and "[" not in line.split("(")[0]:
            continue  # function prototype, not a data declaration
        m = DECL.match(line)
        if not m:
            continue
        name = m.group("name")
        dims = [d for d in re.findall(r"\[([^\]]*)\]", m.group("dims"))]
        out.setdefault(name, (norm(m.group("type")), len(m.group("stars")),
                              dims, path.name, line.strip()))
    return out


def shape(stars: int, dims: list) -> str:
    return "*" * stars + "".join(f"[{d or ''}]" for d in dims)


LOCALS = ROOT / "reference" / "psxsym-locals.tsv"
# A declaration line: a type, then a separator that is whitespace or stars,
# then the name. The separator is what keeps `break;` from parsing as a
# declaration of `reak`.
DECL_LINE = re.compile(
    r"^\s+([A-Za-z_][\w \t]*[\w])[ \t]*(?:\*+[ \t]*|[ \t]+)"
    r"(\w+)[ \t]*(\[[^;=]*\])?[ \t]*(?:=[^;]*)?;[ \t]*$")


KEYWORD = {"return", "goto", "if", "else", "break", "continue", "do",
           "while", "for", "switch", "case", "default"}


def strip_comments(txt: str) -> str:
    """Blank out comment bodies, keeping line structure.

    Braces inside prose would otherwise unbalance the body scan -- these
    files carry long comment blocks, and one stray `{` in one sends the
    scan off the end of the function.
    """
    out, i, n = [], 0, len(txt)
    while i < n:
        if txt.startswith("/*", i):
            end = txt.find("*/", i + 2)
            end = n if end < 0 else end + 2
            out.append("".join(c if c == "\n" else " " for c in txt[i:end]))
            i = end
        elif txt.startswith("//", i):
            end = txt.find("\n", i)
            end = n if end < 0 else end
            out.append(" " * (end - i))
            i = end
        else:
            out.append(txt[i])
            i += 1
    return "".join(out)


def our_locals(path: pathlib.Path, func: str):
    """Every local the function declares, nested block scopes included.

    Counting only the top-level block would report a nested declaration as
    one the original had and we dropped, which is backwards: declaring
    inside the block that uses it is the shape we are trying to recover.
    """
    txt = strip_comments(path.read_text(errors="replace"))
    m = re.search(rf"^[\w \t*]*\b{re.escape(func)}\s*\([^;{{]*\)\s*\n?\{{",
                  txt, re.M)
    if not m:
        return None
    out, depth, type_depth, in_type = [], 1, 0, False
    for line in txt[m.end():].splitlines():
        s = line.strip()
        if not s or s.startswith(("/*", "*", "//")):
            continue
        # A function-local type definition is not a declaration of a local.
        if not in_type and re.match(r"(enum|struct|union)\b[^;]*$", s):
            in_type, type_depth = True, 0
        if in_type:
            type_depth += line.count("{") - line.count("}")
            if type_depth <= 0 and s.endswith(";"):
                in_type = False
            continue
        d = DECL_LINE.match(line)
        if d and d.group(1).split()[0] not in KEYWORD:
            out.append(d.group(2))
        depth += line.count("{") - line.count("}")
        if depth <= 0:
            break  # end of the function body
    return out


def locals_audit(only: str | None):
    theirs: dict[str, list[tuple[str, str, str]]] = {}
    for line in LOCALS.read_text(errors="replace").splitlines():
        if line.startswith("#"):
            continue
        f = line.split("\t")
        if len(f) < 6:
            continue
        # column 6 is the lexical block depth (1 = the function body);
        # it is what distinguishes a repeated name's separate scopes
        depth = f[6] if len(f) > 6 else ""
        theirs.setdefault(f[0], []).append((f[2], f[4], f[5], depth))

    rows = []
    for func, recs in theirs.items():
        if only and func != only:
            continue
        path = SU.source_for_function(func, SRC)
        if not path.exists():
            continue
        ours = our_locals(path, func)
        if ours is None:
            continue
        # Parameters are declared in the signature, not the body.
        want = [n for k, _, n, _d in recs if k != "param"]
        wantset, ourset = set(want), set(ours)
        extra = [n for n in ours if n not in wantset]
        missing = [n for n in want if n not in ourset]
        if not extra and not missing:
            continue
        rows.append((len(extra), func, ours, want, extra, missing,
                     {n: (t, d) for k, t, n, d in recs}))
    rows.sort(reverse=True)
    return rows


def fmt_missing(rec, name):
    if not rec:
        return f"? {name}"
    t, d = rec
    return f"{t} {name}" + (f" @depth {d}" if d and d != "1" else "")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--kind", help="only this mismatch class")
    ap.add_argument("--uses", help="show the use sites for one name")
    ap.add_argument("--locals", nargs="?", const="", metavar="FUNC",
                    help="audit function locals against PSX.SYM instead")
    args = ap.parse_args()

    if args.locals is not None:
        rows = locals_audit(args.locals or None)
        if not rows:
            print("no local-set differences")
            return
        print(f"{len(rows)} function(s) whose locals differ from PSX.SYM's.")
        print("PSX.SYM is the DEMO build: a difference is a lead, not a bug.\n")
        for n_extra, func, ours, want, extra, missing, types in rows:
            print(f"{func}  (ours {len(ours)}, PSX.SYM {len(want)})")
            if extra:
                print(f"    not in PSX.SYM: {', '.join(extra)}")
            if missing:
                print("    PSX.SYM had:    " +
                      ", ".join(fmt_missing(types.get(n), n) for n in missing))
        return

    theirs = parse(REF)
    ours = {}
    for h in sorted(SRC.glob("*.h")):
        for name, rec in parse(h).items():
            ours.setdefault(name, rec)

    rows = []
    for name, (ttype, tstars, tdims, _, tline) in theirs.items():
        if name not in ours:
            continue
        otype, ostars, odims, ohdr, oline = ours[name]
        tshape, oshape = shape(tstars, tdims), shape(ostars, odims)
        if (ttype, tshape) == (otype, oshape):
            continue
        # An array spelled as a pointer changes every indexing site's
        # instruction selection; a base-type disagreement changes access
        # widths; a bare size disagreement changes nothing but the bound.
        if bool(tdims) != bool(odims) or tstars != ostars:
            kind, rank = "array-vs-pointer", 0
        elif ttype != otype:
            kind, rank = "base-type", 1
        else:
            kind, rank = "extent", 2
        rows.append((rank, kind, name, f"{ttype} {tshape}", f"{otype} {oshape}",
                     ohdr, tline, oline))

    rows.sort()
    if args.uses:
        hits = subprocess.run(
            ["git", "grep", "-n", rf"\b{args.uses}\b", "--",
             "src/main.exe/*.c", "src/main.exe/*.C"],
            cwd=ROOT, capture_output=True, text=True).stdout
        print(hits or f"no .c uses of {args.uses}")
        return

    shown = [r for r in rows if not args.kind or r[1] == args.kind]
    if not shown:
        print("no mismatches" if not rows else f"no {args.kind} mismatches")
        return

    print(f"{len(shown)} mismatch(es) between PSX.SYM and our headers\n")
    last = None
    for _, kind, name, t, o, hdr, tline, oline in shown:
        if kind != last:
            print(f"--- {kind} ---")
            last = kind
        print(f"  {name}")
        print(f"      PSX.SYM: {t}")
        print(f"      ours:    {o}   ({hdr})")
    print(f"\nUse `tools/symtypes.py --uses <Name>` to see what a fix would touch.")
    return


if __name__ == "__main__":
    sys.exit(main())

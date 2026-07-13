#!/usr/bin/env python3
"""Derive and synchronize per-function maspsx compatibility flags.

Two properties are authoritative in the target split assembly but live in
mirrored hand-maintained Python/Haskell tables when a C draft is compiled:

* every ``%gp_rel(SYM)`` operand needs ``--gp-extern SYM``;
* an ASPSX-style variable ``div``/``divu`` guarded by ``break 7`` (and, for
  signed division, ``break 6``) needs ``--expand-div``.

``gpsyms.py`` already owns robust gp-symbol extraction and table editing.  This
front-end composes it with guarded-division detection and updates both Build.hs
and permute.py in one operation.

  tools/maspsxflags.py <Name>          report required flags
  tools/maspsxflags.py <Name> --write  sync both mirrored flag tables
"""
import argparse
import ast
import glob
import os
import re
import sys

import fuzzy_inventory
import gpsyms

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

BUILD_HS = "shake/src/Build.hs"
PERMUTE = "tools/permute.py"
SRC = "src/main.exe"


def _read(path):
    with open(path, errors="replace") as stream:
        return stream.read()


def _write(path, text):
    with open(path, "w") as stream:
        stream.write(text)


def asm_files(name):
    directory = gpsyms.ASM.format(name=name)
    files = sorted(glob.glob(f"{directory}/*.s")) or glob.glob(
        f".shake/gen/main.exe/asm/nonmatchings/*/{name}.s"
    )
    if not files:
        sys.exit(f"maspsxflags: no target .s for {name}; split and build it first")
    return files


def instructions(text):
    """Yield ``(mnemonic, operands)`` from splat's annotated assembly."""
    for raw in text.splitlines():
        line = raw.split("*/", 1)[-1] if "*/" in raw else raw
        match = re.match(r"\s*([a-z][a-z0-9.]*)\s+(.+?)\s*$", line, re.I)
        if match:
            yield match.group(1).lower(), match.group(2).strip().lower()


def guarded_division_count(text):
    """Count variable divisions carrying ASPSX's runtime guard(s).

    Signed ``div`` has both divide-by-zero (break 7) and INT_MIN/-1 overflow
    (break 6) guards.  Unsigned ``divu`` has no overflow case, so ASPSX emits
    only break 7.  Requiring both silently missed every unsigned division.
    """
    insns = list(instructions(text))
    count = 0
    for index, (op, _operands) in enumerate(insns):
        if op not in {"div", "divu"}:
            continue
        breaks = set()
        for next_op, operands in insns[index + 1:index + 15]:
            if next_op != "break":
                continue
            token = operands.split(",", 1)[0].strip()
            try:
                breaks.add(int(token, 0))
            except ValueError:
                pass
        required_breaks = {7} if op == "divu" else {6, 7}
        if required_breaks.issubset(breaks):
            count += 1
    return count


def target_requirements(name):
    files = asm_files(name)
    text = "\n".join(_read(path) for path in files)
    return {
        "gp_symbols": gpsyms.gp_symbols(name),
        "guarded_divisions": guarded_division_count(text),
    }


def _quoted_values(raw):
    return re.findall(r'"([^"]+)"', raw)


def build_tables():
    """Return the gp/extra tables encoded in Build.hs."""
    text = _read(BUILD_HS)
    try:
        body = text.split("maspsxGpExterns src =", 1)[1]
        extra_body, gp_body = body.split("extra _ = []", 1)
        gp_body = gp_body.split("syms _ = []", 1)[0]
    except (IndexError, ValueError):
        sys.exit("maspsxflags: couldn't parse maspsxGpExterns in Build.hs")

    extras = {}
    for name, raw in re.findall(
            r'^\s*extra "([A-Za-z0-9_]+)" = \[([^\n]*)\]$',
            extra_body, re.M):
        extras[name] = _quoted_values(raw)
    gp = {}
    for name, raw in re.findall(
            r'^\s*syms "([A-Za-z0-9_]+)" = \[([^\n]*)\]$',
            gp_body, re.M):
        gp[name] = _quoted_values(raw)
    return gp, extras


def permute_tables():
    """Return literal GP_EXTERNS/MASPSX_EXTRA assignments from permute.py."""
    wanted = {"GP_EXTERNS", "MASPSX_EXTRA"}
    found = {}
    for node in ast.parse(_read(PERMUTE), filename=PERMUTE).body:
        if not isinstance(node, ast.Assign) or len(node.targets) != 1:
            continue
        target = node.targets[0]
        if isinstance(target, ast.Name) and target.id in wanted:
            found[target.id] = ast.literal_eval(node.value)
    missing = wanted - set(found)
    if missing:
        sys.exit("maspsxflags: couldn't parse " + ", ".join(sorted(missing))
                 + " in permute.py")
    return found["GP_EXTERNS"], found["MASPSX_EXTRA"]


def _normalized(table):
    return {name: frozenset(flags) for name, flags in table.items()}


def audit_guarded_drafts():
    """Return target-derived metadata errors for every guarded C draft."""
    build_gp, build_extra = build_tables()
    permute_gp, permute_extra = permute_tables()
    errors = []

    if _normalized(build_gp) != _normalized(permute_gp):
        errors.append("Build.hs and permute.py gp tables are not mirrored")
    if _normalized(build_extra) != _normalized(permute_extra):
        errors.append("Build.hs and permute.py extra-flag tables are not mirrored")

    for name in sorted(fuzzy_inventory.guarded_sources(SRC)):
        req = target_requirements(name)
        expected_gp = set(req["gp_symbols"])
        for label, table in (("Build.hs", build_gp),
                             ("permute.py", permute_gp)):
            actual_gp = set(table.get(name, []))
            if expected_gp != actual_gp:
                missing = sorted(expected_gp - actual_gp)
                stale = sorted(actual_gp - expected_gp)
                detail = []
                if missing:
                    detail.append("missing " + ", ".join(missing))
                if stale:
                    detail.append("stale " + ", ".join(stale))
                errors.append(f"{name}: {label} gp table " + "; ".join(detail))

        needs_div = req["guarded_divisions"] > 0
        for label, table in (("Build.hs", build_extra),
                             ("permute.py", permute_extra)):
            has_div = "--expand-div" in table.get(name, [])
            if needs_div != has_div:
                state = "missing" if needs_div else "stale"
                errors.append(f"{name}: {label} {state} --expand-div")
    return errors


def _with_flag(raw, flag):
    flags = re.findall(r'"([^"]+)"', raw)
    if flag not in flags:
        flags.append(flag)
    return "[" + ", ".join(f'"{value}"' for value in flags) + "]"


def patch_build_extra(name, flag="--expand-div"):
    text = _read(BUILD_HS)
    pattern = re.compile(
        rf'^(\s*extra "{re.escape(name)}" = )(\[[^\n]*\])$', re.M
    )
    if pattern.search(text):
        text = pattern.sub(lambda m: m.group(1) + _with_flag(m.group(2), flag),
                           text, count=1)
        action = "updated"
    else:
        line = f'    extra "{name}" = ["{flag}"]'
        text, count = re.subn(r'^    extra _ = \[\]$',
                              line + "\n    extra _ = []", text,
                              count=1, flags=re.M)
        if count == 0:
            sys.exit("maspsxflags: couldn't find `extra _ = []` in Build.hs")
        action = "inserted"
    _write(BUILD_HS, text)
    return action


def patch_permute_extra(name, flag="--expand-div"):
    text = _read(PERMUTE)
    match = re.search(r'^MASPSX_EXTRA = \{$.*?^\}$', text, re.M | re.S)
    if not match:
        sys.exit("maspsxflags: couldn't find MASPSX_EXTRA in permute.py")
    block = match.group(0)
    pattern = re.compile(
        rf'^(\s*"{re.escape(name)}": )(\[[^\n]*\])(,)$', re.M
    )
    if pattern.search(block):
        new_block = pattern.sub(
            lambda m: m.group(1) + _with_flag(m.group(2), flag) + m.group(3),
            block, count=1,
        )
        action = "updated"
    else:
        line = f'    "{name}": ["{flag}"],'
        new_block = block[:-1] + line + "\n}"
        action = "inserted"
    text = text[:match.start()] + new_block + text[match.end():]
    _write(PERMUTE, text)
    return action


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("name", nargs="?")
    parser.add_argument("--write", action="store_true",
                        help="sync Build.hs and permute.py")
    parser.add_argument("--check", action="store_true",
                        help="audit every guarded draft against target asm")
    args = parser.parse_args()

    if args.check:
        if args.name or args.write:
            parser.error("--check does not take a name or --write")
        errors = audit_guarded_drafts()
        if errors:
            for error in errors:
                print("maspsxflags: " + error, file=sys.stderr)
            sys.exit(1)
        count = len(fuzzy_inventory.guarded_sources(SRC))
        print(f"maspsxflags: OK — {count} guarded drafts have target-derived "
              "gp/div metadata")
        return
    if not args.name:
        parser.error("NAME is required unless --check is used")

    req = target_requirements(args.name)
    gp = req["gp_symbols"]
    divs = req["guarded_divisions"]
    print(f"maspsxflags: {args.name}")
    print("  gp externs: " + (", ".join(gp) if gp else "(none)"))
    print(f"  guarded variable divisions: {divs}"
          + ("  -> --expand-div" if divs else ""))
    if not args.write:
        print("  (run with --write to synchronize both build/permuter tables)")
        return

    if gp:
        print(f"  Build.hs gp table: {gpsyms.patch_build_hs(args.name, gp)}")
        print(f"  permute.py gp table: {gpsyms.patch_permute(args.name, gp)}")
    if divs:
        print(f"  Build.hs extra table: {patch_build_extra(args.name)}")
        print(f"  permute.py extra table: {patch_permute_extra(args.name)}")
    if not gp and not divs:
        print("  no per-function maspsx flags required")


if __name__ == "__main__":
    main()

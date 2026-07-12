#!/usr/bin/env python3
"""Report the target/candidate stack workspace for one matching function.

Large functions often contain mutually-exclusive aggregate locals whose source
scopes nevertheless make old cc1 reserve separate slots.  The retail assembly
already tells us the useful layout: outgoing-argument space ends where locals
begin, and the first callee-saved spill marks the end of the local workspace.
This tool makes that boundary and every accessed ``sp+offset`` mechanical.

  tools/stackplan.py ProcItemFire
  tools/stackplan.py ProcItemFire -n --json

The report does not claim that a union is always the original source.  It shows
the exact byte window an explicit scratch union must occupy when independent
block locals inflate the candidate frame.
"""
import argparse
from collections import Counter, defaultdict
import glob
import json
import os
import re
import subprocess
import sys

from matchlock import MatchToolBusy, matching_tool_lock

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

TARGET = ".shake/gen/main.exe/asm/nonmatchings/{name}"
CANDIDATE = ".shake/processed/main.exe/{name}.s"
SAVED_REGS = ({f"s{index}" for index in range(9)} |
              {"fp", "ra", "30", "31"} |
              {str(index) for index in range(16, 24)})


def parse_number(value):
    return int(value, 0)


def instruction_text(raw):
    """Remove splat address comments and compiler diagnostic comments."""
    if "*/" in raw:
        raw = raw.split("*/", 1)[1]
    return raw.split("#", 1)[0].strip()


def frame_info(text):
    """Return frame/vars/args sizes exposed by assembly, when available."""
    result = {"frame": None, "vars": None, "args": None}
    match = re.search(
        r"^[ \t]*\.frame\s+\$?sp,\s*(0x[0-9a-f]+|\d+),", text,
        re.M | re.I,
    )
    if match:
        result["frame"] = parse_number(match.group(1))
        line = text[match.start():text.find("\n", match.start())]
        for key in ("vars", "args"):
            field = re.search(rf"\b{key}\s*=\s*(0x[0-9a-f]+|\d+)",
                              line, re.I)
            if field:
                result[key] = parse_number(field.group(1))

    if result["frame"] is None:
        for raw in text.splitlines():
            line = instruction_text(raw)
            add = re.match(
                r"addiu?\s+\$?sp,\s*\$?sp,\s*(-?(?:0x[0-9a-f]+|\d+))$",
                line, re.I,
            )
            if add and parse_number(add.group(1)) < 0:
                result["frame"] = -parse_number(add.group(1))
                break
            sub = re.match(
                r"subu\s+\$?sp,\s*\$?sp,\s*(0x[0-9a-f]+|\d+)$",
                line, re.I,
            )
            if sub:
                result["frame"] = parse_number(sub.group(1))
                break
    return result


def saved_area_start(text):
    """Lowest callee-saved spill offset in the prologue."""
    offsets = []
    for raw in text.splitlines():
        line = instruction_text(raw)
        if re.match(r"jalr?\b", line):
            break
        match = re.match(
            r"sw\s+\$?([a-z0-9]+),\s*(-?(?:0x[0-9a-f]+|\d+))\(\$?sp\)$",
            line, re.I,
        )
        if match and match.group(1).lower() in SAVED_REGS:
            offsets.append(parse_number(match.group(2)))
    return min(offsets) if offsets else None


def stack_accesses(text):
    """Map each stack offset to its load/store mnemonic counts."""
    accesses = defaultdict(Counter)
    pattern = re.compile(r"(-?(?:0x[0-9a-f]+|\d+))\(\$?sp\)", re.I)
    for raw in text.splitlines():
        line = instruction_text(raw)
        insn = re.match(r"([a-z][a-z0-9.]*)\b", line, re.I)
        if not insn:
            continue
        for match in pattern.finditer(line):
            accesses[parse_number(match.group(1))][insn.group(1).lower()] += 1
    return {offset: dict(counts) for offset, counts in sorted(accesses.items())}


def analyze(text, args_hint=None):
    info = frame_info(text)
    if info["args"] is None:
        info["args"] = args_hint
    info["saved_start"] = saved_area_start(text)
    info["accesses"] = stack_accesses(text)
    floor = info["args"] if info["args"] is not None else 16
    ceiling = info["saved_start"]
    working = [offset for offset in info["accesses"]
               if offset >= floor and (ceiling is None or offset < ceiling)]
    info["workspace_start"] = min(working) if working else None
    info["workspace_end"] = ceiling
    info["workspace_size"] = (
        ceiling - info["workspace_start"]
        if working and ceiling is not None else None
    )
    return info


def target_text(name):
    files = sorted(glob.glob(f"{TARGET.format(name=name)}/*.s")) or glob.glob(
        f".shake/gen/main.exe/asm/nonmatchings/*/{name}.s"
    )
    if not files:
        sys.exit(f"stackplan: no target .s for {name}; split and build it first")
    return "\n".join(open(path, errors="replace").read() for path in files)


def fmt(value):
    return "unknown" if value is None else f"0x{value:x}"


def print_side(label, info):
    print(f"{label}: frame {fmt(info['frame'])}, args {fmt(info['args'])}, "
          f"saved area {fmt(info['saved_start'])}")
    if info["workspace_start"] is not None and info["workspace_end"] is not None:
        print(f"  working window: sp+{fmt(info['workspace_start'])}.."
              f"sp+{fmt(info['workspace_end'] - 1)} "
              f"({fmt(info['workspace_size'])} bytes)")
    offsets = [offset for offset in info["accesses"]
               if (info["workspace_start"] is not None and
                   offset >= info["workspace_start"] and
                   (info["workspace_end"] is None or offset < info["workspace_end"]))]
    if offsets:
        print("  working accesses:")
        for offset in offsets:
            counts = ", ".join(
                f"{op}x{count}" for op, count in sorted(info["accesses"][offset].items())
            )
            print(f"    sp+0x{offset:02x}: {counts}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("name")
    parser.add_argument("-n", "--no-build", action="store_true")
    parser.add_argument("--json", action="store_true",
                        help="emit the report as machine-readable JSON")
    args = parser.parse_args()

    if not args.no_build:
        subprocess.run(["./Build"], stdout=subprocess.DEVNULL, check=True)
    candidate_path = CANDIDATE.format(name=args.name)
    candidate = (open(candidate_path, errors="replace").read()
                 if os.path.exists(candidate_path) else "")
    candidate_info = analyze(candidate) if candidate else None
    args_hint = candidate_info["args"] if candidate_info else None
    target_info = analyze(target_text(args.name), args_hint=args_hint)
    report = {"name": args.name, "target": target_info,
              "candidate": candidate_info}
    if args.json:
        print(json.dumps(report, indent=2, sort_keys=True))
        return

    print(f"stackplan: {args.name}")
    print_side("target", target_info)
    if candidate_info:
        print_side("candidate", candidate_info)
        if (target_info["frame"] is not None and
                candidate_info["frame"] is not None):
            delta = candidate_info["frame"] - target_info["frame"]
            print(f"frame delta: {delta:+d} bytes")
    if target_info["workspace_size"] is not None:
        print("union hint: an explicit scratch overlay for the target working "
              f"window is {fmt(target_info['workspace_size'])} bytes")


if __name__ == "__main__":
    target = next((value for value in sys.argv[1:] if not value.startswith("-")), "-")
    try:
        with matching_tool_lock("stackplan", target):
            main()
    except MatchToolBusy as error:
        sys.exit(f"stackplan: {error}")

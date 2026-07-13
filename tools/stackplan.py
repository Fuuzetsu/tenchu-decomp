#!/usr/bin/env python3
"""Report the target/candidate stack workspace for one matching function.

Large functions often contain mutually-exclusive aggregate locals whose source
scopes nevertheless make old cc1 reserve separate slots.  The retail assembly
already tells us the useful layout: outgoing-argument space ends where locals
begin, and the first callee-saved spill marks the end of the local workspace.
This tool makes that boundary and every accessed ``sp+offset`` mechanical.

  tools/stackplan.py ProcItemFire
  tools/stackplan.py ProcItemFire -n --json
  tools/stackplan.py LoadConstruction -n --emit-overlay

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
    """Map each stack offset to load/store and address-taking counts."""
    accesses = defaultdict(Counter)
    pattern = re.compile(r"(-?(?:0x[0-9a-f]+|\d+))\(\$?sp\)", re.I)
    for raw in text.splitlines():
        line = instruction_text(raw)
        insn = re.match(r"([a-z][a-z0-9.]*)\b", line, re.I)
        if not insn:
            continue
        for match in pattern.finditer(line):
            accesses[parse_number(match.group(1))][insn.group(1).lower()] += 1
        address = re.match(
            r"addiu?\s+\$?([a-z0-9]+),\s*\$?sp,\s*"
            r"(-?(?:0x[0-9a-f]+|\d+))$",
            line, re.I,
        )
        if address and address.group(1).lower() != "sp":
            offset = parse_number(address.group(2))
            if offset >= 0:
                accesses[offset]["addr"] += 1
    return {offset: dict(counts) for offset, counts in sorted(accesses.items())}


def infer_args_from_accesses(accesses):
    """Infer o32 outgoing space below the first address-taken local.

    Split target assembly often loses cc1's ``args=`` frame comment.  Stores
    below the first formed local address are the stack-passed call arguments;
    round their end to the ABI's 8-byte stack alignment.  A slot which is also
    loaded by the function is local workspace, not outgoing-only argument
    space, even when it precedes the first address-taken aggregate.  Use the
    first such load-bearing slot as a stronger upper bound; this prevents a
    VECTOR at sp+0x10/14/18 from being hidden merely because a following
    SVECTOR first has its address formed at sp+0x20.

    Return None when the target provides no address boundary rather than
    guessing across locals.
    """
    address_offsets = [offset for offset, counts in accesses.items()
                       if counts.get("addr")]
    if not address_offsets:
        return None
    boundary = min(address_offsets)
    loaded_offsets = [
        offset for offset, counts in accesses.items()
        if offset < boundary and any(op.startswith("l") for op in counts)
    ]
    argument_ceiling = min(loaded_offsets) if loaded_offsets else boundary
    end = 0x10
    for offset, counts in accesses.items():
        if offset >= argument_ceiling:
            continue
        width, _ctype = access_width(counts)
        if any(op.startswith("s") for op in counts if op != "addr"):
            end = max(end, offset + max(width, 4))
    return (end + 7) & ~7


def analyze(text, args_hint=None):
    info = frame_info(text)
    info["saved_start"] = saved_area_start(text)
    info["accesses"] = stack_accesses(text)
    if info["args"] is None:
        info["args"] = (args_hint if args_hint is not None else
                        infer_args_from_accesses(info["accesses"]))
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


def access_width(counts):
    """Best scalar C width/type implied at one stack offset."""
    operations = set(counts) - {"addr"}
    if operations & {"ld", "sd"}:
        return 8, "unsigned long long"
    if operations & {"lw", "lwl", "lwr", "sw", "swl", "swr"}:
        return 4, "unsigned int"
    if operations & {"lh", "sh"}:
        return 2, "short"
    if "lhu" in operations:
        return 2, "unsigned short"
    if "lb" in operations:
        return 1, "signed char"
    if operations & {"lbu", "sb"}:
        return 1, "unsigned char"
    return 0, None


def emit_overlay(name, info):
    """Emit a conservative C struct pinned to the target workspace offsets.

    Address-only regions become byte arrays up to the next known access.  The
    scaffold intentionally uses neutral field names: PSX.SYM/local evidence is
    still needed to replace them with the original aggregates and semantics.
    """
    start, end = info["workspace_start"], info["workspace_end"]
    if start is None or end is None or start >= end:
        return f"/* {name}: target workspace is not recoverable */"
    offsets = [offset for offset in info["accesses"] if start <= offset < end]
    lines = ["typedef struct", "{"]
    cursor = start
    for index, offset in enumerate(offsets):
        if offset < cursor:
            continue
        if offset > cursor:
            lines.append(
                f"    unsigned char pad_{cursor - start:03x}[0x{offset - cursor:x}];"
            )
        counts = info["accesses"][offset]
        width, ctype = access_width(counts)
        next_offset = offsets[index + 1] if index + 1 < len(offsets) else end
        if width == 0:
            width = max(1, next_offset - offset)
            declaration = f"unsigned char bytes_{offset - start:03x}[0x{width:x}]"
        else:
            declaration = f"{ctype} field_{offset - start:03x}"
        operations = ", ".join(
            f"{op}x{count}" for op, count in sorted(counts.items())
        )
        lines.append(f"    {declaration}; /* sp+0x{offset:x}: {operations} */")
        cursor = offset + width
    if cursor < end:
        lines.append(
            f"    unsigned char pad_{cursor - start:03x}[0x{end - cursor:x}];"
        )
    lines.extend([
        f"}} {name}StackOverlay;",
        f"/* Declare `{name}StackOverlay stack;`: expected base sp+0x{start:x}, "
        f"size 0x{end - start:x}. Replace neutral fields with proven locals. */",
    ])
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("name")
    parser.add_argument("-n", "--no-build", action="store_true")
    parser.add_argument("--args", type=parse_number,
                        help="override/inject the outgoing-argument byte size")
    parser.add_argument("--json", action="store_true",
                        help="emit the report as machine-readable JSON")
    parser.add_argument("--emit-overlay", action="store_true",
                        help="append a padded C workspace-overlay scaffold")
    args = parser.parse_args()

    if not args.no_build:
        subprocess.run(["./Build"], stdout=subprocess.DEVNULL, check=True)
    candidate_path = CANDIDATE.format(name=args.name)
    candidate = (open(candidate_path, errors="replace").read()
                 if os.path.exists(candidate_path) else "")
    candidate_info = analyze(candidate, args_hint=args.args) if candidate else None
    args_hint = (args.args if args.args is not None else
                 candidate_info["args"] if candidate_info else None)
    target_info = analyze(target_text(args.name), args_hint=args_hint)
    report = {"name": args.name, "target": target_info,
              "candidate": candidate_info}
    if args.json:
        if args.emit_overlay:
            parser.error("--json and --emit-overlay are mutually exclusive")
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
    if args.emit_overlay:
        print()
        print(emit_overlay(args.name, target_info))


if __name__ == "__main__":
    target = next((value for value in sys.argv[1:] if not value.startswith("-")), "-")
    try:
        with matching_tool_lock("stackplan", target):
            main()
    except MatchToolBusy as error:
        sys.exit(f"stackplan: {error}")

#!/usr/bin/env python3
"""Resolve additive matching-metadata conflicts after a cherry-pick.

Parallel function branches commonly add independent entries to the same two
tables in ``shake/src/Build.hs`` and ``tools/permute.py``. Git quite reasonably
conflicts on the shared insertion point, but the intended merge is just the
ordered union. This helper performs only that narrow operation:

    tools/merge_metadata_conflicts.py

Every nonblank line inside a conflict must be a recognized ``extra``/``syms``
Haskell entry or Python dictionary entry. A repeated key with different text,
any ordinary code, malformed markers, or a conflict in another file is a hard
error and leaves both files unchanged. After success, stage the files and run
``git cherry-pick --continue`` as usual.
"""
from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_PATHS = ("shake/src/Build.hs", "tools/permute.py")

HASKELL_ENTRY = re.compile(r'^\s*(extra|syms)\s+"([A-Za-z0-9_]+)"\s*=\s*.+$')
PYTHON_ENTRY = re.compile(r'^\s*"([A-Za-z0-9_]+)"\s*:\s*.+,\s*$')


def entry_key(path: str, line: str) -> tuple[str, str] | None:
    """Stable (table-kind, function) key for one recognized metadata line."""
    if not line.strip():
        return None
    if path.endswith("Build.hs"):
        match = HASKELL_ENTRY.match(line)
        return (match.group(1), match.group(2)) if match else None
    if path.endswith("permute.py"):
        match = PYTHON_ENTRY.match(line)
        return ("python-entry", match.group(1)) if match else None
    return None


def merge_entries(path: str, ours: list[str], theirs: list[str]) -> list[str]:
    """Ordered union of one validated conflict's two sides."""
    merged: list[str] = []
    seen: dict[tuple[str, str], str] = {}
    for side, lines in (("ours", ours), ("theirs", theirs)):
        for line in lines:
            if not line.strip():
                continue
            key = entry_key(path, line)
            if key is None:
                raise ValueError(
                    f"{path}: refusing non-metadata {side} line: {line.rstrip()}"
                )
            normalized = line.strip()
            if key in seen:
                if seen[key] != normalized:
                    raise ValueError(
                        f"{path}: {key[1]} has different {key[0]} entries"
                    )
                continue
            seen[key] = normalized
            merged.append(line)
    return merged


def resolve_text(path: str, text: str) -> tuple[str, int]:
    """Return (resolved text, conflict count), or raise without partial output."""
    lines = text.splitlines(keepends=True)
    output: list[str] = []
    conflicts = 0
    index = 0
    while index < len(lines):
        line = lines[index]
        if not line.startswith("<<<<<<< "):
            if line.startswith(("=======", ">>>>>>> ")):
                raise ValueError(f"{path}: unmatched conflict marker at line {index + 1}")
            output.append(line)
            index += 1
            continue
        conflicts += 1
        index += 1
        ours: list[str] = []
        while index < len(lines) and not lines[index].startswith("======="):
            if lines[index].startswith(("<<<<<<< ", ">>>>>>> ")):
                raise ValueError(f"{path}: nested/malformed conflict")
            ours.append(lines[index])
            index += 1
        if index >= len(lines):
            raise ValueError(f"{path}: unterminated conflict (missing =======)")
        index += 1
        theirs: list[str] = []
        while index < len(lines) and not lines[index].startswith(">>>>>>> "):
            if lines[index].startswith(("<<<<<<< ", "=======")):
                raise ValueError(f"{path}: nested/malformed conflict")
            theirs.append(lines[index])
            index += 1
        if index >= len(lines):
            raise ValueError(f"{path}: unterminated conflict (missing >>>>>>>)")
        index += 1
        output.extend(merge_entries(path, ours, theirs))
    return "".join(output), conflicts


def atomic_write(path: str, text: str) -> None:
    directory = os.path.dirname(os.path.abspath(path))
    descriptor, staged = tempfile.mkstemp(prefix=".metadata-merge-", dir=directory)
    try:
        with os.fdopen(descriptor, "w") as stream:
            stream.write(text)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(staged, path)
    except BaseException:
        try:
            os.unlink(staged)
        except FileNotFoundError:
            pass
        raise


def unexpected_unmerged(paths: list[str], unmerged: list[str]) -> list[str]:
    """Unmerged git paths outside the explicitly authorized metadata files."""
    allowed = {os.path.normpath(path) for path in paths}
    return sorted(path for path in map(os.path.normpath, unmerged)
                  if path not in allowed)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("paths", nargs="*", default=list(DEFAULT_PATHS))
    parser.add_argument("--check", action="store_true",
                        help="validate and report, but do not write")
    args = parser.parse_args()

    status = subprocess.run(
        ["git", "diff", "--name-only", "--diff-filter=U"],
        capture_output=True, text=True,
    )
    if status.returncode != 0:
        sys.exit("merge-metadata-conflicts: cannot inspect git unmerged paths")
    unexpected = unexpected_unmerged(args.paths, status.stdout.splitlines())
    if unexpected:
        joined = ", ".join(unexpected)
        sys.exit("merge-metadata-conflicts: refusing non-metadata conflict(s): " +
                 joined)

    planned: list[tuple[str, str, int]] = []
    try:
        for path in args.paths:
            if not os.path.exists(path):
                raise ValueError(f"{path}: file does not exist")
            with open(path, errors="replace") as stream:
                original = stream.read()
            resolved, count = resolve_text(path, original)
            if count:
                planned.append((path, resolved, count))
    except ValueError as error:
        sys.exit(f"merge-metadata-conflicts: {error}")

    if not planned:
        print("merge-metadata-conflicts: no recognized conflict markers")
        return
    if not args.check:
        # All files were validated before the first write.
        for path, resolved, _count in planned:
            atomic_write(path, resolved)
    for path, _resolved, count in planned:
        action = "would resolve" if args.check else "resolved"
        print(f"merge-metadata-conflicts: {action} {count} conflict(s) in {path}")


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Shared integrity checks for guarded-draft fuzzy progress rows."""
from __future__ import annotations

import hashlib
import os
import re

GUARD = re.compile(r"^\s*#\s*ifndef\s+NON_MATCHING\b", re.M)


def guarded_sources(src_dir: str) -> dict[str, str]:
    out = {}
    for filename in sorted(os.listdir(src_dir)):
        if not filename.endswith(".c"):
            continue
        path = os.path.join(src_dir, filename)
        with open(path) as fh:
            text = fh.read()
        if GUARD.search(text):
            out[filename[:-2]] = path
    return out


def source_hash(path: str) -> str:
    with open(path, "rb") as fh:
        return hashlib.sha256(fh.read()).hexdigest()


def load_rows(path: str) -> tuple[dict[str, list[str]], list[str]]:
    rows = {}
    errors = []
    if not os.path.exists(path):
        return rows, errors
    with open(path) as fh:
        for lineno, line in enumerate(fh, 1):
            if not line.strip() or line.startswith("#"):
                continue
            fields = line.rstrip("\n").split("\t")
            if len(fields) < 3:
                errors.append(f"{path}:{lineno}: malformed fuzzy row")
                continue
            if fields[0] in rows:
                errors.append(f"{path}:{lineno}: duplicate row for {fields[0]}")
                continue
            rows[fields[0]] = fields
    return rows, errors


def retain_guarded(rows: dict[str, object], guarded: object) -> tuple[dict[str, object], list[str]]:
    """Return only rows whose names still identify guarded C drafts.

    Partial fuzzy rescoring must preserve unrequested *live* scores, but a
    function rename or promotion makes its old row invalid regardless of which
    new name was requested.  Keep this policy here so the writer and integrity
    checker share the same definition of an orphan.
    """
    keep = set(guarded)
    removed = sorted(set(rows) - keep)
    return {name: row for name, row in rows.items() if name in keep}, removed


def validate(src_dir: str, tsv: str) -> list[str]:
    sources = guarded_sources(src_dir)
    rows, errors = load_rows(tsv)
    for name in sorted(set(sources) - set(rows)):
        errors.append(f"missing fuzzy row for guarded draft {name}")
    for name in sorted(set(rows) - set(sources)):
        errors.append(f"orphan fuzzy row for non-guarded function {name}")
    for name in sorted(set(rows) & set(sources)):
        fields = rows[name]
        if len(fields) < 4 or not fields[3]:
            errors.append(f"unstamped fuzzy row for {name}")
        elif fields[3] != source_hash(sources[name]):
            errors.append(f"source-stale fuzzy row for {name}")
    return errors

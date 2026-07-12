#!/usr/bin/env python3
"""Per-worktree exclusion for matching tools that drive/read one Shake state.

Shake serializes individual build invocations, but that is not enough for
matching tools: autorules temporarily rewrites the live source between builds,
while the permuter expects a stable baseline.  Running both in one worktree can
therefore produce a perfectly serialized sequence of *logically torn* builds.

The advisory flock is scoped to `.shake/`, so separate git worktrees remain
independent. A driver token permits read-only child helpers (autorules invokes
matchdiff `-n`) to share the same lease. The kernel releases the real lock if a
process exits or is killed; the text inside is only a useful owner diagnostic.
"""
from __future__ import annotations

from contextlib import contextmanager
import fcntl
import os
import re
import secrets


LOCK_ENV = "TENCHU_MATCH_TOOL_TOKEN"


class MatchToolBusy(RuntimeError):
    pass


@contextmanager
def matching_tool_lock(tool: str, target: str):
    os.makedirs(".shake", exist_ok=True)
    path = os.path.join(".shake", "matching-tool.lock")
    stream = open(path, "a+", encoding="utf-8")
    acquired = False
    old_token = os.environ.get(LOCK_ENV)
    try:
        # A driver may invoke a read-only helper (autorules -> matchdiff -n).
        # Let that child share the parent's lease instead of deadlocking itself.
        stream.seek(0)
        owner = stream.read()
        if old_token and f"token={old_token}" in owner:
            yield
            return
        try:
            fcntl.flock(stream.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            stream.seek(0)
            owner = stream.read().strip() or "unknown owner"
            owner = re.sub(r"\s+token=\S+", "", owner)
            raise MatchToolBusy(
                f"another matching tool owns this worktree ({owner}); wait for "
                "that exact process/session to exit before starting another"
            )
        acquired = True
        token = secrets.token_hex(16)
        os.environ[LOCK_ENV] = token
        stream.seek(0)
        stream.truncate()
        stream.write(
            f"pid={os.getpid()} tool={tool} target={target} token={token}\n"
        )
        stream.flush()
        yield
    finally:
        try:
            if acquired:
                if old_token is None:
                    os.environ.pop(LOCK_ENV, None)
                else:
                    os.environ[LOCK_ENV] = old_token
                fcntl.flock(stream.fileno(), fcntl.LOCK_UN)
        finally:
            stream.close()

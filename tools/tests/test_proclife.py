#!/usr/bin/env python3
"""Tests for tools/proclife.py — the process-lifecycle helpers permute/autorules share.

`proclife` had NO tests, and its behaviour was being reasoned about from the source
instead of measured. That produced a wrong diagnosis that reached the matcher contract:
a lane reported `tools/permute.py` overrunning the harness's 600 s cap when piped and
attributed it to "the `-j4` workers inherit the pipe's write end and outlive the
SIGTERM, so `tail` never sees EOF". That mechanism is FALSE, and these tests are why we
know: `terminate_process_group` escalates SIGTERM -> SIGKILL across the whole session,
so even grandchildren that explicitly `SIG_IGN` SIGTERM are reaped and the pipe's write
end is released. Measured teardown for the exact reported shape: ~2 s.

The real cost model is in `authoritative_rescore`'s comment and is not about pipes:
**`timeout N tools/permute.py` bounds the SEARCH, not the tool.** The rescore runs after
the search's timeout fires, so the tool costs N + PERMUTE_RESCORE_SECONDS (90 default)
+ one full link. That arithmetic — not a pipe — explains the recorded observation of a
process alive at 526 s under a 420 s bound (420 + 90 = 510).

Run: nix develop -c python -m unittest tools.tests.test_proclife
"""
import os
import select
import signal
import subprocess
import sys
import time
import unittest

TOOLS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if TOOLS not in sys.path:
    sys.path.insert(0, TOOLS)

import proclife

# A child that spawns N grandchildren which IGNORE SIGTERM and inherit stdout, then
# blocks. This is the shape the lane described for decomp-permuter's `-j4` workers,
# made deliberately harder: SIG_IGN means a plain SIGTERM cannot reap them.
HOLDER = """
import signal, subprocess, sys, time
kids = [subprocess.Popen([sys.executable, "-c",
        "import signal,time;signal.signal(signal.SIGTERM,signal.SIG_IGN);"
        "time.sleep(120)"]) for _ in range({n})]
time.sleep(120)
"""


def _pipe_is_at_eof(read_fd, timeout):
    """True if read_fd reaches EOF within `timeout` — i.e. every writer let go."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        ready, _, _ = select.select([read_fd], [], [], 0.1)
        if ready:
            return os.read(read_fd, 4096) == b""
    return False


class TerminateProcessGroupTest(unittest.TestCase):
    def _spawn_holder(self, workers=4):
        read_fd, write_fd = os.pipe()
        proc = proclife.owned_popen(
            [sys.executable, "-c", HOLDER.format(n=workers)], stdout=write_fd)
        # Drop the parent's copy so the ONLY remaining writers are proc and its
        # grandchildren. Without this the test could never observe EOF and would
        # "pass" vacuously in the failure direction.
        os.close(write_fd)
        self.addCleanup(lambda: os.close(read_fd) if not self._closed else None)
        self._closed = False
        time.sleep(1.0)  # let the grandchildren come up and inherit the fd
        return proc, read_fd

    def setUp(self):
        self._closed = False

    def test_reaps_grandchildren_that_ignore_sigterm(self):
        """The reported mechanism, made harder: SIG_IGN workers still die."""
        proc, read_fd = self._spawn_holder()
        started = time.monotonic()
        proclife.terminate_process_group(proc)
        elapsed = time.monotonic() - started
        # SIGTERM is ignored by the grandchildren, so this only passes via the
        # SIGKILL escalation. Budget covers terminate_process_group's own 2 s wait.
        self.assertTrue(_pipe_is_at_eof(read_fd, timeout=10),
                        "pipe never reached EOF: a writer outlived teardown — this "
                        "IS the failure the matcher contract once claimed happens")
        self.assertLess(elapsed, 10, "teardown should not block on ignored SIGTERM")
        os.close(read_fd)
        self._closed = True

    def test_sigterm_mid_wait_releases_the_pipe(self):
        """permute.py's exact structure: SIGTERM lands inside proc.wait()."""
        proc, read_fd = self._spawn_holder()
        interrupted = False
        try:
            with proclife.interruption_handlers("test"):
                # Deliver SIGTERM to ourselves the way `timeout` would, while we are
                # blocked in wait(). The handler must raise THROUGH wait().
                signal.setitimer(signal.ITIMER_REAL, 0.5)
                old = signal.signal(signal.SIGALRM,
                                    lambda *_: os.kill(os.getpid(), signal.SIGTERM))
                try:
                    proc.wait()
                except BaseException:
                    proclife.terminate_process_group(proc)
                    raise
                finally:
                    signal.setitimer(signal.ITIMER_REAL, 0)
                    signal.signal(signal.SIGALRM, old)
        except InterruptedError:
            interrupted = True
        self.assertTrue(interrupted, "SIGTERM must surface as InterruptedError")
        self.assertTrue(_pipe_is_at_eof(read_fd, timeout=10),
                        "pipe still had a writer after the SIGTERM teardown path")
        os.close(read_fd)
        self._closed = True

    def test_teardown_is_what_closes_the_pipe(self):
        """Fault injection: WITHOUT teardown the pipe stays open.

        A guard never observed failing is not known to work. This pins that the two
        tests above are measuring teardown and not something incidental — if this
        test ever starts seeing EOF, they have gone vacuous.
        """
        proc, read_fd = self._spawn_holder()
        try:
            self.assertFalse(_pipe_is_at_eof(read_fd, timeout=2),
                             "pipe hit EOF with the holder still alive — the other "
                             "tests in this file no longer prove anything")
        finally:
            proclife.terminate_process_group(proc)
            os.close(read_fd)
            self._closed = True

    def test_owned_popen_starts_its_own_session(self):
        """killpg(proc.pid) is only correct because the child leads its own group."""
        proc = proclife.owned_popen([sys.executable, "-c", "import time;time.sleep(30)"])
        try:
            self.assertEqual(os.getpgid(proc.pid), proc.pid,
                             "child is not its own process-group leader, so "
                             "terminate_process_group's killpg(proc.pid) would "
                             "signal the WRONG group")
            self.assertNotEqual(os.getpgid(proc.pid), os.getpgid(0))
        finally:
            proclife.terminate_process_group(proc)

    def test_terminate_is_idempotent_on_a_dead_child(self):
        """Teardown runs on paths where the child already exited; must not raise."""
        proc = proclife.owned_popen([sys.executable, "-c", ""])
        proc.wait()
        proclife.terminate_process_group(proc)  # ProcessLookupError must be swallowed


class RescoreBudgetTest(unittest.TestCase):
    """The real reason a bounded permuter run overruns — arithmetic, not pipes."""

    def test_rescore_budget_is_documented_and_env_overridable(self):
        import permute
        with open(permute.__file__) as stream:
            source = stream.read()
        self.assertIn('PERMUTE_RESCORE_SECONDS', source)
        # The whole point of the deadline is that `timeout N` does NOT bound the
        # tool. If this default ever changes, the contract's budget guidance
        # (timeout <= cap - rescore - one link) has to change with it.
        self.assertIn('"90"', source,
                      "rescore default changed — update the matcher contract's "
                      "permuter budget arithmetic to match")


if __name__ == "__main__":
    unittest.main()

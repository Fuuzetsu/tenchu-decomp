#!/usr/bin/env python3
"""Tests for tools/ccsrc.py — reading the pinned gcc-2.8.1 source.

The tool exists because lanes modelled cc1's behaviour from recollection instead of
reading it, and were wrong (see the tool's docstring). Its own correctness therefore
matters more than usual: a ccsrc that silently finds nothing would send a lane back to
guessing, which is the exact failure it was built to stop. So every not-found path here
must exit non-zero, and the fake-tree tests pin the extraction rules independently of
whether the nix store is realised.

Run: nix develop -c python -m unittest tools.tests.test_ccsrc
"""
import os
import sys
import tempfile
import unittest

TOOLS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if TOOLS not in sys.path:
    sys.path.insert(0, TOOLS)

import ccsrc

# A K&R definition shaped exactly like gcc 2.8.1's: return type on its own line, name
# at column 0, parameter declarations before the brace.
FAKE_SCHED = """\
/* Some leading comment. */

static int
other_thing (x)
     int x;
{
  return x;
}

__inline static void
adjust_priority (prev)
     rtx prev;
{
  if (reload_completed == 0)
    {
      if (birthing_insn_p (PATTERN (prev)))
        INSN_PRIORITY (prev) = max_priority;
    }
}

#define LAUNCH_PRIORITY\t0x7f000001
#define MULTILINE(a, b) \\
  do { (a) = (b); } while (0)
"""


class FakeTreeTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = self.tmp.name
        with open(os.path.join(self.root, "sched.c"), "w") as stream:
            stream.write(FAKE_SCHED)
        os.makedirs(os.path.join(self.root, "config", "mips"))
        with open(os.path.join(self.root, "config", "mips", "mips.h"), "w") as stream:
            stream.write("/* no REG_ALLOC_ORDER here, and that is load-bearing */\n")
        os.environ["CC1_SOURCE"] = self.root
        self.addCleanup(os.environ.pop, "CC1_SOURCE", None)

    def test_finds_knr_function_and_macro_separately(self):
        hits = ccsrc.find_definition(self.root, "adjust_priority")
        self.assertEqual(len(hits), 1)
        self.assertEqual(hits[0][2], "function")
        hits = ccsrc.find_definition(self.root, "LAUNCH_PRIORITY")
        self.assertEqual(len(hits), 1)
        self.assertEqual(hits[0][2], "macro")

    def test_knr_extraction_includes_return_type_and_stops_at_brace(self):
        """The return type is on a SEPARATE line above the name — miss it and the
        reader cannot tell a static from an extern, or void from int."""
        hits = ccsrc.find_definition(self.root, "adjust_priority")
        path, line, _kind = hits[0]
        import io
        import contextlib
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            ccsrc.print_function(path, line)
        out = buf.getvalue()
        self.assertIn("__inline static void", out)
        self.assertIn("adjust_priority (prev)", out)
        self.assertIn("INSN_PRIORITY (prev) = max_priority;", out)
        # Must stop at ITS closing brace, not run into the next definition.
        self.assertNotIn("LAUNCH_PRIORITY", out)
        # And must not have swallowed the PRECEDING function.
        self.assertNotIn("other_thing", out)

    def test_macro_extraction_follows_backslash_continuations(self):
        hits = ccsrc.find_definition(self.root, "MULTILINE")
        path, line, kind = hits[0]
        self.assertEqual(kind, "macro")
        import io
        import contextlib
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            ccsrc.print_macro(path, line)
        out = buf.getvalue()
        self.assertIn("#define MULTILINE", out)
        self.assertIn("while (0)", out, "stopped before the continuation line")

    def test_unknown_symbol_exits_nonzero_and_suggests_grep(self):
        """Silently finding nothing is the one failure this tool must never have."""
        with self.assertRaises(SystemExit) as caught:
            ccsrc.main(["NoSuchThing"])
        self.assertNotEqual(caught.exception.code, 0)
        self.assertIn("--grep", str(caught.exception))

    def test_tree_resolves_from_env(self):
        self.assertEqual(ccsrc.tree(), self.root)


class MissingTreeTest(unittest.TestCase):
    def test_absent_cc1_source_fails_loudly(self):
        """Fault injection: if the tree is gone the tool must refuse, not degrade.

        A tool that quietly returns nothing here would send the lane back to
        recalling what gcc does — the exact thing ccsrc exists to prevent — so the
        message says so explicitly.
        """
        os.environ["CC1_SOURCE"] = "/nonexistent-tree-for-test"
        self.addCleanup(os.environ.pop, "CC1_SOURCE", None)
        with self.assertRaises(SystemExit) as caught:
            ccsrc.tree()
        self.assertNotEqual(caught.exception.code, 0)


class RealTreeTest(unittest.TestCase):
    """Pins against the ACTUAL pinned compiler — these are the facts lanes cite."""

    def setUp(self):
        os.environ.pop("CC1_SOURCE", None)
        if not os.path.isdir(ccsrc.PINNED):
            self.skipTest("pinned gcc tree not realised in the nix store")

    def test_the_tree_is_flat(self):
        """The trap: sched.c is at the ROOT. Every instinct says gcc/sched.c."""
        self.assertTrue(os.path.exists(os.path.join(ccsrc.PINNED, "sched.c")))
        self.assertFalse(os.path.exists(os.path.join(ccsrc.PINNED, "gcc", "sched.c")))

    def test_target_files_are_the_exception(self):
        self.assertTrue(os.path.exists(
            os.path.join(ccsrc.PINNED, "config", "mips", "mips.h")))

    def test_cited_scheduler_symbols_resolve(self):
        """If these move, every cookbook citation of them is stale."""
        for symbol, expect_file in [
            ("adjust_priority", "sched.c"),
            ("birthing_insn_p", "sched.c"),
            ("rank_for_schedule", "sched.c"),
            ("priority", "sched.c"),
            ("LAUNCH_PRIORITY", "sched.c"),
        ]:
            hits = ccsrc.find_definition(ccsrc.PINNED, symbol)
            self.assertTrue(hits, f"{symbol} not found in the pinned tree")
            self.assertTrue(any(os.path.basename(p) == expect_file for p, _l, _k in hits),
                            f"{symbol} is not in {expect_file} any more")

    def test_mips_h_defines_no_reg_alloc_order_or_adjust_priority(self):
        """ABSENCE is load-bearing here: it is why find_reg walks numerically, and
        why 'MIPS defines no ADJUST_PRIORITY' is true but irrelevant (the bump lives
        in the FUNCTION adjust_priority, not the macro)."""
        with open(os.path.join(ccsrc.PINNED, "config", "mips", "mips.h"),
                  errors="replace") as stream:
            body = stream.read()
        self.assertNotIn("#define REG_ALLOC_ORDER", body)
        self.assertNotIn("#define ADJUST_PRIORITY", body)


if __name__ == "__main__":
    unittest.main()

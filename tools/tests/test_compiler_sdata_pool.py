"""Tests for restoring compiler-pooled aggregate initializers."""

from __future__ import annotations

import unittest

from tools import compiler_sdata_pool as pool


class CompilerSdataPoolTests(unittest.TestCase):
    SOURCE = """\
.section .sdata
.align\t2
$LC0:
.half\t0
.half\t-100
.half\t0
.space\t2
.text
lui\t$2,%hi($LC0)
addiu\t$2,$2,%lo($LC0)
"""

    def test_owner_exports_the_compiler_constant(self) -> None:
        output = pool.transform(
            self.SOURCE,
            symbol="__compiler_sdata_pool_y_n100",
            halfwords=(0, -100, 0),
            owner=True,
        )
        self.assertIn(
            ".align\t2\n"
            ".globl\t__compiler_sdata_pool_y_n100\n"
            ".hidden\t__compiler_sdata_pool_y_n100\n"
            "__compiler_sdata_pool_y_n100:\n"
            ".half\t0\n.half\t-100\n.half\t0\n.space\t2\n",
            output,
        )
        self.assertEqual(output.count("__compiler_sdata_pool_y_n100"), 5)
        self.assertNotIn("$LC0", output)

    def test_user_references_owner_without_emitting_a_second_copy(self) -> None:
        output = pool.transform(
            self.SOURCE,
            symbol="__compiler_sdata_pool_y_n100",
            halfwords=(0, -100, 0),
            owner=False,
        )
        self.assertEqual(
            output,
            ".section .sdata\n"
            ".text\n"
            "lui\t$2,%hi(__compiler_sdata_pool_y_n100)\n"
            "addiu\t$2,$2,%lo(__compiler_sdata_pool_y_n100)\n",
        )

    def test_user_removes_only_the_selected_initializer(self) -> None:
        source = self.SOURCE.replace(
            ".text\n",
            ".align\t2\n"
            "$LC1:\n"
            ".half\t0\n"
            ".half\t-25\n"
            ".half\t0\n"
            ".space\t2\n"
            ".text\n"
            "lui\t$3,%hi($LC1)\n",
        )
        output = pool.transform(
            source,
            symbol="__compiler_sdata_pool_y_n25",
            halfwords=(0, -25, 0),
            owner=False,
        )
        self.assertIn("$LC0:", output)
        self.assertIn(".half\t-100", output)
        self.assertNotIn("$LC1", output)
        self.assertIn("%hi(__compiler_sdata_pool_y_n25)", output)

    def test_rejects_a_missing_initializer(self) -> None:
        with self.assertRaisesRegex(pool.PoolError, "found 0"):
            pool.transform(
                self.SOURCE,
                symbol="__compiler_sdata_pool_y_n25",
                halfwords=(0, -25, 0),
                owner=False,
            )


if __name__ == "__main__":
    unittest.main()

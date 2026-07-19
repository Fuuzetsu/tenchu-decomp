from __future__ import annotations

import struct
import unittest

from tools import scaffold_exe


def word(w: int) -> bytes:
    return struct.pack("<I", w)


def jal(target: int) -> int:
    return 0x0C000000 | ((target & 0x0FFFFFFF) >> 2)


def j(target: int) -> int:
    return 0x08000000 | ((target & 0x0FFFFFFF) >> 2)


def beq(pc: int, target: int) -> int:
    return 0x10000000 | (((target - pc - 4) >> 2) & 0xFFFF)


class MergeSplitFunctionTests(unittest.TestCase):
    BASE = 0x80011000

    def blob(self, words: list[int]) -> bytes:
        return bytes(0x800) + b"".join(word(w) for w in words)

    def test_branch_across_a_start_removes_it(self) -> None:
        base = self.BASE
        words = [0] * 16
        words[2] = beq(base + 8, base + 0x20)  # branch over the false start
        blob = self.blob(words)
        starts = [base, base + 0x10, base + 0x30]
        kept = scaffold_exe.merge_split_functions(
            blob, base, 0x800, 0x800 + len(words) * 4, starts
        )
        self.assertEqual(kept, [base, base + 0x30])

    def test_j_to_existing_start_is_a_tail_call_not_a_merge(self) -> None:
        base = self.BASE
        words = [0] * 16
        words[2] = j(base + 0x10)  # jumps exactly to the next function
        blob = self.blob(words)
        starts = [base, base + 0x10]
        kept = scaffold_exe.merge_split_functions(
            blob, base, 0x800, 0x800 + len(words) * 4, starts
        )
        self.assertEqual(kept, starts)

    def test_j_into_a_chunk_middle_merges_the_intervening_start(self) -> None:
        base = self.BASE
        words = [0] * 16
        words[2] = j(base + 0x18)  # target is not a start: local jump
        blob = self.blob(words)
        starts = [base, base + 0x10]
        kept = scaffold_exe.merge_split_functions(
            blob, base, 0x800, 0x800 + len(words) * 4, starts
        )
        self.assertEqual(kept, [base])

    def test_wild_span_branches_are_distrusted(self) -> None:
        base = self.BASE
        words = [0] * 16
        words[2] = beq(base + 8, base + 8 + 0x7FFC * 2)  # data-word fake
        blob = self.blob(words)
        starts = [base, base + 0x10]
        kept = scaffold_exe.merge_split_functions(
            blob, base, 0x800, 0x800 + len(words) * 4, starts
        )
        self.assertEqual(kept, starts)


class BranchTargetTests(unittest.TestCase):
    def test_conditional_and_regimm_and_cop_branches_decode(self) -> None:
        pc = 0x80020000
        self.assertEqual(
            scaffold_exe.branch_target(beq(pc, pc + 0x40), pc), pc + 0x40
        )
        bltz = 0x04000000 | ((0x10 >> 2) & 0xFFFF)
        self.assertEqual(scaffold_exe.branch_target(bltz, pc), pc + 4 + 0x10)
        bc2t = 0x49010000 | ((-8 >> 2) & 0xFFFF)
        self.assertEqual(scaffold_exe.branch_target(bc2t, pc), pc + 4 - 8)

    def test_non_branches_do_not_decode(self) -> None:
        self.assertIsNone(scaffold_exe.branch_target(0x27BDFFE8, 0x80020000))
        self.assertIsNone(scaffold_exe.branch_target(jal(0x80020000), 0x80020000))


if __name__ == "__main__":
    unittest.main()

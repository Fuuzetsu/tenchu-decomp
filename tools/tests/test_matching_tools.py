#!/usr/bin/env python3
import os
import sys
import tempfile
import unittest
from unittest import mock
import contextlib
import io
import inspect
import fcntl
import signal
import subprocess

TOOLS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if TOOLS not in sys.path:
    sys.path.insert(0, TOOLS)

import autorules
import asmdiff
import callmatch
import datamatch
import function_inventory
import matchlock
import matchdiff
import maspsxflags
import merge_metadata_conflicts
import permute
import regalloc
import rtlguide
import stackplan
import symnear


class FunctionInventoryTests(unittest.TestCase):
    def test_current_c_names_overlay_stale_boundaries(self):
        with tempfile.TemporaryDirectory() as td:
            funcs = os.path.join(td, "functions.tsv")
            splat = os.path.join(td, "splat.yaml")
            with open(funcs, "w") as fh:
                fh.write("80011000\t16\tFUN_80011000\n")
                fh.write("80011010\t20\tAlreadyCurrent\n")
                fh.write("80011024\t12\tFUN_80011024\n")
            with open(splat, "w") as fh:
                fh.write("segments:\n")
                fh.write("  - [0x800, c, RecoveredName]\n")
                fh.write("  - [0x810, c, AlreadyCurrent]\n")
                fh.write("  - [0x824, data, MisleadingDataName]\n")

            rows = function_inventory.load_functions(funcs)
            rows, changed = function_inventory.overlay_current_names(rows, splat)

        self.assertEqual(changed, 1)
        self.assertEqual(rows, [
            (0x80011000, 16, "RecoveredName"),
            (0x80011010, 20, "AlreadyCurrent"),
            (0x80011024, 12, "FUN_80011024"),
        ])

    def test_splat_name_loader_ignores_non_c_aliases(self):
        with tempfile.NamedTemporaryFile("w", delete=False) as fh:
            path = fh.name
            fh.write("  - [0x800, c, Function]\n")
            fh.write("  - [0x804, asm, StaleAsmAlias]\n")
            fh.write("  - { start: 0x800, type: .rodata, name: JumpTable }\n")
        try:
            self.assertEqual(
                function_inventory.load_splat_c_names(path),
                {0x80011000: "Function"},
            )
        finally:
            os.unlink(path)

    def test_datamatch_uses_current_c_names(self):
        with tempfile.TemporaryDirectory() as td:
            funcs = os.path.join(td, "functions.tsv")
            splat = os.path.join(td, "splat.yaml")
            with open(funcs, "w") as fh:
                fh.write("80011000\t16\tFUN_80011000\n")
            with open(splat, "w") as fh:
                fh.write("  - [0x800, c, RecoveredName]\n")

            functions, changed = datamatch.load_retail_functions(funcs, splat)

        self.assertEqual(changed, 1)
        self.assertEqual(functions, {"RecoveredName": (0x80011000, 16)})


class CallMatchAmbiguityTests(unittest.TestCase):
    def test_pareto_better_containment_fit_blocks_confirmation(self):
        from collections import Counter

        names = {
            0x1000: (300, "Current"),
            0x2000: (232, "BetterSibling"),
            0x3000: (900, "TooLarge"),
        }
        demo = Counter(("A", "B", "C"))
        calls = {
            0x1000: Counter(("A", "B", "C", "X", "Y")),
            0x2000: Counter(("A", "B", "C", "X")),
            0x3000: Counter(("A", "B", "C")),
        }

        alternatives = callmatch.ambiguity_candidates(
            0x1000, names, calls, demo, 208
        )

        self.assertEqual([a[2] for a in alternatives], [0x2000])

    def test_worse_call_or_size_fit_does_not_create_noise(self):
        from collections import Counter

        names = {
            0x1000: (220, "Current"),
            0x2000: (210, "MoreExtras"),
            0x3000: (800, "FartherSize"),
        }
        demo = Counter(("A", "B", "C"))
        calls = {
            0x1000: Counter(("A", "B", "C", "X")),
            0x2000: Counter(("A", "B", "C", "X", "Y")),
            0x3000: Counter(("A", "B", "C")),
        }

        self.assertEqual(
            callmatch.ambiguity_candidates(0x1000, names, calls, demo, 208),
            [],
        )


class AutoRulesAdvancedTests(unittest.TestCase):
    def setUp(self):
        autorules.GUIDED_LINES = set()
        autorules._TARGET_GP_CACHE.clear()

    def candidates(self, rule, source):
        return list(rule(source, "F", (0, len(source))))

    def test_cmp_polarity_swaps_local_operands(self):
        source = """int F(int direction, int turn) {
    if (direction < turn) return 1;
    return 0;
}
"""
        out = self.candidates(autorules.rule_cmp_polarity, source)
        self.assertEqual(len(out), 1)
        self.assertIn("turn > direction", out[0][1])

    def test_eq_literal_swap_reverses_commutative_operands(self):
        source = """int Global;
int F(void) {
    if (Global != 0x906) return 1;
    return 0;
}
"""
        out = self.candidates(autorules.rule_eq_literal_swap, source)
        self.assertEqual(len(out), 1)
        self.assertIn("0x906 != Global", out[0][1])

    def test_if_else_invert_swaps_compound_arms_once(self):
        source = """int F(int result) {
    if (result != 0) {
        fail();
    } else {
        copy();
    }
}
"""
        autorules.GUIDED_LINES = {2}
        out = self.candidates(autorules.rule_if_else_invert, source)
        self.assertEqual(len(out), 1)
        self.assertIn("if (!(result != 0)) {\n        copy();", out[0][1])
        self.assertIn("else {\n        fail();", out[0][1])

    def test_adjacent_literal_field_stores_swap(self):
        source = """void F(Frame *frame) {
    frame->size = 0x3000;
    frame->mode = 0;
}
"""
        autorules.GUIDED_LINES = {2}
        out = self.candidates(autorules.rule_adjacent_field_store_swap, source)
        self.assertEqual(len(out), 1)
        self.assertLess(out[0][1].index("frame->mode = 0"),
                        out[0][1].index("frame->size = 0x3000"))

    def test_assignment_chain_merges_same_literal_field_stores(self):
        source = """void F(Frame *frame) {
    Frame *p = frame;
    p->r = 0x80;
    p->g = 0x80;
    p->b = 0x80;
}
"""
        autorules.GUIDED_LINES = {3}
        out = self.candidates(autorules.rule_assignment_chain, source)
        merged = [text for label, text in out
                  if label.startswith("assignment-chain merge 3 L3")]
        self.assertEqual(len(merged), 1)
        self.assertIn("p->r = p->g = p->b = 0x80;", merged[0])

    def test_assignment_chain_splits_forward_and_reverse(self):
        source = """void F(Frame *frame) {
    Frame *p = frame;
    p->x = p->y = 0x10;
}
"""
        autorules.GUIDED_LINES = {3}
        out = self.candidates(autorules.rule_assignment_chain, source)
        forward = [text for label, text in out if "split-forward" in label]
        reverse = [text for label, text in out if "split-reverse" in label]
        self.assertEqual((len(forward), len(reverse)), (1, 1))
        self.assertIn("p->x = 0x10;\n    p->y = 0x10;", forward[0])
        self.assertIn("p->y = 0x10;\n    p->x = 0x10;", reverse[0])

    def test_adjacent_field_store_swap_rejects_nonliteral_rhs(self):
        source = """void F(Frame *frame, int value) {
    frame->size = value;
    frame->mode = 0;
}
"""
        autorules.GUIDED_LINES = {2}
        self.assertEqual(
            self.candidates(autorules.rule_adjacent_field_store_swap, source),
            [],
        )

    def test_ptr_index_sum_handles_later_double_pointer_assignment(self):
        source = """void F(Node **base, int index) {
    Node **slot;
    slot = base + index;
    use(slot);
}
"""
        out = self.candidates(autorules.rule_ptr_index_sum, source)
        assignments = [text for label, text in out
                       if label.startswith("ptr-sum-assign base+index")]
        self.assertEqual(len(assignments), 1)
        self.assertIn(
            "slot = (Node **)((u32)base + index * sizeof(*slot));",
            assignments[0],
        )

    def test_ptr_base_split_names_global_array_before_indexing(self):
        source = """Node *F(int index) {
    Node *stage = &StageConfig[index];
    return stage;
}
"""
        autorules.GUIDED_LINES = {2}
        out = self.candidates(autorules.rule_ptr_base_split, source)
        self.assertEqual(len(out), 1)
        self.assertIn("Node *_match_base = StageConfig;", out[0][1])
        self.assertIn("Node *stage = &_match_base[index];", out[0][1])

    def test_late_pointer_direct_drops_reassignment_and_rewrites_members(self):
        source = """void F(void) {
    Node *p;
    p = Global;
    p->first = 1;
    p = Global;
    value = __builtin_abs(value);
    p->second = 2;
    value = p->third;
}
"""
        out = self.candidates(autorules.rule_late_pointer_direct, source)
        self.assertEqual(len(out), 1)
        candidate = out[0][1]
        self.assertEqual(candidate.count("p = Global;"), 1)
        self.assertIn("Global->second = 2;", candidate)
        self.assertIn("value = Global->third;", candidate)

    def test_late_pointer_direct_rejects_call_crossing_region(self):
        source = """void F(void) {
    Node *p;
    p = Global;
    p->first = 1;
    p = Global;
    mutate();
    p->second = 2;
}
"""
        out = self.candidates(autorules.rule_late_pointer_direct, source)
        self.assertEqual(out, [])

    def test_param_width_flips_plain_integer_parameters_only(self):
        source = """void F(s16 x, u32 y, Node *pointer) {
    use(x, y, pointer);
}
"""
        out = self.candidates(autorules.rule_param_width, source)
        labels = [label for label, _candidate in out]
        self.assertEqual(len(out), 5)
        self.assertIn("param x: s16→s32", labels)
        self.assertIn("param x: s16→u16", labels)
        self.assertIn("param y: u32→s32", labels)
        self.assertFalse(any("pointer" in label for label in labels))

    def test_eq_literal_swap_rejects_two_values_or_side_effects(self):
        values = "int F(int a, int b) { return a == b; }\n"
        effect = "int next(void); int F(void) { return next() != 1; }\n"
        self.assertEqual(
            self.candidates(autorules.rule_eq_literal_swap, values), [])
        self.assertEqual(
            self.candidates(autorules.rule_eq_literal_swap, effect), [])

    def test_pointee_volatile_toggles_local_integer_pointer(self):
        source = """typedef unsigned short u16;
int F(u16 *input) {
    u16 *view;
    view = input;
    return *view;
}
"""
        added = self.candidates(autorules.rule_pointee_volatile, source)
        self.assertEqual(len(added), 1)
        self.assertIn("volatile u16 *view;", added[0][1])
        removed = self.candidates(autorules.rule_pointee_volatile, added[0][1])
        self.assertEqual(len(removed), 1)
        self.assertIn("u16 *view;", removed[0][1])
        self.assertNotIn("volatile u16 *view;", removed[0][1])

    def test_pointee_volatile_rejects_multiple_declarators(self):
        source = """typedef unsigned short u16;
int F(void) { u16 *first, *second; return 0; }
"""
        self.assertEqual(
            self.candidates(autorules.rule_pointee_volatile, source), [])

    def test_loop_fence_wraps_an_if(self):
        source = """int F(int value) {
    if (value) value++;
    return value;
}
"""
        out = self.candidates(autorules.rule_loop_fence, source)
        self.assertEqual(len(out), 1)
        self.assertIn("do {", out[0][1])
        self.assertIn("} while (0);", out[0][1])

    def test_loop_fence_wraps_an_assignment(self):
        source = """int F(int value) {
    value = 0;
    return value;
}
"""
        out = self.candidates(autorules.rule_loop_fence, source)
        self.assertEqual(len(out), 1)
        self.assertIn("value = 0;", out[0][1])
        self.assertIn("} while (0);", out[0][1])

    def test_paired_loop_fence_is_one_atomic_candidate(self):
        source = """int F(int value) {
    value = value + 1;
    value = value * 2;
    value = value - 3;
    return value;
}
"""
        autorules.GUIDED_LINES = {2}
        out = self.candidates(autorules.rule_paired_loop_fence, source)
        pair = [text for label, text in out
                if label.startswith("paired-loop-fence 2+1 L2-4")]
        self.assertEqual(len(pair), 1)
        self.assertEqual(pair[0].count("} while (0);"), 2)

    def test_nested_loop_fence_adds_two_weights_atomically(self):
        source = """int F(int value) {
    value = value + 1;
    return value;
}
"""
        autorules.GUIDED_LINES = {2}
        out = self.candidates(autorules.rule_nested_loop_fence, source)
        depth_two = [text for label, text in out
                     if label.startswith("nested-loop-fence 2 L2")]
        self.assertEqual(len(depth_two), 1)
        self.assertEqual(depth_two[0].count("} while (0);"), 2)

    def test_loop_fence_rejects_if_with_switch_break(self):
        source = """int F(int a) {
    switch (a) {
    case 1:
        if (a) break;
    }
    return a;
}
"""
        self.assertEqual(self.candidates(autorules.rule_loop_fence, source), [])

    def test_sparse_equality_ladder_enumerates_case_body_orders(self):
        source = """void F(int pad) {
    if (pad == 0x20) { accept(); }
    else if (pad == 0x2000) { left(); }
    else if (pad == 0x8000) { right(); }
}
"""
        autorules.GUIDED_LINES = {2}
        out = self.candidates(autorules.rule_sparse_eq_switch, source)
        self.assertEqual(len(out), 6)
        labels = {label for label, _text in out}
        self.assertTrue(any("0x2000,0x20,0x8000" in label for label in labels))
        self.assertTrue(all("switch (pad)" in text for _label, text in out))

    def test_sparse_equality_switch_rejects_different_indices(self):
        source = """void F(int pad, int other) {
    if (pad == 1) { a(); }
    else if (other == 2) { b(); }
    else if (pad == 3) { c(); }
}
"""
        self.assertEqual(
            self.candidates(autorules.rule_sparse_eq_switch, source), [])

    def test_loop_range_can_fence_three_adjacent_statements(self):
        source = """int F(int value) {
    if (value) value++;
    if (value > 2) value--;
    if (value < 4) value += 3;
    return value;
}
"""
        out = self.candidates(autorules.rule_loop_range, source)
        three = [text for label, text in out if label.startswith("loop-range 3 ")]
        self.assertTrue(three)
        self.assertIn("do {", three[0])
        self.assertIn("if (value < 4) value += 3;", three[0])

    def test_loop_range_rejects_captured_control_flow(self):
        source = """int F(int value) {
    while (value) {
        if (value == 2) break;
        value--;
    }
    return value;
}
"""
        self.assertEqual(self.candidates(autorules.rule_loop_range, source), [])

    def test_loop_boundary_shift_moves_next_statement_before_loop_end(self):
        source = """int F(int x) {
    int y;
    do {
        x++;
    } while (0);
    y = x + 1;
    return y;
}
"""
        out = self.candidates(autorules.rule_loop_boundary_shift, source)
        self.assertEqual(len(out), 1)
        candidate = out[0][1]
        self.assertLess(candidate.index("y = x + 1;"), candidate.index("while (0);"))

    def test_identical_arm_fence_uses_initialized_local(self):
        source = """int F(int input) {
    int id;
    int value;
    id = input;
    value = input + 1;
    return value;
}
"""
        out = self.candidates(autorules.rule_identical_arm_fence, source)
        candidates = [text for label, text in out
                      if label.startswith("identical-arm-fence id ")]
        self.assertEqual(len(candidates), 1)
        self.assertEqual(candidates[0].count("value = input + 1;"), 2)
        self.assertIn("if (id != 0)", candidates[0])

    def test_vector_fields_collapse_to_aggregate_copy(self):
        source = """typedef struct { long vx, vy, vz, pad; } VECTOR;
int F(VECTOR *out, VECTOR *src) {
    out->vx = src->vx;
    out->vy = src->vy;
    out->vz = src->vz;
    out->pad = src->pad;
    return 0;
}
"""
        out = self.candidates(autorules.rule_vector_copy, source)
        self.assertEqual(len(out), 1)
        self.assertIn("*(out) = *(src);", out[0][1])

    def test_vector_copy_adjust_splits_and_merges_literal_component(self):
        source = """typedef struct { long vx, vy, vz; } VECTOR;
typedef struct { VECTOR query; } Scratch;
int F(VECTOR *position) {
    Scratch scratch;
    scratch.query.vx = position->vx;
    scratch.query.vy = position->vy - 2000;
    scratch.query.vz = position->vz;
    return scratch.query.vy;
}
"""
        split = self.candidates(autorules.rule_vector_copy_adjust, source)
        self.assertEqual(len(split), 1)
        self.assertIn("scratch.query.vy = position->vy;", split[0][1])
        self.assertIn("scratch.query.vz = position->vz;\n"
                      "    scratch.query.vy -= 2000;", split[0][1])

        merged = self.candidates(autorules.rule_vector_copy_adjust, split[0][1])
        merge_text = [text for label, text in merged if " merge " in label]
        self.assertEqual(len(merge_text), 1)
        self.assertIn("scratch.query.vy = position->vy - 2000;", merge_text[0])
        self.assertNotIn("scratch.query.vy -= 2000;", merge_text[0])

    def test_flag_assignments_move_after_comparisons(self):
        source = """int F(int outer, int inner) {
    int flag;
    flag = 0;
    if (outer) {
        flag = 1;
        if (inner) outer++;
    }
    return flag;
}
"""
        out = self.candidates(autorules.rule_flag_arm_assign, source)
        self.assertEqual(len(out), 1)
        candidate = out[0][1]
        self.assertLess(candidate.index("if (inner)"), candidate.index("flag = 1;"))
        self.assertIn("else\n    {\n        flag = 0;", candidate)

    def test_flag_assignment_move_rejects_use_or_early_exit(self):
        used = """int F(int outer) {
    int flag;
    flag = 0;
    if (outer) { flag = 1; outer += flag; }
    return flag;
}
"""
        exits = """int F(int outer) {
    int flag;
    flag = 0;
    if (outer) { flag = 1; if (outer > 2) return 3; }
    return flag;
}
"""
        self.assertEqual(self.candidates(autorules.rule_flag_arm_assign, used), [])
        self.assertEqual(self.candidates(autorules.rule_flag_arm_assign, exits), [])

    def test_shared_tail_assignment_duplicates_into_both_arms(self):
        source = """int F(int select) {
    int value;
    int flag;
    if (select) {
        value = 4;
    } else {
        value = 5;
    }
    flag = 1;
    return value + flag;
}
"""
        out = self.candidates(autorules.rule_shared_tail_assign, source)
        self.assertEqual(len(out), 1)
        candidate = out[0][1]
        self.assertEqual(candidate.count("flag = 1;"), 2)
        self.assertIn("value = 4;\n        flag = 1;", candidate)
        self.assertIn("value = 5;\n        flag = 1;", candidate)
        self.assertLess(candidate.rindex("flag = 1;"), candidate.index("return value"))

    def test_shared_tail_assignment_requires_compound_else_and_adjacency(self):
        unbraced = """int F(int select) {
    int value;
    int flag;
    if (select) { value = 4; } else value = 5;
    flag = 1;
    return value + flag;
}
"""
        separated = """int F(int select) {
    int value;
    int flag;
    if (select) { value = 4; } else { value = 5; }
    /* keep this join visible */
    flag = 1;
    return value + flag;
}
"""
        self.assertEqual(self.candidates(autorules.rule_shared_tail_assign, unbraced), [])
        self.assertEqual(self.candidates(autorules.rule_shared_tail_assign, separated), [])

    def test_shared_return_split_duplicates_a_direct_return_label(self):
        source = """int F(int fast) {
    int result;
    if (fast) {
        result = helper();
        goto done;
    }
    result = fallback();
done:
    return result;
}
"""
        # rtlguide maps the epilogue residual to the return, not the remote
        # goto.  The associated label must still make that goto eligible.
        autorules.GUIDED_LINES = {9}
        out = self.candidates(autorules.rule_shared_return_split, source)
        self.assertEqual(len(out), 1)
        candidate = out[0][1]
        self.assertIn("result = helper();\n        return result;", candidate)
        self.assertIn("done:\n    return result;", candidate)
        self.assertEqual(candidate.count("return result;"), 2)

    def test_shared_return_split_rejects_a_label_with_work_before_return(self):
        source = """int F(int fast) {
    int result;
    if (fast) goto done;
    result = fallback();
done:
    result++;
    return result;
}
"""
        self.assertEqual(
            self.candidates(autorules.rule_shared_return_split, source), [])

    def test_shift16_mul_respelled_for_declared_short(self):
        source = """typedef unsigned short u16;
typedef unsigned int u32;
int F(void) {
    u16 damage;
    return (u32)(u16)damage << 16;
}
"""
        out = self.candidates(autorules.rule_shift16_mul, source)
        self.assertEqual(len(out), 1)
        self.assertIn("damage * 0x10000", out[0][1])

    def test_shift_stage_enumerates_constant_right_shift_splits(self):
        source = """int F(int value) {
    return (value >> 8) & 0xff;
}
"""
        autorules.GUIDED_LINES = {2}
        out = self.candidates(autorules.rule_shift_stage, source)
        self.assertEqual(len(out), 7)
        staged = [text for label, text in out
                  if label.startswith("shift-stage 7+1 L2")]
        self.assertEqual(len(staged), 1)
        self.assertIn("((value >> 7) >> 1)", staged[0])

    def test_shift_stage_does_not_resplit_the_outer_staged_shift(self):
        source = """int F(int value) {
    return ((value >> 7) >> 1);
}
"""
        autorules.GUIDED_LINES = {2}
        out = self.candidates(autorules.rule_shift_stage, source)
        self.assertEqual(len(out), 6)

    def test_mul_affine_shape_toggles_crossjump_equivalent_spelling(self):
        expanded = """int F(int x) {
    return x * 3 + 480;
}
"""
        grouped = """int F(int x) {
    return (x + 160) * 3;
}
"""
        autorules.GUIDED_LINES = {2}
        out = self.candidates(autorules.rule_mul_affine_shape, expanded)
        self.assertTrue(any("((x + 160) * 3)" in text for _label, text in out))
        out = self.candidates(autorules.rule_mul_affine_shape, grouped)
        self.assertTrue(any("((x * 3) + 480)" in text for _label, text in out))

    def test_mul_affine_shape_rejects_nondivisible_constant(self):
        source = "int F(int x) { return x * 3 + 481; }\n"
        self.assertEqual(
            self.candidates(autorules.rule_mul_affine_shape, source), [])

    def test_extern_array_skips_target_gp_symbol(self):
        source = """extern int Global;
int F(void) { return Global; }
"""
        with mock.patch.object(autorules, "_target_gp_symbols",
                               return_value={"Global"}):
            self.assertEqual(self.candidates(autorules.rule_extern_array, source), [])
        with mock.patch.object(autorules, "_target_gp_symbols", return_value=set()):
            self.assertTrue(self.candidates(autorules.rule_extern_array, source))

    def test_case_fence_unwraps_else_clause(self):
        source = """int F(int a) {
    if (a) { a++; } else { a--; }
    return a;
}
"""
        out = self.candidates(autorules.rule_case_fence, source)
        self.assertEqual(len(out), 2)
        for _, text in out:
            self.assertIn("switch (!!(a))", text)
            self.assertNotIn("{ else", text)

    def test_case_fence_rejects_break_semantics(self):
        source = """int F(int a) {
    while (a) {
        if (a == 1) { break; } else { a--; }
    }
    return a;
}
"""
        self.assertEqual(self.candidates(autorules.rule_case_fence, source), [])

    def test_plus_group_enumerates_constant_placement_without_reordering_calls(self):
        source = """int F(int a) {
    return abs(a) / 2 + 25 + rand() % 25;
}
"""
        out = self.candidates(autorules.rule_plus_group, source)
        self.assertEqual(len(out), 5)
        texts = [text for _label, text in out]
        self.assertTrue(any(
            "(abs(a) / 2) + ((rand() % 25) + (25))" in text
            for text in texts
        ))
        self.assertTrue(all(text.index("abs(a)") < text.index("rand()")
                            for text in texts))

    def test_plus_group_requires_exactly_one_literal_leaf(self):
        source = """int F(void) {
    return first() + second() + third();
}
"""
        self.assertEqual(self.candidates(autorules.rule_plus_group, source), [])

    def test_add_prefix_temp_names_both_contiguous_promoted_seams(self):
        source = """typedef short s16;
typedef int s32;
int sink(s16);
int F(int a, int b, int c) {
    return sink((s16)(a + b + c));
}
"""
        out = self.candidates(autorules.rule_add_prefix_temp, source)
        self.assertEqual(len(out), 2)
        texts = [candidate for _label, candidate in out]
        self.assertTrue(any("_match_sum = a + b;" in candidate and
                            "sink((s16)(_match_sum + c))" in candidate
                            for candidate in texts))
        self.assertTrue(any("_match_sum = b + c;" in candidate and
                            "sink((s16)(a + _match_sum))" in candidate
                            for candidate in texts))

    def test_add_prefix_temp_rejects_effectful_leaves(self):
        source = """typedef short s16;
int next(void);
int sink(s16);
int F(int a, int b) { return sink((s16)(a + next() + b)); }
"""
        self.assertEqual(
            self.candidates(autorules.rule_add_prefix_temp, source), [])

    def test_rand_mod_split_names_call_result_before_remainder(self):
        source = """typedef int s32;
int rand(void);
int F(void) {
    s32 x;
    x = rand() % 200;
    return x;
}
"""
        out = self.candidates(autorules.rule_rand_mod_split, source)
        self.assertEqual(len(out), 1)
        self.assertIn("x = rand();\n    x = x % 200;", out[0][1])

    def test_rand_mod_split_merges_the_exact_adjacent_inverse(self):
        source = """typedef int s32;
int rand(void);
int F(void) {
    s32 x;
    x = rand();
    x = x % 100;
    return x;
}
"""
        out = self.candidates(autorules.rule_rand_mod_split, source)
        self.assertEqual(len(out), 1)
        self.assertIn("x = rand() % 100;", out[0][1])

    def test_rand_mod_split_rejects_global_or_volatile_destinations(self):
        global_source = """int x;
int rand(void);
int F(void) { x = rand() % 30; return x; }
"""
        volatile_source = """int rand(void);
int F(void) { volatile int x; x = rand() % 30; return x; }
"""
        self.assertEqual(
            self.candidates(autorules.rule_rand_mod_split, global_source), [])
        self.assertEqual(
            self.candidates(autorules.rule_rand_mod_split, volatile_source), [])

    def test_call_result_split_preserves_order_and_handles_separate_blocks(self):
        source = """typedef int s32;
int rand(void);
void sink(int);
void adjust(void);
int F(int status) {
    s32 r;
    if (status) {
        r = rand();
        adjust();
        sink(r);
        r = rand();
        sink(r);
    } else {
        r = rand();
        sink(r);
        r = rand();
        sink(r);
    }
    return 0;
}
"""
        out = self.candidates(autorules.rule_call_result_split, source)
        self.assertEqual(len(out), 1)
        candidate = out[0][1]
        self.assertNotIn("s32 r;", candidate)
        self.assertEqual(candidate.count("s32 _match_r_"), 4)
        self.assertEqual(candidate.count("sink(_match_r_"), 4)
        self.assertLess(candidate.index("s32 _match_r_0 = rand();"),
                        candidate.index("adjust();"))
        self.assertLess(candidate.index("adjust();"),
                        candidate.index("sink(_match_r_0);"))

    def test_call_result_split_rejects_multiple_uses_or_escaping_use(self):
        multiple_uses = """int next(void);
void sink(int);
int F(void) {
    int r;
    r = next();
    sink(r);
    sink(r);
    r = next();
    return r;
}
"""
        escaping_use = """int next(void);
int F(int flag) {
    int r;
    if (flag) {
        r = next();
    }
    if (flag) return r;
    r = next();
    return r;
}
"""
        self.assertEqual(
            self.candidates(autorules.rule_call_result_split, multiple_uses), [])
        self.assertEqual(
            self.candidates(autorules.rule_call_result_split, escaping_use), [])

    def test_builtin_abs_makes_inlining_explicit(self):
        source = """int abs(int);
int F(int value) {
    return abs(value) > 400;
}
"""
        out = self.candidates(autorules.rule_builtin_abs, source)
        self.assertEqual(len(out), 1)
        self.assertIn("__builtin_abs(value)", out[0][1])
        self.assertIn("int abs(int);", out[0][1])

    def test_builtin_abs_collapses_explicit_sign_fix(self):
        source = """typedef struct { int r; } Param;
int Width;
int F(Param *param) {
    int r;
    int w;
    r = param->r;
    w = Width;
    if (r < 0)
        r = -r;
    return r + w;
}
"""
        out = self.candidates(autorules.rule_builtin_abs, source)
        self.assertEqual(len(out), 1)
        self.assertIn("r = __builtin_abs(param->r);", out[0][1])
        self.assertNotIn("if (r < 0)", out[0][1])
        self.assertIn("w = Width;", out[0][1])

    def test_builtin_abs_keeps_intervening_local_observation(self):
        source = """int F(int value) {
    int r;
    r = value;
    use(r);
    if (r < 0)
        r = -r;
    return r;
}
"""
        self.assertEqual(
            self.candidates(autorules.rule_builtin_abs, source), [])

        call = """int F(int value) {
    int r;
    r = value;
    update_world();
    if (r < 0)
        r = -r;
    return r;
}
"""
        self.assertEqual(
            self.candidates(autorules.rule_builtin_abs, call), [])

    def test_call_argument_pair_inlines_same_call_producers_atomically(self):
        source = """int rand(void);
void sink(int, int);
int F(void) {
    int x;
    int y;
    x = rand();
    y = rand();
    sink((x & 7) << 12, (y & 7) << 12);
    return 0;
}
"""
        out = self.candidates(autorules.rule_call_arg_pair_inline, source)
        self.assertEqual(len(out), 1)
        candidate = out[0][1]
        self.assertNotIn("x = rand();", candidate)
        self.assertNotIn("y = rand();", candidate)
        self.assertIn("sink((rand() & 7) << 12, (rand() & 7) << 12);",
                      candidate)

    def test_call_argument_pair_rejects_later_use_or_different_calls(self):
        later = """int rand(void); void sink(int, int);
int F(void) { int x; int y; x=rand(); y=rand(); sink(x,y); return x; }
"""
        different = """int first(void); int second(void); void sink(int, int);
int F(void) { int x; int y; x=first(); y=second(); sink(x,y); return 0; }
"""
        self.assertEqual(
            self.candidates(autorules.rule_call_arg_pair_inline, later), [])
        self.assertEqual(
            self.candidates(autorules.rule_call_arg_pair_inline, different), [])

    def test_subscript_postincrement_splits_and_merges(self):
        merged_source = """int F(int *array) {
    int i;
    i = 0;
    array[i] = 7;
    i++;
    return i;
}
"""
        merged = self.candidates(autorules.rule_subscript_postinc, merged_source)
        self.assertEqual(len(merged), 1)
        self.assertIn("array[i++] = 7;", merged[0][1])
        self.assertNotIn("\n    i++;", merged[0][1])

        split = self.candidates(autorules.rule_subscript_postinc, merged[0][1])
        self.assertEqual(len(split), 1)
        self.assertIn("array[i] = 7;\n    i++;", split[0][1])

    def test_subscript_postincrement_rejects_other_or_escaped_counter_use(self):
        reused = """int F(int *array) {
    int i;
    array[i] = i;
    i++;
    return i;
}
"""
        escaped = """int F(int *array) {
    int i;
    int *p;
    p = &i;
    array[i] = 1;
    i++;
    return *p;
}
"""
        self.assertEqual(
            self.candidates(autorules.rule_subscript_postinc, reused), [])
        self.assertEqual(
            self.candidates(autorules.rule_subscript_postinc, escaped), [])

    def test_switch_cse_eviction_uses_dead_entry_index_local(self):
        source = """int F(int *item) {
    int mode_index;
    mode_index = item[0];
    if (mode_index == 255) return 0;
    switch (item[0]) {
    case 0: return 1;
    default: return 2;
    }
}
"""
        out = self.candidates(autorules.rule_switch_cse_evict, source)
        self.assertEqual(len(out), 1)
        self.assertIn("mode_index = 0;\n    switch (item[0])", out[0][1])
        self.assertEqual(
            self.candidates(autorules.rule_switch_cse_evict, out[0][1]), [])

    def test_switch_cse_eviction_rejects_live_local(self):
        source = """int F(int *item) {
    int mode_index;
    mode_index = item[0];
    switch (item[0]) { case 0: break; }
    return mode_index;
}
"""
        self.assertEqual(
            self.candidates(autorules.rule_switch_cse_evict, source), [])

    def test_registered_rules_never_emit_inline_asm(self):
        self.assertNotIn("__asm__", inspect.getsource(autorules))
        for key, _description, rule in autorules.RULES + autorules.AGGRESSIVE_RULES:
            with self.subTest(rule=key):
                self.assertNotIn("__asm__", inspect.getsource(rule))

    def test_beam_retains_neutral_enabling_edit(self):
        def first(text, _name, _span):
            if text == "A":
                yield "first", "B"

        def second(text, _name, _span):
            if text == "B":
                yield "second", "C"

        scores = {"B": (False, 10, None, None), "C": (True, 0, None, None)}
        with tempfile.NamedTemporaryFile("w+", delete=False) as f:
            path = f.name
            f.write("A")
        try:
            def fake_score(_name, _partial, _source_override):
                with open(path) as f:
                    return scores[f.read()]

            rules = [("first", "", first), ("second", "", second)]
            with mock.patch.object(autorules, "score", side_effect=fake_score):
                with contextlib.redirect_stdout(io.StringIO()):
                    text, score, matched, applied = autorules.beam_search(
                        path, "A", "F", False, rules, 10, width=2, depth=2,
                        allow_regress=0, budget=4,
                    )
            self.assertEqual((text, score, matched), ("C", 0, True))
            self.assertEqual([x[0] for x in applied], ["first", "second"])
        finally:
            os.unlink(path)

    def test_beam_rejects_wrong_length_win_from_exact_length_state(self):
        def shorten(text, _name, _span):
            if text == "exact":
                yield "shorten", "shifted"

        with tempfile.NamedTemporaryFile("w+", delete=False) as f:
            path = f.name
            f.write("exact")
        try:
            def fake_score(_name, _partial, _source_override):
                with open(path) as stream:
                    self.assertEqual(stream.read(), "shifted")
                return (False, 1, 1, 1)

            rules = [("shorten", "", shorten)]
            with mock.patch.object(autorules, "score", side_effect=fake_score):
                with contextlib.redirect_stdout(io.StringIO()):
                    result = autorules.beam_search(
                        path, "exact", "F", False, rules, 20,
                        width=2, depth=2, allow_regress=0, budget=4,
                        shape=(8, 0),
                    )
            self.assertEqual(result, ("exact", 20, False, []))
        finally:
            os.unlink(path)


class AutoRulesLifecycleTests(unittest.TestCase):
    def test_shape_regression_note_flags_equal_length_local_regression(self):
        self.assertEqual(
            autorules.shape_regression_note((2, 0), (5, 0)),
            "  LOCAL-SHAPE REGRESSION 2→5 lines",
        )
        self.assertEqual(autorules.shape_regression_note((5, 0), (2, 0)), "")
        self.assertEqual(
            autorules.shape_regression_note((2, 0), (5, 0), match=True), "",
        )

    def test_exact_length_shape_rejects_nonmatching_length_regression(self):
        self.assertFalse(
            autorules.shape_candidate_allowed((10, 0), (1, 1)),
        )
        self.assertTrue(
            autorules.shape_candidate_allowed((10, 0), (1, 0)),
        )
        self.assertTrue(
            autorules.shape_candidate_allowed((10, 2), (1, 1)),
        )
        self.assertTrue(
            autorules.shape_candidate_allowed((10, 0), (0, 1), match=True),
        )

    def test_greedy_rejects_wrong_length_win_from_exact_length_state(self):
        def shorten(text, _name, _span):
            if text == "exact":
                yield "shorten", "shifted"

        with tempfile.NamedTemporaryFile("w+", delete=False) as f:
            path = f.name
            f.write("exact")
        try:
            def fake_score(_name, _partial, _source_override):
                with open(path) as stream:
                    self.assertEqual(stream.read(), "shifted")
                return (False, 1, 1, 1)

            rules = [("shorten", "", shorten)]
            with mock.patch.object(autorules, "score", side_effect=fake_score):
                with contextlib.redirect_stdout(io.StringIO()):
                    result = autorules.greedy_search(
                        path, "exact", "F", False, rules, 20,
                        shape=(8, 0),
                    )
            self.assertEqual(result, ("exact", 20, False, []))
        finally:
            os.unlink(path)

    def test_score_retains_aligned_shape_as_a_secondary_diagnostic(self):
        def fake_run(args, **kwargs):
            if args == ["./Build"]:
                return subprocess.CompletedProcess(args, 0)
            if args[1].endswith("matchdiff.py"):
                return subprocess.CompletedProcess(
                    args, 1, "F: 4 differing bytes (4 in the whole image)\n", "",
                )
            self.assertTrue(args[1].endswith("asmdiff.py"))
            return subprocess.CompletedProcess(
                args, 1,
                "[F: 3 displayed differing lines in 1 blocks; raw aligned "
                "residual 5 lines in 2 blocks; length ours 8 vs target 8; "
                "exact instruction sequence: NO]\n",
                "",
            )

        with mock.patch.object(autorules, "_run_owned", side_effect=fake_run):
            self.assertEqual(autorules.score("F", False), (False, 4, 3, 0))

    def test_greedy_search_never_rewrites_the_live_source(self):
        def candidate(text, _name, _span):
            if text == "original":
                yield "improve", "winner"

        with tempfile.TemporaryDirectory() as directory:
            live = os.path.join(directory, "F.c")
            staged = os.path.join(directory, "candidate.c")
            with open(live, "w") as stream:
                stream.write("original")
            with open(staged, "w") as stream:
                stream.write("original")

            def fake_score(_name, _partial, source_override):
                self.assertEqual(source_override, staged)
                with open(live) as stream:
                    self.assertEqual(stream.read(), "original")
                with open(staged) as stream:
                    text = stream.read()
                return (False, {"winner": 1}[text], None, None)

            rules = [("candidate", "", candidate)]
            with mock.patch.object(autorules, "score", side_effect=fake_score):
                with contextlib.redirect_stdout(io.StringIO()):
                    result = autorules.greedy_search(
                        staged, "original", "F", False, rules, 2,
                    )
            self.assertEqual(result[:2], ("winner", 1))
            with open(live) as stream:
                self.assertEqual(stream.read(), "original")

    def test_score_passes_per_function_staged_source_to_every_child(self):
        calls = []

        def fake_run(args, **kwargs):
            calls.append((args, kwargs["env"]))
            if args == ["./Build"]:
                return subprocess.CompletedProcess(args, 0)
            return subprocess.CompletedProcess(args, 0, "MATCH!\n", "")

        staged = "/tmp/private/F.c"
        with mock.patch.object(autorules, "_run_owned", side_effect=fake_run):
            self.assertEqual(autorules.score("F", False, staged)[:2], (True, 0))
        variable = autorules._source_override_var("F")
        self.assertEqual(len(calls), 2)
        self.assertTrue(all(call[1][variable] == staged for call in calls))

    def test_baseline_score_ignores_an_inherited_candidate_override(self):
        calls = []

        def fake_run(args, **kwargs):
            calls.append(kwargs["env"])
            if args == ["./Build"]:
                return subprocess.CompletedProcess(args, 0)
            return subprocess.CompletedProcess(args, 0, "MATCH!\n", "")

        variable = autorules._source_override_var("F")
        with mock.patch.dict(os.environ, {variable: "/stale/candidate.c"}), \
                mock.patch.object(autorules, "_run_owned", side_effect=fake_run):
            autorules.score("F", False)
        self.assertTrue(all(variable not in env for env in calls))

    def test_run_owned_terminates_process_group_on_interruption(self):
        process = mock.Mock(pid=1234)
        process.communicate.side_effect = InterruptedError("stop")
        with mock.patch.object(autorules.subprocess, "Popen", return_value=process), \
                mock.patch.object(autorules, "_terminate_process_group") as terminate:
            with self.assertRaises(InterruptedError):
                autorules._run_owned(["worker"])
        terminate.assert_called_once_with(process)

    def test_teardown_kills_descendants_after_group_leader_exits(self):
        process = mock.Mock(pid=1234)
        process.wait.return_value = 0
        process.poll.return_value = 0
        with mock.patch.object(autorules.os, "killpg") as killpg:
            autorules._terminate_process_group(process)
        self.assertEqual(
            killpg.call_args_list,
            [mock.call(1234, signal.SIGTERM), mock.call(1234, signal.SIGKILL)],
        )

    def test_parent_death_setup_closes_the_setup_race(self):
        with mock.patch.object(autorules, "_LIBC", object()), \
                mock.patch.object(autorules.os, "getppid", side_effect=[10, 11]), \
                mock.patch.object(autorules, "_set_parent_death_signal") as arm:
            with self.assertRaises(InterruptedError):
                autorules.arm_parent_death_signal()
        arm.assert_called_once_with(signal.SIGTERM)


class AsmDiffPresentationTests(unittest.TestCase):
    def test_structural_filter_never_claims_same_length_replacement_matches(self):
        stats = asmdiff.aligned_opcodes(
            ["move s0,a0"], ["move s1,a0"], structural=True,
        )
        self.assertEqual(stats["displayed_lines"], 0)
        self.assertEqual(stats["raw_lines"], 1)
        self.assertFalse(stats["identical"])

    def test_suppressed_branch_target_drift_is_still_not_identical(self):
        stats = asmdiff.aligned_opcodes(
            ["beqz v0,0x80001000"],
            ["beqz v0,0x80001004"],
        )
        self.assertEqual(stats["displayed_lines"], 0)
        self.assertEqual(stats["raw_lines"], 1)
        self.assertFalse(stats["identical"])

    def test_structural_filter_displays_insertions(self):
        stats = asmdiff.aligned_opcodes(
            ["move s0,a0"],
            ["move s0,a0", "nop"],
            structural=True,
        )
        self.assertEqual(stats["displayed_lines"], 1)
        self.assertEqual(stats["displayed_blocks"], 1)
        self.assertFalse(stats["identical"])


class RtlGuideTests(unittest.TestCase):
    @staticmethod
    def hunk(target, ours):
        return {
            "target": [(0x1000 + i * 4, x) for i, x in enumerate(target)],
            "ours": [(0x1000 + i * 4, x) for i, x in enumerate(ours)],
        }

    def test_register_rotation_is_regalloc(self):
        h = self.hunk(["addu v0,a0,a1", "sw v0,0(s0)"],
                      ["addu v1,a1,a0", "sw v1,0(s1)"])
        self.assertEqual(rtlguide.classify_hunk(h), "regalloc")

    def test_stack_address_retention_hint_finds_saved_cached_local(self):
        target = [
            (0x1000, "addiu a1,sp,352"),
            (0x1010, "addiu a0,sp,352"),
            (0x1020, "addiu v0,sp,352"),
        ]
        candidate = [(0x1000, "addiu s2,sp,352")]
        self.assertEqual(
            rtlguide.stack_address_rematerialization_hints(target, candidate),
            [{
                "offset": 352,
                "target_sites": 3,
                "candidate_sites": 1,
                "candidate_saved_registers": ["s2"],
            }],
        )

    def test_stack_address_hint_ignores_frame_adjust_and_equal_counts(self):
        target = [
            (0x1000, "addiu sp,sp,-440"),
            (0x1004, "addiu a0,sp,24"),
            (0x1008, "addiu a1,sp,24"),
        ]
        candidate = [
            (0x1000, "addiu sp,sp,-448"),
            (0x1004, "addiu v0,sp,24"),
            (0x1008, "addiu v1,sp,24"),
        ]
        self.assertEqual(
            rtlguide.stack_address_rematerialization_hints(target, candidate),
            [],
        )

    def test_missing_move_is_cse(self):
        h = self.hunk(["move v0,a0"], ["nop"])
        self.assertEqual(rtlguide.classify_hunk(h), "cse/coalescing")

    def test_reorder_is_scheduler(self):
        h = self.hunk(["lw v0,0(a0)", "addiu v1,v1,1"],
                      ["addiu v1,v1,1", "lw v0,0(a0)"])
        self.assertEqual(rtlguide.classify_hunk(h), "schedule/delay")

    def test_known_adjacent_load_order_signature(self):
        h = self.hunk(["lh v1,88(fp)", "lh a3,30(a2)"],
                      ["lh a3,30(a2)", "lh v1,88(fp)"])
        self.assertEqual(rtlguide.classify_hunk(h), "schedule/delay")
        self.assertEqual(rtlguide.known_residual_signatures([h]),
                         ["adjacent-independent-load-order"])

    def test_known_shared_return_extension_schedule_signature(self):
        target = [
            (0x1000, "sll v0,s0,0x10"),
            (0x1004, "sra v0,v0,0x10"),
            (0x1008, "lw ra,20(sp)"),
            (0x100c, "lw s0,16(sp)"),
            (0x1010, "jr ra"),
            (0x1014, "addiu sp,sp,24"),
        ]
        ours = [
            (0x1000, "sll v0,s0,0x10"),
            (0x1004, "lw ra,20(sp)"),
            (0x1008, "lw s0,16(sp)"),
            (0x100c, "sra v0,v0,0x10"),
            (0x1010, "jr ra"),
            (0x1014, "addiu sp,sp,24"),
        ]
        self.assertEqual(
            rtlguide.known_residual_signatures([], target, ours),
            ["shared-return-extension-schedule"],
        )

    def test_shared_return_signature_rejects_other_tail_differences(self):
        target = [
            (0x1000, "sra v0,v0,16"),
            (0x1004, "lw ra,20(sp)"),
            (0x1008, "jr ra"),
        ]
        ours = [
            (0x1000, "lw ra,24(sp)"),
            (0x1004, "sra v0,v0,16"),
            (0x1008, "jr ra"),
        ]
        self.assertEqual(
            rtlguide.known_residual_signatures([], target, ours), [])

    def test_loop_boundary_lines_parse_first_post_loop_statement(self):
        dump = """;; Function F
(note 10 9 11 \"\" NOTE_INSN_LOOP_END)
(note 11 10 12 (\"src/F.c\") 44)
(insn 12 11 13 (set (reg:SI 2) (const_int 1)))
(note 13 12 14 (\"src/F.c\") 45)
(insn 14 13 15 (set (reg:SI 3) (const_int 2)))
"""
        with tempfile.NamedTemporaryFile("w+", suffix=".sched", delete=False) as f:
            path = f.name
            f.write(dump)
        try:
            self.assertEqual(
                rtlguide.loop_boundary_source_lines([path], "F"),
                {"sched": [44]},
            )
        finally:
            os.unlink(path)

    def test_known_commutative_plus_destination_signature(self):
        h = self.hunk(["addu a0,a0,v1", "sw a0,0(gp)"],
                      ["addu v1,v1,a0", "sw v1,0(gp)"])
        self.assertEqual(rtlguide.known_residual_signatures([h]),
                         ["commutative-plus-destination"])

    def test_known_copy_then_adjust_signature(self):
        target = [
            (0x1000, "sw v0,44(sp)"),
            (0x1004, "lw v1,8(s0)"),
            (0x1008, "addiu v0,v0,-2000"),
            (0x100c, "sw v0,44(sp)"),
        ]
        ours = [
            (0x1000, "nop"),
            (0x1004, "addiu v0,v0,-2000"),
            (0x1008, "sw v0,44(sp)"),
            (0x100c, "lw v1,8(s0)"),
        ]
        self.assertEqual(
            rtlguide.known_residual_signatures([], target, ours),
            ["copy-then-inplace-adjust"],
        )

    def test_known_post_comparison_flag_signature(self):
        target = [
            (0x1000, "slt v0,v1,a0"),
            (0x1004, "bnez v0,0x1040"),
            (0x1008, "move v0,zero"),
            (0x100c, "slt v0,v1,a0"),
            (0x1010, "beqz v0,0x1020"),
            (0x1014, "li v0,1"),
            (0x1018, "beqz v0,0x1040"),
        ]
        ours = [
            (0x1000, "slt v0,v1,a0"),
            (0x1004, "bnez v0,0x1040"),
            (0x1008, "move a1,zero"),
            (0x100c, "slt v0,v1,a0"),
            (0x1010, "beqz v0,0x1020"),
            (0x1014, "li a1,1"),
            (0x1018, "beqz a1,0x1040"),
        ]
        self.assertEqual(
            rtlguide.known_residual_signatures([], target, ours),
            ["post-comparison-flag-definition"],
        )

    def test_known_path_result_copy_join_signature(self):
        target = [
            (0x1000, "move v1,v0"),
            (0x1004, "move v0,v1"),
            (0x1008, "beqz v0,0x1040"),
        ]
        ours = [
            (0x1000, "move v1,v0"),
            (0x1004, "beqz v1,0x1040"),
        ]
        self.assertEqual(
            rtlguide.known_residual_signatures([], target, ours),
            ["path-result-copy-join"],
        )

    def test_path_result_copy_join_rejects_unrelated_moves(self):
        target = [
            (0x1000, "move v1,v0"),
            (0x1004, "move v0,v1"),
            (0x1008, "beqz v0,0x1040"),
        ]
        ours = [
            (0x1000, "move a0,v0"),
            (0x1004, "beqz a0,0x1040"),
        ]
        self.assertEqual(
            rtlguide.known_residual_signatures([], target, ours),
            [],
        )

    def test_known_enclosing_global_field_load_signature(self):
        h = self.hunk(
            ["lui v0,0x8009", "lw v1,-24828(v0)"],
            ["lui v1,0x8009", "lw v1,-24828(v1)"],
        )
        self.assertEqual(
            rtlguide.known_residual_signatures([h]),
            ["enclosing-global-field-load"],
        )

    def test_known_guard_return_island_layout_signature(self):
        h = self.hunk(
            ["bnez v0,0x80028f90"],
            ["beqz v0,0x80028d0c", "nop", "j 0x80028f94"],
        )
        self.assertEqual(
            rtlguide.known_residual_signatures([h]),
            ["guard-return-island-layout"],
        )

    def test_known_builtin_abs_signature(self):
        target = [
            (0x1000, "bgez a0,0x1010"),
            (0x1004, "move v0,a0"),
            (0x1008, "negu v0,v0"),
        ]
        ours = [
            (0x1000, "jal 0x80076074 <abs>"),
            (0x1004, "nop"),
        ]
        self.assertEqual(
            rtlguide.known_residual_signatures([], target, ours),
            ["builtin-abs-inline"],
        )

    def test_known_postincrement_working_copy_signature(self):
        h = self.hunk(
            ["addiu v0,v1,1", "move v1,v0", "sll v0,v0,0x10"],
            ["addiu v1,v1,1", "sll v0,v1,0x10"],
        )
        self.assertEqual(
            rtlguide.known_residual_signatures([h]),
            ["postincrement-working-copy"],
        )

    def test_known_call_result_argument_pipeline_signature(self):
        target = [
            (0x1000, "jal 0x800761d4"),
            (0x1004, "nop"),
            (0x1008, "andi s0,v0,0x7"),
            (0x100c, "jal 0x800761d4"),
            (0x1010, "sll s0,s0,0xc"),
        ]
        ours = [
            (0x1000, "jal 0x800761d4"),
            (0x1004, "nop"),
            (0x1008, "andi s0,v0,0x7"),
            (0x100c, "sll s0,s0,0xc"),
            (0x1010, "jal 0x800761d4"),
            (0x1014, "nop"),
        ]
        self.assertEqual(
            rtlguide.known_residual_signatures([], target, ours),
            ["call-result-argument-pipeline"],
        )

    def test_known_commutative_equality_register_order_signature(self):
        h = self.hunk(
            ["lh v0,2164(gp)", "li v1,2310", "bne v0,v1,0x1040"],
            ["lh v1,2164(gp)", "li v0,2310", "bne v1,v0,0x1040"],
        )
        self.assertEqual(
            rtlguide.known_residual_signatures([h]),
            ["commutative-equality-register-order"],
        )

    def test_relocated_addiu_is_combine_reassociation(self):
        target = [
            (0x1000, "addiu s0,s0,25"),
            (0x1004, "sra v1,v0,0x1f"),
            (0x1008, "mfhi t0"),
            (0x100c, "subu v0,v0,v1"),
        ]
        ours = [
            (0x1000, "sra v1,v0,0x1f"),
            (0x1004, "mfhi t0"),
            (0x1008, "subu v0,v0,v1"),
            (0x100c, "addiu v0,v0,25"),
        ]
        hunks, _pairs = rtlguide.diff_hunks(target, ours)
        self.assertEqual(len(hunks), 1)
        self.assertEqual(rtlguide.classify_hunk(hunks[0]), "combine/expression")
        self.assertEqual(rtlguide.known_residual_signatures(hunks),
                         ["constant-add-reassociation"])

    def test_branch_target_drift_has_same_shape(self):
        self.assertEqual(rtlguide.shape("bnez v0,0x80012340"),
                         rtlguide.shape("bnez v1,0x80012400"))

    def test_target_only_call_sets_call_tail_hint(self):
        target = [(0x1000, "jal 0x80001000 <DeleteConflict>"),
                  (0x1004, "nop")]
        ours = [(0x1000, "j 0x80002000"), (0x1004, "nop")]
        with mock.patch.object(
                rtlguide, "_candidate_asm",
                return_value=(0x1000, 8, 8, target, ours)):
            guide = rtlguide.assembly_guide("F")
        self.assertTrue(guide["call_tail_hint"])
        self.assertEqual(guide["call_counts"], {"target": 1, "candidate": 0})
        self.assertEqual(guide["target_only_calls"], [
            {"callee": "DeleteConflict", "count": 1},
        ])
        self.assertEqual(guide["control_flow_counts"], {
            "target": {
                "conditional_branches": 0,
                "unconditional_jumps": 0,
                "calls": 1,
                "returns": 0,
            },
            "candidate": {
                "conditional_branches": 0,
                "unconditional_jumps": 1,
                "calls": 0,
                "returns": 0,
            },
        })

    def test_control_flow_counts_exposes_target_absent_branch(self):
        target = [(0x1000, "addiu v0,v0,1"), (0x1004, "jr ra")]
        candidate = [
            (0x1000, "beqz a0,0x1010"),
            (0x1004, "nop"),
            (0x1008, "jr ra"),
        ]
        self.assertEqual(
            rtlguide.control_flow_counts(target)["conditional_branches"], 0)
        self.assertEqual(
            rtlguide.control_flow_counts(candidate)["conditional_branches"], 1)

    def test_call_rtl_fingerprint_includes_result_mode_and_usage(self):
        body = """;; Function F

(call_insn 10 9 11 (parallel[
            (set (reg:HI 2 v0)
                (call (mem:SI (symbol_ref:SI (\"DeleteConflict\")))
                    (const_int 16)))
            (clobber (reg:SI 31 ra))
        ] ) -1 (nil)
    (nil)
    (expr_list (use (reg:SI 5 a1))
        (expr_list (use (reg:SI 4 a0))
            (nil))))

;; Function Other

(call_insn 20 19 21 (parallel[
            (call (mem:SI (symbol_ref:SI ("Unrelated"))) (const_int 16))
            (clobber (reg:SI 31 ra))
        ] ) -1 (nil) (nil) (nil))
"""
        with tempfile.NamedTemporaryFile("w+", delete=False) as f:
            path = f.name
            f.write(body)
        try:
            self.assertEqual(rtlguide.parse_call_rtl(path, "F"), [{
                "callee": "DeleteConflict", "result": "HI",
                "uses": ["SI:$a1", "SI:$a0"],
            }])
        finally:
            os.unlink(path)


    def test_call_rtl_evidence_tracks_jump_merging(self):
        before = [
            {"callee": "DeleteConflict", "result": "void", "uses": ["SI:$a0"]},
            {"callee": "DeleteConflict", "result": "HI", "uses": ["SI:$a0"]},
        ]
        after = [before[0]]
        with mock.patch.object(rtlguide, "parse_call_rtl",
                               side_effect=[before, after]):
            evidence = rtlguide.call_rtl_evidence(
                ["F.rtl", "F.jump2"],
                [{"callee": "DeleteConflict", "count": 1}], "F",
            )
        self.assertEqual(evidence[0]["expanded_sites"], 2)
        self.assertEqual(evidence[0]["after_jump_sites"], 1)
        self.assertEqual({x["result"] for x in evidence[0]["fingerprints"]},
                         {"void", "HI"})

    def test_guided_registry_is_pure_c(self):
        rules = {rule for values in rtlguide.CATEGORY_RULES.values() for rule in values}
        self.assertIn("cmp-polarity", rules)
        self.assertIn("loop-fence", rules)
        self.assertIn("loop-range", rules)
        self.assertIn("shift16-mul", rules)
        self.assertIn("plus-group", rules)
        self.assertIn("type-width", rules)
        self.assertFalse(any("barrier" in rule or "clobber" in rule for rule in rules))

    def test_json_keeps_hunk_instructions_not_full_streams(self):
        report = {
            "target": [(1, "nop")], "ours": [(1, "move v0,v1")],
            "hunks": [{
                "target": [(1, "nop")], "ours": [(1, "move v0,v1")],
                "target_indices": [0], "ours_indices": [0],
            }],
        }
        out = rtlguide._jsonable(report)
        self.assertNotIn("target", out)
        self.assertEqual(out["hunks"][0]["target"], [[1, "nop"]])


class SymbolNeighborTests(unittest.TestCase):
    def test_parse_resolve_and_bounded_candidates(self):
        text = """CamState = 0x80089EF0;
CURRENTLY_SELECTED = 0x80089F00;
SCALAR_ALIAS = 0x80089F04;
AFTER = 0x80089F08;
"""
        with tempfile.NamedTemporaryFile("w+", delete=False) as stream:
            path = stream.name
            stream.write(text)
        try:
            symbols = symnear.load_symbols(path)
            address = symnear.resolve_query("SCALAR_ALIAS", symbols)
            self.assertEqual(address, 0x80089F04)
            self.assertEqual(
                symnear.nearby(symbols, address, before=0x20, after=0),
                [
                    (0x80089EF0, "CamState", 0x14),
                    (0x80089F00, "CURRENTLY_SELECTED", 4),
                    (0x80089F04, "SCALAR_ALIAS", 0),
                ],
            )
            self.assertEqual(
                symnear.common_bases(
                    symbols, [0x80089F04, 0x80089F08], before=0x20),
                [
                    (0x80089EF0, "CamState", [0x14, 0x18]),
                    (0x80089F00, "CURRENTLY_SELECTED", [4, 8]),
                    (0x80089F04, "SCALAR_ALIAS", [0, 4]),
                ],
            )
        finally:
            os.unlink(path)


class MatchDiffArtifactTests(unittest.TestCase):
    def test_exact_source_completion_rejects_guards_and_inline_asm(self):
        source = """#ifndef NON_MATCHING
INCLUDE_ASM("stub", F);
#else
void F(void) { __asm__ volatile ("" ::: "memory"); }
#endif
"""
        with tempfile.NamedTemporaryFile("w+", delete=False) as stream:
            path = stream.name
            stream.write(source)
        try:
            self.assertEqual(matchdiff.source_completion_blockers(path), [
                "NON_MATCHING guard", "INCLUDE_ASM fallback",
                "inline __asm__",
            ])
        finally:
            os.unlink(path)

    def test_exact_plain_c_has_no_completion_blocker(self):
        with tempfile.NamedTemporaryFile("w+", delete=False) as stream:
            path = stream.name
            stream.write("void F(void) { return; }\n")
        try:
            self.assertEqual(matchdiff.source_completion_blockers(path), [])
        finally:
            os.unlink(path)

    def test_nonmatching_stub_is_detected_from_link_map(self):
        content = """
 .text 0x80012340 0x20 .shake/build/main.exe/F.c.o
                0x80012340                F.NON_MATCHING
"""
        with tempfile.NamedTemporaryFile("w+", delete=False) as f:
            path = f.name
            f.write(content)
        try:
            with mock.patch.object(matchdiff, "MAP", path):
                self.assertTrue(matchdiff.linked_nonmatching_stub("F"))
                self.assertFalse(matchdiff.linked_nonmatching_stub("Other"))
        finally:
            os.unlink(path)


class MaspsxFlagsTests(unittest.TestCase):
    def test_guarded_variable_division_is_detected(self):
        asm = """
    div $zero, $v0, $v1
    bnez $v1, ok
    nop
    break 7
ok:
    li $at, -1
    bne $v1, $at, done
    lui $at, 0x8000
    bne $v0, $at, done
    nop
    break 6
done:
    mfhi $v0
"""
        self.assertEqual(maspsxflags.guarded_division_count(asm), 1)

    def test_unguarded_division_does_not_request_expand_div(self):
        asm = """
    div $zero, $v0, $v1
    mfhi $v0
"""
        self.assertEqual(maspsxflags.guarded_division_count(asm), 0)

    def test_extra_patches_are_scoped_and_idempotent(self):
        build = """flags name = extra name
  where
    extra "Other" = ["--something"]
    extra _ = []
"""
        permute_src = """GP_EXTERNS = {
    "F": ["keep"],
}
MASPSX_EXTRA = {
    "Other": ["--something"],
}
"""
        with tempfile.TemporaryDirectory() as directory:
            build_path = os.path.join(directory, "Build.hs")
            permute_path = os.path.join(directory, "permute.py")
            with open(build_path, "w") as stream:
                stream.write(build)
            with open(permute_path, "w") as stream:
                stream.write(permute_src)
            with mock.patch.object(maspsxflags, "BUILD_HS", build_path), \
                    mock.patch.object(maspsxflags, "PERMUTE", permute_path):
                maspsxflags.patch_build_extra("F")
                maspsxflags.patch_build_extra("F")
                maspsxflags.patch_permute_extra("F")
                maspsxflags.patch_permute_extra("F")
            with open(build_path) as stream:
                build_after = stream.read()
            with open(permute_path) as stream:
                permute_after = stream.read()
        self.assertEqual(build_after.count('extra "F" = ["--expand-div"]'), 1)
        self.assertIn('"F": ["keep"]', permute_after)
        self.assertEqual(
            permute_after.count('"F": ["--expand-div"]'), 1)


class RegallocParserTests(unittest.TestCase):
    def test_preferences_are_exposed(self):
        body = """;; 80 conflicts: 80 2 3
;; 80 preferences: 2
;; Register dispositions:
80 in 3
;; Hard regs used: 2 3
"""
        result = regalloc.analyse("F", body)
        self.assertEqual(result["disp"][80], 3)
        self.assertEqual(result["preferences"][80], [2])
        self.assertEqual(result["conflicts"][80], [80, 2, 3])
        self.assertTrue(regalloc.conflicts_with_hard_register(result, 80, 2))
        self.assertFalse(regalloc.conflicts_with_hard_register(result, 80, 4))
        self.assertEqual(regalloc.parse_hard_register("$v0"), 2)
        self.assertEqual(regalloc.preferred_allocnos(result, 2), [{
            "pseudo": 80,
            "assigned": "v1",
            "refs": None,
            "live_length": None,
            "calls": None,
            "priority": None,
        }])

    def test_lreg_usage_and_priority_are_exposed(self):
        greg = """;; 2 regs to allocate: 80 81
;; Register dispositions:
80 in 16
81 in 17
;; Hard regs used: 16 17
"""
        lreg = """Register 80 used 3 times across 17 insns; crosses 1 calls; GR_REGS.
Register 81 used 4 times across 22 insns in block 0; GR_REGS.
"""
        result = regalloc.analyse("F", greg, lreg)
        self.assertEqual(result["allocnos"], {80, 81})
        self.assertEqual(result["usage"][80]["calls"], 1)
        self.assertEqual(result["usage"][81]["priority"],
                         regalloc.alloc_priority(4, 22))
        self.assertEqual(regalloc.extra_refs_to_outrank(
            result["usage"][80], result["usage"][81]), 1)
        self.assertEqual(regalloc.wrapper_depths(5, 2), 3)

    def test_priority_window_finds_narrow_weighted_ref_interval(self):
        subject = {"refs": 67, "live_length": 846}
        lower = {"priority": 4901}
        upper = {"priority": 5000}
        self.assertEqual(
            regalloc.extra_refs_for_window(subject, lower, upper), (3, 3)
        )
        self.assertEqual(regalloc.alloc_priority(70, 846), 4964)
        self.assertEqual(regalloc.wrapper_depth_window((3, 3), 1), (3, 3))
        self.assertEqual(regalloc.wrapper_depth_window((3, 3), 3), (1, 1))
        self.assertIsNone(regalloc.wrapper_depth_window((3, 3), 2))

    def test_priority_window_rejects_reversed_bounds(self):
        subject = {"refs": 4, "live_length": 10}
        self.assertIsNone(regalloc.extra_refs_for_window(
            subject, {"priority": 5000}, {"priority": 4000}
        ))


class MatchToolLockTests(unittest.TestCase):
    def test_lock_is_reentrant_for_driver_helpers_and_excludes_other_owner(self):
        old = os.getcwd()
        with tempfile.TemporaryDirectory() as directory:
            os.chdir(directory)
            try:
                with matchlock.matching_tool_lock("outer", "F"):
                    with matchlock.matching_tool_lock("helper", "F"):
                        pass
                os.makedirs(".shake", exist_ok=True)
                path = os.path.join(".shake", "matching-tool.lock")
                with open(path, "a+") as owner:
                    fcntl.flock(owner.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
                    with self.assertRaises(matchlock.MatchToolBusy):
                        with matchlock.matching_tool_lock("other", "G"):
                            pass
            finally:
                os.chdir(old)


class PermuteRescoreTests(unittest.TestCase):
    def test_permuter_declares_gcc_intrinsics_for_type_parser(self):
        source = "int F(int value) { return __builtin_abs(value); }\n"
        result = permute.add_permuter_parser_declarations(source)
        self.assertTrue(result.startswith("extern int __builtin_abs(int);\n"))
        self.assertEqual(result.count("extern int __builtin_abs(int);"), 1)
        self.assertEqual(
            permute.add_permuter_parser_declarations(result), result)

    def test_asmdiff_summary_accepts_structural_filter_annotations(self):
        summary = (
            "[F: 27 displayed differing lines in 14 blocks; "
            "raw aligned residual 28 lines in 15 blocks; "
            "length ours 346 vs target 346; exact instruction sequence: NO]"
        )
        match = permute.ASMDIFF_SUMMARY.search(summary)
        self.assertIsNotNone(match)
        self.assertEqual(tuple(map(int, match.groups())), (27, 14, 346, 346))

    def test_permuter_preflight_accepts_near_final_allocation_residual(self):
        ready, reason = permute.permuter_readiness(42, 12, 249, 249)
        self.assertTrue(ready)
        self.assertIn("near-final", reason)
        ready, _reason = permute.permuter_readiness(80, 20, 268, 269)
        self.assertTrue(ready)

    def test_permuter_preflight_rejects_structure_and_accidental_length_balance(self):
        ready, reason = permute.permuter_readiness(88, 27, 359, 362)
        self.assertFalse(ready)
        self.assertIn("length differs by 3", reason)
        ready, reason = permute.permuter_readiness(163, 46, 362, 362)
        self.assertFalse(ready)
        self.assertIn("residual is still broad", reason)

    def test_raw_byte_diff_counts_content_and_length(self):
        self.assertEqual(permute.raw_byte_diff(b"abcd", b"abXdY"), 2)

    def test_rewrite_linker_object_replaces_every_section_reference(self):
        linker = """SECTIONS {
  .text : { .shake/build/main.exe/F.c.o(.text); }
  .rodata : { .shake/build/main.exe/F.c.o(.rodata); }
}
"""
        result = permute.rewrite_linker_object(linker, "F", "/tmp/candidate.o")
        self.assertEqual(result.count("/tmp/candidate.o"), 2)
        self.assertNotIn(".shake/build/main.exe/F.c.o", result)
        with self.assertRaises(ValueError):
            permute.rewrite_linker_object(linker, "Missing", "/tmp/x.o")

    def test_candidate_sources_orders_base_then_proxy_scores(self):
        with tempfile.TemporaryDirectory() as directory:
            paths = [
                os.path.join(directory, "base.c"),
                os.path.join(directory, "output-25-2", "source.c"),
                os.path.join(directory, "output-10-1", "source.c"),
            ]
            for path in paths:
                os.makedirs(os.path.dirname(path), exist_ok=True)
                with open(path, "w") as stream:
                    stream.write("void F(void) {}\n")
            found = [os.path.relpath(path, directory)
                     for path in permute.candidate_sources(directory)]
            self.assertEqual(found, ["base.c", "output-10-1/source.c",
                                     "output-25-2/source.c"])

    def test_contextualize_function_only_candidate(self):
        base = """typedef unsigned int u32;
extern void helper(u32 value);
void Before(void) { helper(1); }
void Target(u32 value)
{
    helper(value);
}
void After(void) { helper(3); }
"""
        candidate = """
void Target(u32 value)
{
    /* A brace in a comment must not end the definition: } */
    helper(value + 2);
}
"""
        result = permute.contextualize_candidate("Target", base, candidate)
        self.assertIsNotNone(result)
        self.assertIn("typedef unsigned int u32;", result)
        self.assertIn("void Before(void) { helper(1); }", result)
        self.assertIn("helper(value + 2);", result)
        self.assertIn("void After(void) { helper(3); }", result)
        self.assertNotIn("helper(value);", result)
        self.assertEqual(result.count("void Target("), 1)

    def test_contextualize_requires_both_definitions(self):
        base = "void Target(void) {}\n"
        self.assertIsNone(permute.contextualize_candidate(
            "Missing", base, "void Other(void) {}\n"
        ))


class StackPlanTests(unittest.TestCase):
    def test_successful_build_diagnostics_stay_in_log(self):
        with tempfile.TemporaryDirectory() as directory:
            log_path = os.path.join(directory, "stackplan-build.log")

            def successful_build(_command, stdout, stderr):
                self.assertIs(stdout, subprocess.DEVNULL)
                stderr.write(b"compiler warning that must not bury the report\n")
                return mock.Mock(returncode=0)

            visible_stderr = io.StringIO()
            with mock.patch.object(stackplan, "BUILD_LOG", log_path), \
                    mock.patch.object(stackplan.subprocess, "run",
                                      side_effect=successful_build), \
                    contextlib.redirect_stderr(visible_stderr):
                stackplan.build_candidate()

            self.assertEqual(visible_stderr.getvalue(), "")
            with open(log_path) as log:
                self.assertIn("compiler warning", log.read())

    def test_failed_build_reports_log_tail(self):
        with tempfile.TemporaryDirectory() as directory:
            log_path = os.path.join(directory, "stackplan-build.log")

            def failed_build(_command, stdout, stderr):
                for line in range(20):
                    stderr.write(f"diagnostic {line}\n".encode())
                return mock.Mock(returncode=3)

            with mock.patch.object(stackplan, "BUILD_LOG", log_path), \
                    mock.patch.object(stackplan.subprocess, "run",
                                      side_effect=failed_build):
                with self.assertRaises(SystemExit) as raised:
                    stackplan.build_candidate()

            message = str(raised.exception)
            self.assertIn("./Build FAILED (rc=3)", message)
            self.assertIn(log_path, message)
            self.assertNotIn("diagnostic 4\n", message)
            self.assertIn("diagnostic 5\n", message)
            self.assertTrue(message.endswith("diagnostic 19"))

    def test_address_formation_marks_start_of_hidden_stack_aggregate(self):
        assembly = """
glabel F
    addiu $sp, $sp, -0x60
    sw $s0, 0x50($sp)
    sw $ra, 0x5c($sp)
    addiu $a0, $sp, 0x18
    lhu $v0, 0x38($sp)
"""
        info = stackplan.analyze(assembly, args_hint=0x10)
        self.assertEqual(info["workspace_start"], 0x18)
        self.assertEqual(info["accesses"][0x18], {"addr": 1})
        overlay = stackplan.emit_overlay("F", info)
        self.assertIn("unsigned char bytes_000[0x20]", overlay)
        self.assertIn("unsigned short field_020", overlay)
        self.assertIn("expected base sp+0x18, size 0x38", overlay)

    def test_overlay_uses_signed_width_and_explicit_padding(self):
        info = {
            "workspace_start": 0x20,
            "workspace_end": 0x30,
            "accesses": {0x20: {"lh": 1}, 0x28: {"sw": 2}},
        }
        overlay = stackplan.emit_overlay("F", info)
        self.assertIn("short field_000", overlay)
        self.assertIn("unsigned char pad_002[0x6]", overlay)
        self.assertIn("unsigned int field_008", overlay)
        self.assertIn("unsigned char pad_00c[0x4]", overlay)

    def test_target_frame_and_workspace_are_inferred(self):
        assembly = """
glabel F
    addiu $sp, $sp, -0x60
    sw $s0, 0x50($sp)
    sw $ra, 0x5c($sp)
    sw $a0, 0x18($sp)
    lw $v0, 0x1c($sp)
    jal helper
    sw $zero, 0x10($sp)
"""
        info = stackplan.analyze(assembly, args_hint=0x18)
        self.assertEqual(info["frame"], 0x60)
        self.assertEqual(info["saved_start"], 0x50)
        self.assertEqual(info["workspace_start"], 0x18)
        self.assertEqual(info["workspace_end"], 0x50)
        self.assertEqual(info["workspace_size"], 0x38)
        self.assertEqual(info["accesses"][0x18], {"sw": 1})
        self.assertNotIn(0x10, [offset for offset in info["accesses"]
                               if offset >= info["workspace_start"]])

    def test_loaded_slots_precede_addressed_aggregate_are_locals(self):
        assembly = """
glabel F
    addiu $sp, $sp, -0x38
    sw $s0, 0x28($sp)
    sw $ra, 0x34($sp)
    sw $v0, 0x10($sp)
    sw $v1, 0x14($sp)
    sw $a0, 0x18($sp)
    lw $v0, 0x10($sp)
    lw $v1, 0x14($sp)
    lw $a0, 0x18($sp)
    addiu $a2, $sp, 0x20
"""
        info = stackplan.analyze(assembly)
        self.assertEqual(info["args"], 0x10)
        self.assertEqual(info["workspace_start"], 0x10)
        self.assertEqual(info["workspace_size"], 0x18)

    def test_compiler_frame_comment_supplies_args_and_vars(self):
        assembly = """
.frame $sp,112,$31 # vars= 72, regs= 4/0, args= 24, extra= 0
subu $sp,$sp,112
sw $16,96($sp)
sw $31,108($sp)
sw $2,24($sp)
"""
        info = stackplan.analyze(assembly)
        self.assertEqual(info["frame"], 112)
        self.assertEqual(info["vars"], 72)
        self.assertEqual(info["args"], 24)
        self.assertEqual(info["saved_start"], 96)
        self.assertEqual(info["workspace_size"], 72)


class BuildConfigurationTests(unittest.TestCase):
    def test_get_tpage_uses_one_full_width_sdk_prototype(self):
        root = os.path.dirname(TOOLS)
        header = os.path.join(root, "include", "psxsdk", "libgs.h")
        with open(header) as f:
            declarations = f.read()
        self.assertIn(
            "u16 GetTPage(s32 tp, s32 abr, s32 x, s32 y);",
            declarations,
        )

        local_declarations = []
        source_root = os.path.join(root, "src", "main.exe")
        pattern = __import__("re").compile(
            r"^\s*(?:extern\s+)?(?:u16|unsigned\s+short)\s+"
            r"GetTPage\s*\(",
            __import__("re").M,
        )
        for directory, _, names in os.walk(source_root):
            for name in names:
                if not name.endswith(".c"):
                    continue
                path = os.path.join(directory, name)
                with open(path) as f:
                    if pattern.search(f.read()):
                        local_declarations.append(os.path.relpath(path, root))
        self.assertEqual(local_declarations, [])

    def test_matching_source_override_is_per_function_and_used_by_cpp(self):
        path = os.path.join(os.path.dirname(TOOLS), "shake", "src", "Build.hs")
        with open(path) as f:
            build = f.read()
        self.assertIn(
            'getEnv ("TENCHU_MATCH_SOURCE_" <> takeBaseName src)', build,
        )
        self.assertIn("compileSrc <- matchingSource src", build)
        self.assertRegex(build, r"cmd_ cpp .* compileSrc out")

    def test_expand_div_table_matches_build(self):
        path = os.path.join(os.path.dirname(TOOLS), "shake", "src", "Build.hs")
        with open(path) as f:
            build = f.read()
        block = build.split("maspsxGpExterns src =", 1)[1].split("extra _ = []", 1)[0]
        names = set(__import__("re").findall(
            r'extra "([A-Za-z0-9_]+)" = \["--expand-div"\]', block
        ))
        self.assertEqual(names, set(permute.MASPSX_EXTRA))

    def test_gp_extern_table_matches_build(self):
        path = os.path.join(os.path.dirname(TOOLS), "shake", "src", "Build.hs")
        with open(path) as f:
            build = f.read()
        block = build.split("maspsxGpExterns src =", 1)[1].split("syms _ = []", 1)[0]
        found = {}
        pattern = __import__("re").compile(
            r'^\s*syms "([A-Za-z0-9_]+)" = \[(.*)\]$', __import__("re").M
        )
        for name, values in pattern.findall(block):
            found[name] = __import__("re").findall(r'"([^"]+)"', values)
        self.assertEqual(found, permute.GP_EXTERNS)


class MetadataConflictMergeTests(unittest.TestCase):
    START = "<" * 7 + " HEAD"
    MIDDLE = "=" * 7
    END = ">" * 7 + " worker"

    def test_build_metadata_conflict_is_ordered_union(self):
        source = f'''  where
{self.START}
    extra "First" = ["--expand-div"]
    extra "Shared" = ["--expand-div"]
{self.MIDDLE}
    extra "Second" = ["--expand-div"]
    extra "Shared" = ["--expand-div"]
{self.END}
    extra _ = []
'''
        resolved, count = merge_metadata_conflicts.resolve_text(
            "shake/src/Build.hs", source
        )
        self.assertEqual(count, 1)
        self.assertNotIn("<<<<<<<", resolved)
        self.assertLess(resolved.index('extra "First"'),
                        resolved.index('extra "Second"'))
        self.assertEqual(resolved.count('extra "Shared"'), 1)

    def test_python_metadata_resolves_multiple_conflicts(self):
        source = f'''GP_EXTERNS = {{
{self.START}
    "First": ["A"],
{self.MIDDLE}
    "Second": ["B"],
{self.END}
}}
MASPSX_EXTRA = {{
{self.START}
    "Third": ["--expand-div"],
{self.MIDDLE}
    "Fourth": ["--expand-div"],
{self.END}
}}
'''
        resolved, count = merge_metadata_conflicts.resolve_text(
            "tools/permute.py", source
        )
        self.assertEqual(count, 2)
        for name in ("First", "Second", "Third", "Fourth"):
            self.assertIn(f'"{name}"', resolved)

    def test_metadata_merge_refuses_code_or_disagreeing_key(self):
        code = f'''{self.START}
    doSomething()
{self.MIDDLE}
    doSomethingElse()
{self.END}
'''
        disagreement = f'''{self.START}
    extra "F" = ["--one"]
{self.MIDDLE}
    extra "F" = ["--two"]
{self.END}
'''
        with self.assertRaises(ValueError):
            merge_metadata_conflicts.resolve_text("shake/src/Build.hs", code)
        with self.assertRaises(ValueError):
            merge_metadata_conflicts.resolve_text(
                "shake/src/Build.hs", disagreement
            )

    def test_metadata_merge_rejects_other_unmerged_paths(self):
        self.assertEqual(
            merge_metadata_conflicts.unexpected_unmerged(
                ["shake/src/Build.hs", "tools/permute.py"],
                ["tools/permute.py", "src/main.exe/F.c"],
            ),
            ["src/main.exe/F.c"],
        )


if __name__ == "__main__":
    unittest.main()

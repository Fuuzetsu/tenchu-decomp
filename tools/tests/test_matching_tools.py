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

TOOLS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if TOOLS not in sys.path:
    sys.path.insert(0, TOOLS)

import autorules
import matchlock
import maspsxflags
import permute
import regalloc
import rtlguide
import stackplan


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
            def fake_score(_name, _partial):
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

    def test_known_commutative_plus_destination_signature(self):
        h = self.hunk(["addu a0,a0,v1", "sw a0,0(gp)"],
                      ["addu v1,v1,a0", "sw v1,0(gp)"])
        self.assertEqual(rtlguide.known_residual_signatures([h]),
                         ["commutative-plus-destination"])

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


if __name__ == "__main__":
    unittest.main()

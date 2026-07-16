#!/usr/bin/env python3
import os
import sys
import tempfile
import unittest
import collections
from unittest import mock
import contextlib
import io
import inspect
import importlib.util
import fcntl
import signal
import subprocess

TOOLS = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
if TOOLS not in sys.path:
    sys.path.insert(0, TOOLS)

import autorules
import access
import asmdiff
import callmatch
import datamatch
import findsimilar
import function_inventory
import fuzzy_inventory
import matchlock
import matchdiff
import maspsxflags
import merge_metadata_conflicts
import permute
import progress
import regalloc
import reghist
import rtlguide
import siblingdiff
import stackplan
import symnear
import triage
import xref

MATCHER_PROMPT_SPEC = importlib.util.spec_from_file_location(
    "matcher_prompt", os.path.join(TOOLS, "matcher-prompt.py")
)
matcher_prompt = importlib.util.module_from_spec(MATCHER_PROMPT_SPEC)
MATCHER_PROMPT_SPEC.loader.exec_module(matcher_prompt)

SYMNOTE_SPEC = importlib.util.spec_from_file_location(
    "symnote", os.path.join(TOOLS, "symnote.py")
)
symnote = importlib.util.module_from_spec(SYMNOTE_SPEC)
SYMNOTE_SPEC.loader.exec_module(symnote)


class WorktreeInitTests(unittest.TestCase):
    def test_long_worktree_list_does_not_trip_pipefail(self):
        with tempfile.TemporaryDirectory() as td:
            primary = os.path.join(td, "primary")
            secondary = os.path.join(td, "secondary")
            fakebin = os.path.join(td, "fakebin")
            os.makedirs(os.path.join(primary, "disks"))
            os.makedirs(os.path.join(primary, ".shake", "ghidra-export"))
            os.makedirs(os.path.join(secondary, "disks"))
            os.makedirs(fakebin)

            disk = os.path.join(primary, "disks", "tenchu.bin")
            export = os.path.join(
                primary, ".shake", "ghidra-export", "functions.tsv"
            )
            open(disk, "w").close()
            open(export, "w").close()

            fake_git = os.path.join(fakebin, "git")
            with open(fake_git, "w") as fh:
                fh.write("""#!/bin/sh
if [ "$1" = rev-parse ]; then
    printf '%s\\n' "$FAKE_HERE"
elif [ "$1" = worktree ] && [ "$2" = list ]; then
    printf 'worktree %s\\n\\n' "$FAKE_MAIN"
    i=0
    while [ "$i" -lt 20000 ]; do
        printf 'worktree /tmp/fake-worktree-%s\\n\\n' "$i"
        i=$((i + 1))
    done
else
    exit 2
fi
""")
            os.chmod(fake_git, 0o755)

            env = os.environ.copy()
            env.update({
                "PATH": fakebin + os.pathsep + env["PATH"],
                "FAKE_HERE": secondary,
                "FAKE_MAIN": primary,
            })
            script = os.path.join(TOOLS, "wt-init.sh")
            result = subprocess.run(
                ["bash", script], cwd=secondary, env=env,
                text=True, capture_output=True,
            )

            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn("wt-init: ready", result.stdout)
            self.assertEqual(
                os.readlink(os.path.join(secondary, "disks", "tenchu.bin")),
                disk,
            )
            self.assertEqual(
                os.readlink(os.path.join(
                    secondary, ".shake", "ghidra-export"
                )),
                os.path.join(primary, ".shake", "ghidra-export"),
            )


class FunctionInventoryTests(unittest.TestCase):
    def test_known_truncated_boundaries_are_corrected(self):
        with tempfile.NamedTemporaryFile("w", delete=False) as fh:
            path = fh.name
            fh.write("80056f50\t276\tLoadCard\n")
            fh.write("800593a0\t300\tFUN_800593a0\n")
            fh.write("8005fd3c\t248\tAdtVsprintf\n")
            fh.write("8005fe34\t84\tFUN_8005fe34\n")
        try:
            self.assertEqual(
                function_inventory.load_functions(path),
                [
                    (0x80056F50, 0x168, "LoadCard"),
                    (0x800593A0, 0x27C, "FUN_800593a0"),
                    (0x8005FD3C, 0xFC, "AdtVsprintf"),
                    (0x8005FE38, 0x50, "FUN_8005fe38"),
                ],
            )
        finally:
            os.unlink(path)

    def test_target_pickers_share_phantom_boundary_correction(self):
        with tempfile.TemporaryDirectory() as td:
            funcs = os.path.join(td, "functions.tsv")
            symbols = os.path.join(td, "symbols.txt")
            splat = os.path.join(td, "splat.yaml")
            with open(funcs, "w") as fh:
                fh.write("8005fd3c\t248\tAdtVsprintf\n")
                fh.write("8005fe34\t84\tFUN_8005fe34\n")
                fh.write("8005fe88\t68\tFUN_8005fe88\n")
            with open(symbols, "w") as fh:
                fh.write("AdtVsprintf = 0x8005fd3c;\n")
                fh.write("FUN_8005fe38 = 0x8005fe38;\n")
                fh.write("FUN_8005fe88 = 0x8005fe88;\n")
            with open(splat, "w") as fh:
                fh.write("  - [0x4F53C, c, AdtVsprintf]\n")
                fh.write("  - [0x4F638, c, FUN_8005fe38]\n")
                fh.write("  - [0x4F688, c, FUN_8005fe88]\n")

            expected = [
                (0x8005FD3C, 0xFC, "AdtVsprintf"),
                (0x8005FE38, 0x50, "FUN_8005fe38"),
                (0x8005FE88, 0x44, "FUN_8005fe88"),
            ]
            consumers = [
                progress.load_functions(funcs, splat),
                triage.load_functions(funcs, symbols, splat),
                findsimilar.load_functions(funcs, symbols, splat),
                siblingdiff.load_functions(funcs, symbols, splat),
            ]
            self.assertEqual(
                access.bounds("FUN_8005fe38", symbols, funcs),
                (0x8005FE38, 0x50),
            )

        for rows in consumers:
            self.assertEqual(rows, expected)
            self.assertNotIn("FUN_8005fe34", {n for _, _, n in rows})

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

    def test_datamatch_infers_dominant_data_relocation_from_unique_names(self):
        psx = {
            "One": {0x1000},
            "Two": {0x2000},
            # Duplicate statics must not calibrate the relocation.
            "Me": {0x3000, 0x4000},
        }
        demo = {
            "One": {0x1358},
            "Two": {0x2358},
            "Me": {0x3358, 0x4358},
        }

        self.assertEqual(
            datamatch.infer_data_delta(psx, demo, min_anchors=2),
            (0x358, 2, 2),
        )

    def test_datamatch_rejects_weak_or_nondominant_data_relocation(self):
        with self.assertRaises(ValueError):
            datamatch.infer_data_delta(
                {"One": {0x1000}}, {"One": {0x1358}}, min_anchors=2
            )
        with self.assertRaises(ValueError):
            datamatch.infer_data_delta(
                {"One": {0x1000}, "Two": {0x2000}, "Three": {0x3000}},
                {"One": {0x1358}, "Two": {0x2358}, "Three": {0x3468}},
                min_anchors=2,
                min_dominance=0.99,
            )

    def test_datamatch_synthesis_prefers_semantic_alias(self):
        merged, pairs, addrs = datamatch.synthesize_demo_data_labels(
            {},
            {
                0x1000: {
                    "__data_org",
                    "__text_objend",
                    "UnitVector2",
                    "vector.17",
                }
            },
            0x358,
        )

        self.assertEqual(merged, {0x1358: {"UnitVector2"}})
        self.assertEqual((pairs, addrs), (1, 1))

    def test_datamatch_duplicate_static_requires_matching_translation_unit(self):
        labels = {0x1358: {"CID"}}
        by_name = {"CID": {0x1000, 0x2000}}
        owners = {(0x1000, "CID"): {"INFOVIEW.C"}}

        self.assertIsNone(
            datamatch.resolve_data_name(0x1358, labels, by_name, 0x358)
        )
        self.assertIsNone(
            datamatch.resolve_data_name(
                0x1358, labels, by_name, 0x358, {"MEMCARD.C"}, owners
            )
        )
        self.assertEqual(
            datamatch.resolve_data_name(
                0x1358, labels, by_name, 0x358, {"INFOVIEW.C"}, owners
            ),
            "CID",
        )

    def test_datamatch_reverse_uniqueness_uses_conflicting_raw_votes(self):
        votes = {
            0x80001000: collections.Counter({"Name": 2}),
            0x80002000: collections.Counter({"Name": 1, "Other": 1}),
        }
        targets = {
            (0x80001000, "Name"): {0x90001000},
            (0x80002000, "Name"): {0x90001000},
            (0x80002000, "Other"): {0x90002000},
        }

        clean, stats = datamatch.select_clean_votes(votes, targets)

        self.assertEqual(clean, {})
        self.assertEqual(stats["nonunanimous"], 1)
        self.assertEqual(stats["reverse_conflicts"], 1)

    def test_symnote_typed_owner_uses_object_extent_not_nearest_name(self):
        typed = {
            0x8008F188: ("short *StageAppearance[10]", 0x28),
            0x801D5848: ("struct TPadPort PadPort[2][4]", 0x60),
        }

        # A generated D_ label at PadPort+7 is an interior access, while the
        # unrelated Adt global 0x30 bytes after StageAppearance is not.
        self.assertEqual(
            symnote.typed_owner(0x801D584F, typed, []), 0x801D5848
        )
        self.assertIsNone(symnote.typed_owner(0x8008F1B8, typed, []))

    def test_symnote_meaningful_retail_symbol_splits_demo_extent(self):
        typed = {0x80001000: ("struct OldLayout table", 0x100)}

        self.assertIsNone(
            symnote.typed_owner(0x80001090, typed, [0x80001080])
        )
        self.assertEqual(
            symnote.typed_owner(0x80001070, typed, [0x80001080]), 0x80001000
        )

    def test_symnote_parses_function_pointer_array_global(self):
        self.assertEqual(
            symnote.parse_global_decl(
                "extern short (*Think1Func[10])();  "
                "/* 0x80089d94, size 0x28, static in THINK_4.C */\n"
            ),
            (0x80089D94, "short (*Think1Func[10])()", 0x28),
        )

    def test_xref_uses_current_c_names_with_ghidra_extents(self):
        with tempfile.TemporaryDirectory() as td:
            funcs = os.path.join(td, "functions.tsv")
            splat = os.path.join(td, "splat.yaml")
            with open(funcs, "w") as fh:
                fh.write("80011000\t16\tFUN_80011000\n")
                fh.write("80011010\t20\tAlreadyCurrent\n")
            with open(splat, "w") as fh:
                fh.write("  - [0x800, c, RecoveredName]\n")
                fh.write("  - [0x810, c, AlreadyCurrent]\n")

            rows = xref.functions(funcs, splat)

        self.assertEqual(rows, [
            (0x80011000, 0x80011010, "RecoveredName"),
            (0x80011010, 0x80011024, "AlreadyCurrent"),
        ])


class FuzzyInventoryTests(unittest.TestCase):
    def test_detects_missing_orphaned_and_source_stale_rows(self):
        with tempfile.TemporaryDirectory() as td:
            src = os.path.join(td, "src")
            os.makedirs(src)
            guarded = os.path.join(src, "Guarded.c")
            with open(guarded, "w") as fh:
                fh.write("#ifndef NON_MATCHING\n#else\nint Guarded(void) { return 1; }\n#endif\n")
            with open(os.path.join(src, "Exact.c"), "w") as fh:
                fh.write("int Exact(void) { return 1; }\n")
            tsv = os.path.join(td, "fuzzy.tsv")
            with open(tsv, "w") as fh:
                fh.write("Orphan\t12.00\tok\tdeadbeef\n")

            errors = fuzzy_inventory.validate(src, tsv)
            self.assertIn("missing fuzzy row for guarded draft Guarded", errors)
            self.assertIn("orphan fuzzy row for non-guarded function Orphan", errors)

            with open(tsv, "w") as fh:
                fh.write("Guarded\t12.00\tok\tdeadbeef\n")
            self.assertEqual(
                fuzzy_inventory.validate(src, tsv),
                ["source-stale fuzzy row for Guarded"],
            )

            with open(tsv, "w") as fh:
                fh.write(
                    "Guarded\t12.00\tok\t"
                    + fuzzy_inventory.source_hash(guarded)
                    + "\n"
                )
            self.assertEqual(fuzzy_inventory.validate(src, tsv), [])

    def test_partial_rescore_prunes_orphans_but_retains_live_rows(self):
        rows = {
            "RenamedOld": "old row",
            "StillGuarded": "live row",
            "UnrequestedGuarded": "other live row",
        }

        kept, removed = fuzzy_inventory.retain_guarded(
            rows, {"StillGuarded", "UnrequestedGuarded"}
        )

        self.assertEqual(removed, ["RenamedOld"])
        self.assertEqual(kept, {
            "StillGuarded": "live row",
            "UnrequestedGuarded": "other live row",
        })


class MatcherPromptTests(unittest.TestCase):
    def test_function_lookup_overlays_current_splat_name(self):
        with tempfile.TemporaryDirectory() as td:
            funcs = os.path.join(td, "functions.tsv")
            splat = os.path.join(td, "splat.yaml")
            with open(funcs, "w") as fh:
                fh.write("80011000\t16\tFUN_80011000\n")
            with open(splat, "w") as fh:
                fh.write("  - [0x800, c, RecoveredName]\n")
            with mock.patch.object(matcher_prompt, "TSV", funcs), \
                    mock.patch.object(matcher_prompt, "SPLAT", splat):
                self.assertEqual(
                    matcher_prompt.func("RecoveredName"),
                    (0x80011000, 16),
                )

    def test_repeated_debug_locals_emit_scope_identity_hint(self):
        rows = [
            ["AttackControl", "0", "reg", "$s0", "struct Humanoid *", "enemy"],
            ["AttackControl", "1", "reg", "$a1", "short", "mid"],
            ["AttackControl", "2", "reg", "$v0", "struct Humanoid *", "enemy"],
        ]

        hints = matcher_prompt.repeated_local_scope_hints(rows)

        self.assertIn("Keep them block-scoped initially", hints[0])
        self.assertEqual(hints[1], "    `enemy` x2 (reg $s0, reg $v0)")

    def test_unique_debug_locals_do_not_emit_scope_hint(self):
        rows = [
            ["Function", "0", "reg", "$s0", "long", "x"],
            ["Function", "1", "stack", "sp+16", "short", "y"],
        ]

        self.assertEqual(matcher_prompt.repeated_local_scope_hints(rows), [])


class SiblingDiffTests(unittest.TestCase):
    def test_psx_text_start_comes_from_exe_header(self):
        header = bytearray(0x800)
        header[:8] = b"PS-X EXE"
        header[0x18:0x1C] = (0x80010100).to_bytes(4, "little")
        with tempfile.NamedTemporaryFile(delete=False) as fh:
            path = fh.name
            fh.write(header)
        try:
            self.assertEqual(siblingdiff.psx_text_start(path), 0x80010100)
        finally:
            os.unlink(path)

    def test_demo_mode_normalizes_internal_labels_before_named_calls(self):
        insns = {
            0x80010000: ("jal", "80020000"),
            0x80010004: ("j", "8001000c"),
        }
        names = {0x80020000: "SharedCallee", 0x8001000C: "WrongExternalName"}

        self.assertEqual(
            siblingdiff.norm_line(
                insns, 0x80010000, 0x80010000, 0x80010010, names
            ),
            "jal SharedCallee",
        )
        self.assertEqual(
            siblingdiff.norm_line(
                insns, 0x80010004, 0x80010000, 0x80010010, names
            ),
            "j Lc",
        )

    def test_demo_call_deficit_with_embedded_helper_shape_hints_inline(self):
        demo = [
            "jal FixName", "nop", "jal ScanTable", "nop",
            "jal ScanTable", "nop",
        ]
        retail = [
            "lbu v0,0(a0)", "beqz v0,L10", "jal toupper", "sb v0,0(a0)",
            "lw v0,16(s0)", "beqz v0,L20", "jal strncmp", "bnez v0,L24",
            "lw v0,12(s0)", "lhu v0,0(v0)", "and v0,a2,v0",
            "bnez v0,L28", "addiu a1,a1,1",
        ]
        helpers = {
            "FixName": [
                "lbu v0,0(a0)", "beqz v0,L10", "jal toupper",
                "sb v0,0(a0)",
            ],
            "ScanTable": [
                "lw v0,16(a0)", "beqz v0,L20", "jal strncmp",
                "bnez v0,L24", "lw v0,12(a0)", "lhu v0,0(v0)",
                "and v0,a2,v0", "bnez v0,L28", "addiu a1,a1,1",
            ],
        }

        hints = siblingdiff.inline_hints(
            demo, retail, helpers, min_hits=3, min_coverage=0.60
        )

        self.assertEqual([row[0] for row in hints], ["FixName", "ScanTable"])
        self.assertEqual([row[1] for row in hints], [1, 2])

    def test_inline_hint_rejects_missing_call_without_helper_shape(self):
        hints = siblingdiff.inline_hints(
            ["jal ChangedAway", "nop"],
            ["lw v0,0(a0)", "jr ra", "nop"],
            {"ChangedAway": [
                "lbu v0,0(a0)", "jal toupper", "sb v0,0(a0)",
                "bnez v0,L0", "addiu a0,a0,1", "jr ra", "nop",
            ]},
        )

        self.assertEqual(hints, [])

    def test_composition_prefers_long_runs_and_fills_demo_gaps(self):
        target = ["a", "b", "c", "d", "e", "f", "g", "h", "i", "j"]
        references = [
            ("demo:F", ["a", "b", "c", "d", "x", "g", "h", "z"]),
            ("Sibling", ["q", "e", "f", "g", "h", "i", "r"]),
        ]

        rows = siblingdiff.compose_provenance(target, references, min_run=3)

        self.assertEqual(rows, [
            (0, 4, "demo:F", 0, 4),
            (4, 9, "Sibling", 1, 6),
            (9, 10, None, None, None),
        ])

    def test_composition_rejects_trivial_nop_runs(self):
        rows = siblingdiff.compose_provenance(
            ["nop", "nop", "nop", "nop", "real"],
            [("Noise", ["nop", "nop", "nop", "nop"])],
            min_run=4,
        )

        self.assertEqual(rows, [(0, 5, None, None, None)])

    def test_composition_source_order_preserves_demo_provenance(self):
        rows = siblingdiff.compose_provenance(
            ["a", "b", "c", "d", "e", "f"],
            [
                ("demo:F", ["b", "c", "d", "e"]),
                ("Sibling", ["a", "b", "c", "d", "e", "f"]),
            ],
            min_run=4,
        )

        self.assertEqual(rows, [
            (0, 1, None, None, None),
            (1, 5, "demo:F", 0, 4),
            (5, 6, None, None, None),
        ])


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
        autorules.GUIDED_VARIABLES = set()
        autorules._TARGET_GP_CACHE.clear()

    def candidates(self, rule, source):
        return list(rule(source, "F", (0, len(source))))

    def test_guided_rule_selection_preserves_rtl_priority(self):
        priority = [
            "allocation-donor-fence",
            "disjoint-local-alias",
            "type-width",
            "case-fence",
        ]
        selected = autorules.select_rules(guided=priority)
        self.assertEqual([entry[0] for entry in selected], priority)

    def test_explicit_rule_selection_preserves_cli_order_and_deduplicates(self):
        requested = ["case-fence", "type-width", "case-fence"]
        selected = autorules.select_rules(requested=requested)
        self.assertEqual(
            [entry[0] for entry in selected],
            ["case-fence", "type-width"],
        )

    def test_type_width_skips_pointer_and_array_declarations(self):
        source = """typedef short s16;
typedef unsigned short u16;
void F(void) {
    s16 scalar;
    u16 *view;
    u16 samples[4];
    use(scalar, view, samples);
}
"""
        labels = [label for label, _candidate in
                  self.candidates(autorules.rule_type_width, source)]
        self.assertTrue(any(label.startswith("scalar:") for label in labels))
        self.assertFalse(any(label.startswith("view:") for label in labels))
        self.assertFalse(any(label.startswith("samples:") for label in labels))

    def test_type_width_skips_shared_base_multi_declarators(self):
        source = """typedef short s16;
void F(void) {
    s16 scalar, *view;
    s16 count, samples[4];
    s16 first, second;
    s16 multiline,
        *later_view;
    s16 lone;
    use(scalar, view, count, samples, first, second, multiline, later_view, lone);
}
"""
        labels = [label for label, _candidate in
                  self.candidates(autorules.rule_type_width, source)]
        self.assertTrue(any(label.startswith("lone:") for label in labels))
        for skipped in ("scalar", "view", "count", "samples", "first",
                        "second", "multiline", "later_view"):
            self.assertFalse(any(label.startswith(skipped + ":")
                                 for label in labels))

    def test_type_width_does_not_unsigned_flip_an_explicit_sign_test(self):
        source = """typedef int s32;
void F(void) {
    s32 amount;
    amount = produce();
    if (amount < 0)
        amount += 0x3ff;
    consume(amount);
}
"""
        labels = [label for label, _candidate in
                  self.candidates(autorules.rule_type_width, source)]
        self.assertIn("amount: s32→s16", labels)
        self.assertNotIn("amount: s32→u32", labels)

    def test_type_width_does_not_unsigned_flip_an_arithmetic_shift(self):
        source = """typedef int s32;
void F(void) {
    s32 scaled;
    scaled = produce();
    consume((scaled) >> 10);
}
"""
        labels = [label for label, _candidate in
                  self.candidates(autorules.rule_type_width, source)]
        self.assertIn("scaled: s32→s16", labels)
        self.assertNotIn("scaled: s32→u32", labels)

    def test_type_width_keeps_unsigned_flip_without_signed_semantics(self):
        source = """typedef int s32;
void F(void) {
    s32 count;
    count = produce();
    consume(count + 1);
}
"""
        labels = [label for label, _candidate in
                  self.candidates(autorules.rule_type_width, source)]
        self.assertIn("count: s32→u32", labels)

    def test_cmp_polarity_swaps_local_operands(self):
        source = """int F(int direction, int turn) {
    if (direction < turn) return 1;
    return 0;
}
"""
        out = self.candidates(autorules.rule_cmp_polarity, source)
        self.assertEqual(len(out), 1)
        self.assertIn("turn > direction", out[0][1])

    def test_initialized_global_compound_emits_both_accumulator_orders(self):
        source = """extern int AttackActionCount;
extern int GameClock;
extern short EngageLevel;
void F(void) {
    AttackActionCount = EngageLevel * 10 + GameClock;
}
"""
        autorules.GUIDED_LINES = {5}
        out = self.candidates(autorules.rule_initialized_global_compound, source)
        self.assertEqual(len(out), 2)
        candidates = [candidate for _label, candidate in out]
        self.assertTrue(any(
            "AttackActionCount = GameClock;\n"
            "    AttackActionCount += EngageLevel * 10;" in candidate
            for candidate in candidates
        ))
        self.assertTrue(any(
            "AttackActionCount = EngageLevel * 10;\n"
            "    AttackActionCount += GameClock;" in candidate
            for candidate in candidates
        ))

    def test_initialized_global_compound_rejects_local_or_effectful_sum(self):
        local = """void F(int a, int b) {
    int result;
    result = a + b;
}
"""
        effectful = """extern int Result;
void F(int value) {
    Result = helper() + value;
}
"""
        self_reference = """extern int Result;
void F(int value) {
    Result = Result + value;
}
"""
        for source in (local, effectful, self_reference):
            self.assertEqual(
                self.candidates(autorules.rule_initialized_global_compound, source),
                [],
            )

    def test_stack_decl_swap_reorders_address_taken_objects(self):
        source = """typedef struct { int x; } VECTOR;
void F(void) {
    VECTOR tmp;
    VECTOR pos;
    use(&pos);
    use(&tmp);
}
"""
        out = self.candidates(autorules.rule_stack_decl_swap, source)
        self.assertEqual(len(out), 1)
        self.assertIn("stack-decl-swap tmp/pos L3", out[0][0])
        self.assertLess(out[0][1].index("VECTOR pos;"),
                        out[0][1].index("VECTOR tmp;"))

    def test_stack_decl_swap_rejects_unsafe_declarations(self):
        initializer = """typedef struct { int x; } VECTOR;
void F(void) {
    VECTOR tmp = {0};
    VECTOR pos;
    use(&tmp);
    use(&pos);
}
"""
        missing_address = """typedef struct { int x; } VECTOR;
void F(void) {
    VECTOR tmp;
    VECTOR pos;
    use(tmp);
    use(&pos);
}
"""
        separated = """typedef struct { int x; } VECTOR;
void F(void) {
    VECTOR tmp;
    /* stack roles */
    VECTOR pos;
    use(&tmp);
    use(&pos);
}
"""
        for source in (initializer, missing_address, separated):
            self.assertEqual(
                self.candidates(autorules.rule_stack_decl_swap, source), [])

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

    def test_literal_indirect_inline_changes_both_temp_families_atomically(self):
        source = """typedef unsigned char u8;
typedef struct Item Item;
struct Item { u8 mode; void (*proc)(Item *); };
void F(Item *item) {
    u8 ff;
    ff = 0xff;
    if (item->mode == ff) use();
    {
        void (*proc)(Item *);
        proc = item->proc;
        if (proc != 0) {
            item->mode = ff;
            proc(item);
        }
    }
}
"""
        out = self.candidates(autorules.rule_literal_indirect_inline, source)
        self.assertEqual(len(out), 1)
        self.assertIn("literal-indirect-inline ff/proc", out[0][0])
        candidate = out[0][1]
        self.assertNotIn("u8 ff;", candidate)
        self.assertNotIn("        void (*proc)", candidate)
        self.assertIn("if (item->mode == 0xff)", candidate)
        self.assertIn("if (item->proc != 0)", candidate)
        self.assertIn("item->proc(item);", candidate)

    def test_literal_indirect_inline_requires_literal_and_call_pair(self):
        literal_only = """typedef unsigned char u8;
void F(void) { u8 ff; ff = 0xff; use(ff); use(ff); }
"""
        proc_only = """typedef struct Item Item;
struct Item { void (*proc)(Item *); };
void F(Item *item) {
    void (*proc)(Item *);
    proc = item->proc;
    if (proc) proc(item);
}
"""
        self.assertEqual(
            self.candidates(autorules.rule_literal_indirect_inline, literal_only), [])
        self.assertEqual(
            self.candidates(autorules.rule_literal_indirect_inline, proc_only), [])

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

    def test_terminal_arm_flip_swaps_adjacent_goto_arms(self):
        source = """void F(int pressed) {
    if (pressed == 0) {
        Human *human;
        human = Me;
        Move(human);
        goto common;
    }
    {
        Move(Me);
        goto common;
    }
common:
    use();
}
"""
        autorules.GUIDED_LINES = {2}
        out = self.candidates(autorules.rule_terminal_arm_flip, source)
        self.assertEqual(len(out), 1)
        self.assertIn("terminal-arm-flip L2", out[0][0])
        self.assertIn(
            "if (!(pressed == 0)) {\n        Move(Me);\n"
            "        goto common;\n    }\n    {\n        Human *human;",
            out[0][1],
        )

    def test_terminal_arm_flip_wraps_a_plain_terminal_sequence(self):
        source = """void F(int pressed) {
    if (pressed == 0) {
        Human *human;
        human = Me;
        Move(human);
        goto common;
    }
    Move(Me);
    goto common;
common:
    use();
}
"""
        autorules.GUIDED_LINES = {2}
        out = self.candidates(autorules.rule_terminal_arm_flip, source)
        self.assertEqual(len(out), 1)
        self.assertIn(
            "if (!(pressed == 0)) {\n        Move(Me);\n"
            "        goto common;\n    }\n    {\n        Human *human;",
            out[0][1],
        )

    def test_terminal_arm_flip_rejects_unproven_or_interrupted_arms(self):
        nonterminal = """void F(int x) {
    if (x) { a(); goto common; }
    { b(); }
common:
    use();
}
"""
        different_labels = """void F(int x) {
    if (x) { a(); goto left; }
    { b(); goto right; }
left:
right:
    use();
}
"""
        with_comment = """void F(int x) {
    if (x) { a(); goto common; }
    /* keep this physical anchor */
    { b(); goto common; }
common:
    use();
}
"""
        autorules.GUIDED_LINES = {2}
        for source in (nonterminal, different_labels, with_comment):
            self.assertEqual(
                self.candidates(autorules.rule_terminal_arm_flip, source), [])

    def test_terminal_guard_flip_reverses_return_goto_layout(self):
        source = """void F(int command) {
    if (command != 0x14) {
        return;
    }
    goto command_14;
command_14:
    use();
}
"""
        autorules.GUIDED_LINES = {2}
        out = self.candidates(autorules.rule_terminal_guard_flip, source)
        self.assertEqual(len(out), 1)
        self.assertIn("terminal-guard-flip return-to-goto L2", out[0][0])
        self.assertIn(
            "if (command == 0x14)\n    {\n        goto command_14;\n"
            "    }\n    return;",
            out[0][1],
        )

    def test_terminal_guard_flip_is_reversible(self):
        source = """void F(int command) {
    if (command == 0x14)
        goto command_14;
    return;
command_14:
    use();
}
"""
        autorules.GUIDED_LINES = {2}
        out = self.candidates(autorules.rule_terminal_guard_flip, source)
        self.assertEqual(len(out), 1)
        self.assertIn("terminal-guard-flip goto-to-return L2", out[0][0])
        self.assertIn(
            "if (command != 0x14)\n    {\n        return;\n"
            "    }\n    goto command_14;",
            out[0][1],
        )

    def test_terminal_guard_flip_rejects_nonlocal_layouts(self):
        with_else = """void F(int x) {
    if (x != 1) { return; } else { use(); }
    goto body;
body:
    use();
}
"""
        with_comment = """void F(int x) {
    if (x != 1) return;
    /* physical body anchor */
    goto body;
body:
    use();
}
"""
        non_equality = """void F(int x) {
    if (x < 1) return;
    goto body;
body:
    use();
}
"""
        for source in (with_else, with_comment, non_equality):
            self.assertEqual(
                self.candidates(autorules.rule_terminal_guard_flip, source), [])

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

    def test_field_capture_rhs_fuses_both_dependency_orders(self):
        source = """void F(Sprite *sprite, int remainder) {
    int base_u;
    base_u = sprite->u;
    sprite->u = base_u + remainder * sprite->w;
}
"""
        autorules.GUIDED_LINES = {3}
        out = self.candidates(autorules.rule_field_capture_rhs, source)
        expr_first = [text for label, text in out if "fuse-expr-first" in label]
        capture_first = [text for label, text in out
                         if "fuse-capture-first" in label]
        self.assertEqual((len(expr_first), len(capture_first)), (1, 1))
        self.assertIn(
            "sprite->u = (remainder * sprite->w) + (base_u = sprite->u);",
            expr_first[0],
        )
        self.assertIn(
            "sprite->u = (base_u = sprite->u) + (remainder * sprite->w);",
            capture_first[0],
        )

    def test_field_capture_rhs_splits_fused_form(self):
        source = """void F(Sprite *sprite, int remainder) {
    int base_u;
    sprite->u = remainder * sprite->w + (base_u = sprite->u);
}
"""
        autorules.GUIDED_LINES = {3}
        out = self.candidates(autorules.rule_field_capture_rhs, source)
        saved_first = [text for label, text in out if "split-saved-first" in label]
        expr_first = [text for label, text in out if "split-expr-first" in label]
        self.assertEqual((len(saved_first), len(expr_first)), (1, 1))
        self.assertIn(
            "base_u = sprite->u;\n    sprite->u = base_u + remainder * sprite->w;",
            saved_first[0],
        )

    def test_field_capture_rhs_rejects_effectful_other_operand(self):
        source = """void F(Sprite *sprite) {
    int base_u;
    base_u = sprite->u;
    sprite->u = base_u + next_value();
}
"""
        autorules.GUIDED_LINES = {3}
        self.assertEqual(
            self.candidates(autorules.rule_field_capture_rhs, source), [])

    def test_field_capture_rhs_rejects_volatile_pointee_and_other_read(self):
        volatile_target = """void F(volatile Sprite *sprite, int remainder) {
    int base_u;
    base_u = sprite->u;
    sprite->u = base_u + remainder;
}
"""
        volatile_other = """void F(Sprite *sprite, volatile Sprite *other) {
    int base_u;
    base_u = sprite->u;
    sprite->u = base_u + other->w;
}
"""
        global_target = """Sprite *sprite;
void F(int remainder) {
    int base_u;
    base_u = sprite->u;
    sprite->u = base_u + remainder;
}
"""
        autorules.GUIDED_LINES = {3}
        for source in (volatile_target, volatile_other, global_target):
            self.assertEqual(
                self.candidates(autorules.rule_field_capture_rhs, source), [])

    def test_field_capture_rhs_maps_an_invoked_macro_capture(self):
        source = """#define DRAW() { \\
    base_u = sprite->u; \\
    sprite->u = base_u + remainder * sprite->w; \\
}
void F(Sprite *sprite, int remainder) {
    int base_u;
    DRAW();
}
#undef DRAW
"""
        autorules.GUIDED_LINES = {7}
        out = self.candidates(autorules.rule_field_capture_rhs, source)
        fused = [(label, text) for label, text in out
                 if "macro-DRAW" in label and "fuse-expr-first" in label]
        self.assertEqual(len(fused), 1)
        self.assertIn(
            "sprite->u = (remainder * sprite->w) + (base_u = sprite->u);",
            fused[0][1],
        )

    def test_field_capture_rhs_rejects_uninvoked_macro(self):
        source = """#define DRAW() { \\
    base_u = sprite->u; \\
    sprite->u = base_u + remainder * sprite->w; \\
}
void F(Sprite *sprite, int remainder) {
    int base_u;
    use(base_u, sprite, remainder);
}
"""
        self.assertEqual(
            self.candidates(autorules.rule_field_capture_rhs, source), [])

    def test_field_capture_rhs_rejects_unmapped_macro_formal(self):
        source = """#define DRAW(sprite_) { \\
    base_u = sprite_->u; \\
    sprite_->u = base_u + remainder; \\
}
void F(Sprite *sprite, int remainder) {
    int base_u;
    DRAW(sprite);
}
"""
        autorules.GUIDED_LINES = {7}
        self.assertEqual(
            self.candidates(autorules.rule_field_capture_rhs, source), [])

    def test_disjoint_local_alias_joins_dead_until_overwrite_scalar(self):
        source = """typedef int s32;
int F(int input) {
    s32 xx;
    s32 vx;
    s32 distance;
    xx = input + 1;
    distance = xx * xx;
    vx = input - 1;
    return vx + distance;
}
"""
        autorules.GUIDED_VARIABLES = {b"xx", b"vx"}
        out = self.candidates(autorules.rule_disjoint_local_alias, source)
        mixed = [text for label, text in out
                 if label.startswith("disjoint-local-alias vx=xx") and
                 "distance = vx * xx;" in text]
        self.assertEqual(len(mixed), 1)
        self.assertIn("vx = xx = input + 1;", mixed[0])

    def test_disjoint_local_alias_rejects_prior_alias_read(self):
        source = """typedef int s32;
int F(int input) {
    s32 xx;
    s32 vx;
    xx = input + 1;
    input += vx;
    vx = input - 1;
    return xx + vx;
}
"""
        autorules.GUIDED_VARIABLES = {b"xx", b"vx"}
        self.assertEqual(
            self.candidates(autorules.rule_disjoint_local_alias, source), [])

    def test_disjoint_local_alias_rejects_shadowed_source(self):
        source = """typedef int s32;
int F(int input) {
    s32 xx;
    s32 vx;
    s32 distance;
    xx = input + 1;
    {
        s32 xx;
        distance = xx;
    }
    vx = input - 1;
    return vx + distance;
}
"""
        autorules.GUIDED_VARIABLES = {b"xx", b"vx"}
        self.assertEqual(
            self.candidates(autorules.rule_disjoint_local_alias, source), [])

    def test_disjoint_local_alias_rejects_initialized_alias(self):
        source = """typedef int s32;
int F(int input) {
    s32 xx;
    s32 vx = 5;
    xx = input + 1;
    input += xx;
    vx = input - 1;
    return xx + vx;
}
"""
        autorules.GUIDED_VARIABLES = {b"xx", b"vx"}
        self.assertEqual(
            self.candidates(autorules.rule_disjoint_local_alias, source), [])

    def test_disjoint_local_alias_rejects_static_alias(self):
        source = """typedef int s32;
int F(int input) {
    s32 xx;
    static s32 vx;
    xx = input + 1;
    input += xx;
    vx = input - 1;
    return xx + vx;
}
"""
        autorules.GUIDED_VARIABLES = {b"xx", b"vx"}
        self.assertEqual(
            self.candidates(autorules.rule_disjoint_local_alias, source), [])

    def test_disjoint_local_alias_rejects_loop_carried_alias(self):
        source = """typedef int s32;
int F(int input) {
    s32 xx;
    s32 vx;
    while (input) {
        xx = input + 1;
        input += xx;
        vx = input - 1;
    }
    return vx;
}
"""
        autorules.GUIDED_VARIABLES = {b"xx", b"vx"}
        self.assertEqual(
            self.candidates(autorules.rule_disjoint_local_alias, source), [])

    def test_member_scalar_alias_toggles_plain_long_field_read(self):
        source = """typedef int s32;
void F(Node *param) {
    long y;
    y = param->py;
}
"""
        autorules.GUIDED_LINES = {4}
        added = self.candidates(autorules.rule_member_scalar_alias, source)
        self.assertEqual(len(added), 1)
        self.assertIn("y = *(s32 *)&param->py;", added[0][1])
        removed = self.candidates(
            autorules.rule_member_scalar_alias, added[0][1])
        self.assertEqual(len(removed), 1)
        self.assertIn("y = param->py;", removed[0][1])

    def test_array_alias_remat_emits_single_and_atomic_member_stores(self):
        source = """typedef unsigned char u8;
typedef struct { int mx; int my; } Sprite;
void F(int value) {
    Sprite bank[5];
    Sprite *sprite;
    int i;
    sprite = &bank[i];
    sprite->mx = value;
    sprite->my = 0;
    use(sprite);
}
"""
        out = self.candidates(autorules.rule_array_alias_remat, source)
        self.assertEqual(len(out), 3)
        pair = [candidate for label, candidate in out
                if "sprite->mx/my L8/9" in label]
        self.assertEqual(len(pair), 1)
        self.assertIn(
            "((Sprite *)((u8 *)(bank) + (i) * sizeof(Sprite)))->mx = value;",
            pair[0],
        )
        self.assertIn(
            "((Sprite *)((u8 *)(bank) + (i) * sizeof(Sprite)))->my = 0;",
            pair[0],
        )

    def test_array_alias_remat_accepts_stable_parameter_index(self):
        source = """typedef unsigned char u8;
typedef struct { int x; } Sprite;
void F(int i) {
    Sprite bank[2];
    Sprite *sprite;
    sprite = &bank[i];
    sprite->x = 1;
}
"""
        out = self.candidates(autorules.rule_array_alias_remat, source)
        self.assertEqual(len(out), 1)
        self.assertIn("(i) * sizeof(Sprite)", out[0][1])

    def test_array_alias_remat_rejects_unstable_or_escaped_identities(self):
        changed_index = """typedef struct { int x; } Sprite;
void F(void) {
    Sprite bank[2]; Sprite *sprite; int i;
    sprite = &bank[i];
    i++;
    sprite->x = 1;
}
"""
        escaped_alias = """typedef struct { int x; } Sprite;
void F(void) {
    Sprite bank[2]; Sprite *sprite; int i;
    take(&sprite);
    sprite = &bank[i];
    sprite->x = 1;
}
"""
        control_boundary = """typedef struct { int x; } Sprite;
void F(int flag) {
    Sprite bank[2]; Sprite *sprite; int i;
    sprite = &bank[i];
    if (flag) use(sprite);
    sprite->x = 1;
}
"""
        mismatched_type = """typedef struct { int x; } Sprite;
typedef struct { int x; } Other;
void F(void) {
    Sprite bank[2]; Other *sprite; int i;
    sprite = (Other *)&bank[i];
    sprite->x = 1;
}
"""
        volatile_parameter = """typedef struct { int x; } Sprite;
void F(volatile int i) {
    Sprite bank[2];
    Sprite *sprite;
    sprite = &bank[i];
    sprite->x = 1;
}
"""
        for source in (changed_index, escaped_alias, control_boundary,
                       mismatched_type, volatile_parameter):
            self.assertEqual(
                self.candidates(autorules.rule_array_alias_remat, source), [])

    def test_member_scalar_alias_rejects_unsafe_shapes(self):
        non_long = """typedef int s32;
void F(Node *param) {
    s32 y;
    y = param->py;
}
"""
        side_effect = """void F(void) {
    long y;
    y = next();
}
"""
        multi_decl = """void F(Node *param) {
    long y, z;
    y = param->py;
}
"""
        self.assertEqual(
            self.candidates(autorules.rule_member_scalar_alias, non_long), [])
        self.assertEqual(
            self.candidates(autorules.rule_member_scalar_alias, side_effect), [])
        self.assertEqual(
            self.candidates(autorules.rule_member_scalar_alias, multi_decl), [])

    def test_member_scalar_alias_resolves_a_later_nested_shadow(self):
        source = """typedef int s32;
void F(Node *param) {
    long z;
    z = param->pz;
    {
        s32 z;
        z = param->short_field;
    }
}
"""
        out = self.candidates(autorules.rule_member_scalar_alias, source)
        self.assertEqual(len(out), 1)
        self.assertIn("z = *(s32 *)&param->pz;", out[0][1])
        self.assertIn("z = param->short_field;", out[0][1])

    def test_member_scalar_alias_emits_atomic_adjacent_pair(self):
        source = """typedef int s32;
void F(Node *param) {
    long y;
    long z;
    y = param->py;
    z = param->pz;
}
"""
        out = self.candidates(autorules.rule_member_scalar_alias, source)
        pair = [candidate for label, candidate in out
                if "add y/z L5/6" in label]
        self.assertEqual(len(pair), 1)
        self.assertIn("y = *(s32 *)&param->py;", pair[0])
        self.assertIn("z = *(s32 *)&param->pz;", pair[0])

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

    def test_deref_address_split_names_pointer_and_scalar_identities(self):
        source = """typedef short s16;
typedef unsigned char u8;
void F(s16 *table, int index) {
    s16 type;
    type = *(s16 *)((u8 *)table + index * 2);
    use(type);
}
"""
        autorules.GUIDED_LINES = {5}
        out = self.candidates(autorules.rule_deref_address_split, source)
        self.assertEqual(len(out), 1)
        candidate = out[0][1]
        self.assertIn("s16 * _match_type_ptr;", candidate)
        self.assertIn(
            "_match_type_ptr = (s16 *)((u8 *)table + index * 2);",
            candidate,
        )
        self.assertIn("type = *_match_type_ptr;", candidate)

    def test_deref_address_split_rejects_calls_and_nonlocal_results(self):
        called = """typedef short s16;
void F(void) {
    s16 type;
    type = *(s16 *)address_for_type();
    use(type);
}
"""
        global_result = """typedef short s16;
s16 Type;
void F(char *address) {
    Type = *(s16 *)address;
}
"""
        for source in (called, global_result):
            self.assertEqual(
                self.candidates(autorules.rule_deref_address_split, source), [])

    def test_ptr_base_split_keeps_array_base_live_across_call(self):
        source = """typedef struct Conflict Conflict;
extern Conflict ConflictObject[];
void F(void) {
    int result;
    result = GetConflictResult();
    if (ConflictObject[result].common != 0) use();
}
"""
        autorules.GUIDED_LINES = {5}
        out = self.candidates(autorules.rule_ptr_base_split, source)
        self.assertEqual(len(out), 1)
        self.assertIn(
            "Conflict *_match_base = ConflictObject;\n"
            "    result = GetConflictResult();",
            out[0][1],
        )
        self.assertIn("_match_base[result].common", out[0][1])

    def test_ptr_base_split_rejects_call_after_executable_statement(self):
        source = """typedef struct Conflict Conflict;
extern Conflict ConflictObject[];
void F(void) {
    int result;
    prepare();
    result = GetConflictResult();
    if (ConflictObject[result].common != 0) use();
}
"""
        self.assertEqual(
            self.candidates(autorules.rule_ptr_base_split, source),
            [],
        )

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
        self.assertEqual(len(out), 8)
        self.assertIn("param x: s16→s32", labels)
        self.assertIn("param x: s16→u16", labels)
        self.assertIn("param x: s16→u32", labels)
        self.assertIn("param y: u32→s32", labels)
        self.assertIn("param y: u32→s16", labels)
        self.assertFalse(any("pointer" in label for label in labels))

    def test_param_width_tries_joint_narrow_unsigned_change(self):
        source = """void F(int time) {
    use(time);
}
"""
        labels = [label for label, _candidate in
                  self.candidates(autorules.rule_param_width, source)]
        self.assertIn("param time: int→u16", labels)

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

    def test_pointer_slot_volatile_wraps_one_extern_array_access(self):
        source = """typedef struct Node Node;
extern Node *Nodes[];
int F(int i) {
    return Nodes[i]->value;
}
"""
        out = self.candidates(autorules.rule_pointer_slot_volatile, source)
        self.assertEqual(len(out), 1)
        self.assertIn(
            "return (*(Node *volatile *)&Nodes[i])->value;", out[0][1])
        self.assertIn("pointer-slot-volatile Nodes[i]->value", out[0][0])

    def test_pointer_slot_volatile_requires_extern_pointer_array(self):
        source = """typedef struct Node Node;
Node *Nodes[4];
int F(int i) { return Nodes[i]->value; }
"""
        self.assertEqual(
            self.candidates(autorules.rule_pointer_slot_volatile, source), [])

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

    def test_empty_loop_boundary_does_not_wrap_either_statement(self):
        source = """int F(int value) {
    value = value + 1;
    value = value * 2;
    return value;
}
"""
        autorules.GUIDED_LINES = {2}
        out = self.candidates(autorules.rule_empty_loop_boundary, source)
        boundary = [text for label, text in out
                    if label.startswith("empty-loop-boundary L2-L3")]
        self.assertEqual(len(boundary), 1)
        self.assertIn(
            "value = value + 1;\n    do {\n    } while (0);\n"
            "    value = value * 2;",
            boundary[0],
        )

    def test_empty_loop_boundary_removes_but_does_not_stack_empty_fences(self):
        source = """void F(int value) {
    value++;
    do {
    } while (0);
    value--;
}
"""
        out = self.candidates(autorules.rule_empty_loop_boundary, source)
        self.assertEqual(len(out), 1)
        self.assertIn("empty-loop-boundary remove L3", out[0][0])
        self.assertNotIn("do {", out[0][1])

    def test_length_win_that_costs_aligned_lines_is_flagged(self):
        # (lines, length_delta): a length-mismatched draft that gets SHORTER but
        # gains differing lines is the "shortened by deleting correct code"
        # signature (ChasetoTarget's deg: short->s32 false adoption).
        note = autorules.shape_regression_note((20, 3), (26, 2))
        self.assertIn("LENGTH-WIN SHAPE REGRESSION", note)
        # A length win that also improves (or holds) the aligned lines is normal
        # length-first progress and must stay silent.
        self.assertEqual(autorules.shape_regression_note((20, 3), (14, 2)), "")
        self.assertEqual(autorules.shape_regression_note((20, 3), (20, 2)), "")
        # A real match is never second-guessed.
        self.assertEqual(
            autorules.shape_regression_note((20, 3), (26, 2), match=True), "")

    def test_fence_unwrap_removes_fence_but_keeps_body(self):
        source = """void F(int value) {
    value++;
    do {
        value = 0;
        value--;
    } while (0);
    value++;
}
"""
        out = self.candidates(autorules.rule_fence_unwrap, source)
        self.assertEqual(len(out), 1)
        self.assertIn("fence-unwrap L3", out[0][0])
        self.assertNotIn("do {", out[0][1])
        self.assertIn("value = 0;", out[0][1])
        self.assertIn("value--;", out[0][1])

    def test_fence_unwrap_ignores_empty_fences(self):
        source = """void F(int value) {
    value++;
    do {
    } while (0);
    value--;
}
"""
        self.assertEqual(
            self.candidates(autorules.rule_fence_unwrap, source), [])

    def test_working_copy_seed_merge_moves_identity_atomically(self):
        source = """typedef int s32;
void F(void) {
    s32 work;
    s32 persistent;
    short consumer;
    work = seed();
    persistent = work;
    use(persistent);
    do {
        work = persistent - 10;
    } while (0);
    work += rand() % 20;
    persistent = work;
    consumer = work;
    use(persistent, consumer);
}
"""
        autorules.GUIDED_LINES = {9, 12}
        out = self.candidates(autorules.rule_working_copy_seed_merge, source)
        self.assertEqual(len(out), 1)
        candidate = out[0][1]
        self.assertIn("persistent = seed();", candidate)
        self.assertNotIn("work = seed();", candidate)
        self.assertIn("work = persistent - 10 + rand() % 20;", candidate)
        self.assertNotIn("} while (0);\n    work +=", candidate)

    def test_working_copy_seed_merge_rejects_intervening_work_use(self):
        source = """typedef int s32;
void F(void) {
    s32 work;
    s32 persistent;
    short consumer;
    work = seed();
    persistent = work;
    observe(work);
    do {
        work = persistent - 10;
    } while (0);
    work += rand() % 20;
    persistent = work;
    consumer = work;
}
"""
        autorules.GUIDED_LINES = {9, 12}
        self.assertEqual(
            self.candidates(autorules.rule_working_copy_seed_merge, source),
            [],
        )

    def test_working_copy_seed_merge_parses_guarded_include_asm_draft(self):
        source = """#ifndef NON_MATCHING
INCLUDE_ASM("broken", F);
#else
typedef int s32;
void F(void) {
    s32 work;
    s32 persistent;
    short consumer;
    work = seed();
    persistent = work;
    use(persistent);
    do {
        work = persistent - 10;
    } while (0);
    work += rand() % 20;
    persistent = work;
    consumer = work;
}
#endif
"""
        autorules.GUIDED_LINES = {12, 15}
        out = list(autorules.rule_working_copy_seed_merge(
            source, "F", autorules.region_span(source, True)))
        self.assertEqual(len(out), 1)
        self.assertIn("persistent = seed();", out[0][1])

    def test_deferred_global_capture_moves_reload_to_join(self):
        source = """extern short Degree;
void F(void) {
    int result;
    int degree;
    degree = Degree;
    if (degree > 0) {
        result = 1;
    } else {
        if (degree < 0) {
            result = -1;
        }
        degree = Degree;
    }
    use(degree, result);
}
"""
        autorules.GUIDED_LINES = {5, 6}
        out = self.candidates(autorules.rule_deferred_global_capture, source)
        self.assertEqual(len(out), 1)
        candidate = out[0][1]
        self.assertIn("if (Degree > 0)", candidate)
        self.assertIn("if (Degree < 0)", candidate)
        self.assertEqual(candidate.count("degree = Degree;"), 1)
        self.assertLess(candidate.index("if (Degree > 0)"),
                        candidate.index("degree = Degree;"))

    def test_deferred_global_capture_rejects_call_in_decision(self):
        source = """extern short Degree;
void F(void) {
    int degree;
    degree = Degree;
    if (degree > 0) {
        mutate();
    } else {
        degree = Degree;
    }
    use(degree);
}
"""
        autorules.GUIDED_LINES = {4, 5}
        self.assertEqual(
            self.candidates(autorules.rule_deferred_global_capture, source),
            [],
        )

    def test_deferred_global_capture_pairs_later_s16_widening(self):
        source = """extern short Degree;
void F(void) {
    int result;
    int degree;
    degree = Degree;
    if (degree > 0) {
        result = 1;
    } else {
        if (degree < 0) {
            result = -1;
        }
        degree = Degree;
    }
    {
        short degree2;
        degree2 = Degree;
        use(degree2);
    }
    use(degree, result);
}
"""
        autorules.GUIDED_LINES = {5, 6}
        out = self.candidates(autorules.rule_deferred_global_capture, source)
        self.assertEqual(len(out), 2)
        combined = [candidate for label, candidate in out
                    if "+ widen degree2" in label]
        self.assertEqual(len(combined), 1)
        self.assertIn("s32 degree2;", combined[0])
        self.assertIn("if (Degree > 0)", combined[0])

    def test_redundant_field_donor_repeats_pure_aggregate_assignment(self):
        source = """typedef struct { int a, b, c, d; } T;
void F(void) {
    T target;
    T input;
    target.a = input.a - input.b;
    target.b = input.a + input.b;
    target.c = input.c - input.d;
    target.d = input.c + input.d;
    use(target);
}
"""
        autorules.GUIDED_LINES = {5}
        out = self.candidates(autorules.rule_redundant_field_donor, source)
        donor = [text for label, text in out
                 if label.startswith("redundant-field-donor target.a L5->L8")]
        self.assertEqual(len(donor), 1)
        self.assertEqual(donor[0].count("target.a = input.a - input.b;"), 2)
        self.assertIn(
            "target.c = input.c - input.d;\n"
            "    target.a = input.a - input.b;\n"
            "    target.d = input.c + input.d;",
            donor[0],
        )

    def test_redundant_field_donor_rejects_address_taken_aggregate(self):
        source = """typedef struct { int a, b; } T;
void F(void) {
    T target;
    T input;
    target.a = input.a;
    target.b = input.b;
    observe(&target);
}
"""
        self.assertEqual(
            self.candidates(autorules.rule_redundant_field_donor, source), [])

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

    def test_dead_host_split_reuses_last_dead_scalar_inside_nested_fence(self):
        source = """typedef int s32;
void use(s32 value);
void F(s32 *position) {
    s32 direction;
    s32 magnitude;
    magnitude = position[0];
    use(magnitude);
    do {
        do {
            do {
                direction = (s32)(position[1] - position[2]) / 2;
            } while (0);
        } while (0);
    } while (0);
    use(direction);
}
"""
        autorules.GUIDED_LINES = {11}
        autorules.GUIDED_VARIABLES = {b"direction", b"magnitude"}
        out = self.candidates(autorules.rule_dead_host_split, source)
        self.assertEqual(len(out), 1)
        self.assertTrue(out[0][0].startswith("dead-host-split magnitude L11"))
        self.assertIn(
            "magnitude = (s32)(position[1] - position[2]);\n"
            "                direction = magnitude / 2;",
            out[0][1],
        )

    def test_dead_host_split_accepts_signed_s32_parameters(self):
        source = """typedef int s32;
void use(s32 value);
void F(s32 left, s32 right) {
    s32 direction;
    s32 magnitude;
    magnitude = 1;
    use(magnitude);
    direction = (left - right) / 2;
    use(direction);
}
"""
        autorules.GUIDED_LINES = {8}
        autorules.GUIDED_VARIABLES = {b"direction", b"magnitude"}
        out = self.candidates(autorules.rule_dead_host_split, source)
        self.assertEqual(len(out), 1)
        self.assertIn("magnitude = left - right;", out[0][1])

    def test_dead_host_split_rejects_a_host_read_later(self):
        source = """typedef int s32;
void use(s32 value);
void F(s32 *position) {
    s32 direction;
    s32 magnitude;
    magnitude = position[0];
    direction = (s32)(position[1] - position[2]) / 2;
    use(direction);
    use(magnitude);
}
"""
        autorules.GUIDED_LINES = {7}
        autorules.GUIDED_VARIABLES = {b"direction", b"magnitude"}
        self.assertEqual(
            self.candidates(autorules.rule_dead_host_split, source), [])

    def test_dead_host_split_requires_same_32_bit_type(self):
        source = """typedef int s32;
typedef short s16;
void F(s32 *position) {
    s32 direction;
    s16 magnitude;
    magnitude = position[0];
    direction = (s32)(position[1] - position[2]) / 2;
}
"""
        autorules.GUIDED_LINES = {7}
        autorules.GUIDED_VARIABLES = {b"direction", b"magnitude"}
        self.assertEqual(
            self.candidates(autorules.rule_dead_host_split, source), [])

    def test_dead_host_split_rejects_a_repeating_loop(self):
        source = """typedef int s32;
void use(s32 value);
void F(s32 *position) {
    s32 direction;
    s32 magnitude;
    magnitude = position[0];
    while (position[3]) {
        use(magnitude);
        direction = (s32)(position[1] - position[2]) / 2;
    }
    use(direction);
}
"""
        autorules.GUIDED_LINES = {9}
        autorules.GUIDED_VARIABLES = {b"direction", b"magnitude"}
        self.assertEqual(
            self.candidates(autorules.rule_dead_host_split, source), [])

    def test_dead_host_split_rejects_unsigned_numerator_arithmetic(self):
        source = """typedef int s32;
typedef unsigned int u32;
void use(s32 value);
void F(u32 left, u32 right) {
    s32 direction;
    s32 magnitude;
    magnitude = 1;
    use(magnitude);
    direction = (left - right) / 2;
    use(direction);
}
"""
        autorules.GUIDED_LINES = {9}
        autorules.GUIDED_VARIABLES = {b"direction", b"magnitude"}
        self.assertEqual(
            self.candidates(autorules.rule_dead_host_split, source), [])

    def test_dead_host_split_rejects_backward_goto_reentry(self):
        source = """typedef int s32;
void use(s32 value);
void F(s32 left, s32 right) {
    s32 direction;
    s32 magnitude;
    magnitude = 1;
retry:
    use(magnitude);
    direction = (left - right) / 2;
    if (direction) goto retry;
    use(direction);
}
"""
        autorules.GUIDED_LINES = {9}
        autorules.GUIDED_VARIABLES = {b"direction", b"magnitude"}
        self.assertEqual(
            self.candidates(autorules.rule_dead_host_split, source), [])

    def test_dead_host_split_rejects_later_capturing_macro(self):
        source = """typedef int s32;
void use(s32 value);
#define OBSERVE_MAGNITUDE() use(magnitude)
void F(s32 left, s32 right) {
    s32 direction;
    s32 magnitude;
    magnitude = 1;
    use(magnitude);
    direction = (left - right) / 2;
    OBSERVE_MAGNITUDE();
    use(direction);
}
"""
        autorules.GUIDED_LINES = {9}
        autorules.GUIDED_VARIABLES = {b"direction", b"magnitude"}
        self.assertEqual(
            self.candidates(autorules.rule_dead_host_split, source), [])

    def test_dead_host_split_rejects_later_object_macro_capture(self):
        source = """typedef int s32;
void use(s32 value);
#define OBSERVE_MAGNITUDE use(magnitude)
void F(s32 left, s32 right) {
    s32 direction;
    s32 magnitude;
    magnitude = 1;
    use(magnitude);
    direction = (left - right) / 2;
    OBSERVE_MAGNITUDE;
    use(direction);
}
"""
        autorules.GUIDED_LINES = {9}
        autorules.GUIDED_VARIABLES = {b"direction", b"magnitude"}
        self.assertEqual(
            self.candidates(autorules.rule_dead_host_split, source), [])

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

    def test_identical_arm_fence_atomically_widens_narrow_return_carrier(self):
        source = """typedef short s16;
typedef int s32;
extern s32 Degree;
s16 F(int input) {
    s16 result;
    s32 masked;
    s32 degree;
    result = input;
    masked = result & 0xa000;
    degree = Degree;
    if (degree < 0) degree = -degree;
    return result;
}
"""
        autorules.GUIDED_LINES = {10}
        out = self.candidates(autorules.rule_identical_arm_fence, source)
        paired = [text for label, text in out
                  if label.startswith("identical-arm-fence masked L10 ") and
                  label.endswith("+ widen result")]
        self.assertEqual(len(paired), 1)
        self.assertIn("s32 result;", paired[0])
        self.assertEqual(paired[0].count("degree = Degree;"), 2)
        self.assertIn("if (masked != 0)", paired[0])

    def test_identical_arm_condition_reuses_nearby_pure_probe(self):
        source = """typedef struct { int count; int attribute; } State;
State *Motion;
State *Human;
void F(void) {
    if (Motion->count != 0) use();
    value = 0x501;
    if ((Human->attribute & 0x40) != 0) {
        flag = 1;
    } else {
        flag = 1;
    }
}
"""
        autorules.GUIDED_LINES = {7}
        out = self.candidates(autorules.rule_identical_arm_condition, source)
        probes = [text for label, text in out
                  if label.startswith("identical-arm-condition L5->L7")]
        self.assertEqual(len(probes), 1)
        self.assertIn("if (Motion->count != 0)", probes[0])
        self.assertEqual(probes[0].count("flag = 1;"), 2)

    def test_identical_arm_condition_rejects_effects_and_different_arms(self):
        effect = """void F(void) {
    if (next()) use();
    if (ready) { flag = 1; } else { flag = 1; }
}
"""
        different = """void F(void) {
    if (other) use();
    if (ready) { flag = 1; } else { flag = 2; }
}
"""
        autorules.GUIDED_LINES = {3}
        self.assertEqual(
            self.candidates(autorules.rule_identical_arm_condition, effect), [])
        self.assertEqual(
            self.candidates(autorules.rule_identical_arm_condition, different), [])

    def test_allocation_donor_fence_uses_guided_parameter_off_residual(self):
        source = """typedef struct { int x; } Node;
void F(Node *pos, int spread) {
    if (spread > 0) {
        pos->x = spread;
    } else {
        pos->x = -spread;
    }
}
"""
        # The visible register residual may map to a later source line.  The
        # allocation donor deliberately searches earlier conditional arms.
        autorules.GUIDED_LINES = {99}
        autorules.GUIDED_VARIABLES = {b"pos"}
        out = self.candidates(autorules.rule_allocation_donor_fence, source)
        candidates = [text for label, text in out
                      if label.startswith("allocation-donor-fence pos L6")]
        self.assertEqual(len(candidates), 1)
        self.assertEqual(candidates[0].count("pos->x = -spread;"), 2)
        self.assertIn("if (pos != 0)", candidates[0])

    def test_allocation_donor_fence_rejects_automatic_local(self):
        source = """void F(int input) {
    int donor;
    if (input) {
        input = donor;
    }
}
"""
        autorules.GUIDED_VARIABLES = {b"donor"}
        self.assertEqual(
            self.candidates(autorules.rule_allocation_donor_fence, source), [])

    def test_allocation_donor_fence_uses_local_read_by_enclosing_guard(self):
        source = """void F(int input) {
    int distance;
    int absolute;
    distance = input;
    if (distance < 4001) {
        absolute = input < 0 ? -input : input;
    }
}
"""
        # Reaching the body proves distance was initialized and read, so it is
        # safe to add as a zero-code discriminator even off the residual line.
        autorules.GUIDED_LINES = {99}
        autorules.GUIDED_VARIABLES = {b"distance"}
        out = self.candidates(autorules.rule_allocation_donor_fence, source)
        candidates = [text for label, text in out
                      if label.startswith(
                          "allocation-donor-fence distance L6")]
        self.assertEqual(len(candidates), 1)
        self.assertEqual(candidates[0].count(
            "absolute = input < 0 ? -input : input;"), 2)
        self.assertIn("if (distance != 0)", candidates[0])

    def test_allocation_donor_fence_rejects_unevaluated_guard_local(self):
        source = """void F(int input) {
    int donor;
    if (0 && donor) {
        input = 1;
    }
}
"""
        autorules.GUIDED_VARIABLES = {b"donor"}
        self.assertEqual(
            self.candidates(autorules.rule_allocation_donor_fence, source), [])

    def test_allocation_donor_fence_rejects_address_only_guard_local(self):
        source = """void F(int input) {
    int donor;
    if (&donor) {
        input = 1;
    }
}
"""
        autorules.GUIDED_VARIABLES = {b"donor"}
        self.assertEqual(
            self.candidates(autorules.rule_allocation_donor_fence, source), [])

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

    def test_guard_flag_assignment_moves_both_directions(self):
        hoisted = """int F(int reject) {
    short done;
    done = 1;
    if (reject) {
        goto finish;
    }
    consume();
finish:
    return done;
}
"""
        sunk = self.candidates(autorules.rule_guard_flag_assign, hoisted)
        self.assertEqual(len(sunk), 1)
        candidate = sunk[0][1]
        self.assertEqual(candidate.count("done = 1;"), 2)
        self.assertIn("done = 1;\n        goto finish;", candidate)
        self.assertLess(candidate.rindex("done = 1;"), candidate.index("consume();"))

        restored = self.candidates(autorules.rule_guard_flag_assign, candidate)
        self.assertEqual(len(restored), 1)
        restored_text = restored[0][1]
        self.assertEqual(restored_text.count("done = 1;"), 1)
        self.assertLess(restored_text.index("done = 1;"),
                        restored_text.index("if (reject)"))

    def test_guard_flag_assignment_accepts_true_and_rejects_observation(self):
        boolean = """int F(int reject) {
    short done;
    done = true;
    if (reject) { goto finish; }
finish:
    return done;
}
"""
        self.assertEqual(
            len(self.candidates(autorules.rule_guard_flag_assign, boolean)), 1)

        observed = boolean.replace("if (reject)", "if (done && reject)")
        self.assertEqual(
            self.candidates(autorules.rule_guard_flag_assign, observed), [])

    def test_guard_flag_assignment_accepts_actdamage_shape(self):
        source = """int get_weapon_kind(void);
void use_weapon(void);
int F(void) {
    short done;
    short weapon_kind;
    done = false;
    weapon_kind = get_weapon_kind();
    done = true;
    if (weapon_kind != 0x2a) {
        goto check_done;
    }
    use_weapon();
check_done:
    return done;
}
"""
        candidates = self.candidates(autorules.rule_guard_flag_assign, source)
        self.assertEqual(len(candidates), 1)
        self.assertIn("done = true;\n        goto check_done;",
                      candidates[0][1])

    def test_guard_flag_assignment_rejects_macro_or_unknown_condition(self):
        object_macro = """#define REJECT (done && reject)
int F(int reject) {
    short done;
    done = true;
    if (REJECT) { goto finish; }
finish:
    return done;
}
"""
        unknown_object = object_macro.replace(
            "#define REJECT (done && reject)\n", "")
        unknown_call = unknown_object.replace(
            "if (REJECT)", "if (REJECT(reject))")
        for source in (object_macro, unknown_object, unknown_call):
            self.assertEqual(
                self.candidates(autorules.rule_guard_flag_assign, source), [])

    def test_guard_flag_assignment_rejects_hidden_macro_capture(self):
        argument_capture = """#define CAPTURE(x) remember(&(x))
int F(int reject) {
    short done;
    CAPTURE(done);
    done = true;
    if (reject) { goto finish; }
finish:
    return done;
}
"""
        implicit_capture = """#define CAPTURE() remember(&done)
int F(int reject) {
    short done;
    CAPTURE();
    done = true;
    if (reject) { goto finish; }
finish:
    return done;
}
"""
        unknown_capture = argument_capture.replace(
            "#define CAPTURE(x) remember(&(x))\n", "")
        for source in (argument_capture, implicit_capture, unknown_capture):
            self.assertEqual(
                self.candidates(autorules.rule_guard_flag_assign, source), [])

    def test_guard_flag_assignment_rejects_parenthesized_address_alias(self):
        source = """int F(void) {
    short done;
    short *alias = &(/* keep alias visible */ done);
    done = true;
    if (*alias) { goto finish; }
finish:
    return done;
}
"""
        self.assertEqual(
            self.candidates(autorules.rule_guard_flag_assign, source), [])

    def test_flag_assignments_move_to_prezero_form(self):
        source = """int F(int value) {
    int flag;
    if (value < 0) {
        value = -value;
        flag = 1;
    } else {
        flag = 0;
    }
    return value + flag;
}
"""
        out = self.candidates(autorules.rule_flag_arm_assign, source)
        self.assertEqual(len(out), 1)
        label, candidate = out[0]
        self.assertIn("flag-arm-prezero", label)
        self.assertIn("flag = 0;\n    if (value < 0)", candidate)
        self.assertIn("value = -value;\n        flag = 1;", candidate)
        self.assertNotIn("else", candidate)

    def test_flag_prezero_rejects_observation_or_early_exit(self):
        observed = """int F(int value) {
    int flag;
    if (value < 0) { flag += value; flag = 1; }
    else { flag = 0; }
    return flag;
}
"""
        exits = """int F(int value) {
    int flag;
    if (value < 0) { if (value < -3) return 2; flag = 1; }
    else { flag = 0; }
    return flag;
}
"""
        self.assertEqual(self.candidates(autorules.rule_flag_arm_assign,
                                         observed), [])
        self.assertEqual(self.candidates(autorules.rule_flag_arm_assign,
                                         exits), [])

    def test_flag_prezero_rejects_shadowed_binding(self):
        source = """int F(int value) {
    int flag;
    if (value < 0) {
        int flag;
        value = -value;
        flag = 1;
    } else {
        flag = 0;
    }
    return flag;
}
"""
        self.assertEqual(self.candidates(autorules.rule_flag_arm_assign,
                                         source), [])

    def test_flag_prezero_rejects_parenthesized_address_alias(self):
        source = """int F(int value) {
    int flag;
    int *alias;
    alias = &(/* alias */ flag);
    if (*alias != 0) {
        value = -value;
        flag = 1;
    } else {
        flag = 0;
    }
    return value + flag;
}
"""
        self.assertEqual(self.candidates(autorules.rule_flag_arm_assign,
                                         source), [])

    def test_flag_prezero_rejects_object_and_function_macro_reads(self):
        object_macro = """#define READ_FLAG flag
int F(int value) {
    int flag;
    if (READ_FLAG < value) { value = -value; flag = 1; }
    else { flag = 0; }
    return value + flag;
}
"""
        function_macro = """#define READ_FLAG() flag
int F(int value) {
    int flag;
    if (value < 0) { value += READ_FLAG(); flag = 1; }
    else { flag = 0; }
    return value + flag;
}
"""
        unknown_object = object_macro.replace("#define READ_FLAG flag\n", "")
        for source in (object_macro, function_macro, unknown_object):
            self.assertEqual(self.candidates(autorules.rule_flag_arm_assign,
                                             source), [])

    def test_flag_prezero_rejects_call_bearing_condition_or_prefix(self):
        condition = """int observe(int value);
int F(int value) {
    int flag;
    if (observe(value)) { value = -value; flag = 1; }
    else { flag = 0; }
    return value + flag;
}
"""
        prefix = """int observe(int value);
int F(int value) {
    int flag;
    if (value < 0) { observe(value); flag = 1; }
    else { flag = 0; }
    return value + flag;
}
"""
        for source in (condition, prefix):
            self.assertEqual(self.candidates(autorules.rule_flag_arm_assign,
                                             source), [])

    def test_default_ladder_hoist_moves_literal_default_before_comparisons(self):
        source = """int F(int turn, int degree) {
    int result;
    if (turn < degree) {
        result = 0x2000;
    } else {
        result = 0;
        if (degree < -turn) {
            result = -0x8000;
        }
    }
    return result;
}
"""
        out = self.candidates(autorules.rule_default_ladder_hoist, source)
        self.assertEqual(len(out), 1)
        candidate = out[0][1]
        self.assertIn("result = 0;\n    if (turn < degree)", candidate)
        self.assertIn("else if (degree < -turn)", candidate)
        self.assertEqual(candidate.count("result = 0;"), 1)

    def test_default_ladder_hoist_rejects_work_or_observed_default(self):
        work = """int F(int turn, int degree) {
    int result;
    if (turn < degree) { result = 1; }
    else { result = 0; observe(); if (degree < -turn) { result = 2; } }
    return result;
}
"""
        observed = """int F(int turn, int degree) {
    int result;
    if (result < degree) { result = 1; }
    else { result = 0; if (degree < -turn) { result = 2; } }
    return result;
}
"""
        for source in (work, observed):
            self.assertEqual(
                self.candidates(autorules.rule_default_ladder_hoist, source), [])

    def test_flag_return_split_moves_override_to_success_exit(self):
        source = """typedef int s32;
int helper(void);
s32 F(int ready) {
    s32 result;
    result = 0;
    if (ready) {
        helper();
        result = 1;
        helper();
    }
    return result;
}
"""
        out = self.candidates(autorules.rule_flag_return_split, source)
        self.assertEqual(len(out), 1)
        candidate = out[0][1]
        self.assertNotIn("s32 result;", candidate)
        self.assertNotIn("result = 0;", candidate)
        self.assertNotIn("result = 1;", candidate)
        self.assertIn("helper();\n        return 1;\n    }\n    return 0;", candidate)

    def test_flag_return_split_rejects_observation_or_early_exit(self):
        observed = """typedef int s32;
s32 F(int ready) {
    s32 result;
    result = 0;
    if (ready) { result = 1; ready += result; }
    return result;
}
"""
        exits = """typedef int s32;
s32 F(int ready) {
    s32 result;
    result = 0;
    if (ready) { result = 1; if (ready > 2) return 3; }
    return result;
}
"""
        self.assertEqual(
            self.candidates(autorules.rule_flag_return_split, observed), [])
        self.assertEqual(
            self.candidates(autorules.rule_flag_return_split, exits), [])

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

    def test_guard_exit_copy_moves_both_directions(self):
        arm_local = """int F(int pad) {
    int exit_pad;
    while (pad) {
        if (pad & 0x100) {
            exit_pad = pad;
            break;
        }
        pad--;
    }
    return exit_pad;
}
"""
        hoisted = self.candidates(autorules.rule_guard_exit_copy, arm_local)
        self.assertEqual(len(hoisted), 1)
        candidate = hoisted[0][1]
        self.assertLess(candidate.index("exit_pad = pad;"),
                        candidate.index("if (pad & 0x100)"))

        sunk = self.candidates(autorules.rule_guard_exit_copy, candidate)
        self.assertEqual(len(sunk), 1)
        restored = sunk[0][1]
        self.assertLess(restored.index("if (pad & 0x100)"),
                        restored.index("exit_pad = pad;"))

    def test_guard_exit_copy_rejects_fallthrough_use_or_second_break(self):
        fallthrough_use = """int F(int pad) {
    int exit_pad;
    while (pad) {
        if (pad & 1) { exit_pad = pad; break; }
        pad += exit_pad;
    }
    return exit_pad;
}
"""
        second_break = """int F(int pad) {
    int exit_pad;
    while (pad) {
        if (pad & 1) { exit_pad = pad; break; }
        if (pad & 2) break;
        pad--;
    }
    return exit_pad;
}
"""
        self.assertEqual(
            self.candidates(autorules.rule_guard_exit_copy, fallthrough_use), [])
        self.assertEqual(
            self.candidates(autorules.rule_guard_exit_copy, second_break), [])

    def test_guard_exit_copy_checks_forward_goto_path(self):
        safe = """int F(int pad) {
    int exit_pad;
    while (pad) {
        if (pad & 1) { exit_pad = pad; break; }
        if (pad & 2) goto alternate;
        pad--;
    }
    if (exit_pad & 4) {
alternate:
        pad++;
    }
    return pad;
}
"""
        self.assertEqual(
            len(self.candidates(autorules.rule_guard_exit_copy, safe)), 1)
        unsafe = safe.replace("pad++;", "pad += exit_pad;")
        self.assertEqual(
            self.candidates(autorules.rule_guard_exit_copy, unsafe), [])

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

    def test_shared_writeback_compound_collapses_all_edges_and_pairs_inversion(self):
        source = """typedef struct { short vy; } Rotation;
Rotation *GlobalRotation;
void F(int reverse, int turn) {
    Rotation *rotation;
    int current;
    int result;
    if (reverse) {
        rotation = GlobalRotation;
        current = (unsigned short)rotation->vy;
        result = current - turn;
        goto writeback;
    } else {
        rotation = GlobalRotation;
        current = (unsigned short)rotation->vy;
        result = turn + current;
        goto writeback;
    }
writeback:
    rotation->vy = result;
    goto done;
done:
    use();
}
"""
        out = self.candidates(autorules.rule_shared_writeback_compound, source)
        self.assertEqual(len(out), 2)
        collapsed = next(candidate for label, candidate in out
                         if "if-else-invert" not in label)
        self.assertIn("GlobalRotation->vy -= turn;\n        goto done;", collapsed)
        self.assertIn("GlobalRotation->vy += turn;\n        goto done;", collapsed)
        self.assertNotIn("writeback:", collapsed)
        inverted = next(candidate for label, candidate in out
                        if "if-else-invert" in label)
        self.assertIn("if (!(reverse))", inverted)
        self.assertLess(inverted.index("GlobalRotation->vy += turn"),
                        inverted.index("GlobalRotation->vy -= turn"))

    def test_shared_writeback_compound_rejects_incomplete_or_impure_edges(self):
        one_edge = """void F(int turn) {
    Rotation *rotation;
    int current;
    int result;
    rotation = GlobalRotation;
    current = rotation->vy;
    result = current + turn;
    goto writeback;
writeback:
    rotation->vy = result;
    return;
}
"""
        impure_base = """void F(int select, int turn) {
    Rotation *rotation;
    int current;
    int result;
    if (select) {
        rotation = get_rotation();
        current = rotation->vy;
        result = current + turn;
        goto writeback;
    } else {
        rotation = get_rotation();
        current = rotation->vy;
        result = current - turn;
        goto writeback;
    }
writeback:
    rotation->vy = result;
    return;
}
"""
        for source in (one_edge, impure_base):
            self.assertEqual(
                self.candidates(autorules.rule_shared_writeback_compound, source), [])

    def test_shared_terminal_tail_completes_both_arms(self):
        source = """int F(int select) {
    int motion;
    int done;
    if (select) {
        motion = 0x501;
    } else {
        motion = 0;
    }
    done = 1;
    return motion;
}
"""
        autorules.GUIDED_LINES = {9}
        out = self.candidates(autorules.rule_shared_terminal_tail, source)
        self.assertEqual(len(out), 1)
        candidate = out[0][1]
        self.assertEqual(candidate.count("done = 1;"), 2)
        self.assertEqual(candidate.count("return motion;"), 2)
        self.assertIn(
            "motion = 0x501;\n        done = 1;\n        return motion;",
            candidate,
        )
        self.assertIn(
            "motion = 0;\n        done = 1;\n        return motion;",
            candidate,
        )

    def test_shared_terminal_tail_rejects_shadowing_or_comments(self):
        shadowed = """int F(int select) {
    int value;
    if (select) {
        int done;
        value = done;
    } else {
        value = 0;
    }
    done = 1;
    return value;
}
"""
        commented = """int F(int select) {
    int value;
    if (select) { value = 1; } else { value = 0; }
    done = 1;
    /* terminal island */
    return value;
}
"""
        for source in (shadowed, commented):
            self.assertEqual(
                self.candidates(autorules.rule_shared_terminal_tail, source), [])

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

    def test_shared_result_return_hoists_and_funnels_equal_width_local(self):
        source = """typedef short s16;
typedef unsigned short u16;
s16 F(int fast) {
    int degree;
    if (fast) {
        u16 result;
        result = helper();
        update();
        return result;
    }
    return fallback();
}
"""
        # The residual can map only to the final conversion/return; that must
        # still admit the distant nested return and declaration.
        autorules.GUIDED_LINES = {11}
        out = self.candidates(autorules.rule_shared_result_return, source)
        self.assertEqual(len(out), 1)
        candidate = out[0][1]
        self.assertIn("int degree;\n    u16 result;", candidate)
        self.assertEqual(candidate.count("u16 result;"), 1)
        self.assertIn("update();\n        goto _match_return_result;", candidate)
        self.assertIn("result = fallback();\n_match_return_result:\n"
                      "    return result;", candidate)

    def test_shared_result_return_rejects_width_scope_and_extra_return_risks(self):
        width = """typedef short s16;
typedef int s32;
s16 F(int fast) {
    if (fast) { s32 result; result = helper(); return result; }
    return fallback();
}
"""
        captured = """typedef short s16;
typedef unsigned short u16;
s16 F(int fast) {
    use(result);
    if (fast) { u16 result; result = helper(); return result; }
    return fallback();
}
"""
        extra = """typedef short s16;
typedef unsigned short u16;
s16 F(int fast) {
    if (fast) { u16 result; result = helper(); return result; }
    if (fallback_ready()) return 2;
    return fallback();
}
"""
        for source in (width, captured, extra):
            self.assertEqual(
                self.candidates(autorules.rule_shared_result_return, source), [])

    def test_terminal_call_return_splits_one_terminal_branch(self):
        source = """typedef short s16;
typedef unsigned short u16;
s16 F(int status) {
    u16 result;
    if (status == 7) {
        clear();
        result = succession();
    } else if (status != 0) {
        result = indirect();
    } else {
        result = fallback();
    }
    return result;
}
"""
        autorules.GUIDED_LINES = {13}
        out = self.candidates(autorules.rule_terminal_call_return, source)
        self.assertEqual(len(out), 3)
        candidate = next(text for label, text in out if "L7" in label)
        self.assertIn("clear();\n        return succession();", candidate)
        self.assertIn("result = indirect();", candidate)

    def test_terminal_call_return_rejects_work_after_assignment(self):
        source = """int F(int status) {
    int result;
    if (status) { result = helper(); observe(); }
    else { result = fallback(); }
    return result;
}
"""
        out = self.candidates(autorules.rule_terminal_call_return, source)
        self.assertEqual(len(out), 1)
        self.assertNotIn("return helper();", out[0][1])

    def test_difference_role_fuse_builds_direct_subtraction_identities(self):
        source = """typedef struct Node { int chase_x; int chase_z; int x; int z; } Node;
int F(Node *p, Node *locate) {
    int dx, dz, scratch;
    dx = p->chase_x;
    scratch = locate->x;
    dz = p->chase_z;
    dx = dx - scratch;
    dz = dz - locate->z;
    scratch = dx < 0 ? -dx : dx;
    return use(dx, dz, scratch);
}
"""
        out = self.candidates(autorules.rule_difference_role_fuse, source)
        self.assertEqual(len(out), 1)
        candidate = out[0][1]
        self.assertIn("dx = p->chase_x - locate->x;", candidate)
        self.assertIn("dz = p->chase_z - locate->z;", candidate)
        self.assertNotIn("scratch = locate->x;", candidate)

    def test_difference_role_fuse_rejects_scratch_read_before_overwrite(self):
        source = """typedef struct Node { int a; int b; int c; int d; } Node;
int F(Node *p) {
    int x, z, scratch;
    x = p->a;
    scratch = p->b;
    z = p->c;
    x = x - scratch;
    z = z - p->d;
    use(scratch);
    return x + z;
}
"""
        self.assertEqual(
            self.candidates(autorules.rule_difference_role_fuse, source), [])

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

    def test_clamp_shared_return_funnels_literal_bounds_through_local(self):
        source = """typedef int s32;
s32 next(void);
s32 F(void) {
    s32 angle;
    angle = next();
    if (angle > 0x155)
        return 0x155;
    if (angle < -0x155)
        return -0x155;
    return angle;
}
"""
        out = self.candidates(autorules.rule_clamp_shared_return, source)
        self.assertEqual(len(out), 1)
        candidate = out[0][1]
        self.assertIn("angle = 0x155;", candidate)
        self.assertIn("else if (angle < -0x155)", candidate)
        self.assertIn("angle = -0x155;", candidate)
        self.assertEqual(candidate.count("return angle;"), 1)

    def test_clamp_shared_return_rejects_narrow_or_nonlocal_result(self):
        narrow = """typedef short s16;
int F(void) {
    s16 angle;
    if (angle > 10) return 10;
    if (angle < -10) return -10;
    return angle;
}
"""
        parameter = """int F(int angle) {
    if (angle > 10) return 10;
    if (angle < -10) return -10;
    return angle;
}
"""
        self.assertEqual(
            self.candidates(autorules.rule_clamp_shared_return, narrow), [])
        self.assertEqual(
            self.candidates(autorules.rule_clamp_shared_return, parameter), [])

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

    def test_mod_bias_temp_emits_block_and_shared_typed_candidates(self):
        source = """typedef unsigned int u32;
void F(void) {
    u32 delta_y;
    u32 delta_x;
    int view_y;
    int view_x;
    int y;
    int x;
    if (delta_y) {
        y = view_y + (delta_y % 6000 - 3000);
    }
    if (delta_x) {
        x = view_x + (delta_x % 6000 - 3000);
    }
}
"""
        out = self.candidates(autorules.rule_mod_bias_temp, source)
        self.assertEqual(len(out), 3)
        labels = [candidate[0] for candidate in out]
        self.assertTrue(any("block-temp y" in label for label in labels))
        self.assertTrue(any("block-temp x" in label for label in labels))
        shared = next(candidate for label, candidate in out
                      if "shared-temp x2" in label)
        self.assertEqual(shared.count("u32 _match_mod_offset;"), 1)
        self.assertEqual(shared.count("_match_mod_offset ="), 2)
        self.assertIn("y = view_y + _match_mod_offset;", shared)
        self.assertIn("x = view_x + _match_mod_offset;", shared)

    def test_mod_bias_temp_rejects_calls_and_qualified_inputs(self):
        call = """typedef unsigned int u32;
u32 next(void);
void F(void) {
    u32 delta;
    int x;
    x = next() + (delta % 10 - 5);
}
"""
        qualified = """typedef unsigned int u32;
void F(void) {
    volatile u32 delta;
    int base;
    int x;
    x = base + (delta % 10 - 5);
}
"""
        self.assertEqual(
            self.candidates(autorules.rule_mod_bias_temp, call), [])
        self.assertEqual(
            self.candidates(autorules.rule_mod_bias_temp, qualified), [])

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
    def test_candidate_extent_comes_from_link_map_not_first_return(self):
        candidate = [
            (0x80001000, "jr ra"),
            (0x80001004, "nop"),
            (0x80001008, "addiu v0,v0,1"),
            (0x8000100C, "jr ra"),
            (0x80001010, "nop"),
        ]
        with mock.patch.object(matchdiff, "linked_text_size", return_value=20), \
                mock.patch.object(asmdiff, "dis", return_value=candidate) as dis:
            result = asmdiff.candidate_disassembly("F", 0x80001000, 16)

        self.assertEqual(result, candidate)
        dis.assert_called_once_with(asmdiff.OURS, 0x80001000, 20)

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

    def reused_build_fixture(self, source, processed):
        temporary = tempfile.TemporaryDirectory()
        root = temporary.name
        source_path = os.path.join(root, "src", "main.exe", "F.c")
        processed_path = os.path.join(
            root, ".shake", "processed", "main.exe", "F.c")
        linked_path = os.path.join(
            root, ".shake", "build", "tenchu", "main.exe")
        for path in (source_path, processed_path, linked_path):
            os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(source_path, "w") as stream:
            stream.write(source)
        with open(processed_path, "w") as stream:
            stream.write(processed)
        with open(linked_path, "wb") as stream:
            stream.write(b"linked")
        return temporary, root

    def test_no_build_rejects_default_nonmatching_stub(self):
        temporary, root = self.reused_build_fixture(
            "#ifndef NON_MATCHING\nvoid F(void) {}\n#endif\n",
            '# 0 "src/main.exe/F.c"\nasm/nonmatchings/F/F.s\n',
        )
        with temporary, mock.patch.object(rtlguide, "ROOT", root):
            with self.assertRaisesRegex(RuntimeError, "INCLUDE_ASM stub"):
                rtlguide.validate_reused_build("F")

    def test_no_build_rejects_staged_candidate_provenance(self):
        temporary, root = self.reused_build_fixture(
            "void F(void) {}\n",
            '# 0 "/tmp/autorules-F/F.c"\nvoid F(void) {}\n',
        )
        with temporary, mock.patch.object(rtlguide, "ROOT", root):
            with self.assertRaisesRegex(RuntimeError, "staged/foreign"):
                rtlguide.validate_reused_build("F")

    def test_no_build_accepts_current_processed_draft(self):
        temporary, root = self.reused_build_fixture(
            "#ifndef NON_MATCHING\nvoid F(void) {}\n#endif\n",
            '# 0 "src/main.exe/F.c"\nvoid F(void) {}\n',
        )
        with temporary, mock.patch.object(rtlguide, "ROOT", root):
            rtlguide.validate_reused_build("F")

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
                "source_scope_hint": "repeat-block-local-pointer",
            }],
        )

    def test_stack_address_retention_hint_needs_strong_gap_for_scope_split(self):
        target = [
            (0x1000, "addiu a1,sp,352"),
            (0x1010, "addiu a0,sp,352"),
        ]
        candidate = [(0x1000, "addiu s2,sp,352")]
        self.assertEqual(
            rtlguide.stack_address_rematerialization_hints(target, candidate),
            [{
                "offset": 352,
                "target_sites": 2,
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

    def test_stack_address_hint_prioritizes_alias_remat_rule(self):
        target = [
            (0x1000, "addiu a0,sp,24"),
            (0x1004, "sw v0,0(a0)"),
            (0x1008, "addiu a1,sp,24"),
            (0x100c, "sw v1,0(a1)"),
            (0x1010, "jr ra"),
            (0x1014, "nop"),
        ]
        candidate = [
            (0x1000, "addiu s0,sp,24"),
            (0x1004, "sw v0,0(s0)"),
            (0x1008, "sw v1,4(s0)"),
            (0x100c, "jr ra"),
            (0x1010, "nop"),
        ]
        with mock.patch.object(
                rtlguide, "_candidate_asm",
                return_value=(0x1000, 24, 20, target, candidate)):
            guide = rtlguide.assembly_guide("F")
        self.assertTrue(guide["stack_address_rematerializations"])
        self.assertEqual(guide["rules"][0], "array-alias-remat")

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

    def test_known_narrow_copy_zero_extension_signature(self):
        h = self.hunk(["move v0,s0"], ["andi v0,s0,0xffff"])
        self.assertEqual(
            rtlguide.known_residual_signatures([h]),
            ["narrow-copy-zero-extension"],
        )

    def test_known_branch_phi_register_tie_signature_and_rule_priority(self):
        target = [
            (0x1000, "bnez v1,0x1014"),
            (0x1004, "li v0,1"),
            (0x1008, "beqz v1,0x1010"),
            (0x100c, "li v0,2"),
            (0x1010, "li v0,1"),
            (0x1014, "sb v0,0(a0)"),
        ]
        candidate = [
            (0x1000, "bnez v1,0x1014"),
            (0x1004, "li a1,1"),
            (0x1008, "beqz v1,0x1014"),
            (0x100c, "li a1,2"),
            (0x1010, "li a1,1"),
            (0x1014, "sb a1,0(a0)"),
        ]
        hunks = [
            self.hunk(["li v0,1"], ["li a1,1"]),
            self.hunk(["li v0,2", "li v0,1"],
                      ["li a1,2", "li a1,1"]),
            self.hunk(["sb v0,0(a0)"], ["sb a1,0(a0)"]),
        ]
        self.assertEqual(
            rtlguide.known_residual_signatures(hunks, target, candidate),
            ["branch-phi-register-tie"],
        )
        with mock.patch.object(
                rtlguide, "_candidate_asm",
                return_value=(0x1000, 24, 24, target, candidate)):
            guide = rtlguide.assembly_guide("F")
        self.assertIn("branch-phi-register-tie",
                      guide["known_residual_signatures"])
        self.assertEqual(
            guide["rules"][:4],
            ["shared-tail-assign", "flag-arm-assign", "guard-flag-assign",
             "type-width"],
        )

    def test_branch_phi_signature_requires_distinct_arm_literals(self):
        target = [
            (0x1000, "bnez v1,0x1010"),
            (0x1004, "li v0,1"),
            (0x1008, "beqz v1,0x1010"),
            (0x100c, "li v0,1"),
            (0x1010, "sb v0,0(a0)"),
        ]
        candidate = [
            (0x1000, "bnez v1,0x1010"),
            (0x1004, "li a1,1"),
            (0x1008, "beqz v1,0x1010"),
            (0x100c, "li a1,1"),
            (0x1010, "sb a1,0(a0)"),
        ]
        hunks = [self.hunk(
            ["li v0,1", "li v0,1", "sb v0,0(a0)"],
            ["li a1,1", "li a1,1", "sb a1,0(a0)"],
        )]
        self.assertEqual(
            rtlguide.known_residual_signatures(hunks, target, candidate), [])

    def test_adjacent_load_order_prioritizes_comparison_swap(self):
        target = [
            (0x1000, "lh v0,88(fp)"),
            (0x1004, "lh v1,30(a2)"),
            (0x1008, "slt v0,v1,v0"),
            (0x100c, "jr ra"),
        ]
        candidate = [
            (0x1000, "lh v1,30(a2)"),
            (0x1004, "lh v0,88(fp)"),
            (0x1008, "slt v0,v1,v0"),
            (0x100c, "jr ra"),
        ]
        with mock.patch.object(
                rtlguide, "_candidate_asm",
                return_value=(0x1000, 16, 16, target, candidate)):
            guide = rtlguide.assembly_guide("F")
        self.assertIn("adjacent-independent-load-order",
                      guide["known_residual_signatures"])
        self.assertEqual(guide["rules"][0], "cmp-swap")

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

    def test_known_dbr_duplicated_literal_producer_signature(self):
        target = [
            (0x1000, "beqz v0,0x1020"),
            (0x1004, "nop"),
            (0x1008, "beqz a0,0x1020"),
            (0x100c, "nop"),
            (0x1010, "lw a0,2140(gp)"),
            (0x1014, "li v1,32767"),
            (0x1018, "sh v1,24(a0)"),
            (0x101c, "jr ra"),
            (0x1020, "nop"),
        ]
        ours = [
            (0x1000, "beqz v0,0x1020"),
            (0x1004, "li v0,32767"),
            (0x1008, "beqz a0,0x1020"),
            (0x100c, "li v0,32767"),
            (0x1010, "li v0,32767"),
            (0x1014, "lw a0,2140(gp)"),
            (0x1018, "sh v0,24(a0)"),
            (0x101c, "jr ra"),
            (0x1020, "nop"),
        ]
        self.assertEqual(
            rtlguide.known_residual_signatures([], target, ours),
            ["dbr-duplicated-literal-producer"],
        )

    def test_dbr_literal_signature_requires_multiple_nop_delay_sites(self):
        target = [
            (0x1000, "beqz v0,0x1010"),
            (0x1004, "nop"),
            (0x1008, "li v0,32767"),
            (0x100c, "jr ra"),
        ]
        ours = [
            (0x1000, "beqz v0,0x1010"),
            (0x1004, "li v0,32767"),
            (0x1008, "li v0,32767"),
            (0x100c, "jr ra"),
        ]
        self.assertEqual(
            rtlguide.known_residual_signatures([], target, ours), [])

    def test_dbr_literal_signature_prioritizes_loop_range(self):
        target = [
            (0x1000, "beqz v0,0x1020"),
            (0x1004, "nop"),
            (0x1008, "beqz a0,0x1020"),
            (0x100c, "nop"),
            (0x1010, "lw a0,2140(gp)"),
            (0x1014, "li v1,32767"),
            (0x1018, "sh v1,24(a0)"),
            (0x101c, "jr ra"),
            (0x1020, "nop"),
        ]
        ours = [
            (0x1000, "beqz v0,0x1020"),
            (0x1004, "li v0,32767"),
            (0x1008, "beqz a0,0x1020"),
            (0x100c, "li v0,32767"),
            (0x1010, "li v0,32767"),
            (0x1014, "lw a0,2140(gp)"),
            (0x1018, "sh v0,24(a0)"),
            (0x101c, "jr ra"),
            (0x1020, "nop"),
        ]
        with mock.patch.object(
                rtlguide, "_candidate_asm",
                return_value=(0x1000, 36, 36, target, ours)):
            guide = rtlguide.assembly_guide("F")
        self.assertIn("dbr-duplicated-literal-producer",
                      guide["known_residual_signatures"])
        self.assertEqual(guide["rules"][0], "loop-range")

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

    def test_commutative_plus_destination_prioritizes_initialized_global(self):
        target = [(0x1000, "addu a0,a0,v1"), (0x1004, "sw a0,0(gp)")]
        ours = [(0x1000, "addu v1,v1,a0"), (0x1004, "sw v1,0(gp)")]
        with mock.patch.object(
                rtlguide, "_candidate_asm",
                return_value=(0x1000, 8, 8, target, ours)):
            guide = rtlguide.assembly_guide("F")
        self.assertEqual(guide["rules"][0], "initialized-global-compound")

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

    def test_guard_return_signature_prioritizes_terminal_flip(self):
        target = [(0x1000, "bnez v0,0x1040")]
        ours = [
            (0x1000, "beqz v0,0x1010"),
            (0x1004, "nop"),
            (0x1008, "j 0x1044"),
        ]
        with mock.patch.object(
                rtlguide, "_candidate_asm",
                return_value=(0x1000, 4, 12, target, ours)):
            guide = rtlguide.assembly_guide("F")
        self.assertIn("guard-return-island-layout",
                      guide["known_residual_signatures"])
        self.assertEqual(guide["rules"][0], "terminal-guard-flip")

    def test_terminal_arm_layout_signature_prioritizes_arm_flip(self):
        target = [
            (0x1000, "beqz v0,0x1010"),
            (0x1004, "li a1,60"),
            (0x1008, "lw a0,4(gp)"),
            (0x100c, "j 0x1040"),
        ]
        ours = [
            (0x1000, "bnez v0,0x1020"),
            (0x1004, "lw a0,4(gp)"),
            (0x1008, "j 0x1040"),
            (0x100c, "li a1,60"),
        ]
        self.assertEqual(
            rtlguide.known_residual_signatures([], target, ours),
            ["terminal-arm-layout-flip"],
        )
        with mock.patch.object(
                rtlguide, "_candidate_asm",
                return_value=(0x1000, 16, 16, target, ours)):
            guide = rtlguide.assembly_guide("F")
        self.assertEqual(guide["rules"][:2],
                         ["terminal-arm-flip", "if-else-invert"])

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

    def test_known_arithmetic_working_copy_prioritizes_paired_rule(self):
        h = self.hunk(
            ["addu s0,s0,v0", "move s2,s0", "sll s0,s0,0x10"],
            ["addu s2,s0,v0", "sll s0,s2,0x10"],
        )
        self.assertEqual(
            rtlguide.known_residual_signatures([h]),
            ["arithmetic-working-copy"],
        )
        target = [(0x1000 + index * 4, insn)
                  for index, insn in enumerate(
                      ["addu s0,s0,v0", "move s2,s0", "sll s0,s0,0x10"])]
        ours = [(0x1000 + index * 4, insn)
                for index, insn in enumerate(
                    ["addu s2,s0,v0", "sll s0,s2,0x10"])]
        with mock.patch.object(
                rtlguide, "_candidate_asm",
                return_value=(0x1000, 12, 8, target, ours)):
            guide = rtlguide.assembly_guide("F")
        self.assertEqual(guide["rules"][0], "working-copy-seed-merge")

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
        self.assertIn("empty-loop-boundary", rules)
        self.assertIn("redundant-field-donor", rules)
        self.assertIn("loop-fence", rules)
        self.assertIn("loop-range", rules)
        self.assertIn("shift16-mul", rules)
        self.assertIn("plus-group", rules)
        self.assertIn("type-width", rules)
        self.assertIn("guard-flag-assign", rules)
        self.assertIn("shared-result-return", rules)
        self.assertIn("array-alias-remat", rules)
        self.assertIn("member-scalar-alias", rules)
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

    def test_asmdiff_rejects_stale_nonmatching_artifact(self):
        with mock.patch.object(matchdiff, "linked_nonmatching_stub",
                               return_value=True):
            error = asmdiff.candidate_artifact_error("ActSQUAT")
        self.assertIn("ActSQUAT.NON_MATCHING", error)
        self.assertIn("rebuild the draft without -n", error)

        with mock.patch.object(matchdiff, "linked_nonmatching_stub",
                               return_value=False):
            self.assertIsNone(asmdiff.candidate_artifact_error("ActSQUAT"))


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

    def test_guarded_unsigned_division_needs_only_divide_by_zero_guard(self):
        asm = """
    divu $zero, $v0, $v1
    bnez $v1, ok
    nop
    break 7
ok:
    mflo $v0
"""
        self.assertEqual(maspsxflags.guarded_division_count(asm), 1)

    def test_signed_division_with_only_break_7_is_not_aspsx_expansion(self):
        asm = """
    div $zero, $v0, $v1
    bnez $v1, ok
    nop
    break 7
ok:
    mflo $v0
"""
        self.assertEqual(maspsxflags.guarded_division_count(asm), 0)

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

    def test_global_audit_reports_target_metadata_omissions(self):
        guarded = {"F": "/tmp/F.c"}
        requirements = {"gp_symbols": ["Small"], "guarded_divisions": 1}
        tables = ({}, {}, {}, {})
        with mock.patch.object(maspsxflags.fuzzy_inventory,
                               "guarded_sources", return_value=guarded), \
                mock.patch.object(maspsxflags, "target_requirements",
                                  return_value=requirements), \
                mock.patch.object(maspsxflags, "build_tables",
                                  return_value=tables[:2]), \
                mock.patch.object(maspsxflags, "permute_tables",
                                  return_value=tables[2:]):
            errors = maspsxflags.audit_guarded_drafts()
        self.assertIn("F: Build.hs gp table missing Small", errors)
        self.assertIn("F: permute.py gp table missing Small", errors)
        self.assertIn("F: Build.hs missing --expand-div", errors)
        self.assertIn("F: permute.py missing --expand-div", errors)


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


class RegHistTests(unittest.TestCase):
    """The register histogram is the first move on a big Ghidra function."""

    def test_counts_objdump_and_dollar_forms_but_not_hex_literals(self):
        listing = [
            # objdump prints registers WITHOUT `$`; the tool must count these.
            (0x80000000, "lw v0,-2536(v0)"),
            (0x80000004, "addiu a0,a0,20740"),
            # A hex literal containing a register-like substring must NOT count.
            (0x80000008, "lui v1,0x800a0"),
            # The `$` form (hand-written asm / RTL dumps) counts too.
            (0x8000000C, "move $s0,$a3"),
            # A symbolic branch target must not be mined for register names.
            (0x80000010, "jal 800764b4 <printf_a0>"),
        ]
        counts = reghist.mentions(listing)
        self.assertEqual(counts["v0"], 2)
        self.assertEqual(counts["a0"], 2)      # addiu only; not 0x800a0, not <printf_a0>
        self.assertEqual(counts["v1"], 1)
        self.assertEqual(counts["s0"], 1)
        self.assertEqual(counts["a3"], 1)

    def test_opcode_histogram_catches_a_wrong_mnemonic(self):
        # A register histogram is blind to lh vs lhu: identical register profile,
        # structurally wrong code. ControlHumanoid's 17-byte park was exactly this
        # and would have read as "pure renames of identical instructions".
        target = [(0, "lh v0,4(a0)"), (4, "addu v0,v0,v1")]
        draft = [(0, "lhu v0,4(a0)"), (4, "addu v0,v0,v1")]
        self.assertEqual(reghist.mentions(target), reghist.mentions(draft))
        self.assertNotEqual(reghist.opcodes(target), reghist.opcodes(draft))
        self.assertEqual(reghist.opcodes(draft)["lhu"], 1)
        self.assertEqual(reghist.opcodes(target)["lh"], 1)

    def test_guard_detection_drives_the_draft_build(self):
        # reghist must build the DRAFT for a guarded function. Building the stub
        # compares the target against its own bytes and reports "every register
        # matches exactly" — a vacuous truth, and the worst possible false
        # negative, since it tells an agent that no lever remains.
        old = os.getcwd()
        with tempfile.TemporaryDirectory() as directory:
            os.chdir(directory)
            try:
                os.makedirs(os.path.join("src", "main.exe"))
                guarded = os.path.join("src", "main.exe", "Guarded.c")
                with open(guarded, "w") as stream:
                    stream.write("#ifndef NON_MATCHING\n"
                                 'INCLUDE_ASM("x", Guarded);\n'
                                 "#else\nvoid Guarded(void) {}\n#endif\n")
                plain = os.path.join("src", "main.exe", "Plain.c")
                with open(plain, "w") as stream:
                    stream.write("void Plain(void) {}\n")
                self.assertTrue(reghist.is_guarded("Guarded"))
                self.assertFalse(reghist.is_guarded("Plain"))
                self.assertFalse(reghist.is_guarded("DoesNotExist"))
            finally:
                os.chdir(old)

    def test_unallocatable_registers_are_ignored(self):
        # $at/$sp/$gp/$zero are not allocator-assigned, so a difference in them
        # is never a decomposition lever and must not perturb the delta sum.
        counts = reghist.mentions([(0, "addiu sp,sp,-24"), (4, "lui at,0x8009")])
        self.assertEqual(dict(counts), {})


class PermuteResultReportTests(unittest.TestCase):
    """A bounded run's findings must survive the SIGTERM that ends it."""

    def test_dead_declarations_flags_only_unused_identifiers(self):
        source = (
            "void F(void) {\n"
            "    GsSPRITE *new_var;\n"
            "    int live = 1;\n"
            "    int used_once;\n"
            "    used_once = live + 1;\n"
            "    g(used_once);\n"
            "}\n"
        )
        self.assertEqual(permute.dead_declarations(source), ["new_var"])

    def test_semantic_delta_ignores_pure_reformatting(self):
        with tempfile.TemporaryDirectory() as directory:
            base = os.path.join(directory, "base.c")
            candidate_dir = os.path.join(directory, "output-1")
            os.makedirs(candidate_dir)
            candidate = os.path.join(candidate_dir, "source.c")
            with open(base, "w") as stream:
                stream.write("int F(int a) {\n    return a + 1;\n}\n")
            # Same code, permuter-style reflow: no semantic delta should show.
            with open(candidate, "w") as stream:
                stream.write("int F(int a)\n{\n\n        return a + 1;\n\n}\n")
            self.assertEqual(permute.semantic_delta(base, candidate), [])

            with open(candidate, "w") as stream:
                stream.write("int F(int a)\n{\n    return a + 2;\n}\n")
            delta = permute.semantic_delta(base, candidate)
            self.assertTrue(any(line.startswith("-return a + 1;") for line in delta))
            self.assertTrue(any(line.startswith("+return a + 2;") for line in delta))

    def test_report_is_written_for_an_interrupted_run(self):
        with tempfile.TemporaryDirectory() as work:
            with open(os.path.join(work, "base.c"), "w") as stream:
                stream.write("int F(int a) { return a + 1; }\n")
            candidate_dir = os.path.join(work, "output-7")
            os.makedirs(candidate_dir)
            source = os.path.join(candidate_dir, "source.c")
            with open(source, "w") as stream:
                stream.write("int F(int a) { int dead_one; return a + 2; }\n")
            valid = [{"whole": 12, "window": 8, "text_size": 40, "source": source}]
            path = permute.write_result_report("F", work, valid, interrupted=True)
            report = open(path).read()
        self.assertIn("INTERRUPTED", report)
        self.assertIn("output-7/source.c", report)
        self.assertIn("12 whole-image differing bytes", report)
        self.assertIn("+return a + 2;", report)
        self.assertIn("dead_one", report)

    def test_report_records_an_empty_search(self):
        with tempfile.TemporaryDirectory() as work:
            path = permute.write_result_report("F", work, [], interrupted=True)
            self.assertIn("retained nothing usable", open(path).read())


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
    def test_stack_copy_hints_recover_vector_copy_chain(self):
        assembly = """
    lw $t1, 0x38($sp)
    lw $t2, 0x3c($sp)
    lw $t3, 0x40($sp)
    lw $t0, 0x44($sp)
    sw $t1, 0x28($sp)
    sw $t2, 0x2c($sp)
    sw $t3, 0x30($sp)
    sw $t0, 0x34($sp)
    lw $t1, 0x48($sp)
    lw $t2, 0x4c($sp)
    lw $t3, 0x50($sp)
    lw $t0, 0x54($sp)
    sw $t1, 0x38($sp)
    sw $t2, 0x3c($sp)
    sw $t3, 0x40($sp)
    sw $t0, 0x44($sp)
"""
        hints = stackplan.stack_copy_hints(assembly)
        self.assertEqual(hints, [
            {"source": 0x38, "destination": 0x28,
             "size": 0x10, "words": 4},
            {"source": 0x48, "destination": 0x38,
             "size": 0x10, "words": 4},
        ])
        self.assertEqual(stackplan.stack_copy_chains(hints), [
            {"size": 0x10, "offsets": [0x48, 0x38, 0x28]},
        ])

    def test_stack_copy_hints_reject_spills_and_register_reordering(self):
        assembly = """
    lw $t0, 0x20($sp)
    sw $t0, 0x30($sp)
    lw $t0, 0x40($sp)
    lw $t1, 0x44($sp)
    lw $t2, 0x48($sp)
    sw $t1, 0x50($sp)
    sw $t0, 0x54($sp)
    sw $t2, 0x58($sp)
"""
        self.assertEqual(stackplan.stack_copy_hints(assembly), [])

    def test_vector_array_hint_finds_reused_second_xyz_triplet(self):
        assembly = """
glabel F
    addiu $sp, $sp, -0x88
    sw $s0, 0x70($sp)
    addiu $a1, $sp, 0x48
    lhu $v0, 0x48($sp)
    lhu $v1, 0x4c($sp)
    lhu $a0, 0x50($sp)
    sw $v0, 0x58($sp)
    sw $v1, 0x5c($sp)
    sw $a0, 0x60($sp)
    lw $v0, 0x58($sp)
    lw $v1, 0x5c($sp)
    lw $a0, 0x60($sp)
"""
        info = stackplan.analyze(assembly, args_hint=0x10)
        self.assertEqual(
            info["vector_array_hints"],
            [{"base": 0x48, "second": 0x58}],
        )

    def test_vector_array_hint_rejects_unreloaded_word_spills(self):
        accesses = {
            0x48: {"addr": 1, "lhu": 1},
            0x4c: {"lhu": 1},
            0x50: {"lhu": 1},
            0x58: {"sw": 1},
            0x5c: {"sw": 1},
            0x60: {"sw": 1},
        }
        self.assertEqual(stackplan.vector_array_hints(accesses), [])

    def test_analyze_drops_vector_hint_crossing_into_saved_area(self):
        assembly = """
    addiu $sp, $sp, -0x68
    sw $s0, 0x58($sp)
    addiu $a0, $sp, 0x48
    lw $t0, 0x48($sp)
    lw $t1, 0x4c($sp)
    lw $t2, 0x50($sp)
    sw $t0, 0x58($sp)
    sw $t1, 0x5c($sp)
    sw $t2, 0x60($sp)
    lw $t0, 0x58($sp)
    lw $t1, 0x5c($sp)
    lw $t2, 0x60($sp)
"""
        info = stackplan.analyze(assembly, args_hint=0x18)
        self.assertEqual(info["saved_start"], 0x58)
        self.assertEqual(info["vector_array_hints"], [])

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

    def test_plain_report_points_to_overlay_scaffold(self):
        info = {"workspace_size": 0x28}
        self.assertEqual(
            stackplan.overlay_followup("ActDEAD", info),
            "next: tools/stackplan.py ActDEAD -n --emit-overlay",
        )
        self.assertIsNone(stackplan.overlay_followup(
            "NoWorkspace", {"workspace_size": None}
        ))

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

    def test_cc_extra_flags_match_build(self):
        path = os.path.join(os.path.dirname(TOOLS), "shake", "src", "Build.hs")
        with open(path) as f:
            build = f.read()
        block = build.split("ccExtraFlags src =", 1)[1].split("_ -> []", 1)[0]
        found = {}
        pattern = __import__("re").compile(
            r'^\s*"([A-Za-z0-9_]+)"\s*->\s*\[(.*)\]$', __import__("re").M
        )
        for name, values in pattern.findall(block):
            found[name] = __import__("re").findall(r'"([^"]+)"', values)
        self.assertEqual(found, permute.CC_EXTRA_FLAGS)
        self.assertIn(
            "-mno-split-addresses", permute.cc_flags_for("MemCardCallback")
        )
        self.assertNotIn("-mno-split-addresses", permute.cc_flags_for("Other"))

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

    def test_atomic_write_preserves_executable_mode(self):
        with tempfile.NamedTemporaryFile("w", delete=False) as fh:
            path = fh.name
            fh.write("old\n")
        try:
            os.chmod(path, 0o755)
            merge_metadata_conflicts.atomic_write(path, "new\n")
            self.assertEqual(os.stat(path).st_mode & 0o777, 0o755)
            with open(path) as fh:
                self.assertEqual(fh.read(), "new\n")
        finally:
            os.unlink(path)

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


class GtePolicyTests(unittest.TestCase):
    """docs/gte-policy.md containment: gte.h is the only asm-bearing header,
    and every .c that includes it is whitelisted.

    .c files with literal __asm__ are already blocked from ever completing by
    matchdiff's source_completion_blockers; the gap these tests close is asm
    smuggled through a header (which that blocker cannot see) and gte.h use
    spreading beyond the owner-approved whitelist. The three parked
    investigation drafts (PClseek/GetPad/FUN_8001b174) legitimately contain
    __asm__ inside their NON_MATCHING-guarded bodies and are exempt here
    because they are .c files, not headers.
    """

    GTE_HEADER = os.path.join("src", "main.exe", "gte.h")

    @staticmethod
    def _strip_comments(text):
        import re
        text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
        return re.sub(r"//[^\n]*", "", text)

    def test_gte_h_is_the_only_asm_bearing_header_under_src(self):
        import re
        root = os.path.dirname(TOOLS)
        offenders = []
        for directory, _, names in os.walk(os.path.join(root, "src")):
            for name in names:
                if not name.endswith(".h"):
                    continue
                path = os.path.join(directory, name)
                rel = os.path.relpath(path, root)
                if rel == self.GTE_HEADER:
                    continue
                with open(path, errors="replace") as stream:
                    code = self._strip_comments(stream.read())
                if re.search(r"\b__asm__\b", code):
                    offenders.append(rel)
        self.assertEqual(offenders, [])

    def test_handwritten_reconstructions_use_the_raw_command_macros(self):
        """A handwritten original scheduled COP2 latency itself — no macro nops.

        The `gte_<cmd>()` macros carry `nop;nop` because that is what a COMPILED
        caller's PsyQ macro emitted. Using them in a reconstruction of handwritten
        assembly silently adds two instructions per command: it put drawF3 24 bytes
        over its 296-byte carve, and fuzz-score did not catch it (only the linked
        length did). Pin the distinction instead of re-learning it.
        """
        import re
        root = os.path.dirname(TOOLS)
        with open(os.path.join(root, "config", "handwritten-asm.txt")) as stream:
            handwritten = {line.split("#")[0].strip() for line in stream} - {""}
        nop_form = re.compile(r"\bgte_(rtps|rtpt|nclip|dpcs)\s*\(")
        offenders = []
        for name in sorted(handwritten):
            path = os.path.join(root, "src", "main.exe", name + ".c")
            if not os.path.exists(path):
                continue
            with open(path, errors="replace") as stream:
                code = self._strip_comments(stream.read())
            if nop_form.search(code):
                offenders.append(name)
        self.assertEqual(offenders, [])

    def test_every_gte_h_includer_is_whitelisted(self):
        import re
        root = os.path.dirname(TOOLS)
        allowlist_path = os.path.join(root, "config", "gte-allowlist.txt")
        with open(allowlist_path) as stream:
            allowed = {line.strip() for line in stream if line.strip()}
        offenders = []
        src = os.path.join(root, "src", "main.exe")
        include = re.compile(r"^\s*#\s*include\s+\"gte\.h\"", re.M)
        for name in sorted(os.listdir(src)):
            if not name.endswith(".c"):
                continue
            with open(os.path.join(src, name), errors="replace") as stream:
                code = self._strip_comments(stream.read())
            if include.search(code) and name[:-2] not in allowed:
                offenders.append(name)
        self.assertEqual(offenders, [])


if __name__ == "__main__":
    unittest.main()

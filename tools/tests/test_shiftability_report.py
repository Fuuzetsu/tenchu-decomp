"""Tests for the composed user/agent shiftability dashboard."""

from __future__ import annotations

import unittest

from tools import shiftability_report as report


class SourceDebtTests(unittest.TestCase):
    def test_live_conditionals_equal_the_reviewed_non_blocking_debt(self) -> None:
        debt = report.collect_source_debt(report.ROOT)
        self.assertEqual(debt["count"], 5)
        self.assertEqual(debt["initial_inventory"], 6)
        self.assertEqual(
            debt["categories"],
            {"exact_opcode_conflict": 2, "source_reconstruction": 3},
        )
        self.assertTrue(
            all(not entry["normal_relink_blocker"] for entry in debt["entries"])
        )


class BlockerTests(unittest.TestCase):
    def test_fixed_abs_values_are_not_misreported_as_blockers(self) -> None:
        sections = {
            "input_audit": {
                "status": "pass",
                "data": {"findings": []},
            },
            "final_audit": {
                "status": "pass",
                "data": {
                    "absolute_symbols": [
                        {"name": "fixed", "movable": False},
                        {"name": "stale", "movable": True},
                    ],
                    "literal_jumps": [],
                    "conservative_hi_lo": [],
                    "literal_pointers": [],
                },
            },
        }
        blockers = report.active_blockers(sections)
        self.assertEqual(len(blockers), 1)
        self.assertEqual(blockers[0]["name"], "stale")

    def test_proof_errors_and_input_findings_join_one_work_queue(self) -> None:
        sections = {
            "layout": {"status": "fail", "error": "bad pool bounds"},
            "input_audit": {
                "status": "pass",
                "data": {
                    "findings": [
                        {
                            "kind": "literal_lui_main_base",
                            "object": "fault.c.o",
                            "detail": "literal movable base",
                        }
                    ]
                },
            },
        }
        blockers = report.active_blockers(sections)
        self.assertEqual(
            [(item["category"], item["kind"]) for item in blockers],
            [
                ("layout", "proof_error"),
                ("input_audit", "literal_lui_main_base"),
            ],
        )

    def test_finding_failure_does_not_add_a_duplicate_proof_error(self) -> None:
        sections = {
            "input_audit": {
                "status": "fail",
                "data": {
                    "findings": [
                        {
                            "kind": "literal_lui_main_base",
                            "object": "fault.c.o",
                            "detail": "literal movable base",
                        }
                    ]
                },
            }
        }
        blockers = report.active_blockers(sections)
        self.assertEqual(len(blockers), 1)
        self.assertEqual(blockers[0]["kind"], "literal_lui_main_base")

    def test_finding_failure_renders_counts_instead_of_fake_error(self) -> None:
        section = {"status": "pass", "data": {"summary": {"findings": 1}}}
        report._mark_finding_failure(section, 1)
        self.assertEqual(section["status"], "fail")
        lines: list[str] = []
        self.assertFalse(report._render_error(lines, section))
        self.assertEqual(lines, [])

    def test_pool_bounds_are_policy_not_external_fixed_contracts(self) -> None:
        contracts = report.intentional_contracts()
        contract_ranges = {contract["range"] for contract in contracts}
        self.assertNotIn(report.hex32(report.POOL_FLOOR), contract_ranges)
        self.assertNotIn(report.hex32(report.POOL_END), contract_ranges)
        policy_values = {
            item["value"] for item in report.configurable_layout_policy()
        }
        self.assertIn(report.hex32(report.POOL_FLOOR), policy_values)
        self.assertIn(report.hex32(report.POOL_END), policy_values)


if __name__ == "__main__":
    unittest.main()

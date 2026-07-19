"""Regression tests for the opt-in GS_107 original-object lane.

Run: nix develop -c python -m unittest tools.tests.test_psyq_object_lane
"""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
import tempfile
import unittest

from tools import psyq_object_lane as lane


ROOT = Path(__file__).resolve().parents[2]
MANIFEST_PATH = ROOT / "config/psyq-objects.main.exe.json"


class ManifestTests(unittest.TestCase):
    def test_manifest_pins_complete_relocation_table(self) -> None:
        manifest = lane.load_manifest(MANIFEST_PATH)
        self.assertEqual(manifest["object"], "GS_107")
        self.assertEqual(len(manifest["elf"]["relocations"]), 16)
        self.assertEqual(
            manifest["archive"]["member_sha256"],
            "f593e10973684257db59aab1254b773828fa41262d319881609fc4ed9b76921b",
        )
        self.assertEqual(
            set(manifest["link"]["object_defined_symbols"]),
            {"GsSetFlatLight", "GsLIGHTWSMATRIX", "_LC"},
        )

    def test_archive_gate_rejects_an_unlisted_hash(self) -> None:
        manifest = json.loads(MANIFEST_PATH.read_text())
        with tempfile.TemporaryDirectory() as temporary:
            archive = Path(temporary) / "LIBGS.LIB"
            archive.write_bytes(b"not a PsyQ archive")
            with self.assertRaisesRegex(lane.LaneError, "not an accepted"):
                lane.validate_archive(archive, manifest)

    def test_archive_gate_reports_the_matching_version(self) -> None:
        manifest = json.loads(MANIFEST_PATH.read_text())
        with tempfile.TemporaryDirectory() as temporary:
            archive = Path(temporary) / "LIBGS.LIB"
            archive.write_bytes(b"synthetic archive fixture")
            digest = hashlib.sha256(archive.read_bytes()).hexdigest()
            manifest["archive"]["accepted_sha256"] = {"fixture": digest}
            self.assertEqual(lane.validate_archive(archive, manifest), "fixture")


class LinkerRewriteTests(unittest.TestCase):
    def setUp(self) -> None:
        self.link = json.loads(MANIFEST_PATH.read_text())["link"]

    def source_linker(self) -> str:
        lines = [
            "SECTIONS",
            "{",
            "    .main_exe 0x80011000 : AT(main_exe_ROM_START) SUBALIGN(4)",
            "    {",
            "        " + self.link["text_owner"],
        ]
        for owner, sections in self.link["remove_objects"].items():
            lines.extend("        " + f"{owner}({section});" for section in sections)
        lines.extend(
            [
                "        " + self.link["tail_owner"],
                "    }",
                "    /DISCARD/ :",
                "    {",
                "        *(*);",
                "    }",
                "}",
            ]
        )
        return "\n".join(lines) + "\n"

    def test_rewrite_uses_one_text_owner_and_noload_bss(self) -> None:
        rewritten = lane.rewrite_linker_script(
            self.source_linker(),
            self.link,
            ".shake/psyq/GS_107/GS_107.o",
            ".shake/psyq/GS_107/post-GS_107.o",
        )
        self.assertEqual(rewritten.count("GS_107.o(.text);"), 1)
        self.assertEqual(rewritten.count("GS_107.o(.bss COMMON);"), 1)
        self.assertIn("(NOLOAD) : SUBALIGN(4)", rewritten)
        self.assertIn("0x800c64a0", rewritten)
        self.assertIn("post-GS_107.o(.data);", rewritten)
        for owner in self.link["remove_objects"]:
            self.assertNotIn(owner, rewritten)

    def test_rewrite_fails_if_generated_owner_drifted(self) -> None:
        source = self.source_linker().replace(self.link["text_owner"], "drifted.o(.data);")
        with self.assertRaisesRegex(lane.LaneError, "GS_107 text owner"):
            lane.rewrite_linker_script(source, self.link, "GS_107.o", "tail.o")

    def test_rewrite_requires_subalign_four(self) -> None:
        source = self.source_linker().replace("SUBALIGN(4)", "SUBALIGN(8)", 1)
        with self.assertRaisesRegex(lane.LaneError, r"SUBALIGN\(4\)"):
            lane.rewrite_linker_script(source, self.link, "GS_107.o", "tail.o")


class SymbolScriptTests(unittest.TestCase):
    def test_object_symbols_replace_absolute_assignments(self) -> None:
        source = """\
GsSetFlatLight = 0x800655e4;
GsLIGHTWSMATRIX = 0x800c64a0;
_LC = 0x800c64c0;
Unrelated = 0x1234;
"""
        names = ["GsSetFlatLight", "GsLIGHTWSMATRIX", "_LC"]
        stripped = lane.strip_assignments(source, names, required=names)
        self.assertEqual(stripped, "Unrelated = 0x1234;\n")

    def test_required_assignment_must_exist_once(self) -> None:
        with self.assertRaisesRegex(lane.LaneError, "expected one assignment"):
            lane.strip_assignments("Other = 1;\n", ["_LC"], required=["_LC"])


class ComparisonTests(unittest.TestCase):
    def test_first_difference_handles_content_and_length(self) -> None:
        self.assertIsNone(lane.first_difference(b"same", b"same"))
        self.assertEqual(lane.first_difference(b"abc", b"axc"), 1)
        self.assertEqual(lane.first_difference(b"abc", b"abcx"), 3)

    def test_audit_output_is_confined_to_ignored_tree(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            inside = root / ".shake/psyq/GS_107"
            outside = root / "tracked/GS_107"
            self.assertEqual(
                lane.audit_output_name(inside, root), ".shake/psyq/GS_107"
            )
            with self.assertRaisesRegex(lane.LaneError, "must stay below"):
                lane.audit_output_name(outside, root)


if __name__ == "__main__":
    unittest.main()

"""Motion catalog checks for the distinctions that hid script-only IDs."""

from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

from tools import motion_catalog as catalog


class ScriptTests(unittest.TestCase):
    def test_actor_requests_next_stance_and_script_only_variant(self):
        script = b"".join(struct.pack("<6h", *row) for row in (
            (0, 10, -1, -1, -1, 0),
            (3, 141, 0x107, 1, 0, -1),
            (3, 0, 0x108, 1, -1, -1),
            (3, 1, -1, -1, 0x109, -1),  # despawn ignores next motion
        )) + struct.pack("<h", -1) + b"trailing telop strings"
        self.assertEqual(list(catalog.cad_motions(script)),
                         [(0x107, 141), (0x501, 141), (0x108, 0)])

    def test_only_actor_motion_fields_are_motion_ids(self):
        script = struct.pack("<6h", 2, 0, 0x107, 0x108, 0x109, 0x10a)
        script += struct.pack("<h", -1)
        self.assertEqual(list(catalog.cad_motions(script)), [])

    def test_malformed_script_cannot_silently_pass_coverage(self):
        for script in (b"", struct.pack("<6h", 3, 0, 0x107, 1, -1, -1),
                       struct.pack("<6h", 99, 0, 0, 0, 0, 0)):
            with self.subTest(script=script), self.assertRaises(ValueError):
                list(catalog.cad_motions(script))

    def test_esd_status_and_motion_triggers_are_distinct(self):
        status = struct.pack("<IBBh6h", 1, 141, 3, 0x107, *([-1] * 6))
        motion = struct.pack("<IBBh6h", 2, 141, 4, 0x108, *([-1] * 6))
        self.assertEqual(list(catalog.esd_motions(status + motion + struct.pack("<i", -1))),
                         [(0x108, 141)])


class ClipTests(unittest.TestCase):
    def test_amd_ids_come_from_relocated_records_not_array_indices(self):
        data = struct.pack("<iII", 2, 12, 20)
        data += struct.pack("<4Bhh", 0, 0, 0, 0, 1, 475)
        data += struct.pack("<4Bhh", 0, 0, 0, 0, 1, 515)
        self.assertEqual(list(catalog.amd_clips(data)), [475, 515])

    def test_invalid_amd_pointer_fails_coverage(self):
        for data in (b"", struct.pack("<iI", 1, 4), struct.pack("<iI", 1, 1024)):
            with self.subTest(data=data), self.assertRaises(ValueError):
                list(catalog.amd_clips(data))

    def test_afs_parents_distinguish_main_and_trial_motion_packs(self):
        directory = lambda name: (2, 0, 0, 0, name)
        elements = [directory("K:"), directory("@0_WORK"), directory("@1_CDIMAGE"),
                    directory("@2_HUMAN"), directory("@3_MOTION"),
                    (1, 0, 4, 4, "@4_COMMON.AMD"), directory("@2_TRIAL"),
                    directory("@6_HUMAN"), directory("@7_MOTION"),
                    (1, 0, 4, 4, "@8_COMMON.AMD")]
        with patch.object(catalog.voldump, "read_elements", return_value=elements):
            self.assertEqual([path for path, _ in catalog.volume_files(b"data")],
                             ["HUMAN/MOTION/COMMON.AMD", "TRIAL/HUMAN/MOTION/COMMON.AMD"])


class SourceTests(unittest.TestCase):
    def test_comments_and_strings_do_not_manufacture_c_uses(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "src/main.exe"
            source.mkdir(parents=True)
            (source / "example.C").write_text('''
/* MOT_FAKE */
void First(void)
{
    Print("MOT_FAKE");
    SetMotion(MOT_REAL);
}
static inline void Second(
    int parameter)
{
    SetMotion(MOT_REAL);
}
#define REQUEST() \\
    SetMotion(MOT_REAL)
''')
            with patch.object(catalog, "ROOT", root):
                uses = catalog.source_uses({"MOT_REAL": 1, "MOT_FAKE": 2})
        self.assertEqual(uses[1], {("example.C", "First"), ("example.C", "Second"),
                                   ("example.C", "REQUEST")})
        self.assertNotIn(2, uses)

    def test_duplicate_ids_cannot_hide_a_name(self):
        with self.assertRaisesRegex(ValueError, "duplicate motion ID"):
            catalog.state_names("enum { MOT_A = 0x107, MOT_B = 263 };")


if __name__ == "__main__":
    unittest.main()

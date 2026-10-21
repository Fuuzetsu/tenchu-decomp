import struct
import unittest

from tools import menu_data_inventory as inventory


IMAGE = inventory.ROOT / "disks/tenchu/menu.exe"


@unittest.skipUnless(IMAGE.exists(), "retail MENU.EXE is not available")
class MenuDataInventoryTests(unittest.TestCase):
    target = "menu.exe"

    @classmethod
    def setUpClass(cls):
        cls.image = (inventory.ROOT / "disks/tenchu" / cls.target).read_bytes()
        cls.end, cls.tables, _ = inventory.LAYOUTS[cls.target]

    def audit(self, image):
        return inventory.audit(image, self.target)

    def render(self, rows, image):
        return inventory.render(rows, image, self.target)

    def committed(self):
        return inventory.report_path(self.target).read_text()

    def changed(self, address, data):
        image = bytearray(self.image)
        offset = address - inventory.FILE_BIAS
        image[offset:offset + len(data)] = data
        return image

    def test_every_leading_byte_has_one_reviewed_data_span(self):
        rows = self.audit(self.image)
        self.assertEqual(rows[0][0], inventory.START)
        self.assertEqual(rows[-1][1], self.end)
        for left, right in zip(rows, rows[1:]):
            self.assertEqual(left[1], right[0])
        self.assertEqual(self.render(rows, self.image), self.committed())

    def test_switch_table_cannot_point_into_another_function(self):
        image = self.changed(self.tables[0][0], struct.pack("<I", self.end))
        with self.assertRaisesRegex(ValueError, "table target"):
            self.audit(image)

    def test_switch_table_requires_its_consumer_reference(self):
        image = self.changed(self.tables[0][2], b"\0" * 4)
        with self.assertRaisesRegex(ValueError, "missing table LUI"):
            self.audit(image)

    def test_voice_table_requires_its_sentinel(self):
        image = self.changed(0x800117F2, b"\x01")
        with self.assertRaisesRegex(ValueError, "missing final voice sentinel"):
            self.audit(image)

    def test_music_padding_cannot_hide_unclassified_bytes(self):
        image = self.changed(0x800112E5, b"\x01")
        with self.assertRaisesRegex(ValueError, "music row padding"):
            self.audit(image)

    def test_reserved_zero_region_cannot_hide_unclassified_bytes(self):
        image = self.changed(0x80011400, b"\x81")
        with self.assertRaisesRegex(ValueError, "unclassified nonzero byte"):
            self.audit(image)

    def test_valid_but_changed_cue_is_not_the_committed_inventory(self):
        image = self.changed(0x800112E0, b"\x01")
        rows = self.audit(image)
        self.assertNotEqual(self.render(rows, image), self.committed())


@unittest.skipUnless((inventory.ROOT / "disks/tenchu/ending.exe").exists(),
                     "retail ENDING.EXE is not available")
class EndingDataInventoryTests(MenuDataInventoryTests):
    target = "ending.exe"


if __name__ == "__main__":
    unittest.main()

import struct
import unittest

from tools import menu_data_inventory as inventory


IMAGE = inventory.ROOT / "disks/tenchu/menu.exe"


@unittest.skipUnless(IMAGE.exists(), "retail MENU.EXE is not available")
class MenuDataInventoryTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.image = IMAGE.read_bytes()

    def changed(self, address, data):
        image = bytearray(self.image)
        offset = address - inventory.FILE_BIAS
        image[offset:offset + len(data)] = data
        return image

    def test_every_leading_byte_has_one_reviewed_data_span(self):
        rows = inventory.audit(self.image)
        self.assertEqual(rows[0][0], inventory.START)
        self.assertEqual(rows[-1][1], inventory.END)
        for left, right in zip(rows, rows[1:]):
            self.assertEqual(left[1], right[0])
        self.assertEqual(inventory.render(rows, self.image),
                         inventory.REPORT.read_text())

    def test_switch_table_cannot_point_into_another_function(self):
        image = self.changed(0x80011F84, struct.pack("<I", 0x80013AC0))
        with self.assertRaisesRegex(ValueError, "table target"):
            inventory.audit(image)

    def test_switch_table_requires_its_consumer_reference(self):
        image = self.changed(0x80019770, b"\0" * 4)
        with self.assertRaisesRegex(ValueError, "missing table LUI"):
            inventory.audit(image)

    def test_voice_table_requires_its_sentinel(self):
        image = self.changed(0x800117F2, b"\x01")
        with self.assertRaisesRegex(ValueError, "missing final voice sentinel"):
            inventory.audit(image)

    def test_music_padding_cannot_hide_unclassified_bytes(self):
        image = self.changed(0x800112E5, b"\x01")
        with self.assertRaisesRegex(ValueError, "music row padding"):
            inventory.audit(image)

    def test_reserved_zero_region_cannot_hide_unclassified_bytes(self):
        image = self.changed(0x80011400, b"\x81")
        with self.assertRaisesRegex(ValueError, "unclassified nonzero byte"):
            inventory.audit(image)

    def test_valid_but_changed_cue_is_not_the_committed_inventory(self):
        image = self.changed(0x800112E0, b"\x01")
        rows = inventory.audit(image)
        self.assertNotEqual(inventory.render(rows, image),
                            inventory.REPORT.read_text())


if __name__ == "__main__":
    unittest.main()

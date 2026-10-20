import struct
import unittest

from tools.reloc_c_literals import Relocation
from tools.shared_symbols import infer_symbols


def words(*values):
    return struct.pack("<" + "I" * len(values), *values)


class SharedSymbolTests(unittest.TestCase):
    def test_constants_are_not_discarded_like_a_discovery_fingerprint(self):
        with self.assertRaisesRegex(ValueError, "non-relocation"):
            infer_symbols(words(0x24020001), words(0x24020002), [], {}, 0, 0)

    def test_high_low_pair_handles_the_signed_low_half_carry(self):
        result = infer_symbols(
            words(0x3C028009, 0x24427000),
            words(0x3C028006, 0x2442B500),
            [Relocation(0, 5, "Data"), Relocation(4, 6, "Data")],
            {"Data": 0x80097000}, 0, 0
        )
        self.assertEqual(result, {"Data": 0x8005B500})

    def test_gp_offsets_use_the_destination_executables_gp(self):
        result = infer_symbols(
            words(0x8F820008), words(0x8F820000),
            [Relocation(0, 7, "Pool")],
            {"Pool": 0x800976A0}, 0x80097698, 0x8005B48C
        )
        self.assertEqual(result, {"Pool": 0x8005B48C})

    def test_changed_registers_are_rejected_even_at_relocations(self):
        with self.assertRaisesRegex(ValueError, "non-relocation"):
            infer_symbols(
                words(0x8F820008), words(0x8F830000),
                [Relocation(0, 7, "Pool")],
                {"Pool": 0x800976A0}, 0x80097698, 0x8005B48C
            )

    def test_disagreeing_references_do_not_produce_a_symbol(self):
        with self.assertRaisesRegex(ValueError, "conflicting placements"):
            infer_symbols(
                words(0x8F820008, 0x8F820008),
                words(0x8F820000, 0x8F820004),
                [Relocation(0, 7, "Pool"), Relocation(4, 7, "Pool")],
                {"Pool": 0x800976A0}, 0x80097698, 0x8005B48C
            )


if __name__ == "__main__":
    unittest.main()

import struct
import unittest

from render_cearth import read_bank


class BankBoundsTests(unittest.TestCase):
    address = 0x401000

    def make_pe(self, virtual_size, raw_data):
        # PE32 sintético com uma seção .data e sem código executável.
        pe = 0x40
        optional = pe + 24
        section = optional + 32
        raw = section + 40
        header = bytearray(raw)
        struct.pack_into("<2s", header, 0, b"MZ")
        struct.pack_into("<I", header, 0x3C, pe)
        struct.pack_into("<4sHH", header, pe, b"PE\0\0", 0x14C, 1)
        struct.pack_into("<H", header, pe + 20, 32)
        struct.pack_into("<H", header, optional, 0x10B)
        struct.pack_into("<I", header, optional + 28, 0x400000)
        struct.pack_into(
            "<8sIIII", header, section, b".data", virtual_size, 0x1000, len(raw_data), raw
        )
        return bytes(header) + raw_data

    def test_bank_start_in_zero_filled_virtual_tail_is_rejected(self):
        data = self.make_pe(16, struct.pack("<hhhh", -1, 0, 0, 0))
        with self.assertRaisesRegex(ValueError, "fora dos dados gravados"):
            read_bank(data, self.address + 8)

    def test_explicit_bank_limit_cannot_extend_into_virtual_tail(self):
        data = self.make_pe(16, struct.pack("<hhhh", -1, 0, 0, 0))
        self.assertEqual(read_bank(data, self.address, self.address + 8), [])
        with self.assertRaisesRegex(ValueError, "limite inválido"):
            read_bank(data, self.address, self.address + 16)

    def test_terminator_in_raw_padding_past_virtual_end_is_not_used(self):
        record = struct.pack("<hhhh", 0, 0, 1, 1)
        terminator = struct.pack("<hhhh", -1, 0, 0, 0)
        self.assertEqual(read_bank(self.make_pe(16, record + terminator), self.address),
                         [(0, 0, 1, 1)])
        with self.assertRaisesRegex(ValueError, "terminador ausente"):
            read_bank(self.make_pe(8, record + terminator), self.address)

    def test_terminator_does_not_make_truncated_raw_section_valid(self):
        record = struct.pack("<hhhh", 0, 0, 1, 1)
        terminator = struct.pack("<hhhh", -1, 0, 0, 0)
        data = self.make_pe(24, record + terminator + bytes(8))
        next_address = self.address + len(record + terminator)

        self.assertEqual(read_bank(data, self.address, next_address), [(0, 0, 1, 1)])
        with self.assertRaisesRegex(ValueError, "fora dos dados gravados"):
            read_bank(data[:-1], self.address, next_address)

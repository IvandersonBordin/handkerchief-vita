import struct
import unittest

from tools.inspect_hdk import iter_section_classes, iter_sections
from render_cearth import read_bank, read_bitmap, read_cearth, resolve_pieces


def section(name, body):
    payload = name + b"\0" + body
    return struct.pack("<I", len(payload) + 4) + payload


class SectionTests(unittest.TestCase):
    def test_payload_does_not_change_section_offsets(self):
        first = section(b"STAGE_0", b"CGameRoot\0FRONT_0\0")
        data = first + section(b"FRONT_0", b"CTest\0")
        self.assertEqual(list(iter_sections(data)), [(0, "STAGE_0"), (len(first), "FRONT_0")])
        self.assertEqual(
            list(iter_section_classes(data)),
            [(0, "STAGE_0", "CGameRoot", 22), (len(first), "FRONT_0", "CTest", len(data))],
        )

    def test_class_requires_terminator_within_section(self):
        data = section(b"STAGE_0", b"CGameRoot") + section(b"FRONT_0", b"CTest\0")
        with self.assertRaisesRegex(ValueError, "nome da classe sem terminador"):
            list(iter_section_classes(data))

    def test_empty_class_is_rejected_only_in_class_mode(self):
        data = section(b"TEST_0", b"\0opaque")
        self.assertEqual(list(iter_sections(data)), [(0, "TEST_0")])
        with self.assertRaisesRegex(ValueError, "bloco em 0x0000: nome da classe inválido"):
            list(iter_section_classes(data))

    def test_partial_length_after_valid_section_is_rejected(self):
        first = section(b"TEST_0", b"CTest\0")
        data = first + b"\x06\x00\x00"
        message = f"bloco em 0x{len(first):04X}: campo de comprimento truncado"
        for inspect in (iter_sections, iter_section_classes):
            with self.subTest(inspect=inspect.__name__):
                with self.assertRaisesRegex(ValueError, message):
                    list(inspect(data))

    def test_truncated_section_is_rejected(self):
        data = section(b"STAGE_0", b"CGameRoot\0")
        with self.assertRaisesRegex(ValueError, "bloco truncado"):
            list(iter_sections(data[:-1]))


class BitmapTests(unittest.TestCase):
    def test_truncated_header_is_rejected_before_reading_fields(self):
        header = bytearray(54)
        header[:2] = b"BM"
        struct.pack_into("<IIiiHHI", header, 10, 58, 40, 1, 1, 1, 8, 0)
        for length in range(2, len(header)):
            with self.subTest(length=length):
                with self.assertRaisesRegex(ValueError, "BMP: cabeçalho truncado"):
                    read_bitmap(header[:length])


class RectangleTests(unittest.TestCase):
    def test_cearth_fixed_fields_must_fit_within_vista_section(self):
        body = b"CGameRoot\0" + bytes(8) + b"\xff\xffCEarth\0\x0b\x0b" + bytes(17)
        data = section(b"VISTA_0", body) + section(b"STAGE_0", b"CTest\0")
        with self.assertRaisesRegex(ValueError, "campos do primeiro CEarth truncados em VISTA_0"):
            read_cearth(data)

    def test_bank_terminator_must_precede_next_bank(self):
        # PE32 sintético contendo somente os campos lidos pelo inspetor.
        pe, optional, section_table, raw = 0x40, 0x58, 0x78, 0xA0
        address = 0x401000
        data = bytearray(raw + 16)
        data[:2] = b"MZ"
        struct.pack_into("<I", data, 0x3C, pe)
        struct.pack_into("<4sHH", data, pe, b"PE\0\0", 0x14C, 1)
        struct.pack_into("<H", data, pe + 20, 32)
        struct.pack_into("<H", data, optional, 0x10B)
        struct.pack_into("<I", data, optional + 28, 0x400000)
        struct.pack_into("<8sIIII", data, section_table, b".data", 16, 0x1000, 16, raw)
        struct.pack_into("<hhhhhhhh", data, raw, 0, 0, 1, 1, -1, 0, 0, 0)

        self.assertEqual(read_bank(data, address, address + 16), [(0, 0, 1, 1)])
        with self.assertRaisesRegex(ValueError, "terminador ausente"):
            read_bank(data, address, address + 8)

    def test_raw_reference_resolves_signed_offsets_and_color_key(self):
        pieces = resolve_pieces((100, 200, 0), [(0x081A8001, -4, 6)], [(0, 0, 1, 1)] * 2, [b"\0"])
        self.assertEqual(pieces, [(1, 96, 206, 0, 0, 1, 1, True)])

    def test_reference_base_can_change_bank_index_and_color_key(self):
        bank = [(0, 0, 1, 1), (1, 0, 1, 1)]
        pieces = resolve_pieces(
            (100, 200, 0x04008001), [(0x04000000, -4, 6)], bank, [b"\0\x01"]
        )

        self.assertEqual(pieces, [(1, 96, 206, 1, 0, 1, 1, True)])

    def test_reference_group_is_checked_before_adding_base(self):
        bank = [(0, 0, 1, 1)]
        with self.assertRaisesRegex(ValueError, "referência inesperada 0x48000000"):
            resolve_pieces((0, 0, 0xC0000000), [(0x48000000, 0, 0)], bank, [b"\0"])

    def test_index_at_bank_length_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "fora do banco 2"):
            resolve_pieces((0, 0, 0), [(0x081A8001, 0, 0)], [(0, 0, 1, 1)], [b"\0"])

    def test_source_rectangle_outside_bitmap_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "recorte inválido"):
            resolve_pieces((0, 0, 0), [(0x081A8000, 0, 0)], [(0, 0, 2, 1)], [b"\0"])

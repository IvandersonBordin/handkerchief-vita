import struct
import unittest

from render_cearth import read_bitmap


class BitmapRowTests(unittest.TestCase):
    def test_rows_use_top_origin_and_exclude_padding(self):
        palette_data = bytes((0, 0, 0, 0, 3, 2, 1, 0))
        top_row = b"\x00\x01\x00"
        bottom_row = b"\x01\x00\x01"
        padding = b"\xff"

        for height, stored_rows in (
            (2, (bottom_row, top_row)),
            (-2, (top_row, bottom_row)),
        ):
            with self.subTest(height=height):
                pixels = padding.join(stored_rows) + padding
                pixel_offset = 54 + len(palette_data)
                header = bytearray(54)
                struct.pack_into("<2sI4xI", header, 0, b"BM", pixel_offset + len(pixels), pixel_offset)
                struct.pack_into("<IiiHHIIiiII", header, 14, 40, 3, height, 1, 8, 0,
                                 len(pixels), 0, 0, 2, 0)

                palette, rows = read_bitmap(bytes(header) + palette_data + pixels)

                self.assertEqual(rows, [top_row, bottom_row])
                self.assertEqual(palette, [b"\x00\x00\x00\xff", b"\x01\x02\x03\xff"])

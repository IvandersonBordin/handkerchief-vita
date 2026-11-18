import unittest

from preview_camera import HEIGHT, WIDTH, render_frame, vista_offset


class CameraPreviewTests(unittest.TestCase):
    def test_parallax_truncates_toward_zero_relative_to_left_limit(self):
        for x, expected in ((4, 6), (7, 8), (8, 8), (12, 12), (13, 12), (16, 14)):
            with self.subTest(x=x):
                self.assertEqual(vista_offset(x, left_limit=10), expected)

    def test_translation_and_clipping_select_correct_source_pixel(self):
        # A composição 2 × 2 identifica cada canto por uma cor.
        red = bytes((255, 0, 0, 255))
        green = bytes((0, 255, 0, 255))
        blue = bytes((0, 0, 255, 255))
        white = bytes((255, 255, 255, 255))
        pixels = red + green + blue + white
        translation = vista_offset(-3, left_limit=0)
        cases = (
            ((1, -1), (0, 0), white),
            ((WIDTH + 1, -1), (WIDTH - 1, 0), blue),
            ((1, HEIGHT - 1), (0, HEIGHT - 1), green),
            ((WIDTH + 1, HEIGHT - 1), (WIDTH - 1, HEIGHT - 1), red),
        )
        for origin, (x, y), color in cases:
            with self.subTest(origin=origin):
                frame = render_frame(pixels, 2, 2, origin, translation)
                expected = bytearray(WIDTH * HEIGHT * 4)
                start = (y * WIDTH + x) * 4
                expected[start:start + 4] = color
                self.assertEqual(frame, expected)

    def test_fully_offscreen_composition_leaves_transparent_frame(self):
        for origin in ((-1, 0), (WIDTH, 0), (0, -1), (0, HEIGHT)):
            with self.subTest(origin=origin):
                frame = render_frame(bytes((255, 0, 0, 255)), 1, 1, origin, 0)
                self.assertEqual(frame, bytearray(WIDTH * HEIGHT * 4))

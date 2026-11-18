from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


INSPECTOR = Path(__file__).resolve().parents[1] / "tools" / "inspect_hdk.py"


def make_block(body):
    return (4 + len(body)).to_bytes(4, "little") + body


class SectionClassCommandTests(unittest.TestCase):
    def inspect(self, data, *options):
        with tempfile.TemporaryDirectory() as directory:
            sample = Path(directory) / "synthetic.hdk"
            sample.write_bytes(data)
            return subprocess.run(
                [sys.executable, "-B", str(INSPECTOR), *options, str(sample)],
                capture_output=True,
                text=True,
            )

    def test_prints_absolute_payload_offsets(self):
        first = make_block(b"ONE\0ClassA\0data")
        second = make_block(b"TWO\0ClassB\0")

        result = self.inspect(first + second, "--classes")

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stderr, "")
        self.assertEqual(
            result.stdout.splitlines(),
            ["0x0000 ONE ClassA dados=0x000F", "0x0013 TWO ClassB dados=0x0022"],
        )

    def test_invalid_second_class_does_not_print_partial_listing(self):
        first = make_block(b"ONE\0ClassA\0data")
        second = make_block(b"TWO\0ClassB")

        result = self.inspect(first + second, "--classes")

        self.assertEqual(result.returncode, 1)
        self.assertEqual(result.stdout, "")
        self.assertIn("bloco em 0x0013: nome da classe sem terminador", result.stderr)

    def test_combines_classes_and_bounds_for_adjacent_blocks(self):
        first = make_block(b"ONE\0ClassA\0data")
        second = make_block(b"TWO\0ClassB\0")

        result = self.inspect(first + second, "--classes", "--bounds")

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(
            result.stdout.splitlines(),
            [
                "0x0000 ONE ClassA dados=0x000F fim=0x0013 comprimento=19",
                "0x0013 TWO ClassB dados=0x0022 fim=0x0022 comprimento=15",
            ],
        )

    def test_empty_class_is_only_rejected_in_class_mode(self):
        block = make_block(b"ONE\0\0")

        result = self.inspect(block)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout, "0x0000 ONE\n")

        result = self.inspect(block, "--classes")
        self.assertEqual(result.returncode, 1)
        self.assertEqual(result.stdout, "")
        self.assertIn("bloco em 0x0000: nome da classe inválido", result.stderr)

    def test_truncated_length_after_valid_block_is_rejected_in_all_modes(self):
        data = make_block(b"ONE\0ClassA\0") + b"\x08\x00\x00"

        for options in ((), ("--classes",), ("--bounds",), ("--classes", "--bounds")):
            with self.subTest(options=options):
                result = self.inspect(data, *options)
                self.assertEqual(result.returncode, 1)
                self.assertEqual(result.stdout, "")
                self.assertIn("bloco em 0x000F: campo de comprimento truncado", result.stderr)

    def test_bounds_lists_adjacent_blocks_without_reading_classes(self):
        first = make_block(b"ONE\0\0")
        second = make_block(b"TWO\0data")

        result = self.inspect(first + second, "--bounds")

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(
            result.stdout.splitlines(),
            [
                "0x0000 ONE fim=0x0009 comprimento=9",
                "0x0009 TWO fim=0x0015 comprimento=12",
            ],
        )

    def test_class_terminator_must_be_inside_its_block(self):
        data = make_block(b"ONE\0ClassA") + make_block(b"TWO\0ClassB\0")

        result = self.inspect(data, "--classes")

        self.assertEqual(result.returncode, 1)
        self.assertEqual(result.stdout, "")
        self.assertIn("bloco em 0x0000: nome da classe sem terminador", result.stderr)

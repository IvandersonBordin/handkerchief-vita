"""Demonstra o parallax de VISTA_0 em três posições experimentais."""

import struct
import sys

from render_cearth import ROOT, encode_png, render_pieces
from render_vista0 import load_vista_scene


# Dimensões do contexto original identificado em HDKWIN.EXE.
WIDTH, HEIGHT = 640, 480


def vista_offset(x, left_limit):
    """Aplica a divisão inteira x86, que trunca em direção a zero."""
    distance = x - left_limit
    quotient = abs(distance) // 3
    if distance < 0:
        quotient = -quotient
    return x - quotient


def render_frame(pixels, width, height, origin, translation):
    """Copia a parte visível da composição para um quadro transparente."""
    left, top = origin
    left += translation
    visible_left = max(0, left)
    visible_right = min(WIDTH, left + width)
    frame = bytearray(WIDTH * HEIGHT * 4)
    if visible_left >= visible_right:
        return frame

    visible_bytes = (visible_right - visible_left) * 4
    for y in range(max(0, top), min(HEIGHT, top + height)):
        source = ((y - top) * width + visible_left - left) * 4
        target = (y * WIDTH + visible_left) * 4
        frame[target:target + visible_bytes] = pixels[source:source + visible_bytes]
    return frame


def main():
    output = ROOT / "build"
    try:
        object_count, pieces, palette, rows = load_vista_scene()
        if (object_count, len(pieces)) != (6, 32):
            raise ValueError("A0.HDK: esperados 6 objetos CEarth e 32 elementos")
        origin, width, height, pixels = render_pieces(pieces, palette, rows)
        output.mkdir(exist_ok=True)
        print(f"VISTA_0 com {object_count} objetos e {len(pieces)} elementos, origem de mundo {origin}.")
        print("Quadros experimentais de 640 × 480, com ajuste gráfico zero e paleta original do BMP.")
        for x in (0, -300, -600):
            translation = vista_offset(x, left_limit=0)
            frame = render_frame(pixels, width, height, origin, translation)
            path = output / f"vista0-camera-x{x}.png"
            path.write_bytes(encode_png(WIDTH, HEIGHT, frame))
            print(f"L=0, X={x}, X_vista={translation} → {path}")
    except (OSError, ValueError, struct.error) as error:
        print(f"Erro ao gerar prévias de câmera: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

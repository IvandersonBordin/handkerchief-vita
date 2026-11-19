"""Prepara a camada VISTA_0 para uso local no visualizador Vita."""
from render_cearth import ROOT, render_pieces
from render_vista0 import load_vista_scene


def main():
    _, pieces, palette, rows = load_vista_scene()
    _, width, height, pixels = render_pieces(pieces, palette, rows)
    if (width, height) != (4464, 384):
        raise ValueError("Dimensões de VISTA_0 incompatíveis com esta versão")
    for i in range(0, len(pixels), 4):
        if pixels[i + 3] == 0:
            pixels[i:i + 4] = b"\x4c\x70\x83\xff"
    output = ROOT / "build" / "vista-walk" / "vista0.rgba"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(pixels)
    print(output)


if __name__ == "__main__":
    main()

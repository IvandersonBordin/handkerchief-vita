"""Gera catálogos locais dos recortes indexados dos bancos gráficos 5 a 8."""

import argparse
import hashlib
import struct
import sys

from render_cearth import ROOT, encode_png, read_bank, read_bitmap


BANKS = (
    (5, "HDKCHRA0.BMP", 0x48E070, 0x48F700),
    (6, "HDKCHRB0.BMP", 0x48F700, 0x490BB0),
    (7, "HDKCHRC0.BMP", 0x490BB0, 0x4911D0),
    (8, "HDKCHRD0.BMP", 0x4911D0, 0x4913B8),
)
PAGE_SIZE = 1024
GAP = 4
LABEL_HEIGHT = 9
# Algarismos de 3 × 5 pixels para identificar os índices sem fontes externas.
DIGITS = (
    "75557", "26227", "71747", "71717", "55711",
    "74717", "74757", "71111", "75757", "75717",
)


def valid_rectangle(rectangle, bmp_width, bmp_height):
    x, y, width, height = rectangle
    return (x >= 0 and y >= 0 and width > 0 and height > 0
            and x + width <= bmp_width and y + height <= bmp_height)


def paginate(entries):
    """Distribui em prateleiras, mantendo a ordem dos índices."""
    x = y = GAP
    shelf = 0
    page = []
    for index, rectangle in entries:
        width, height = rectangle[2:]
        cell_width = max(width, 4 * len(str(index)) - 1) + GAP
        cell_height = height + LABEL_HEIGHT + GAP
        if cell_width + GAP > PAGE_SIZE or cell_height + GAP > PAGE_SIZE:
            raise ValueError(f"índice {index}: recorte excede a página de {PAGE_SIZE} pixels")
        if x + cell_width > PAGE_SIZE:
            x, y, shelf = GAP, y + shelf, 0
        if y + cell_height > PAGE_SIZE:
            yield page
            x = y = GAP
            shelf = 0
            page = []
        page.append((index, rectangle, x, y))
        x += cell_width
        shelf = max(shelf, cell_height)
    if page:
        yield page


def draw_page(page, palette, rows, transparent_zero):
    """Copia cada recorte em escala 1:1 e desenha seu índice acima."""
    height = max(y + LABEL_HEIGHT + rect[3] + GAP for _, rect, _, y in page)
    pixels = bytearray(PAGE_SIZE * height * 4)
    colors = palette.copy()
    if transparent_zero:
        colors[0] = bytes(4)
    for index, (sx, sy, width, crop_height), x, y in page:
        for col, digit in enumerate(str(index)):
            for dy, mask in enumerate(DIGITS[int(digit)]):
                for dx, bit in enumerate((4, 2, 1)):
                    if int(mask) & bit:
                        pos = ((y + dy) * PAGE_SIZE + x + col * 4 + dx) * 4
                        pixels[pos:pos + 4] = b"\xff\xff\xff\xff"
        for dy, row in enumerate(rows[sy:sy + crop_height]):
            pos = ((y + LABEL_HEIGHT + dy) * PAGE_SIZE + x) * 4
            pixels[pos:pos + width * 4] = b"".join(colors[i] for i in row[sx:sx + width])
    return encode_png(PAGE_SIZE, height, pixels)


def catalog_bank(number, filename, table, limit, exe, original, output, transparent_zero):
    entries = read_bank(exe, table, limit)
    try:
        palette, rows = read_bitmap((original / "BITMAP" / filename).read_bytes())
    except (OSError, ValueError, struct.error) as error:
        raise ValueError(f"{filename}: {error}") from error

    seen = set()
    duplicates = 0
    valid = []
    invalid = 0
    for index, rectangle in enumerate(entries):
        if not valid_rectangle(rectangle, len(rows[0]), len(rows)):
            print(f"Banco {number}, índice {index}: recorte inválido {rectangle}")
            invalid += 1
            continue
        valid.append((index, rectangle))
        x, y, width, height = rectangle
        # Mesmo tamanho e mesmos índices de paleta, mesmo em posições distintas.
        digest = hashlib.sha256()
        for row in rows[y:y + height]:
            digest.update(row[x:x + width])
        key = (width, height, digest.digest())
        duplicates += key in seen
        seen.add(key)

    mode = "chave-zero" if transparent_zero else "opaco"
    pages = 0
    for pages, page in enumerate(paginate(valid), 1):
        path = output / f"characters-bank-{number}-{mode}-{pages:02d}.png"
        path.write_bytes(draw_page(page, palette, rows, transparent_zero))
        print(path.relative_to(ROOT))
    print(f"Banco {number}: {len(valid)} válidos, {invalid} inválidos, "
          f"{duplicates} possíveis duplicatas, {pages} página(s).")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--transparent-zero", action="store_true",
                        help="simula a chave de cor zero como hipótese")
    args = parser.parse_args()
    original = ROOT / "original-data"
    output = ROOT / "build"
    try:
        exe = (original / "HDKWIN.EXE").read_bytes()
        output.mkdir(exist_ok=True)
        for bank in BANKS:
            catalog_bank(*bank, exe, original, output, args.transparent_zero)
    except (OSError, ValueError, struct.error) as error:
        print(f"Erro ao gerar o catálogo: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

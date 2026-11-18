"""Confere os recursos sintéticos da LiveArea antes de gerar o VPK."""

from pathlib import Path
import struct
import sys
import xml.etree.ElementTree as ET
import zlib


def check_png(path, width, height, *, opaque=False):
    png = path.read_bytes()
    if len(png) < 33 or png[:16] != b"\x89PNG\r\n\x1a\n\0\0\0\rIHDR":
        raise ValueError(f"{path.name}: cabeçalho PNG inválido")
    if struct.unpack_from(">IIBB", png, 16) != (width, height, 8, 3):
        raise ValueError(f"{path.name}: esperado PNG indexado de 8 bits, {width} × {height}")
    if png[26:28] != b"\0\0" or png[28] not in (0, 1):
        raise ValueError(f"{path.name}: métodos de compressão, filtro ou entrelaçamento inválidos")

    offset = 8
    palette_entries = 0
    has_transparency = False
    image_data = None
    image_data_ended = False
    while offset + 12 <= len(png):
        size, kind = struct.unpack_from(">I4s", png, offset)
        if not kind.isalpha():
            raise ValueError(f"{path.name}: nome de bloco PNG inválido {kind!r}")
        if kind[2:3].islower():
            raise ValueError(f"{path.name}: terceira letra do bloco PNG deve ser maiúscula em {kind!r}")
        data_end = offset + 8 + size
        chunk_end = data_end + 4
        if chunk_end > len(png):
            break
        expected_crc = struct.unpack_from(">I", png, data_end)[0]
        if zlib.crc32(png[offset + 4:data_end]) != expected_crc:
            raise ValueError(f"{path.name}: CRC inválido no bloco {kind!r}")
        # No PNG, a primeira letra maiúscula identifica um bloco crítico.
        if kind[:1].isupper() and kind not in (b"IHDR", b"PLTE", b"IDAT", b"IEND"):
            raise ValueError(f"{path.name}: bloco crítico desconhecido {kind!r}")
        if kind == b"IHDR" and offset != 8:
            raise ValueError(f"{path.name}: bloco IHDR duplicado")
        if image_data is not None and kind != b"IDAT":
            image_data_ended = True
        if kind == b"PLTE":
            if palette_entries or not (0 < size <= 768 and size % 3 == 0):
                raise ValueError(f"{path.name}: paleta duplicada ou inválida")
            palette_entries = size // 3
        elif kind == b"tRNS":
            if opaque:
                raise ValueError(f"{path.name}: o ícone da LiveArea deve ser opaco")
            if has_transparency or image_data is not None or not 0 < size <= palette_entries:
                raise ValueError(f"{path.name}: bloco tRNS duplicado, fora de ordem ou inválido")
            has_transparency = True
        elif kind == b"IDAT":
            if image_data_ended:
                raise ValueError(f"{path.name}: blocos IDAT devem ser consecutivos")
            if not palette_entries:
                raise ValueError(f"{path.name}: dados da imagem sem paleta válida")
            if image_data is None:
                image_data = bytearray()
            image_data.extend(png[offset + 8:data_end])
        elif kind == b"IEND":
            if size != 0 or chunk_end != len(png) or not image_data:
                raise ValueError(f"{path.name}: final do PNG inválido")
            check_image_data(path, image_data, width, height, png[28])
            return
        offset = chunk_end

    raise ValueError(f"{path.name}: PNG truncado ou sem bloco IEND")


def check_image_data(path, compressed, width, height, interlaced):
    passes = [(0, 0, 1, 1)]
    if interlaced:
        # Cada passagem Adam7 seleciona pixels com origem e passo próprios.
        passes = [
            (0, 0, 8, 8), (4, 0, 8, 8), (0, 4, 4, 8), (2, 0, 4, 4),
            (0, 2, 2, 4), (1, 0, 2, 2), (0, 1, 1, 2),
        ]

    rows = []
    for x, y, x_step, y_step in passes:
        columns = len(range(x, width, x_step))
        if columns:
            rows.extend([columns + 1] * len(range(y, height, y_step)))

    expected_size = sum(rows)
    decoder = zlib.decompressobj()
    try:
        decoded = decoder.decompress(compressed, expected_size + 1)
    except zlib.error as error:
        raise ValueError(f"{path.name}: compressão IDAT inválida") from error
    if len(decoded) != expected_size or not decoder.eof or decoder.unused_data:
        raise ValueError(f"{path.name}: fluxo IDAT incompleto, excedente ou com tamanho incorreto")

    offset = 0
    for row_size in rows:
        if decoded[offset] > 4:
            raise ValueError(f"{path.name}: filtro de linha PNG inválido")
        offset += row_size


def main(root):
    livearea = root / "sce_sys/livearea/contents"
    images = {
        root / "sce_sys/icon0.png": (128, 128),
        livearea / "bg.png": (840, 500),
        livearea / "startup.png": (280, 158),
    }
    try:
        for path, (width, height) in images.items():
            check_png(path, width, height, opaque=path.name == "icon0.png")

        template = ET.parse(livearea / "template.xml").getroot()
        if template.tag != "livearea":
            raise ValueError("template.xml: esperado elemento raiz livearea")
        for section, element, filename in (
            ("livearea-background", "image", "bg.png"),
            ("gate", "startup-image", "startup.png"),
        ):
            sections = template.findall(section)
            references = sections[0].findall(element) if len(sections) == 1 else []
            if len(references) != 1 or references[0].text != filename or len(references[0]):
                raise ValueError(f"template.xml: esperado um único {section}/{element} com {filename}")
    except (OSError, ValueError, ET.ParseError) as error:
        print(error, file=sys.stderr)
        return 1

    print("PNGs, opacidade do ícone e referências do XML conferidos.")
    return 0


if __name__ == "__main__":
    sys.exit(main(Path(__file__).resolve().parent))

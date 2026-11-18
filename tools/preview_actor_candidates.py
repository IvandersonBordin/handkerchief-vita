"""Gera folhas locais de candidatos visuais, com rótulos de banco e índice."""

import struct
import sys

from preview_characters import BANKS, draw_page, paginate, valid_rectangle
from render_cearth import ROOT, read_bank, read_bitmap


# Intervalos inclusivos, com os índices originais das tabelas.
SELECTIONS = {
    5: ((129, 140), (157, 168)),
    6: ((1, 20), (32, 44)),
}


def select_entries(number, entries, width, height):
    for first, last in SELECTIONS[number]:
        for index in range(first, last + 1):
            if not 0 <= index < len(entries):
                raise ValueError(f"banco {number}, índice {index}: fora da tabela")
            rectangle = entries[index]
            if not valid_rectangle(rectangle, width, height):
                raise ValueError(f"banco {number}, índice {index}: recorte inválido {rectangle}")
            yield f"{number} {index}", rectangle


def main():
    original = ROOT / "original-data"
    output = ROOT / "build"
    try:
        if output.resolve() != output:
            raise ValueError("build/ deve ser um diretório local do projeto")
        exe = (original / "HDKWIN.EXE").read_bytes()
        output.mkdir(exist_ok=True)
        for number, filename, table, limit in BANKS:
            if number not in SELECTIONS:
                continue
            entries = read_bank(exe, table, limit)
            palette, rows = read_bitmap((original / "BITMAP" / filename).read_bytes())
            selected = select_entries(number, entries, len(rows[0]), len(rows))
            for page_number, page in enumerate(paginate(selected), 1):
                for mode, transparent in (("opaco", False), ("chave-zero", True)):
                    path = output / f"actors-bank-{number}-{mode}-{page_number:02d}.png"
                    if path.is_symlink():
                        raise ValueError(f"saída é um link simbólico: {path.name}")
                    path.write_bytes(draw_page(page, palette, rows, transparent))
                    print(path.relative_to(ROOT))
    except (OSError, ValueError, struct.error) as error:
        print(f"Erro ao gerar as folhas: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

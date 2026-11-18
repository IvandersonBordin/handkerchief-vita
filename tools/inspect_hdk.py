"""Lista os cabeçalhos de seção de um arquivo HDK."""

import argparse
from pathlib import Path


def _iter_section_bounds(data):
    if not data:
        raise ValueError("arquivo vazio")

    offset = 0
    while offset < len(data):
        header = f"bloco em 0x{offset:04X}"
        name_start = offset + 4
        if name_start > len(data):
            raise ValueError(f"{header}: campo de comprimento truncado")

        # O comprimento inclui os quatro bytes do próprio campo.
        length = int.from_bytes(data[offset:name_start], "little")
        if length < 6:
            raise ValueError(f"{header}: comprimento inválido ({length} bytes)")

        end = offset + length
        if end > len(data):
            raise ValueError(f"{header}: bloco truncado (fim previsto em 0x{end:04X})")

        # Procura o byte zero só dentro deste bloco.
        name_end = data.find(b"\x00", name_start, end)
        if name_end == -1:
            raise ValueError(f"{header}: nome da seção sem terminador")

        name = data[name_start:name_end]
        if not name or not all(0x21 <= byte <= 0x7E for byte in name):
            raise ValueError(f"{header}: nome da seção inválido")

        yield offset, name.decode("ascii"), name_end + 1, end
        offset = end


def iter_sections(data):
    """Encontra as seções seguindo os comprimentos gravados no arquivo."""
    for offset, name, _, _ in _iter_section_bounds(data):
        yield offset, name


def _iter_section_class_bounds(data):
    for offset, name, class_start, end in _iter_section_bounds(data):
        class_end = data.find(b"\x00", class_start, end)
        if class_end == -1:
            raise ValueError(f"bloco em 0x{offset:04X}: nome da classe sem terminador")

        class_name = data[class_start:class_end]
        if not class_name or not all(0x21 <= byte <= 0x7E for byte in class_name):
            raise ValueError(f"bloco em 0x{offset:04X}: nome da classe inválido")

        yield offset, name, class_name.decode("ascii"), class_end + 1, end


def iter_section_classes(data):
    """Lista as classes de seção e o primeiro deslocamento após cada classe."""
    for offset, name, class_name, data_start, _ in _iter_section_class_bounds(data):
        yield offset, name, class_name, data_start


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("arquivo", type=Path, help="caminho do arquivo HDK")
    parser.add_argument(
        "--classes", action="store_true",
        help="inclui a classe e o deslocamento dos dados após o cabeçalho",
    )
    parser.add_argument(
        "--bounds", action="store_true",
        help="inclui o fim exclusivo e o comprimento de cada bloco",
    )
    args = parser.parse_args()

    try:
        inspect = iter_sections
        if args.classes:
            inspect = _iter_section_class_bounds
        elif args.bounds:
            inspect = _iter_section_bounds
        sections = list(inspect(args.arquivo.read_bytes()))
    except (OSError, ValueError) as error:
        parser.exit(1, f"Erro ao inspecionar {args.arquivo}: {error}\n")

    if args.classes:
        for offset, name, class_name, data_start, end in sections:
            line = f"0x{offset:04X} {name} {class_name} dados=0x{data_start:04X}"
            if args.bounds:
                line += f" fim=0x{end:04X} comprimento={end - offset}"
            print(line)
    elif args.bounds:
        for offset, name, _, end in sections:
            print(f"0x{offset:04X} {name} fim=0x{end:04X} comprimento={end - offset}")
    else:
        for offset, name in sections:
            print(f"0x{offset:04X} {name}")


if __name__ == "__main__":
    main()

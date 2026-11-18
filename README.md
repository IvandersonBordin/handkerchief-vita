# Handkerchief para PS Vita

Projeto de pesquisa para um port nativo de Handkerchief para PlayStation
Vita. O estágio atual permite inspecionar seções HDK e gerar prévias
gráficas em computador. A viabilidade no console segue em avaliação.

## Usar as ferramentas

É necessário Python 3 e uma cópia própria dos arquivos originais. A
partir da raiz do repositório, mantenha `A0.HDK` em
`original-data/SYSTEM/`, `HDKWIN.EXE` em `original-data/` e
`HDKMAPA0.BMP` em `original-data/BITMAP/`. Os nomes diferenciam
maiúsculas de minúsculas no Linux.

```sh
python3 -B tools/inspect_hdk.py original-data/SYSTEM/A0.HDK
python3 -B tools/render_cearth.py
python3 -B tools/render_vista0.py
```

O primeiro comando lista as seções. O segundo gera
`build/cearth-preview.png`, com 760 × 256 pixels. O terceiro gera
`build/vista0-preview.png` e `build/vista0-debug.png`, ambos com
4464 × 384 pixels, a partir de seis objetos `CEarth` e 32 elementos.
A imagem de depuração usa magenta nas áreas transparentes.

As prévias usam a paleta original do BMP, ajuste gráfico zero e a
ordem dos registros no arquivo. As ferramentas abrem os originais
somente para leitura e usam apenas a biblioteca padrão do Python.
Os originais e as imagens geradas ficam fora do Git.

## Documentação

- [Formato HDK](docs/hdk-format.md)
- [Mapa técnico do motor](docs/engine-map.md)
- [Validação e publicação](AGENTS.md#validação-e-publicação-automática)

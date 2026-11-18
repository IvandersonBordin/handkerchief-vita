# Continuidade — bruno

## Tarefa atual
B01 — inventário de formatos, recursos e evidências. Validação com a
cópia original pendente neste worktree.

## Evidências já verificadas
- [Formato HDK](hdk-format.md#validação-do-conjunto-de-arquivos) registra
  a análise anterior de 108 arquivos. Dos 99 com conteúdo, 98 passaram
  na leitura de classes. `USER01.HDK` passou na validação dos limites,
  mas apresentou campos incompatíveis com a leitura de classes.
- [Mapa técnico](engine-map.md) reúne a análise estática anterior do
  executável PE32 x86, dos bancos de dimensões e da associação do banco 2
  com o BMP indexado `HDKMAPA0.BMP`.
- [Identificação de personagens](character-identification.md#material-local)
  registra o AVI usado como referência visual e as lacunas na identificação.

## Último ciclo
18/11/2026 — revisados o inventário histórico e a cobertura atual da
branch `agent/reverse-engineering`. A suíte local passou com 31 testes
(`PYTHONPATH=tools python3 -B -m unittest discover -s tests -v`), cobrindo
leitura de seções HDK, limites de bancos, leitura de BMP, retângulos e
composição de câmera com dados sintéticos. A análise histórica dos
arquivos comerciais permanece como evidência documental.

## Próxima ação
Com a cópia original disponível em modo somente leitura, conferir a
cobertura do inventário B01 e reproduzir as contagens de limites e classes
com `tools/inspect_hdk.py`. Registrar divergências antes de iniciar B02.

## Bloqueios
- `original-data/` está ausente deste worktree, o que impede conferir o
  inventário diretamente sobre a cópia original neste ciclo.
- A consulta de issues abertas pelo GitHub CLI retornou falta de
  autenticação. O plano local não indica um número de issue para B01.

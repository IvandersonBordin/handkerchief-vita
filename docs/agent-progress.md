# Continuidade — carlos

## Tarefa atual
C01 — CONCLUIDA. A compilação ARM com VitaSDK e os dois verificadores
passaram sobre o VPK gerado neste ciclo.

C02 — PENDENTE. A instalação e a execução no console físico aguardam
confirmação humana.

## Evidências já verificadas
O protótipo independente e o roteiro de console já estão em
`vita-prototype/` e `docs/vita-smoke-test.md`. Os recursos sintéticos da
LiveArea passaram em `python3 vita-prototype/check_livearea.py`.

## Último ciclo
Em 18/11/2026, a configuração Release e a compilação com CMake 4.4.4
usaram o VitaSDK em `/home/ivanderson/vitasdk` e o compilador
`arm-vita-eabi-gcc` 15.2.0. O comando `file` identificou o executável
como ELF de 32 bits ARM EABI5.

- Pacote local: `/tmp/handkerchief-c01.CXQeJk/handkerchief_smoke.vpk` (17.992 bytes).
- SHA-256: `90eccd5f0ff9e51a97ee5709a6efbc47d83564829590940aa9e2f5adcbf55757`.
- `check_livearea.py` conferiu os recursos locais e as seis entradas do VPK.
- `check_vpk.py` retornou zero erros e zero avisos, com ID `HKSM00001`
  e versão `01.00`.

A consulta ao GitHub encontrou a issue de teste da automação, que tem
escopo próprio. Nenhuma issue aberta específica para C01 foi localizada.

## Ciclo anterior
O plano registra uma compilação manual anterior com VitaSDK 2026.08.
Nesta sessão, `/home/ivanderson/vitasdk` e
`/home/ivanderson/Projetos/handkerchief-vita-build-manual` estavam ausentes.
Em uma pasta temporária, a configuração Release com
`VITASDK=/home/ivanderson/vitasdk` parou antes da compilação porque
`share/vita.toolchain.cmake` estava ausente. A busca local também não
encontrou o compilador nem `handkerchief_smoke.vpk` nas áreas acessíveis.
A consulta ao GitHub não encontrou uma issue aberta específica para C01.
A compilação manual registrada no plano continua sem reprodução nesta sessão
e os dois verificadores ainda precisam ser executados sobre o VPK real.

## Checagem anterior de disponibilidade
Na nova checagem, `VITASDK` continuou ausente, e `arm-vita-eabi-gcc`
não apareceu no PATH. A busca em `/opt`, `/usr/local`, `/home/ivanderson`
e `/tmp` não localizou o compilador nem `vita.toolchain.cmake`.
A consulta às issues abertas do GitHub não encontrou uma issue específica
para C01. O ciclo ficou restrito a verificar os pré-requisitos e registrar
o resultado. Compilação ARM e validação do VPK seguem pendentes.

## Checagem anterior da toolchain
Em 2026-11-18, o CMake passou a exigir o indicador `VITA`, definido pela
[toolchain oficial](https://github.com/vitasdk/vita-toolchain/blob/master/cmake_toolchain/vita.toolchain.cmake),
antes de carregar as macros de empacotamento. Em diretórios temporários,
a configuração sem SDK exibiu o diagnóstico esperado. Uma toolchain Linux
simulada que definia apenas `VITASDK` foi rejeitada. Com `VITA` definido,
a simulação avançou até a checagem de macros e relatou o arquivo ausente.
Essas verificações exercitaram apenas a configuração do CMake.

A busca nas issues abertas do GitHub não encontrou uma issue atribuída à C01.

## Próxima ação
Executar o roteiro de console em `docs/vita-smoke-test.md` durante C02.
Confirmar a instalação, a abertura pela LiveArea, as cores, a resposta
ao botão X, a saída com START e a reabertura.

## Bloqueios
C02 requer teste em hardware com confirmação humana.

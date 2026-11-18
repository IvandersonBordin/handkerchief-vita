# Continuidade — carlos

## Tarefa atual
C01 — BLOQUEADA. A conclusão exige compilação ARM com VitaSDK e validação
do VPK gerado.

## Evidências já verificadas
O protótipo independente e o roteiro de console já estão em
`vita-prototype/` e `docs/vita-smoke-test.md`. Os recursos sintéticos da
LiveArea passaram em `python3 vita-prototype/check_livearea.py`.

## Último ciclo
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

## Ciclo anterior
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
Quando houver um VitaSDK disponível, executar a configuração, a compilação
e os dois verificadores do VPK descritos em `docs/vita-smoke-test.md`.
Depois de concluir C01, seguir para C02 conforme a condição do plano.

## Bloqueios
`VITASDK` estava ausente do ambiente e o compilador `arm-vita-eabi-gcc`
não foi localizado no PATH nem nos diretórios consultados. Compilação ARM,
geração e verificação do VPK seguem pendentes. C02 requer teste em hardware
com confirmação humana.

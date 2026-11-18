# Continuidade — carlos

## Tarefa atual
C01 — BLOQUEADA. A conclusão exige compilação ARM com VitaSDK e validação
do VPK gerado.

## Evidências já verificadas
O protótipo independente e o roteiro de console já estão em
`vita-prototype/` e `docs/vita-smoke-test.md`. Os recursos sintéticos da
LiveArea passaram em `python3 vita-prototype/check_livearea.py`.

## Último ciclo
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

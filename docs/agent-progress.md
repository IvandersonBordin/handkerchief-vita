# Continuidade — carlos

## Tarefa atual
C01 — CONCLUIDA. A compilação ARM com VitaSDK e os dois verificadores
passaram sobre o VPK gerado neste ciclo.

C02 — teste básico confirmado no console pelo usuário, conforme o registro
abaixo. O encerramento completo com START segue sem confirmação.

C03 — EM ANDAMENTO. O loop tem entrada, pausa e contagem de tempo ativo
validadas localmente. Falta verificar o comportamento do novo pacote no
console e confirmar o encerramento completo com START.

## Ciclo C03 — registro das observações no console

- O roteiro ganhou um modelo para registrar a pausa, a retomada pelo
  botão PS, a reabertura após START, o estado do processo e os tempos
  das sessões com e sem pausa. O SHA-256 esperado está junto ao roteiro.
- `sha256sum` confirmou que o VPK local coincide com o valor do roteiro.
  `git check-ignore` confirmou que o pacote é ignorado. Foram reaproveitados
  o executável e a validação de estrutura do ciclo do relógio.
- A consulta à issue #4 e a busca de issues abertas retornaram o roteiro
  geral e nenhuma tarefa específica de C03. O teste físico do pacote do
  relógio continua pendente.

## Ciclo C03 — falhas durante a saída com START

- Uma simulação temporária no Linux compilou `vita-prototype/main.c` com
  avisos tratados como erros e UBSan. A saída normal retornou 0. Falhas
  simuladas ao desvincular a tela, aguardar a sincronização ou liberar a
  memória retornaram 1.
- A simulação verificou a ordem das chamadas e confirmou que o aplicativo
  só tenta liberar o bloco após desvincular a tela e sincronizar com sucesso.
  A recuperação do bloco pelo sistema e a saída completa com START ainda
  dependem de observação no PS Vita.
- A busca por issues abertas não identificou uma tarefa específica para
  C03. O código e o VPK do relógio foram reaproveitados neste ciclo.

## Ciclo C03 — caminho de encerramento no executável ARM

- A inspeção com `arm-vita-eabi-objdump -d` do ELF local
  `build/vita-smoke-c03-clock/handkerchief_smoke` mostrou que `_start`
  chama `main` e encaminha seu retorno para `exit`. A função `exit`
  chama `_exit`, que por sua vez chama `sceKernelExitProcess`.
- Essa evidência confirma o caminho de encerramento no executável
  compilado. Ainda falta observar no console se START chega a esse
  caminho e se o processo desaparece antes da reabertura.
- O código e o pacote foram reaproveitados. A consulta à issue #4
  e a busca de issues abertas não encontraram uma tarefa específica
  para C03. Os comandos `vita3k`, `vitacompanion` e `vita-agent-use`
  não estavam no PATH.

## Ciclo C03 — preparar a verificação de START

- O roteiro em `docs/vita-smoke-test.md` identifica o VPK do relógio e
  usa a retomada via botão PS para verificar se a sessão suspensa mantém
  a pausa. O roteiro compara essa observação com a reabertura após START
  e pede evidência do término do processo, se houver diagnóstico disponível.
- O SHA-256 do VPK local coincide com o do ciclo do relógio.
  `git check-ignore` confirmou que o pacote é ignorado. A compilação ARM
  e a validação da estrutura do pacote permanecem as do ciclo anterior.
- A consulta ao GitHub encontrou o roteiro geral na issue #4 e nenhuma
  issue específica para este ciclo do C03. O teste físico do pacote do
  relógio continua pendente, assim como a captura de sua saída padrão.

## Ciclo C03 — relógio do loop (18/11/2026)

- O loop mede o intervalo entre atualizações da tela com o relógio de
  64 bits do VitaSDK. Cada intervalo pertence ao estado anterior à leitura
  dos botões. START registra o tempo ativo acumulado na saída padrão.
- A compilação Release ARM com GCC 15.2.0 e avisos tratados como erros
  gerou `build/vita-smoke-c03-clock/handkerchief_smoke.vpk`. Os dois
  verificadores do pacote passaram com seis entradas, zero erros e zero
  avisos para o ID `HKSM00001`, versão `01.00`. O Git ignora o pacote.
- SHA-256 do VPK:
  `a400b9671e9fbda265dc6a2e0209d8fae11e6335299b72d43559f29255aefa43`.
- A simulação no Linux com UBSan confirmou 105 µs ativos usando intervalos
  irregulares, leitura sem amostra, pausa e retomada. A saída com START em
  pausa registrou 10 µs ativos. Nos dois casos, a liberação dos quadros
  ocorreu após desvincular a tela e esperar a sincronização.
- A consulta às issues abertas não encontrou uma issue específica para
  C03. O ciclo seguiu o plano local. A saída de diagnóstico e o novo VPK
  ainda aguardam teste no console.

## Ciclo C03 — pausa do protótipo (18/11/2026)

- Compilação Release com VitaSDK e GCC ARM 15.2.0 em `build/vita-smoke-c03/`.
- `check_livearea.py` e `check_vpk.py` passaram. O segundo retornou seis
  entradas, zero erros e zero avisos para o ID `HKSM00001`, versão `01.00`.
- O VPK local é ignorado pelo Git. SHA-256
  `83c680a6327d3c5a56a20b0078addbb1fa5c297e2d140a990ec935280d6f338c`.
- A simulação temporária no Linux com avisos como erros e UBSan confirmou
  retenção da cor durante a pausa, ausência de repetição ao segurar Círculo,
  retomada após novo toque e liberação dos quadros ao sair com START em pausa.
  Uma leitura sem amostra entre duas leituras com Círculo pressionado também
  preservou a pausa.
- A busca de issues abertas pelo conector do GitHub não encontrou uma issue
  específica para C03. O trabalho seguiu o plano local.
- Próximos passos de C03 incluem acrescentar o relógio do loop e verificar
  a pausa e o encerramento no console com o novo pacote.

## Evidências já verificadas
O protótipo independente e o roteiro de console já estão em
`vita-prototype/` e `docs/vita-smoke-test.md`. Os recursos sintéticos da
LiveArea passaram em `python3 vita-prototype/check_livearea.py`.

## Ciclo anterior ao teste físico
Em 18/11/2026, `sha256sum` confirmou que o VPK em
`build/vita-smoke/handkerchief_smoke.vpk` tem o mesmo SHA-256 registrado
abaixo. `git check-ignore` confirmou que o Git ignora o pacote. O ciclo
reaproveitou a compilação e os resultados dos verificadores desse arquivo.
A consulta às issues pelo `gh` exigiu autenticação, e a consulta à página
web também falhou. C02 segue pendente do teste físico e C03 depende de C02.

## Ciclo de preparação do VPK para C02
Em 18/11/2026, para preparar C02, os dois caminhos de VPK registrados
anteriormente foram verificados e estavam indisponíveis. Um novo pacote
Release foi compilado com o VitaSDK em `/home/ivanderson/vitasdk` e
`arm-vita-eabi-gcc` 15.2.0.

- Pacote local ignorado pelo Git: `build/vita-smoke/handkerchief_smoke.vpk`.
- SHA-256: `62daa4021a17e95fbda1747e7101e1dc50aaa37e403f4e2c02ada3917528a6d0`.
- `check_livearea.py` conferiu as seis entradas e os recursos sintéticos.
- `check_vpk.py` retornou zero erros e zero avisos, com ID `HKSM00001`
  e versão `01.00`.

A consulta às issues pelo `gh` ficou bloqueada por falta de autenticação.
C02 segue pendente de execução no console e confirmação humana.

## Conclusão de C01
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

## Próxima ação registrada antes do teste físico
Executar o roteiro de console em `docs/vita-smoke-test.md` durante C02.
Confirmar a instalação, a abertura pela LiveArea, as cores, a resposta
ao botão X, a saída com START e a reabertura.

## Bloqueio registrado antes do teste físico
C02 requer teste em hardware com confirmação humana.

## Teste no PS Vita físico — 18/11/2026

O usuário instalou e executou o handkerchief_smoke.vpk
no PS Vita real.

Resultados confirmados:
- Instalação e inicialização: OK.
- Três retângulos verticais: vermelho, verde e azul: OK.
- Botão X altera o retângulo verde para amarelo: OK.
- Retorno à LiveArea e retomada do aplicativo: OK.

Observação:
O usuário relatou que START minimiza o aplicativo.
O encerramento completo pelo START não foi confirmado.

C02: teste básico de hardware validado.
Próxima tarefa: C03, desenvolver o loop de jogo e os
controles, seguindo docs/agent-plan.md.

Não confundir o protótipo com um port jogável do
Handkerchief. Não incluir arquivos comerciais no Git.

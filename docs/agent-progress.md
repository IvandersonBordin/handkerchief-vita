# Continuidade — carlos

## Tarefa atual
C01 — CONCLUIDA. A compilação ARM com VitaSDK e os dois verificadores
passaram sobre o VPK gerado neste ciclo.

C02 — CONCLUIDA conforme a issue #4, que registra a confirmação manual
da abertura do app e da resposta visual ao botão X no PS Vita.

C03 — CONCLUIDA pelo aceite funcional da issue #4. No PS Vita, Círculo
pausou o loop, X manteve o quadro durante a pausa e, após outro toque
em Círculo, X voltou a alternar verde e amarelo. Na demo com sprite,
START retornou à LiveArea com “Iniciar” e a reabertura restaurou o
estado inicial. A listagem de processos e a saída em `tty0:`
não foram observadas.

C04 — EM ANDAMENTO. A demo `71ed7a2` tem revisão local aprovada e
confirmação física da animação branco/magenta, do deslocamento para
a direita e da pausa e retomada da animação. Faltam esquerda, cima,
baixo e os limites da tela no console.

## Ciclo de aceite funcional de C03 e validação parcial de C04

- A issue #4 registra instalação e abertura da demo e a coincidência dos
  seis arquivos instalados com o pacote revisado. Ivan confirmou
  visualmente o quadrado à direita do centro, alternando entre branco e
  magenta, e depois confirmou a pausa e a retomada com Círculo.
  [Animação e direita](https://github.com/IvandersonBordin/handkerchief-vita/issues/4#issuecomment-6994712053)
  [Pausa e retomada](https://github.com/IvandersonBordin/handkerchief-vita/issues/4#issuecomment-6994730047)
- Após START, Ivan viu “Iniciar” na LiveArea antes da reabertura. Ao
  reabrir sem tocar no direcional, viu o quadrado no centro e animado.
  O líder aceitou esse ciclo funcional. O término do processo segue
  sem observação direta.
  [Aceite funcional de START](https://github.com/IvandersonBordin/handkerchief-vita/issues/4#issuecomment-6994757728)
- `sha256sum` confirmou que o VPK reservado coincide com o hash da
  revisão anterior. O ciclo atual reaproveitou o pacote e as
  validações aprovadas. O próximo teste físico de C04 deve cobrir os
  três sentidos restantes e os limites da tela. C05 aguarda o aceite
  de C04 e os insumos de B02 e B03.

## Ciclo de revisão C03 e C04

- A [revisão de `71ed7a2` na issue #4](https://github.com/IvandersonBordin/handkerchief-vita/issues/4#issuecomment-6994613694)
  aprovou a integração local para teste físico. O líder reproduziu os
  testes Linux com UBSan e a compilação ARM com VitaSDK e `-Werror`.
  O VPK reservado e o recompilado passaram nos dois verificadores.
- O pacote reservado é `build/vita-smoke-c04-sprite/handkerchief_smoke.vpk`,
  SHA-256 `ba5c1ae9ca28b46a979002b5732871db2979d1a7dde0140dd25ad8bd58a0a0ec`.
  O roteiro físico deve confirmar a animação branco/magenta, o movimento
  nas quatro direções e os limites da tela, a pausa e a retomada, e o
  fim efetivo do processo após START.
- Este ciclo registrou a revisão e conferiu o diff da documentação.
  C03 aguarda evidência física de START. C04 está aprovada localmente
  e aguarda o roteiro no console. C05 aguarda o aceite das etapas
  anteriores e os insumos de B02 e B03.

## Ciclo C04 — sprite integrado ao protótipo

- O comentário da issue #4 autorizou a integração local a partir de
  `b4ec4e0` enquanto C03 aguarda confirmação física do encerramento
  por START. C04 tem compilação e validação local, com teste físico pendente.
- `vita-prototype/CMakeLists.txt` inclui `sprite.c`. O loop em
  `vita-prototype/main.c` desenha um quadrado de 16 × 16 pixels, branco ou
  magenta, em quadros de 250 ms de tempo ativo. O direcional desloca o
  quadrado quatro pixels por atualização, com limites da tela. O loop
  recompõe o fundo no buffer livre sem alocar memória por quadro.
- `vita-prototype/test_loop.c` passou nos quatro cenários no Linux com
  avisos como erros e UBSan, incluindo a verificação de todos os pixels,
  deslocamento, pausa, retomada e limpeza após START. O relógio
  simulado registrou 1.200.000 µs nos dois cenários que chegam a START.
  Os comandos estão em `docs/vita-smoke-test.md`. A primeira tentativa
  usou `-isystem` e falhou na ligação porque selecionou o `assert.h`
  do VitaSDK. O comando com `-idirafter` passou.
- VitaSDK com GCC 15.2.0 gerou um ELF ARM de 32 bits, EABI5. Os dois
  verificadores aceitaram o VPK `HKSM00001` com seis entradas.
  Para reproduzir na raiz do worktree:

  ```sh
  export VITASDK=/home/ivanderson/vitasdk
  export PATH="$VITASDK/bin:$PATH"
  cmake -S vita-prototype -B build/vita-smoke-c04-sprite \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS=-Werror &&
  cmake --build build/vita-smoke-c04-sprite &&
  python3 vita-prototype/check_livearea.py build/vita-smoke-c04-sprite/handkerchief_smoke.vpk &&
  python3 .agents/skills/vita-port/scripts/check_vpk.py build/vita-smoke-c04-sprite/handkerchief_smoke.vpk &&
  sha256sum build/vita-smoke-c04-sprite/handkerchief_smoke.vpk
  ```

- SHA-256 do pacote local
  `ba5c1ae9ca28b46a979002b5732871db2979d1a7dde0140dd25ad8bd58a0a0ec`.
  A publicação do resumo na issue #4 exigiu aprovação indisponível
  nesta sessão. O próximo passo é confirmar o fim do processo após
  START no console e, com autorização para teste físico, verificar o
  roteiro do pacote com sprite. Alterações locais para revisão.

## Preparação C04 — seleção de quadro por tempo ativo

- `vita-prototype/sprite.c` e `sprite.h` agora selecionam o quadro de
  uma animação circular com duração uniforme por quadro. A seleção usa
  microssegundos de tempo ativo e retorna zero se a duração ou a
  quantidade de quadros for zero.
- `vita-prototype/test_sprite_animation.c` passou em dez verificações
  no Linux com GCC 16.2.1, avisos como erros e UBSan. O teste cobre as
  transições, a volta ao primeiro quadro e o limite de 64 bits.
  Para reproduzir a partir da raiz:

  ```sh
  dest=$(mktemp -d /tmp/hksm-animation-linux.XXXXXX) &&
    gcc -std=c11 -Wall -Wextra -Werror -pedantic -fsanitize=undefined \
      -fno-sanitize-recover=all vita-prototype/sprite.c \
      vita-prototype/test_sprite_animation.c -o "$dest/test" &&
    "$dest/test"
  ```

- VitaSDK com GCC 15.2.0 compilou o módulo com avisos como erros.
  `file` identificou um objeto ELF ARM de 32 bits, EABI5.
- A issue #4 continua sem confirmação física do encerramento por START.
  A integração da animação ao VPK fica para depois do aceite de C03.
- O conector do GitHub exigiu aprovação para publicar o resumo na
  issue #4. A política desta sessão não permite essa aprovação.

## Preparação C04 — desenho de sprite sintético

- `vita-prototype/sprite.c` e `sprite.h` copiam pixels A8B8G8R8 com
  alfa diferente de zero para um buffer fornecido pelo chamador. A função
  respeita o passo de cada imagem, recorta nas bordas e usa os buffers
  existentes. O módulo ainda aguarda integração ao smoke test.
- `vita-prototype/test_sprite.c` passou em sete casos de pixels no Linux,
  incluindo transparência, recorte, preenchimento de linha e coordenadas
  extremas. GCC 16.2.1 tratou avisos como erros e executou ASan e UBSan.
  O rastreamento da sessão impediu o LeakSanitizer, então a segunda
  execução usou `ASAN_OPTIONS=detect_leaks=0`.
- VitaSDK com GCC 15.2.0 compilou o módulo com avisos como erros. O
  comando `file` identificou um objeto ELF ARM de 32 bits, EABI5. O
  pacote de C03 foi reaproveitado. Este ciclo não executou o console.
- A issue #4 foi consultada. O envio do resumo foi bloqueado porque
  o conector exige aprovação indisponível na sessão.
- Para fechar C03, falta confirmar no PS Vita que START encerra o
  processo antes da reabertura. O próximo passo independente de C04 é
  testar a seleção de quadros de uma animação sintética no Linux.

## Ciclo C03 — falha ao desvincular o quadro após START

- O teste permanente em `vita-prototype/test_loop.c` simula a recusa da
  tela em desvincular o quadro após START. Exige retorno 1, ausência
  de nova espera por sincronização e preservação do bloco de vídeo.
  O roteiro em `docs/vita-smoke-test.md` inclui o resultado esperado.
- GCC 16.2.1 no Linux, com cabeçalhos do VitaSDK, avisos como erros e
  UBSan, aprovou os quatro cenários. Uma cópia temporária que ignorava
  a falha foi rejeitada ao tentar liberar o bloco. As duas execuções
  que chegaram a START registraram 120 µs simulados.
- O executável e o VPK anteriores foram reaproveitados. O próximo
  ciclo pode preparar o código de sprites sintéticos da C04 sem
  antecipar sua entrega, conforme a issue #4. O aceite de C03
  aguarda a confirmação de término do processo com START no console.
- A issue #4 foi consultada. A publicação do resumo foi bloqueada
  porque a ferramenta do GitHub exige aprovação indisponível na sessão.

## Ciclo C03 — falha ao obter o endereço da memória de vídeo

- O teste Linux força uma falha na consulta do endereço do bloco já
  alocado. Confere retorno 1, liberação do bloco e ausência de chamadas
  à tela, ao relógio e ao controle. O roteiro local descreve o cenário.
- GCC 16.2.1 com cabeçalhos do VitaSDK, avisos como erros e UBSan
  aprovou os três cenários. O cenário normal registrou 120 µs.
  Uma cópia temporária que retornava sem liberar o bloco falhou na
  asserção esperada.
- A issue #4 traz o roteiro geral e não tinha comentários na consulta.
  A busca não encontrou issue aberta específica de C03. O VPK já
  validado foi reaproveitado. A pausa, a retomada, o relógio e a saída
  completa com START aguardam verificação no console. C04 aguarda C03.

## Ciclo C03 — erro de leitura do controle

- O teste Linux em `vita-prototype/test_loop.c` força um retorno negativo
  na primeira leitura do controle. Confere que o loop retorna 1 sem ler
  novamente e que desvincula o quadro e aguarda a sincronização antes
  de liberar a memória. O roteiro local descreve o cenário.
- GCC 16.2.1 com cabeçalhos do VitaSDK, avisos como erros e UBSan
  aprovou os dois cenários. A saída do cenário normal continuou em
  120 µs. Uma cópia temporária que ignorava o erro falhou na asserção
  assim que tentou ler o controle novamente.
- A issue #4 contém o roteiro geral e não tinha comentários. A busca
  não encontrou issue aberta específica de C03. O ciclo reaproveitou
  o VPK existente. Pausa, retomada, relógio e saída completa com START
  seguem pendentes no console. C04 aguarda C03.

## Ciclo C03 — soltar e pressionar X com o loop ativo

- O teste Linux em `vita-prototype/test_loop.c` agora verifica a volta
  ao quadro verde ao soltar X e ao amarelo ao pressioná-lo novamente,
  em atualizações consecutivas após retomar. O roteiro descreve o caso.
- GCC 16.2.1 com cabeçalhos do VitaSDK, avisos como erros e UBSan
  confirmou 120 µs de tempo ativo. Uma cópia temporária do loop que
  mantinha o quadro ao soltar X falhou na verificação do quadro esperado.
- A consulta ao GitHub encontrou o roteiro geral na issue #4 e nenhuma
  issue aberta específica de C03. O código do aplicativo e o VPK foram
  reaproveitados. Pausa, retomada, relógio e encerramento por START
  seguem pendentes no console com o VPK do relógio. C04 aguarda C03.

## Ciclo C03 — pixels dos quadros sintéticos

- O teste no Linux agora confere o formato A8B8G8R8, as dimensões
  de 960 × 544, o passo de 1024 pixels e as duas extremidades de cada
  faixa e do preenchimento em todas as linhas dos quadros verde e amarelo.
- Com os cabeçalhos do VitaSDK, GCC 16.2.1, avisos como erros e UBSan,
  o teste passou com 120 µs de tempo ativo. Uma cópia temporária que
  desenhava a faixa verde no quadro amarelo falhou na asserção esperada.
- O SHA-256 do VPK do relógio coincide com o registrado no roteiro.
  O pacote foi reaproveitado. A busca no GitHub não encontrou issue aberta
  específica de C03 e a issue #4 não tinha comentários com novas evidências.
  A pausa, a retomada, o relógio e a saída com START seguem pendentes de
  verificação no console com esse VPK. C04 aguarda C03.

## Ciclo C03 — retomada da pausa

- O teste em `vita-prototype/test_loop.c` confere que Círculo com X
  apresenta o quadro amarelo ao retomar, que uma segunda pausa mantém
  esse quadro mesmo após soltar X e que START encerra o loop em pausa.
- No Linux, GCC 16.2.1 com cabeçalhos do VitaSDK, avisos tratados como
  erros e UBSan confirmou 120 µs de tempo ativo. Foram 50 µs antes da
  pausa e 70 µs após a retomada. Uma cópia temporária do código com a
  retomada desativada falhou na asserção de apresentação do quadro.
- A consulta encontrou o roteiro geral na issue #4 e nenhuma issue
  aberta específica para C03. O roteiro de validação local foi atualizado.
  O VPK foi reaproveitado. C03 aguarda a validação em console da pausa,
  da retomada, do relógio e da saída completa com START. C04 aguarda C03.

## Ciclo C03 — pausa com uma amostra do controle ausente

- O teste permanente em `vita-prototype/test_loop.c` simula uma leitura
  sem amostra enquanto Círculo está pressionado. Na amostra seguinte,
  Círculo continua pressionado junto com X. O teste exige que a pausa
  continue e que o quadro verde permaneça na tela até START.
- A simulação com cabeçalhos do VitaSDK, GCC 16.2.1, avisos como erros
  e UBSan passou com 50 µs de tempo ativo. Uma cópia temporária do loop
  que zerava o histórico dos botões na leitura sem amostra falhou na
  checagem de troca de quadro, conforme esperado.
- A consulta ao GitHub encontrou o roteiro geral na issue #4 e nenhuma
  issue aberta específica de C03. O VPK do relógio foi reaproveitado.
  A pausa, o relógio e o encerramento completo com START ainda precisam
  de verificação em console com esse pacote. C04 aguarda C03.

## Ciclo C03 — recuo do relógio e START com outros botões

- `vita-prototype/test_loop.c` executa o loop real no Linux com cabeçalhos
  do VitaSDK e chamadas simuladas. Com avisos tratados como erros e UBSan,
  confirmou a saída `Active loop time: 50 us`. O total soma 20 e 30 µs,
  ignora o recuo de 120 para 110 µs e exclui os dois intervalos em pausa.
- Em pausa, START junto com Círculo e X retornou código 0. O teste
  confirmou que o loop pulou a troca de quadro e liberou o bloco após
  desvincular a tela e aguardar a sincronização.
- O CMake continua compilando apenas `main.c` para o VPK. A issue #4
  contém o roteiro geral e a busca por issues abertas de C03 não encontrou
  uma atribuição específica. A confirmação física de START em C02 e do
  pacote do relógio em C03 continua pendente. C04 aguarda C03.

## Ciclo C03 — identificação do VPK usado no teste

- O roteiro agora identifica o caminho do VPK do relógio já validado
  e o caminho que os comandos de recompilação geram. O resultado em
  console deve incluir o caminho e o SHA-256 do pacote instalado.
- O SHA-256 do pacote local do relógio coincide com o registrado no
  roteiro. O Git ignora os dois caminhos e o CMake confirma o nome
  do VPK. Este ciclo reaproveitou a compilação e os verificadores
  anteriores.
- A issue #4 contém o roteiro geral, sem comentários com novas
  evidências. A busca por issues abertas específicas de C03 não
  retornou resultados. Pausa, relógio e encerramento com START
  seguem pendentes de confirmação no Vita. C04 aguarda C03.

## Ciclo C03 — registro das observações no console

### Canal de saída do relógio

- No ELF local do pacote do relógio, `arm-vita-eabi-objdump` mostrou que
  `_init_vita_io` tenta abrir `tty0:` e só registra o descritor de saída
  padrão se a abertura for bem-sucedida. O roteiro agora indica esse
  canal para a captura da mensagem de tempo ativo.
- A consulta à issue #4 trouxe o roteiro geral. A busca por issues abertas
  com C03 não retornou resultados. O executável já validado foi
  reaproveitado. A captura, a pausa e a saída completa com START ainda
  aguardam verificação no console.

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

## Estação de testes remotos — 18/11/2026

O usuário confirmou a instalação e execução remotas do
Handkerchief Smoke Test no PS Vita físico.

Infraestrutura comprovada:
- VitaSDK: compilação ARM e VPK.
- VitaCompanion 1.10: conexão pelo IP 192.168.2.4.
- FTP: transferência e verificação SHA-256.
- Promote: instalação remota de HKSM00001.
- Launch e quit: comandos remotos aceitos.
- Press/release cross: comandos remotos aceitos.
- Teste visual anterior: botão X alterou a cor central.

Relatório automatizado:
smoke-20261118_180900.json

Resultados: conexão, inicialização e comandos X aprovados.
A renderização NÃO foi verificada automaticamente.

Próximas tarefas de C03:
1. Instrumentar estados e entradas do motor para testes.
2. Criar testes determinísticos de atualização e controles.
3. Registrar resultados verificáveis, não apenas respostas de rede.
4. Preservar o aplicativo HKSM00001 e os backups.
5. Não instalar builds no console sem autorização humana.

Os timers não devem instalar nem interromper aplicativos
automaticamente.

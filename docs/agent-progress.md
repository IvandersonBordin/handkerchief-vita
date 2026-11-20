# Continuidade — bruno

## Estado das tarefas

- B01 — CONCLUIDA em 18/11/2026. Inventário e validação estrutural
  reproduzidos na cópia original autorizada.
- B02 — em andamento. A Etapa 82 foi aprovada e a issue #8 recebeu
  `codex-ready`. A integração de posição e velocidade segue em aberto.

## Ciclo da origem do seletor gráfico

A continuidade da issue #6 retomou o elo pendente do ramo
que grava em `0x4ABFD8`. O slot `+0x10` do subobjeto
`wind` em `ESI+0xB8` retorna seu campo `+0x04`.
O campo começa em zero e pode receber um valor da tabela
local pela chamada em `0x44096B`. O valor efetivo durante
o desenho de A0 segue em aberto. Os endereços e comandos
estão em [hdk-format.md](hdk-format.md#origem-do-seletor-em-0x4abfd8).

A leitura direta do PE32 conferiu o SHA-256, três slots,
quatro offsets, nove transferências e seis trechos. Os 31
testes locais e `git diff --check` passaram. O conector
exigiu aprovação para comentar na issue #6, indisponível
pela política da sessão. B02 segue em andamento.

## Ciclo do vetor de CCastMK — B02 (19/11/2026)

A issue #8 segue aberta com `codex-ready`. No caminho
`0x445F7B → 0x40241E → 0x412CE5`, a entrada do vetor
recebe o próprio `CCastMK` como segundo argumento explícito.
A máscara `0x0E` do leitor de `CCastMK` pula a inserção nesse
vetor. As duas inserções no método de atualização usam um
receptor obtido pelo contexto em `ator+0x48`. A evidência
termina no despacho em `0x412D06`, com a origem das
entradas e a escrita de X/Y do próprio ator em aberto.
Os detalhes estão em
[physics-collision.md](physics-collision.md#origem-do-vetor-percorrido-em-0x445f7b).

A leitura direta do PE32 confirmou o SHA-256, nove
transferências, 12 trechos e dois offsets. Os 31 testes
locais e `git diff --check` passaram. O conector exigiu aprovação
para comentar na issue #8, indisponível pela política da sessão.
B02 segue em andamento.

## Ciclo da chamada em 0x445F70 — B02 (19/11/2026)

A issue #8 está aberta com `codex-ready`. O recorte autorizado
segue `0x445F70 → 0x4035D5 → 0x426953`, com os parâmetros
`(código, X, Y, 0)`. A escrita em `0x45C7A0` grava zero
no campo `+0x18` de uma entrada selecionada por tabela.
As instruções `0x426A4A` e `0x426A50` leem o par
`+0x18/+0x1C` de um ponteiro obtido por outra busca.
A identidade dos receptores em execução e a escrita da
origem do próprio `CCastMK` seguem em aberto. A cadeia e
os comandos estão em
[physics-collision.md](physics-collision.md#destino-da-chamada-em-0x445f70).

A leitura direta do PE32 confirmou o SHA-256, 16 transferências,
seis trechos e dois offsets. Os 31 testes e `git diff --check`
passaram. A política da sessão impediu a aprovação exigida
pelo conector para publicar na issue #8. O ciclo fica para
revisão local e B02 segue em andamento.

## Ciclo da instância e dos destinos virtuais (19/11/2026)

Na continuidade da issue #6, EDI em `0x453707` vem da instância
local inicializada em `0x451309`. Seu campo `+0x38` recebe em
`0x452628` um contêiner com tabela `0x46E200`. Os slots
`+0x08` e `+0x04` chegam a `0x4135E8` e `0x4135A0` e
repassam chamadas às entradas do vetor. O elo com o desenho
de `STAGE_0` de A0.HDK segue em aberto. A escrita em
`0x440C1D` fornece uma origem para o seletor `0x4ABFD8`.
Os endereços e comandos estão em
[hdk-format.md](hdk-format.md#instância-e-destinos-virtuais-do-chamador).

A leitura direta do PE32 conferiu o SHA-256, 14 transferências,
20 trechos, dois slots e três offsets. Os 31 testes locais e
`git diff --check` passaram. O conector exigiu aprovação para
comentar na issue #6, indisponível pela política da sessão.
B02 segue em andamento.

## Ciclo do chamador da atualização da tabela (19/11/2026)

A prioridade da issue #6 continua pelo elo autorizado no comentário
7005659099. A chamada em `0x453707` segue `0x403E9F → 0x452E6A`.
No ramo documentado, os despachos pelos slots `+0x08` e `+0x04`
do objeto em `[receptor+0x38]` precedem a atualização em `0x4530F9`.
As condições e os comandos estão em
[hdk-format.md](hdk-format.md#chamador-da-atualização-da-tabela).

A leitura direta do PE32 confirmou o SHA-256, quatro transferências,
oito trechos e dois offsets. Os 31 testes locais e `git diff --check`
passaram. O conector exigiu aprovação para comentar na issue #6,
indisponível pela política da sessão. B02 segue em andamento.

## Ciclo dos ajustes de STAGE_0 (19/11/2026)

O [comentário da issue #6](https://github.com/IvandersonBordin/handkerchief-vita/issues/6#issuecomment-7005659099)
priorizou investigar escritas indiretas na tabela gráfica. A inicialização
em `0x40D340` zera os 64 slots. A chamada em `0x4530F9` alcança
uma rotina que pode zerar ou copiar valores para os slots 25 e 26,
conforme o inteiro em `0x4ABFD8`. As condições, os offsets e a reprodução
estão em [hdk-format.md](hdk-format.md#escrita-indireta-na-tabela-de-ajustes).

A leitura direta do PE32 conferiu o SHA-256, cinco transferências,
12 trechos, quatro offsets e o mapeamento dos dois slots. Os 31 testes
locais e `git diff --check` passaram. O conector exigiu aprovação
para comentar na issue #6, indisponível pela política da sessão.
A relação do chamador com A0 e a condição ativa durante seu desenho
seguem em aberto. B02 permanece em andamento.

## Ciclo da chamada em 0x445F60 — B02 (18/11/2026)

A issue #8 segue aberta com `codex-ready`. O comentário mais
recente prioriza uma escrita em X/Y do próprio ator durante a
atualização. No slot `+0x18`, a chamada em `0x445F60`
incrementa um contador em endereço fixo e usa o valor para
indexar uma tabela. O corpo da rotina deixa X/Y sem leitura.
A cadeia e a reprodução estão em
[physics-collision.md](physics-collision.md#uso-do-retorno-da-chamada-em-0x445f60).

A leitura direta do PE32 confirmou o SHA-256, quatro transferências,
cinco trechos e dois offsets. Os 31 testes locais e
`git diff --check` passaram. A escrita na origem durante a
atualização segue em aberto. O conector exigiu aprovação para
comentar na issue #8, indisponível pela política da sessão.

## Ciclo de leitura do nome em +0x50 — B02 (18/11/2026)

A issue #8 segue aberta com `codex-ready`. O leitor passa
`CCastMK+0x50` como destino e solicita uma janela de 12 bytes.
O cursor pode avançar menos ao encontrar o terminador. A cadeia
e a reprodução estão em
[physics-collision.md](physics-collision.md#leitura-do-nome-em-0x50).
A associação a um registro específico de A0.HDK e a escrita
em X/Y durante a atualização seguem em aberto.

A leitura direta do PE32 confirmou o SHA-256, cinco
transferências relativas, cinco trechos e um offset de arquivo.
Os 31 testes passaram. A política de aprovação da sessão
impediu a publicação do resumo na issue #8.

## Ciclo de origem do ponteiro +0x78 — B02 (18/11/2026)

A issue #8 permanece aberta com `codex-ready`. O leitor de
`CCastMK` grava em `+0x78` o resultado de uma busca pelo nome
em dois vetores. Isso delimita de onde vem o receptor de
`0x42972C`. O método concreto e seu efeito sobre X/Y seguem
em aberto. A cadeia e a reprodução constam em
[physics-collision.md](physics-collision.md#origem-do-receptor-em-0x78).

A leitura direta conferiu o SHA-256, nove transferências
relativas, nove trechos de instruções e um offset de arquivo.
Os 31 testes locais passaram. A política da sessão impediu
a aprovação exigida pelo conector para comentar na issue #8.

## Ciclo de escrita direta no slot +0x18 — B02 (18/11/2026)

A issue #8 continua aberta com `codex-ready`. A escrita por
ponteiro em `0x445E19` decrementa `CCastMK+0x94`. As demais
escritas diretas no ator nesse método atingem `+0x84`. A
análise também delimitou uma chamada indireta a partir de
`ator+0x78`, cujo efeito sobre X/Y segue em aberto. Os endereços
e a reprodução estão em
[physics-collision.md](physics-collision.md#escritas-diretas-e-chamada-indireta-no-slot-0x18).

A leitura direta conferiu o SHA-256, nove trechos de instruções,
duas transferências relativas e um offset de arquivo. Os 31
testes locais passaram. O conector exigiu aprovação para
comentar na issue #8, indisponível pela política da sessão.

## Ciclo de rastreamento das coordenadas de desenho — B02 (18/11/2026)

A issue #8 segue aberta com `codex-ready`. O ramo opcional do
slot `+0x14` combina a origem de `CCastMK` com as coordenadas
locais da peça e passa o par ao caminho de desenho. O receptor
da chamada final é a superfície obtida de `0x4AA3E0`. A cadeia
e os comandos estão em
[physics-collision.md](physics-collision.md#destino-das-coordenadas-no-ramo-de-desenho).

A leitura direta do PE32 conferiu o SHA-256, um slot, cinco
transferências relativas, oito trechos de instruções e um offset
de arquivo. Os 31 testes locais e `git diff --check` passaram.
O conector exigiu aprovação para comentar na issue #8, indisponível
pela política da sessão. B02 segue em andamento.
O próximo recorte é rastrear o receptor das escritas no ramo
que começa em `0x445DF3` no slot `+0x18`, fora dos caminhos
de inicialização já documentados.

## Ciclo de identificação de CRobot0 — B02 (18/11/2026)

A issue #8 segue aberta com `codex-ready`. Para `CCastMK+0x8C = 1`,
a chamada em `0x43DE28` seleciona o primeiro slot do descritor
de `CRobot0`. Com alocação bem-sucedida e `+0x89 = 0`, a
origem do ator é copiada para o novo `CRobot0`. A cadeia
condicional e os comandos de reprodução estão em
[physics-collision.md](physics-collision.md#receptor-quando-o-campo-0x8c-vale-1).

A checagem direta do PE32 confirmou o SHA-256, um offset de
arquivo, quatro registros de dados, 11 transferências relativas
e sete trechos de instruções. Os 31 testes locais e
`git diff --check` passaram. A publicação na issue #8 exigiu
aprovação indisponível pela política da sessão.

B02 segue em andamento. O próximo recorte é verificar se o
slot `+0x14` de `CCastMK` pode alcançar uma escrita nos
campos `+0x18` e `+0x1C` do próprio ator.

## Ciclo de rastreamento do receptor no ramo zero — B02 (18/11/2026)

A issue #8 segue aberta com `codex-ready`. O experimento de
tradução anterior já estava registrado. Neste ciclo, a leitura
estática confirmou que o ramo com `CCastMK+0x89` igual a
zero pode copiar a origem para o ponteiro guardado em
`CCastMK+0x60`. A identidade desse ponteiro depende do alvo
indireto em `0x43DE28`. A cadeia e a reprodução estão em
[physics-collision.md](physics-collision.md#destino-da-cópia-quando-o-campo-0x89-é-zero).

A conferência do PE32 validou o SHA-256, oito trechos de
instruções, duas transferências relativas e dois offsets de
arquivo. Os 31 testes locais e `git diff --check` passaram.
A B02 segue em andamento. O próximo recorte é identificar
o alvo da chamada em `0x43DE28` para determinar a identidade
do receptor da escrita. O conector exigiu aprovação para comentar
na issue #8, indisponível pela política da sessão.

## Ciclo de tradução de um retângulo de entrada — B02 (18/11/2026)

O comentário mais recente da issue #8 priorizou avaliar o método
de `x86rec.py` em `0x40963E`. Em C99 no host, uma tradução
manual do cálculo de um retângulo passou em 36 casos sintéticos
contra `resolve_pieces` e em três casos-limite. A rotina completa
chama `USER32!UnionRect`. A adaptação de `x86rec.py` exige
Capstone, outra entrada PE32, revisão dos patches de SH3 e os
metadados de instruções. A compilação ARM segue sem verificação.
Os detalhes estão em
[physics-collision.md](physics-collision.md#experimento-de-tradução-de-0x40963e-para-c).

O recorte local de `CCastMK` foi preservado e a condição de
entrada do ramo foi detalhada. Os 31 testes locais passaram.
A política da sessão impediu a aprovação exigida pelo conector
para publicar o resumo na issue #8.

## Ciclo de despacho do slot +0x14 — B02 (18/11/2026)

A issue #8 segue aberta com `codex-ready`. A revisão do coordenador
considera atendido o elo de inserção no vetor. A análise confirmou
que o slot `+0x14` também percorre esse vetor. No ramo opcional
de `CCastMK`, as somas com X/Y são gravadas em argumentos na
pilha. A cadeia e a reprodução estão em
[physics-collision.md](physics-collision.md#uso-da-origem-no-despacho-do-slot-0x14).

A conferência direta do PE32 validou o SHA-256, dois slots, sete
transferências relativas, quatro trechos de instruções e três offsets
de arquivo. Os 31 testes locais e `git diff --check` passaram.
O conector exigiu aprovação para comentar na issue #8, indisponível
pela política da sessão.

## Ciclo de leitura e despacho de CMarkRoot — B02 (18/11/2026)

A issue #8 segue aberta com `codex-ready` e a aprovação restrita
da Etapa 82 consta na issue #6. Os slots de leitura e despacho de
`CMarkRoot` usam o vetor do próprio receptor. O chamador em
`0x42331F` usa a instância guardada no campo `+0x38`. A cadeia
e os comandos de reprodução estão em
[physics-collision.md](physics-collision.md#leitura-e-despacho-de-cmarkroot).

A leitura independente conferiu o SHA-256 do PE, cinco ponteiros,
12 transferências relativas, sete trechos de instruções e o
cabeçalho de `ENEMY_0` em A0.HDK. Os 31 testes locais e
`git diff --check` passaram. O conector do GitHub exigiu aprovação
para comentar na issue #8, indisponível pela política da sessão.

## Ciclo de inserção no vetor — B02 (18/11/2026)

A issue #8 segue aberta com `codex-ready`, com a aprovação restrita
da Etapa 82 registrada na issue #6. A análise estática identificou
um caminho que cria `CCastMK` pelo nome, lê sua origem do fluxo
sob as condições já documentadas e insere o ponteiro no vetor
usado pelo despacho. A cadeia condicional e os comandos estão em
[physics-collision.md](physics-collision.md#inserção-de-ccastmk-no-vetor-durante-a-leitura).

A conferência independente do PE32 confirmou o SHA-256, três slots,
15 transferências relativas e sete trechos de instruções. A leitura
de A0.HDK encontrou 14 ocorrências de `CCastMK` precedidas por
`FF FF` em `ENEMY_0`. Os 31 testes locais e `git diff --check`
passaram. O conector do GitHub exigiu aprovação para comentar na
issue #8, indisponível pela política da sessão.

## Ciclo de despacho e uso da origem — B02 (18/11/2026)

A issue #8 segue aberta com `codex-ready` e a aprovação restrita da
Etapa 82 consta na issue #6. O laço em `0x412CE5` chama o slot
`+0x18` das entradas não nulas. Falta provar qual caminho insere um
`CCastMK` no vetor. O método de `CCastMK` nesse slot lê X e Y e,
no ramo analisado, usa o par para inicializar outro objeto. A cadeia e
os comandos de reprodução estão em
[physics-collision.md](physics-collision.md#despacho-do-slot-0x18-e-uso-da-origem).

A conferência independente do PE32 confirmou a identificação da cópia,
um slot, nove transferências relativas e sete sequências de instruções.
Os 31 testes locais e `git diff --check` passaram. A publicação do
resumo na issue #8 foi bloqueada porque o conector exige aprovação e
a política da sessão não permite essa aprovação.

## Ciclo de escrita da origem — B02 (18/11/2026)

A issue #8 seguia com `codex-ready`, e a aprovação restrita da Etapa 82
estava registrada na issue #6. A análise do slot `+0x0C` de `CCastMK`
identificou um caminho que lê X e Y do fluxo para um objeto já criado e
recalcula seus limites. A cadeia, as condições e os comandos de reprodução
estão em [physics-collision.md](physics-collision.md#escrita-da-origem-durante-leitura-de-dados).

A conferência independente do PE32 confirmou dois slots, 14 transferências
relativas e seis sequências de instruções. Os 31 testes locais e
`git diff --check` passaram. O conector do GitHub exigiu aprovação para
comentar na issue #8, indisponível pela política da sessão.

## Ciclo de consulta de sobreposição — B02 (18/11/2026)

A [issue #6](https://github.com/IvandersonBordin/handkerchief-vita/issues/6)
registra a aprovação restrita da Etapa 82. A
[issue #8](https://github.com/IvandersonBordin/handkerchief-vita/issues/8)
está aberta com `codex-ready`.

O slot `+0x1C` de `CCastMK` leva a uma consulta de sobreposição
entre os limites do ator e um retângulo recebido. A cadeia de chamadas
e os limites da evidência estão em
[physics-collision.md](physics-collision.md#consulta-de-sobreposição).
A reprodução com `objdump` e a leitura independente do PE32
confirmaram o slot, três destinos relativos e seis trechos de
instruções. Os 31 testes locais e `git diff --check` passaram.

B02 segue em andamento. O próximo recorte é rastrear uma escrita na
origem do ator fora da inicialização. O conector exigiu aprovação
para comentar na issue #8, indisponível pela política da sessão.

## Ciclo de análise estática — B02 (18/11/2026)

O corpo atualizado da [issue #6](https://github.com/IvandersonBordin/handkerchief-vita/issues/6)
registra a aprovação restrita da Etapa 82 e libera a
[issue #8](https://github.com/IvandersonBordin/handkerchief-vita/issues/8).
A desmontagem local confirmou o par de origem de `CCastMK`, o cálculo
dos limites a partir dele e um caminho de leitura com Y alternativo.
Os endereços, limites da evidência e comandos de reprodução estão em
[physics-collision.md](physics-collision.md). A próxima análise deve
rastrear um escritor da origem fora da inicialização.

A reprodução com `objdump` e a conferência independente do descritor,
do SHA-256 e de oito destinos de chamadas e saltos passaram. Os 31
testes locais e `git diff --check` passaram. O conector do GitHub exigiu
aprovação para comentar na issue #8, indisponível pela política da sessão.

## Ciclo de verificação — B02 (18/11/2026)

- A issue #8 permanece aberta e condiciona a análise de movimento à
  aprovação técnica da Etapa 82. A entrega `bd1a8bc` segue com revisão
  pendente no [último comentário da issue #6](https://github.com/IvandersonBordin/handkerchief-vita/issues/6#issuecomment-6975282807).
  A cópia original estava acessível em leitura e os 31 testes passaram
  com `PYTHONPATH=tools python3 -B -m unittest discover -s tests`.
  A política de aprovação da sessão impediu a publicação do resumo na
  issue #8.

- A [issue #8](https://github.com/IvandersonBordin/handkerchief-vita/issues/8) segue aberta, sem comentários e sem `codex-ready`. O [comentário de entrega de `bd1a8bc`](https://github.com/IvandersonBordin/handkerchief-vita/issues/6#issuecomment-6975282807) ainda registra revisão técnica pendente. B02 permanece bloqueada pela aprovação da Etapa 82.
- O diretório original estava acessível em leitura. Os 31 testes passaram com `PYTHONPATH=tools python3 -B -m unittest discover -s tests`. A publicação na issue #8 ficou bloqueada porque o conector exige aprovação e a política da sessão impede essa aprovação.

- A [issue #8](https://github.com/IvandersonBordin/handkerchief-vita/issues/8)
  continua aberta, sem comentários e sem `codex-ready`. O comentário de
  [entrega de `bd1a8bc`](https://github.com/IvandersonBordin/handkerchief-vita/issues/6#issuecomment-6975282807)
  ainda indica revisão técnica pendente da Etapa 82. A B02 aguarda essa
  aprovação. A cópia original estava acessível em leitura e os 31 testes
  passaram com `PYTHONPATH=tools python3 -B -m unittest discover -s tests`.
  O conector exigiu aprovação para publicar o resumo, indisponível nesta sessão.

- Neste ciclo, a [issue #8](https://github.com/IvandersonBordin/handkerchief-vita/issues/8)
  seguia aberta, sem comentários e sem `codex-ready`. A
  [entrega de `bd1a8bc`](https://github.com/IvandersonBordin/handkerchief-vita/issues/6#issuecomment-6975282807)
  ainda aguardava revisão técnica da Etapa 82. A análise de movimento
  permanece bloqueada. O diretório original estava acessível para leitura
  e os 31 testes locais passaram com
  `PYTHONPATH=tools python3 -B -m unittest discover -s tests`.
  A publicação do resumo na issue #8 exigiu aprovação indisponível na sessão.

Na conferência deste ciclo, a [issue #8](https://github.com/IvandersonBordin/handkerchief-vita/issues/8)
ainda exigia a aprovação técnica da Etapa 82 e seguia sem `codex-ready`.
A autorização no corpo da issue #6 se refere à investigação da Etapa 82
após a revisão da Etapa 81. O [comentário de entrega de `bd1a8bc`](https://github.com/IvandersonBordin/handkerchief-vita/issues/6#issuecomment-6975282807)
continua registrando a revisão da Etapa 82 como pendente. A análise da B02
permanece bloqueada. O diretório original estava acessível para leitura e
`PYTHONPATH=tools python3 -B -m unittest discover -s tests` passou com 31 testes.
A publicação do resumo na issue #8 exigiu aprovação indisponível na sessão.

- Na consulta deste ciclo, a [issue #8](https://github.com/IvandersonBordin/handkerchief-vita/issues/8) seguia aberta, sem comentários e sem `codex-ready`. O [comentário de entrega de `bd1a8bc`](https://github.com/IvandersonBordin/handkerchief-vita/issues/6#issuecomment-6975282807) ainda registra revisão técnica pendente para a Etapa 82. A análise estática da B02 aguarda essa aprovação.
- O diretório autorizado estava acessível para leitura. Os 31 testes locais passaram com `PYTHONPATH=tools python3 -B -m unittest discover -s tests`.
- O conector do GitHub exigiu aprovação para publicar o resumo na issue #8, indisponível nesta sessão. O resultado ficou registrado localmente.

- Na consulta deste ciclo, a issue #8 estava aberta, sem comentários
  e sem `codex-ready`. A entrega `bd1a8bc` seguia com revisão técnica
  pendente na issue #6. A B02 permanece bloqueada pela aprovação da
  Etapa 82. O diretório original estava acessível em leitura e os 31
  testes passaram com `PYTHONPATH=tools python3 -B -m unittest discover -s tests`.
  O conector do GitHub exigiu aprovação para comentar na issue #8, e a
  política da sessão impediu a publicação.

- A [issue #8](https://github.com/IvandersonBordin/handkerchief-vita/issues/8)
  segue aberta, sem comentários e sem `codex-ready`. A
  [entrega de `bd1a8bc` na issue #6](https://github.com/IvandersonBordin/handkerchief-vita/issues/6#issuecomment-6975282807)
  ainda aguarda revisão técnica da Etapa 82. A B02 segue bloqueada antes
  da análise estática. O diretório autorizado estava acessível em leitura
  e os 31 testes passaram com
  `PYTHONPATH=tools python3 -B -m unittest discover -s tests`.
  A publicação na issue #8 exigiu aprovação, indisponível nesta sessão.

- Neste ciclo, a [issue #8](https://github.com/IvandersonBordin/handkerchief-vita/issues/8)
  seguia aberta, sem comentários e sem `codex-ready`. A revisão de `bd1a8bc`
  continuava pendente no [comentário de entrega da issue #6](https://github.com/IvandersonBordin/handkerchief-vita/issues/6#issuecomment-6975282807).
  A B02 segue bloqueada antes da análise estática. O diretório autorizado
  estava acessível para leitura e os 31 testes passaram com
  `PYTHONPATH=tools python3 -B -m unittest discover -s tests`. A tentativa de
  publicar o resumo na issue #8 foi bloqueada pela política de aprovação da sessão.

- A B02 segue bloqueada. A [issue #8](https://github.com/IvandersonBordin/handkerchief-vita/issues/8) exige a aprovação técnica da Etapa 82 e permanece aberta, sem `codex-ready`. O [último comentário da issue #6](https://github.com/IvandersonBordin/handkerchief-vita/issues/6#issuecomment-6975282807) registra a entrega de `bd1a8bc` com revisão ainda pendente. A cópia original estava acessível para leitura e os 31 testes passaram com `PYTHONPATH=tools python3 -B -m unittest discover -s tests`. O conector exigiu aprovação para publicar o resumo na issue #8, indisponível nesta sessão.

- A issue #8 permanece aberta, sem `codex-ready`, e seu texto exige a aprovação técnica da Etapa 82. O comentário mais recente da issue #6 ainda registra revisão pendente para `bd1a8bc`. A B02 segue bloqueada antes da análise estática. O diretório autorizado estava acessível para leitura e os 31 testes passaram com `PYTHONPATH=tools python3 -B -m unittest discover -s tests`.
  O conector exigiu aprovação para publicar o resumo na issue #8. A política desta sessão impede essa aprovação.

- Na consulta deste ciclo, a issue #6 seguia aberta e o [comentário de entrega](https://github.com/IvandersonBordin/handkerchief-vita/issues/6#issuecomment-6975282807) ainda registrava revisão técnica pendente. A issue #8 continuava aberta, sem comentários e sem `codex-ready`. B02 segue bloqueada antes da análise estática. A cópia original estava acessível para leitura e os 31 testes locais passaram. O conector exigiu aprovação para publicar o resumo na issue #8, e a sessão não permite essa aprovação.

- A consulta atual confirmou que a [revisão técnica da Etapa 82](https://github.com/IvandersonBordin/handkerchief-vita/issues/6#issuecomment-6975282807) segue pendente. A [issue #8](https://github.com/IvandersonBordin/handkerchief-vita/issues/8) permanece aberta, sem comentários e sem `codex-ready`. A B02 aguarda essa aprovação para iniciar a análise estática.
- O diretório original estava acessível para leitura e os 31 testes passaram com `PYTHONPATH=tools python3 -B -m unittest discover -s tests`. O conector do GitHub exigiu aprovação para publicar o comentário na issue #8, indisponível nesta sessão. O registro ficou neste arquivo.

Na consulta deste ciclo, a issue #6 seguia aberta com `codex-done` e seu
comentário mais recente mantinha a revisão técnica da Etapa 82 pendente.
A issue #8 seguia aberta, sem comentários e sem `codex-ready`.
O diretório original estava acessível em leitura e os 31 testes locais
passaram. A B02 permanece bloqueada antes da análise de movimento.

Na verificação mais recente, a issue #8 estava aberta, sem comentários e
sem `codex-ready`. Na issue #6, a aprovação citada no corpo se refere à
Etapa 81. O comentário de entrega de `bd1a8bc` ainda registra revisão
técnica pendente para a Etapa 82. A B02 segue bloqueada antes da análise
estática. O diretório original estava acessível em leitura e os 31 testes
locais passaram.

Na nova consulta, a issue #6 continuava aberta com `codex-done` e o
comentário de entrega ainda indicava revisão técnica pendente. A issue
#8 seguia aberta e sem `codex-ready`. A cópia original estava acessível
para leitura. B02 continua bloqueada pela aprovação técnica, e a suíte
local voltou a passar com 31 testes.

A [issue #8](https://github.com/IvandersonBordin/handkerchief-vita/issues/8)
condiciona a análise de movimento à aprovação técnica da Etapa 82. O
[último comentário da issue #6](https://github.com/IvandersonBordin/handkerchief-vita/issues/6#issuecomment-6975282807)
registra a entrega no commit `bd1a8bc` e informa que a revisão técnica
segue pendente. Na consulta, a issue #6 estava aberta e a #8 seguia sem
`codex-ready`. A análise de movimento permanece pendente.

A suíte local passou com 31 testes (`PYTHONPATH=tools python3 -B -m unittest
discover -s tests -v`). `git diff --check` passou.

- A [última atualização da issue #6](https://github.com/IvandersonBordin/handkerchief-vita/issues/6#issuecomment-6975282807) registra a entrega de `bd1a8bc` com revisão técnica pendente. A [issue #8](https://github.com/IvandersonBordin/handkerchief-vita/issues/8) segue aberta, sem comentários e sem `codex-ready`, e condiciona a B02 à aprovação da Etapa 82. A cópia original estava acessível para leitura e os 31 testes passaram com `PYTHONPATH=tools python3 -B -m unittest discover -s tests`. A publicação do resumo na issue #8 exigiu aprovação, indisponível nesta sessão.

## Último ciclo — B01

A cópia indicada em [agent-plan.md](agent-plan.md) estava acessível.
A contagem recursiva por extensão, sem distinguir maiúsculas de minúsculas,
encontrou 108 HDK, 100 WAV, 19 BMP, 2 DB, 1 EXE e 1 AVI. Para extensões
diferentes de HDK, a contagem identifica os sufixos dos nomes dos arquivos.

Com `PYTHONPATH=tools python3 -B`, a validação percorreu os HDK dessa
cópia com `Path.rglob("*")` e consumiu integralmente os iteradores
`iter_sections` e `iter_section_classes` de `tools/inspect_hdk.py` para
cada arquivo com conteúdo.

- Os 99 HDK com conteúdo passaram na leitura dos limites externos.
- Os 98 aceitos pela leitura de classes repetiram a sequência documentada
  de dez pares seção/classe, totalizando 980 blocos.
- `USER01.HDK` apresentou 17 blocos válidos no modo de limites e retornou
  `bloco em 0x0000: nome da classe inválido` na leitura de classes. O
  significado dos campos internos desse arquivo segue em aberto.
- `USER02.HDK` a `USER10.HDK` tinham zero bytes.

As contagens coincidiram com o [relatório de formato HDK](hdk-format.md#validação-do-conjunto-de-arquivos).
As evidências anteriores de outros recursos continuam no
[mapa técnico](engine-map.md) e na
[identificação de personagens](character-identification.md#material-local).

A suíte local passou com 31 testes
(`PYTHONPATH=tools python3 -B -m unittest discover -s tests -v`).

## Próxima ação

Para a prioridade da issue #6, rastrear os receptores das
chamadas em `0x44C6F1` e `0x44CEC2` e verificar se uma
delas precede a atualização gráfica em `0x4530F9` no
caminho que prepara ou desenha A0.

Para B02, rastrear os chamadores de `0x402275` que possam
receber um `CCastMK` como destino da inserção. Se houver
um caso, resolver a tabela da entrada e verificar o uso do
segundo argumento no slot `+0x18`.
A integração por quadro, a gravidade e o salto seguem em aberto.

O conector do GitHub listou as issues abertas. Nenhuma correspondia à
validação B01 do plano local.

## Ciclo das duas primeiras entradas do contêiner — issue #6

O rastreio anterior de EDI e do campo `+0x38` já constava no histórico.
Este ciclo identificou as tabelas instaladas nas entradas inseridas em
`0x45284F` e `0x4528CB`. A leitura estática com GNU objdump conferiu
as origens, os quatro ponteiros e seus saltos. Os destinos condicionais
e a lacuna até o desenho estão em
[hdk-format.md](hdk-format.md#slots-das-duas-primeiras-inserções-no-contêiner).
Os 31 testes locais passaram. A próxima verificação é seguir o slot
`+0x04` da primeira entrada a partir de `0x435EF4` e procurar um elo
com o desenho de A0.

## Ciclo do slot +0x04 da primeira entrada — issue #6

A leitura estática de `0x435EF4` confirmou um despacho às entradas
filhas em `0x435F13` e quatro chamadas condicionais a `0x408B7B`.
O [rastreio do slot](hdk-format.md#caminho-da-primeira-entrada-pelo-slot-0x04)
distingue o receptor em ESI do argumento em EDI. Permanece em aberto
quem fornece as referências usadas nessa entrada no ramo de A0.
Os 31 testes locais passaram. O próximo rastreio é verificar se
a inicialização chamada em `0x452840` grava os campos `+0x28`,
`+0x2C` ou `+0x30` lidos pelo slot.

## Ciclo da inicialização da primeira entrada — issue #6

A chamada em `0x452840` fornece as três referências lidas pelo slot
`+0x04`. O campo `+0x28` recebe o primeiro argumento do chamador.
Os campos `+0x2C` e `+0x30` recebem valores de duas tabelas acessadas
com o mesmo deslocamento em EDI. Os endereços de leitura e escrita
estão no [rastreio da inicialização](hdk-format.md#inicialização-das-referências-da-primeira-entrada).
A conferência foi estática com GNU objdump e os 31 testes locais passaram.
O próximo passo é identificar a origem de EDI em `0x45282F` e verificar
quais entradas das tabelas são selecionadas no caminho de A0.

## Ciclo da origem de EDI em 0x45282F — issue #6

O índice das três referências vem de um byte lido em `0x44FBC6`.
A escrita em `0x452734` esclarece a origem do campo `+0x28`:
o chamador reutiliza a posição do argumento na pilha para guardar
o valor da primeira tabela. Os endereços e os intervalos conferidos
com objdump estão no [rastreio do índice](hdk-format.md#origem-do-índice-das-três-referências).
Para associar uma linha a A0, o próximo passo é seguir a chamada
em `0x44FBC1` e identificar quem preenche o byte selecionado.
Os 31 testes locais passaram e `git diff --check` passou.

## Ciclo da seleção do byte em 0x44FBC6 — issue #6

A leitura estática delimitou a seleção aos bytes de +0x84 a +0x88 e
localizou uma escrita candidata em 0x4504F2. O [rastreio da seleção](hdk-format.md#seleção-do-byte-em-0x44fbc6)
registra a condição de escrita e as duas bases globais. Os 31 testes
locais passaram. Para vincular essa escrita a A0, o próximo passo é
comparar as atribuições aos ponteiros em 0x4AD16C e 0x4AD228.

## Ciclo das bases da escrita e da leitura — issue #6

Na desmontagem estática de HDKWIN.EXE com GNU objdump, a escrita candidata
em `0x4504F2` depende da base guardada em `0x4AD16C`. O código zera essa
global em `0x45071B`. Em `0x450873`, guarda ESI nela após a comparação
em `0x450864` encontrar o valor de EBP na global.

A leitura em `0x44FBC6` depende da base guardada em `0x4AD228`.
As atribuições em `0x4513ED` e `0x45621F` guardam o retorno da chamada
a `0x401ED3` ou zero, conforme o ramo. Outra atribuição, em `0x451420`,
guarda EDI. Nos dois primeiros caminhos, o código também passa o endereço
da própria global em ECX nas chamadas em `0x4513F2` e `0x456224`.
Ambas chegam a `0x45A8E2` pelo salto em `0x403CFB`.

As atribuições diretas deixam em aberto se as bases apontam para a mesma
instância no ramo de A0. O próximo passo é rastrear o uso de ECX em
`0x45A8E2` e verificar se há uma escrita indireta no ponteiro do leitor.
A suíte local passou nos 31 testes. A conferência dos endereços foi estática.

## Ciclo do uso de ECX em 0x45A8E2 — issue #6

As chamadas em `0x4513F2` e `0x456224` passam o endereço de
`0x4AD228` em ECX. O salto em `0x403CFB` leva a `0x45A8E2`.
A rotina guarda o endereço em ESI e lê o ponteiro em `0x45A8E5`.
Se o ponteiro for nulo, retorna 1. Caso contrário, consulta a
palavra de 32 bits em `0x4AD22C` e chama `0x45A1E2` (valor diferente
de zero) ou `0x45B55C` (zero), com o ponteiro lido em ECX.

Se a chamada retornar zero, `0x45A909` relê o ponteiro. O despacho
em `0x45A916` usa o objeto indicado pelo campo `+0x04` desse ponteiro
e recebe também o endereço do campo `+0x08`. No corpo inspecionado,
os acessos por ESI são leituras. Os efeitos internos das chamadas
permanecem em aberto. Esse trecho não demonstra que a escrita em
`0x4504F2` e a leitura em `0x44FBC6` alcançam a mesma instância em A0.

Conferência reproduzível com GNU objdump em sintaxe Intel, usando
`-d -Mintel --start-address=0x45a8e2 --stop-address=0x45a91e` no EXE
local. Próxima ação: seguir o retorno de `0x44F70A` (destino de
`0x401ED3`) e procurar uma atribuição da mesma instância a `0x4AD16C`.

## A0 — despacho do movimento em 0x419028

Em `0x418E52`, ESI recebe o ECX de entrada de `0x418E47`. Em
`0x419020–0x419028`, EDI recebe a tabela apontada por esse objeto,
ECX volta a receber ESI e o despacho usa a entrada `+0x34` da tabela.
O argumento na pilha é o segundo argumento da rotina externa
(`[ebp+0x0C]`). A classe concreta depende da tabela instalada no objeto.

| Condição | Destino observado |
| --- | --- |
| EAX diferente de zero após o despacho | Copia o primeiro argumento para a variável local `[ebp-4]` e salta para `0x419146`. |
| EAX zero e algum dos bits de `[esi+0x109] & 3` ativo | Prepara uma estrutura local e chama a entrada `+0x38` em `0x419056`. |
| EAX zero e ambos os bits livres | Segue para `0x419073` e consulta a entrada `+0x28`. |

Há outro teste em `0x4190E1`: aplica 3 ao byte **do retorno** de
`0x4190DE` (entrada `+0x20` do objeto lido em `[ebp-0x10]`).
Esse caminho depende, entre outras condições, do sucesso da chamada em
`0x4190A9`. A escrita em `0x419032` seleciona um valor na pilha.
Para demonstrar ajuste de Y ainda é preciso resolver o destino de
`+0x34` para a instância de A0 e seguir as escritas desse método.

Conferência local com objdump em sintaxe Intel nos intervalos
`0x418E47–0x418E63` e `0x41901D–0x419146`. Em A0.HDK, o bloco
em `0x59` traz ROCK_0 em `0x5D` e CRock em `0x64`. Esses nomes
identificam o recurso. Na gravação, em 01:00, há degraus subindo à
direita e uma figura sobre a escada. A imagem não identifica o método
usado naquele instante nem fornece a coordenada interna Y.

Validação no host: reprodução isolada de TEST e dos desvios x86 com
EAX em {0, 1, 256, 0xFFFFFFFF} e os 256 valores possíveis do byte
de flags. Os 1.024 casos concordaram com a tabela. A suíte existente
passou nos 31 testes com `PYTHONPATH=tools python3 -B -m unittest discover -s tests`.

## B02 — segunda chamada ao slot +0x28 em 0x419A55

Após o ramo (1, 0) da issue #8, o despacho candidato (11, 0)
seleciona 0x49 ou 0x4A. A cópia de Y fica na pilha para a chamada
a 0x4268FC. O escritor de Y do protagonista permanece em aberto.
A desmontagem, a entrada da tabela e o limite da comparação visual
em 01:00 estão em [physics-collision.md](physics-collision.md#a0--destino-de-y-após-o-despacho-em-0x419a23).
Conferidos o SHA-256 do PE32 e os 31 testes locais. Próximo passo de B02:
seguir o terceiro argumento de 0x4268FC até seu primeiro uso e identificar
o objeto de destino de uma eventual escrita.

## B02 — primeiro uso aritmético de Y após 0x4268FC

O retorno em 0x45D237 conserva X e Y na pilha para 0x426A11.
Em 0x426A5B o caminho calcula Y menos o campo +0x1C do objeto em
EDI. A diferença fica em ECX. Condições e reprodução constam em
[physics-collision.md](physics-collision.md#a0--primeiro-cálculo-com-a-cópia-de-y).
Quadro de 01:00 conferido e 31 testes locais aprovados. B02 segue
aberta. Próxima ação: seguir ECX de 0x426A5B até seu consumidor e
verificar se alguma escrita posterior alcança o Y da instância de A0.

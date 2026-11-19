# Coordenadas de CCastMK

Análise estática da cópia local de `HDKWIN.EXE` (PE32 x86, SHA-256
`3deeb9b18ac2f6ee7b4af4769d1499fdfe0bde8de44482b57ab4bf6196617b76`).
Os endereços das instruções são virtuais. Os campos com prefixo `+`
são deslocamentos em memória.

## Origem e retângulo em memória

1. O descritor em `0x4718C8` aponta para o nome `CCastMK` em
   `0x488C98`. A rotina em `0x445BB6` retorna esse descritor.
   O caminho de inicialização em `0x445B82` instala a tabela de métodos
   em `0x471EB0` e, em `0x445B88`, chama
   `0x401BDB → 0x445CD2`.
2. Em `0x445CF4`, o inicializador passa `ator+0x18` como base de uma
   estrutura de coordenadas. Em `0x445D5E`, ele chama
   `0x40321F → 0x410E16` com quatro valores locais
   `(-256, -64, 256, 64)`. A rotina guarda os dois primeiros
   como deslocamentos e subtrai cada limite inferior do superior
   para obter as dimensões 512 × 128, nas unidades internas.
3. `0x410E36` chama `0x401D7F → 0x410E48`. Essa rotina soma
   os deslocamentos locais ao par de origem e escreve os quatro
   limites na estrutura. Convertidos para offsets relativos ao ator,
   os campos ficam assim.

| Campos do ator | Operação no caminho observado |
| --- | --- |
| `+0x18`, `+0x1C` | Origem X e Y usada como entrada. |
| `+0x20`, `+0x24` | Dimensões calculadas pela diferença entre os limites locais. |
| `+0x28`, `+0x2C` | Deslocamentos locais da esquerda e do topo. |
| `+0x30`, `+0x34` | Esquerda e topo, somando origem e deslocamento. |
| `+0x38`, `+0x3C` | Direita e base, somando as dimensões à esquerda e ao topo. |

## Leitura da origem

A chamada em `0x43DD6F` passa o ator e um endereço na pilha para
`0x401320 → 0x445FEA`. No caminho em `0x446014`, a rotina
copia `+0x18` e `+0x1C` ao par de saída. Se `+0x78` contém um
ponteiro e o bit `0x20` de `+0x80` está ativo, o X ainda vem de
`+0x18` e o Y vem da chamada indireta em `0x445FFF` com os dois
bits inferiores limpos.

## Consulta de sobreposição

O slot `+0x1C` da tabela de métodos de `CCastMK` fica em
`0x471ECC` e contém `0x401843 → 0x412D29`. Em `0x412D32`,
o método obtém o endereço dos limites em `ator+0x30` e, em
`0x412D36`, passa esse endereço e o primeiro argumento a
`0x40119F → 0x4096E8`.

Para os retângulos A e B, com campos ordenados como esquerda, topo,
direita e base, a rotina devolve 1 quando todas as comparações abaixo
são verdadeiras. As comparações são de inteiros de 32 bits com sinal.

```text
A.esquerda < B.direita
A.topo     < B.base
A.direita  > B.esquerda
A.base     > B.topo
```

Para retângulos de área positiva, apenas encostar pelas bordas dá
resultado zero. Com sobreposição, o método em `0x412D29` retorna o
próprio ator em `0x412D61`, após uma chamada condicional em
`0x412D5C`. Sem sobreposição, retorna zero se o segundo argumento
for nulo ou delega a consulta em `0x412D77`.

A evidência confirma uma consulta geométrica com os limites do ator.
A finalidade da consulta no jogo, os chamadores indiretos que passam
um `CCastMK`, a ligação com `STAGE_0` ou `CRock` e a resposta física
seguem em aberto. Também faltam as unidades das coordenadas e a
origem do Y alternativo.

## Uso do retorno da chamada em 0x445F60

No slot `+0x18` de `CCastMK`, quando o bit `0x01` do byte
`ator+0x81` está ativo e o bit `0x40000000` de `ator+0x84`
está limpo, o ramo alcança `0x445F53`. O código empilha zero,
Y e X e coloca o endereço fixo `0x4AA398` em `ECX`.
A chamada em `0x445F60` (offset de arquivo `0x45360`) segue
`0x4033F0 → 0x40F512`.

Entre a entrada em `0x40F512` (offset `0xE912`) e o retorno
em `0x40F52C`, a única escrita em memória é o incremento do
byte em `ECX+0x1B24` (endereço `0x4ABEBC` neste ramo).
A função usa esse byte incrementado como índice na tabela
em `0x47B148` e retorna o byte da tabela estendido com sinal.
Os valores empilhados de X e Y ficam sem leitura nesse corpo.

O chamador transforma o bit baixo do retorno em `0x1E` ou
`0x1F` e empilha esse código para a chamada de `0x445F70`,
com `[ator+0x48]` em `ECX`. O trecho até `0x445F60` delimita
uma escrita no contador. A chamada seguinte
(`0x4035D5 → 0x426953`) ainda precisa ser rastreada para
verificar se alcança a origem do próprio ator.

Com o `exe` da seção de reprodução, confira estes intervalos.

```bash
for intervalo in 445ee9:445f75 4033f0:4033f5 40f512:40f52d \
                 4035d5:4035da 426953:426999; do
    objdump -d -Mintel --start-address="0x${intervalo%:*}" \
        --stop-address="0x${intervalo#*:}" "$exe"
done
```

## Origem do receptor em +0x78

Na inicialização, a chamada em `0x445CE8` passa zero a
`0x4027ED → 0x425586`, que retorna zero nesse caso. A instrução
em `0x445CED` grava o retorno em `CCastMK+0x78`. O caminho
de cópia em `0x445C22–0x445C28` usa o campo equivalente do
objeto fonte.

Após a leitura em `0x4462B6`, a chamada em `0x4462C2` usa
a mesma função, agora com `[ator+0x48]` como receptor e o endereço
`ator+0x50` como argumento. O retorno vai para `ator+0x78`
em `0x4462C7` (offset de arquivo `0x456C7`).

Em `0x425598`, a função busca no vetor apontado por
`[receptor+0x50]`. Se o retorno for zero, repete a busca em
`0x4255A8`, com o vetor apontado por `[[receptor+0x3C]+0x70]`.
Ambas as chamadas seguem `0x402E28 → 0x42967F`. A busca
percorre o vetor de ponteiros em `+0x04` até a contagem em
`+0x08`. Para cada entrada, `0x429697` chama
`0x4025EF → 0x4293AC`, que usa a comparação de strings em
`0x45DA70` com o argumento da busca e o ponteiro em
`entrada+0x08`. A primeira entrada que coincide é devolvida
em `0x4296AE`. Sem coincidência, a busca retorna zero.

Assim, neste caminho de leitura, o receptor de `0x42972C`
depende do nome em `ator+0x50` e da ordem dos vetores. Falta
demonstrar qual entrada corresponde a um registro de fase e
qual método concreto ocupa o slot `+0x10`. Segue em aberto
se a chamada pode escrever na origem do ator.

Com o `exe` da seção de reprodução, examine os intervalos abaixo.

```bash
for intervalo in 445c17:445c31 445cd2:445cfa 4462bb:4462ca 4027ed:4027f2 \
                 425586:4255b2 402e28:402e2d 42967f:4296b2 4025ef:4025f4 \
                 4293ac:4293c2 45da70:45db00 42970e:429736; do
    objdump -d -Mintel --start-address="0x${intervalo%:*}" \
        --stop-address="0x${intervalo#*:}" "$exe"
done
```

### Leitura do nome em +0x50

Na cadeia `0x4462B6 → 0x40374C → 0x429BA0`, o XOR
transforma a máscara `0x04` em `0x0E`, deixando o bit
`0x10` limpo. Se a verificação em `0x429BC1` retorna
um valor diferente de zero e `[fluxo+0x30] = 0`,
`0x429BE2` calcula o endereço `CCastMK+0x50`. A chamada
em `0x429BEA` (offset de arquivo `0x28FEA`) passa esse
endereço e o limite `0x0C` a `0x402E8C → 0x40AB0E`,
com o fluxo em `ECX`.

No caminho em `0x40AB3C`, a chamada em `0x40AB47` passa
a `0x45D930` o endereço de destino, tamanho de elemento 1,
quantidade 12 e o ponteiro do fluxo em `[fluxo+0x18]`.
Depois de copiar, o leitor procura o byte zero no destino
e ajusta o cursor em `[fluxo+0x24]` em `0x40AB60`.
Quando encontra o zero dentro do limite, o avanço conta
também o terminador. Em outro ramo, `0x40AB35` zera o
primeiro byte do destino. Portanto, a janela pedida à rotina
de leitura tem 12 bytes, mas o avanço pode ser menor.

Esse caminho identifica como o nome usado na busca chega
ao próprio `CCastMK`. O offset dessa janela em um registro
específico de A0.HDK e uma escrita na origem durante a
atualização continuam em aberto.

Com o `exe` da seção de reprodução, confira os intervalos abaixo.

```bash
for intervalo in 4462b1:4462ca 40374c:403751 429ba0:429c01 \
                 402e8c:402e91 40ab0e:40ab6d 45d930:45d956; do
    objdump -d -Mintel --start-address="0x${intervalo%:*}" \
        --stop-address="0x${intervalo#*:}" "$exe"
done
```

## Escrita da origem durante leitura de dados

O slot `+0x0C` da tabela de `CCastMK` (endereço `0x471EBC`)
contém `0x403198 → 0x44611F`. Esse método recebe o ator em
`ECX` e preserva seu endereço em `ESI`. Em `0x4462B6`, ele
passa o mesmo ator, o objeto de fluxo e a máscara `0x04` a
`0x40374C → 0x429BA0`. A rotina aplica XOR com `0x0A`
em `0x429BA8` e, após a verificação em `0x429BC1`, limita
a máscara a quatro bits. Em `0x429BF6`, passa `0x0E` e o mesmo ator a
`0x402C4D → 0x4125E2`.

Após a verificação em `0x4125FC`, se `[fluxo+0x30]` é zero e
o bit `0x02` de `[fluxo+0x34]` está ativo, o bit `0x02` da
máscara `0x0E` seleciona o caminho em `0x412733`. As chamadas
em `0x41273D` e `0x412744` leem quatro bytes para
`ator+0x18` (X) e depois quatro para `ator+0x1C` (Y).
Ambas passam por `0x404142 → 0x40B34C`, que encaminha o
endereço de destino e o tamanho a `0x40356C → 0x40A6D8`.
Em `0x40A71B`, a chamada de leitura recebe o destino, tamanho
de elemento 1, quantidade 4 e o ponteiro do fluxo. Na sequência,
`0x41274B` chama o recálculo dos limites com base em `ator+0x18`.

O caminho observado pode preencher a origem de um objeto já criado
com valores do fluxo. Continua em aberto se o chamador usa esse
caminho no carregamento inicial, na restauração de estado ou em
ambas as situações. O trecho também deixa em aberto a integração
por quadro, o salto e a gravidade. Para investigar uma atualização
em jogo, o próximo ponto é o slot `+0x18`, que contém
`0x402BD5 → 0x445DF3`. Esse método lê X e Y em `0x445EB6`
e `0x445EB9`. Vale rastrear o código que o chama e a origem
dos valores que ele lê.

## Despacho do slot +0x18 e uso da origem

Em `0x412CE5`, o laço zera o índice em `+0x4C` e percorre o
vetor em `+0x04` até o limite em `+0x08`. Para cada elemento
não nulo, lê sua tabela de métodos em `0x412CFF` e chama o
slot `+0x18` em `0x412D06`, passando o argumento recebido pelo
laço e o endereço do objeto que mantém o vetor. O método de
`CCastMK` também invoca esse laço sobre si em `0x445F7B`,
via `0x40241E → 0x412CE5`. A chamada indireta em `0x412D06`
alcança `0x402BD5 → 0x445DF3` quando a entrada usa a tabela
`0x471EB0`, cujo slot fica em `0x471EC8` (offset de arquivo
`0x70EC8`). A seção seguinte identifica um caminho de preenchimento
durante a leitura do fluxo.

No ramo que passa pelos testes em `0x445E70`, `0x445E78` e
`0x445E88`, quando a chamada em `0x445EA1` retorna um ponteiro
não nulo, o método lê Y e X do ator em `0x445EB6` e `0x445EB9`.
Em `0x445EBC`, passa os dois valores e um terceiro argumento a
`0x40259F → 0x415DA2`, com o ponteiro retornado em `ECX`.
A rotina passa X e Y a `0x401276 → 0x410CB0`, que os grava
nos dois primeiros campos de uma estrutura temporária em
`0x410CCB` e `0x410CD2`. Em seguida, chama
`0x40475A → 0x4122BA`, que copia dez palavras de 32 bits
dessa estrutura para o **objeto retornado**, começando em
`+0x18` (`0x4122D6`, `0x4122F3`). A rotina instala a
tabela `0x46E100` durante a cópia e depois troca para
`0x46E478` em `0x415E05`.

A cadeia confirma que outro objeto recebe o par de origem de
`CCastMK` durante sua inicialização. A escrita anterior que
produziu os valores do ator nesse ramo, a frequência do despacho
e a relação com movimento, gravidade e salto seguem em aberto.

### Escritas diretas e chamada indireta no slot +0x18

Em `0x445DF5`, `ESI` recebe o ponteiro de `CCastMK`. Quando
o bit `0x02` do byte em `[ator+0x48]+0x58` está ativo e
`ator+0x94` é diferente de zero, `0x445E0C` carrega em
`EAX` o endereço de `ator+0x94`. A escrita em `0x445E19`
(offset de arquivo `0x45219`) grava nesse endereço o valor
anterior menos um. Quando o resultado é zero, `0x445E1D`
limpa os dois bits inferiores de `ator+0x84`. As demais
escritas diretas no ator no corpo de `0x445DF3–0x445F85`
atingem `+0x84` (`0x445E4F`, `0x445EE3`, `0x445F4D`).
As instruções que acessam `+0x18/+0x1C` nesse corpo
leem o par para passá-lo como argumento.

O recorte deixa em aberto os efeitos das chamadas. Uma cadeia
ainda sem receptor resolvido passa por `0x445E68 → 0x401398
→ 0x42970E`. Quando `ator+0x78` guarda um ponteiro não nulo
e `ator+0x7C & 0x07` é diferente de zero, a chamada indireta
em `0x42972C` usa esse ponteiro como receptor do slot
`+0x10`. O retorno indexa a tabela em `0x46EE68`, cujo
valor participa do teste que controla o salto em `0x445E72`.
Falta identificar a tabela concreta do receptor para avaliar
efeitos indiretos sobre a origem de `CCastMK`. O decremento
comprovado em `+0x94` deixa em aberto a integração de posição.

Com o `exe` da seção de reprodução, confira os intervalos abaixo.

```bash
for intervalo in 445df3:445f86 401398:40139d 42970e:4297e3; do
    objdump -d -Mintel --start-address="0x${intervalo%:*}" \
        --stop-address="0x${intervalo#*:}" "$exe"
done
```

## Inserção de CCastMK no vetor durante a leitura

1. Em `0x446084`, o código registra o descritor de `CCastMK` em
   `0x4AD020` via `0x4035C1 → 0x409F75`. O registro encadeia o
   descritor na lista em `0x4AC00C`, e `0x446093` instala a
   tabela `0x471EF8` nesse registro. Os dois primeiros slots
   apontam para `0x401965 → 0x4460A4` e
   `0x403FAD → 0x4460E7`.
2. No caminho de leitura em `0x412888`, com o bit `0x01` ativo
   em `fluxo+0x34` e na máscara, o laço chama
   `0x4028C4 → 0x40A48D` em `0x4128B0`. Se o índice de
   16 bits lido do fluxo é negativo, `0x40A507` chama
   `0x402734 → 0x40A2DA`. Essa rotina lê o nome e procura
   o registro pela lista em `0x4AC00C`, via
   `0x402BFD → 0x409F9D`. Quando a busca retorna o registro de
   `CCastMK`, as chamadas indiretas em `0x40A322` e
   `0x40A32B` usam os dois slots acima.
3. O primeiro slot chama o inicializador
   `0x403CD3 → 0x445B32` em `0x4460CE` após obter memória
   para o objeto. O inicializador instala a tabela `0x471EB0`.
   O segundo slot recebe o ponteiro retornado e chama, em
   `0x4460F7`, seu slot `+0x0C`. Esse é o caminho que pode
   escrever X e Y do fluxo, nas condições descritas acima.
4. Se a leitura retorna sucesso, `0x40A33E` guarda o ponteiro
   no vetor de referências do fluxo. O ponteiro volta ao laço
   em `0x4128B5`. Quando é não nulo e o bit `0x80` em
   `ator+0x17` está limpo, `0x4128CF` chama
   `0x402275 → 0x459F54` com o objeto pai em `ECX`,
   o índice atual `[pai+0x08]` e o ator. A gravação em
   `0x459F74` (offset de arquivo `0x59374`) usa a base em
   `pai+0x04`, a mesma que o despacho em `0x412CF5` lê.

Em A0.HDK, a seção `ENEMY_0` contém 14 ocorrências do nome
`CCastMK` precedidas por `FF FF`, de `0x3636` a `0x3B30`
(offsets do nome). O caminho estático demonstra como um registro
com esse nome pode chegar ao vetor sob as condições acima.
Ainda falta reproduzir a leitura de um registro específico até
a inserção. A seção seguinte liga a leitura e o despacho no nível
da classe do pai.
A cadeia explica uma escrita da origem na leitura do objeto.
Escritas durante o movimento continuam em investigação.

## Leitura e despacho de CMarkRoot

O cabeçalho de `ENEMY_0` em A0.HDK identifica `CMarkRoot`.
No PE, o descritor em `0x46EEF0` aponta para esse nome em
`0x47DD4C`. A fábrica do descritor (`0x403E59 → 0x429D4B`)
chama o inicializador via `0x4020EA → 0x422A6B`. A instrução
em `0x422AB0` instala a tabela `0x46EBC8` no objeto.

- O slot `+0x0C` (`0x46EBD4`) aponta para
  `0x402252 → 0x429DC2`. Se a abertura do bloco retorna sucesso,
  `0x429DE3` passa o próprio objeto e a máscara `0x1B` para
  `0x40374C → 0x429BA0`. Esse método aplica XOR com `0x0A`
  e mantém os quatro bits baixos em `0x429BEF`, deixando
  a máscara `0x01` para a serialização compartilhada chamada em
  `0x429BF6`. No ramo de leitura, com o bit `0x01` ativo também
  no fluxo, o caminho pode inserir entradas no vetor do próprio
  `CMarkRoot` em `0x4128CF`.
- O slot `+0x18` (`0x46EBE0`) aponta para
  `0x402108 → 0x41233A`. A chamada em `0x41233E` preserva
  o receptor em `ECX` e encaminha o primeiro argumento a
  `0x40241E → 0x412CE5`. O laço lê o vetor do receptor e
  pode chamar o método de `CCastMK` em `0x412D06`.
- Um chamador externo desse slot aparece em `0x42331F`.
  Em `0x422389`, a chamada ao mesmo inicializador produz o
  ponteiro guardado em `[esi+0x38]` em `0x422392`, quando a
  alocação tem sucesso. Em `0x423318`, o chamador carrega esse
  campo para `ECX` e passa dois argumentos zero. A correspondência
  entre campo e tabela identifica o caminho de despacho para o
  objeto criado ali. Ainda falta vincular essa instância à leitura
  de `ENEMY_0` e determinar quando esse chamador roda.

O encadeamento comprova que a classe pode preencher e despachar
seu próprio vetor. Permanece em aberto qual rotina chama o slot
de leitura para a instância despachada e qual escreve X/Y de
`CCastMK` durante movimento.

## Uso da origem no despacho do slot +0x14

Em `0x423157`, o chamador carrega de `[ebx+0x38]` o receptor
e em `0x42315E` chama seu slot `+0x14`. Para a tabela de
`CMarkRoot` (`0x46EBC8`), esse slot contém
`0x402BF3 → 0x412C38`. Em `0x412C5A`, o método chama
`0x401C6C → 0x412C6D` com o mesmo receptor. O laço usa
o vetor em `+0x04` e a contagem em `+0x08`, e chama o slot
`+0x14` das entradas não nulas em `0x412C8E`
(offset de arquivo `0x1208E`).

Para uma entrada `CCastMK`, o slot em `0x471EC4` aponta para
o mesmo método `0x412C38`. Se `[ator+0x44]` é diferente de
zero, a chamada em `0x412C4C` passa o próprio ator como
primeiro argumento a `0x402095 → 0x40953C`. No ramo que
alcança `0x409595`, o código recupera esse ponteiro, lê X/Y
em `0x40959B` e `0x40959E` e soma os valores a
`[ebp+0x14]` e `[ebp+0x18]` em `0x4095A4` e
`0x4095A7` (offsets `0x89A4` e `0x89A7`). Essas
gravações atingem argumentos na pilha da chamada. O método
`0x412C38` também percorre o vetor de filhos do próprio ator.

Esse caminho identifica outro consumidor da origem.

### Destino das coordenadas no ramo de desenho

No caminho que começa em `0x412C42`, a chamada para
`0x402095 → 0x40953C` passa zero como terceiro argumento.
Em `0x409547`, a rotina lê esse zero em `ESI` e o salto
em `0x409558` chega a `0x409595`. Quando a referência de
uma entrada da lista em `ator+0x44` passa pelo filtro em
`0x4095BE–0x4095CA`, as instruções em `0x4095D2` e
`0x4095D6` leem Y e X locais da entrada. As somas em
`0x4095DB` e `0x4095DE` acrescentam a esses valores as
coordenadas acumuladas nos argumentos da pilha, que já
incluem a origem do ator.

Em `0x4095F3` (offset de arquivo `0x89F3`), a rotina
passa X e Y resultantes, a referência gráfica ajustada e
o endereço do retângulo de recorte a
`0x401230 → 0x408C75`. O receptor da chamada indireta
em `0x408D59` vem do ponteiro global em `0x4AA3E0`.
A chamada usa os argumentos de desenho descritos no
[mapa do motor](engine-map.md#associação-com-recursos-gráficos).
Esse recorte mostra o consumo da origem para desenhar uma
peça. A busca por uma escrita na origem do próprio ator
continua nos ramos do despacho `+0x18`.

Para reproduzir, use o mesmo `exe` da seção
[Reprodução local](#reprodução-local).

```bash
objdump -s --start-address=0x471ec4 --stop-address=0x471ec8 "$exe"
for intervalo in 402bf3:402bf8 412c38:412c63 402095:40209a \
                 40953c:40955a 409595:40960b 401230:401235 \
                 408c75:408caf 408d3d:408d5c; do
    objdump -d -Mintel --start-address=0x${intervalo%:*} \
        --stop-address=0x${intervalo#*:} "$exe"
done
```

## Cópia da origem para o objeto retornado em 0x43DE7B

No slot `+0x18`, a chamada em `0x445EDE` passa o `CCastMK`
como argumento de `0x403D37 → 0x43DD53`. A rotina mantém o
ator em `ESI` e, em `0x43DD6F`, usa a [leitura da origem](#leitura-da-origem)
para preencher o par local em `[EBP-0x24]`.

O caminho estudado prossegue quando o bit `0x02` de `ator+0x84`
está limpo e os campos `ator+0x5C` e `ator+0x60` estão zerados
nos testes iniciais (`0x43DD81–0x43DD8D`). A instrução em
`0x43DDA2` lê o byte com sinal de `ator+0x89` e a de
`0x43DDD0` o guarda em `[EBP-0x18]`. Quando o fluxo chega
a `0x43DE4F`, o teste desse valor desvia para `0x43E0CB`
se ele for zero. Com valor diferente de zero, o fluxo segue
para a chamada indireta em `0x43DE7B`. O retorno substitui
o argumento original na pilha em `0x43DE83`. Para valores de
`ator+0x8D` fora do intervalo de 1 a 9, o salto em
`0x43DEB7` leva ao caminho que passa o objeto retornado em
`ECX` e o endereço do par local na pilha, em `0x43E0C6`.

A cadeia `0x40159B → 0x418C93 → 0x402964 → 0x41D14E`
desempacota X e Y e grava o par em `+0x18` e `+0x1C`
do receptor. As escritas ficam em `0x41D15A` e `0x41D160`
(offsets de arquivo `0x1C55A` e `0x1C560`). Em `0x41D163`,
`0x401D7F → 0x410E48` recalcula os limites desse receptor.
A mesma rotina zera outros campos, entre eles `+0xAC` e `+0xB0`.
No ator de origem, esse caminho grava zero em `+0x5C`
(`0x43E0BF`), campo distinto da origem.

O fluxo sustenta uma inicialização de coordenadas do objeto
retornado pela chamada indireta. A classe concreta desse objeto e
a natureza dos campos zerados seguem em aberto. A integração
por quadro, a gravidade e o salto permanecem sem evidência.

Para reproduzir este recorte em leitura, use o mesmo `exe` da
seção abaixo e confira as instruções nestes intervalos.

```bash
for intervalo in 445ed7:445ee3 403d37:403d3c 43dd53:43dde9 \
                 43de4f:43dec4 43e0b6:43e0cb 40159b:4015a0 \
                 418c93:418cb9 402964:402969 41d14e:41d180 \
                 401d7f:401d84 410e48:410e6e; do
    objdump -d -Mintel --start-address="0x${intervalo%:*}" \
        --stop-address="0x${intervalo#*:}" "$exe"
done
```

## Destino da cópia quando o campo +0x89 é zero

No mesmo despacho do slot `+0x18`, `0x43DD53` conserva o
`CCastMK` em `ESI` e lê seu par de origem para `[EBP-0x24]`
em `0x43DD6F`. Depois dos testes iniciais descritos acima,
`0x43DDCD` zera `[EBP-0x4]`. O caminho que passa por
`0x43DE28` requer `ator+0x8C` diferente de zero, bit
`0x04` de `ator+0x84` limpo e retorno diferente de zero
da consulta em `0x43DE0A`.

A chamada indireta em `0x43DE28` usa o receptor retornado
pela chamada em `0x43DE1C`. Seu retorno vai para
`[EBP-0x4]` em `0x43DE30` (offset de arquivo `0x3D230`)
e para `ator+0x60` em `0x43DE4C`. Com `ator+0x89`
igual a zero, o salto em `0x43DE53` chega a `0x43E0CB`.
Esse trecho põe `[EBP-0x4]` em `ECX` e verifica se é
diferente de zero antes de passar o par local à chamada em
`0x43E0E4` (offset de arquivo `0x3D4E4`).

A cadeia já identificada, `0x40159B → 0x418C93 → 0x402964
→ 0x41D14E`, grava X e Y em `+0x18` e `+0x1C` do
receptor em `0x41D15A` e `0x41D160`. Nesse caminho a
escrita tem como destino o ponteiro guardado em `ator+0x60`.
O recorte abaixo identifica esse receptor para `ator+0x8C = 1`.
O valor gravado vem da leitura da origem, sem integração
demonstrada. Gravidade, salto e resposta à sobreposição seguem
em aberto.

Para reproduzir o recorte, use o mesmo `exe` da seção
[Reprodução local](#reprodução-local).

```bash
for intervalo in 43dd53:43de59 43e0cb:43e0ee 40159b:4015a0 \
                 418c93:418cb9 402964:402969 41d14e:41d180; do
    objdump -d -Mintel --start-address="0x${intervalo%:*}" \
        --stop-address="0x${intervalo#*:}" "$exe"
done
```

### Receptor quando o campo +0x8C vale 1

Em `0x43DE01`, o byte em `CCastMK+0x8C` é estendido com
sinal e passado com a tabela `0x4831B0` a
`0x401307 → 0x408594`. A busca percorre pares de 32 bits
(chave, ponteiro) até um ponteiro nulo. O par em `0x4831B8`
liga a chave 1 ao nome `CRobot0` em `0x485980`.

Com o nome retornado, `0x43DE1C` chama
`0x402BFD → 0x409F9D`, que procura o descritor na lista
cuja cabeça está em `0x4AC00C`. O registro em `0x43E3C4`
passa o par nome/tamanho em `0x46FD08` a
`0x4035C1 → 0x409F75`, encadeia o descritor de
`CRobot0` em `0x4ACCE8` e instala nele a tabela `0x471150`
em `0x43E3D3`.

Quando a busca retorna esse descritor, a chamada em `0x43DE28`
(offset de arquivo `0x3D228`) usa o primeiro slot,
`0x40143D → 0x43E3E4`, com `[EBX+0x04]` como argumento.
Em `0x43E3FE`, o slot pede `0x124` bytes ao alocador
`0x4029F0 → 0x409D2D`. Com alocação bem-sucedida,
`0x43E414` chama `0x403CB5 → 0x43E2FB` com o bloco
em `ECX`. O inicializador instala a tabela `0x4710E0`
em `0x43E310` e devolve o mesmo ponteiro em `0x43E316`.

Sob essas condições e com `CCastMK+0x89` igual a zero, as
escritas em `0x41D15A` e `0x41D160` copiam a origem
do `CCastMK` para o `CRobot0` recém-criado. A análise
é condicional à seleção do descritor e ao sucesso da alocação.
Os demais valores de `+0x8C` e um escritor da origem do
próprio `CCastMK` durante atualização seguem em aberto.

Com o mesmo `exe` da seção [Reprodução local](#reprodução-local):

```bash
for intervalo in 43dde9:43de59 401307:40130c 408594:4085b2 \
                 402bfd:402c02 409f9d:409fc9 43e3c4:43e41f \
                 4035c1:4035c6 409f75:409f95 40143d:401442 \
                 4029f0:4029f5 409d2d:409dcb 403cb5:403cba \
                 43e2fb:43e31c; do
    objdump -d -Mintel --start-address="0x${intervalo%:*}" \
        --stop-address="0x${intervalo#*:}" "$exe"
done
for intervalo in 4831b8:4831c0 485980:485988 46fd08:46fd10 471150:471154; do
    objdump -s --start-address="0x${intervalo%:*}" \
        --stop-address="0x${intervalo#*:}" "$exe"
done
```

## Experimento de tradução de 0x40963E para C

O trecho `0x409663–0x40969E` processa uma entrada de 12 bytes
com referência, X e Y. A soma da referência com a base usa 32 bits.
Os bits 26 a 29 selecionam o banco e os bits 0 a 14 selecionam
o índice. Em `0x409683` o ponteiro do banco vem de `0x4ABD74`
com passo de 20 bytes. A entrada do banco tem passo de 8 bytes.
`0x40968D` e `0x409696` leem largura e altura com extensão de
sinal de 16 para 32 bits. As somas com X e Y produzem direita
e inferior com aritmética de 32 bits.

Uma tradução manual temporária em C99 recebeu largura e altura
diretamente. Com GCC local e `ctypes`, a comparação com
`resolve_pieces` de `tools/render_cearth.py` passou em 36 casos.
O teste combinou os índices 0–2, as máscaras
`0`, `0x8000`, `0x80000000`, `0x80008000` e os pares
`(0,0)`, `(-12,3)`, `(8,-20)`. A base foi `0x87FFFFFF`,
a referência foi a máscara combinada com `índice+1` e o
banco sintético usou dimensões `(4,7)`, `(9,2)`, `(2,6)`.
A comparação usou banco, índice, X, Y, X+largura e Y+altura.
Três casos adicionais conferiram o retorno da soma da referência
a zero, o banco 15 com índice 32767 e dimensões -32768/-1, e
o retorno da soma de coordenadas aos limites de 32 bits.
A tradução usou `uint32_t` nas somas. O teste deixou a consulta
da tabela, a lista terminada por zero e a união fora do recorte.

A rotina completa chama a entrada de importação `0x4AFA34`
em `0x4096AB`. A tabela PE32 associa essa entrada a
`USER32!UnionRect`. Uma tradução completa precisaria de uma
substituição com a mesma convenção de chamada e semântica.

O construtor de `x86rec.py` abre `game/sh3.exe`, aplica patches
específicos e lê endereços de instruções exportados pelo Ghidra
em `re/out/functions.json` ou pelo fluxo legado. Também importa
Capstone, ausente no Python local. A aplicação direta ao
Handkerchief exigiria adaptar a entrada PE32, remover os patches
de SH3 para esse alvo e fornecer os metadados do recorte. O
recompilador ficou sem execução. A tradução manual só usa
tipos inteiros de largura fixa e foi compilada no host. A
compilação ARM ficou sem verificação por falta de um compilador
ARM/Vita encontrado nesta sessão.
[Código-fonte consultado, blob f51bac97](https://github.com/matfsouza/sh3vita/blob/main/recomp/x86rec.py).

```bash
exe=/home/ivanderson/Projetos/handkerchief-vita/original-data/HDKWIN.EXE
objdump -d -Mintel --start-address=0x40963e --stop-address=0x4096c6 "$exe"
objdump -p "$exe" | rg -i 'user32|unionrect'
```

## Reprodução local

Execute na raiz do worktree. `objdump` lê as seções em disco e
`sha256sum` identifica a versão analisada.

```bash
exe=/home/ivanderson/Projetos/handkerchief-vita/original-data/HDKWIN.EXE
sha256sum "$exe"
objdump -s --start-address=0x4718c8 --stop-address=0x4718d0 "$exe"
objdump -s --start-address=0x488c98 --stop-address=0x488ca0 "$exe"
objdump -s --start-address=0x471ecc --stop-address=0x471ed0 "$exe"
objdump -s --start-address=0x471ebc --stop-address=0x471ecc "$exe"
objdump -s --start-address=0x471ef8 --stop-address=0x471f00 "$exe"
rg -aob -o 'CCastMK' "${exe%/*}/SYSTEM/A0.HDK"
for intervalo in 445b82:445bbc 401bdb:401be0 445cd2:445d67 \
                 40321f:403224 410e16:410e3e 401d7f:401d84 \
                 410e48:410e6e 43dd53:43dd74 401320:401325 \
                 445fea:446027 401843:401848 412d29:412d81 \
                 40119f:4011a4 4096e8:409717 \
                 403198:40319d 44611f:44614a 4462b1:4462ca \
                 40374c:403751 429ba0:429c01 402c4d:402c52 \
                 4125e2:412614 41271c:41276a 404142:404147 \
                 40b34c:40b360 40356c:403571 40a6d8:40a72a \
                 402bd5:402bda 445df3:445f86 40241e:402423 \
                 412ce5:412d1c 40259f:4025a4 415da2:415e12 \
                 401276:40127b 410cb0:410cf6 40475a:40475f \
                 4122ba:41231c 446084:44609e 4035c1:4035c6 \
                 409f75:409fc9 412888:4128dd 4028c4:4028c9 \
                 40a4f4:40a525 402734:402739 40a2da:40a34b \
                 402bfd:402c02 401965:40196a 4460a4:4460da \
                 403cd3:403cd8 445b32:445b9c 403fad:403fb2 \
                 4460e7:44610a 402275:40227a 459f54:459f7b; do
    objdump -d -Mintel --start-address="0x${intervalo%:*}" \
        --stop-address="0x${intervalo#*:}" "$exe"
done

python3 -B tools/inspect_hdk.py --classes --bounds "${exe%/*}/SYSTEM/A0.HDK"
objdump -s --start-address=0x46eef0 --stop-address=0x46eef8 "$exe"
objdump -s --start-address=0x46f138 --stop-address=0x46f140 "$exe"
objdump -s --start-address=0x46ebc8 --stop-address=0x46ebe4 "$exe"
for intervalo in 429d2b:429d45 429d4b:429d7e 422ab0:422ac2 \
                 429dc2:429dec 41233a:412346 42236f:422395 \
                 42330c:423322 403e59:403e5e 4020ea:4020ef \
                 402252:402257 402108:40210d; do
    objdump -d -Mintel --start-address="0x${intervalo%:*}" \
        --stop-address="0x${intervalo#*:}" "$exe"
done

objdump -s --start-address=0x46ebdc --stop-address=0x46ebe0 "$exe"
objdump -s --start-address=0x471ec4 --stop-address=0x471ec8 "$exe"
for intervalo in 423157:423161 402bf3:402bf8 412c38:412c63 \
                 401c6c:401c71 412c6d:412ca0 402095:40209a \
                 40953c:40955d 409595:4095aa 445ed7:445ee3 \
                 403d37:403d3c 43dd53:43dd74; do
    objdump -d -Mintel --start-address="0x${intervalo%:*}" \
        --stop-address="0x${intervalo#*:}" "$exe"
done
```

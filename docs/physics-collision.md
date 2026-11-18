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
`0x70EC8`). Falta identificar um ponto de preenchimento do vetor
com um `CCastMK` para confirmar essa passagem.

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
                 4122ba:41231c; do
    objdump -d -Mintel --start-address="0x${intervalo%:*}" \
        --stop-address="0x${intervalo#*:}" "$exe"
done
```

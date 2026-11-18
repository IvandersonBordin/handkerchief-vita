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
origem do Y alternativo. O próximo recorte da B02 é rastrear uma
escrita em `+0x18` ou `+0x1C` fora da inicialização.

## Reprodução local

Execute na raiz do worktree. `objdump` lê as seções em disco e
`sha256sum` identifica a versão analisada.

```bash
exe=/home/ivanderson/Projetos/handkerchief-vita/original-data/HDKWIN.EXE
sha256sum "$exe"
objdump -s --start-address=0x4718c8 --stop-address=0x4718d0 "$exe"
objdump -s --start-address=0x488c98 --stop-address=0x488ca0 "$exe"
objdump -s --start-address=0x471ecc --stop-address=0x471ed0 "$exe"
for intervalo in 445b82:445bbc 401bdb:401be0 445cd2:445d67 \
                 40321f:403224 410e16:410e3e 401d7f:401d84 \
                 410e48:410e6e 43dd53:43dd74 401320:401325 \
                 445fea:446027 401843:401848 412d29:412d81 \
                 40119f:4011a4 4096e8:409717; do
    objdump -d -Mintel --start-address="0x${intervalo%:*}" \
        --stop-address="0x${intervalo#*:}" "$exe"
done
```

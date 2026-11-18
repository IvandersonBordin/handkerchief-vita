# Formato HDK

Esta nota descreve a estrutura observada em `A0.HDK`, `B0.HDK` e
`A0_A.HDK`, em `original-data/SYSTEM/`.

## Estrutura confirmada

Cada arquivo começa no deslocamento zero e contém dez blocos consecutivos.
Os quatro primeiros bytes de cada bloco guardam seu comprimento como um
inteiro de 32 bits em little-endian (byte menos significativo primeiro).
O comprimento inclui esses quatro bytes. Assim, o deslocamento inicial
somado ao comprimento aponta para o início do próximo bloco ou para o fim
do arquivo. Em seguida vêm o rótulo da seção e o nome da classe, cada qual
terminado por um byte zero.

Por exemplo, em `A0.HDK`, `ROCK_0` começa em `0x0059`. Os bytes
`70 06 00 00` indicam comprimento `0x0670`, e a soma dá `0x06C9`,
onde começa `VISTA_0`.

## Seções e deslocamentos

Os deslocamentos são hexadecimais e apontam para o campo de comprimento.

| Seção | Classe | A0.HDK | B0.HDK | A0_A.HDK |
|---|---|---:|---:|---:|
| `FLUG_0` | `CFListStage` | `0x0000` | `0x0000` | `0x0000` |
| `ROCK_0` | `CRock` | `0x0059` | `0x005B` | `0x0055` |
| `VISTA_0` | `CGameRoot` | `0x06C9` | `0x0711` | `0x093E` |
| `STAGE_0` | `CGameRoot` | `0x0921` | `0x08ED` | `0x0A3D` |
| `FRONT_0` | `CGameRoot` | `0x1E41` | `0x1B9C` | `0x1FEC` |
| `GIMK_0` | `CGimkRoot` | `0x334F` | `0x3030` | `0x3624` |
| `EXIT_0` | `CGimkRoot` | `0x350C` | `0x3195` | `0x3703` |
| `ITEM_0` | `CGimkRoot` | `0x35F7` | `0x3231` | `0x379D` |
| `ENEMY_0` | `CMarkRoot` | `0x3615` | `0x324F` | `0x37BB` |
| `OUT_0` | `CMarkRoot` | `0x3B8C` | `0x3503` | `0x3CF7` |

## Validação dos limites

Cada rótulo terminado em zero foi localizado independentemente do campo de
comprimento e apareceu uma única vez por arquivo. Para cada bloco, o fim
calculado coincidiu com o início do seguinte. O fim calculado de
`OUT_0` coincidiu com o tamanho real do arquivo nas três amostras.

| Arquivo | Blocos validados | Fim exclusivo | Tamanho em bytes |
|---|---:|---:|---:|
| `A0.HDK` | 10 | `0x3C43` | 15.427 |
| `B0.HDK` | 10 | `0x357C` | 13.692 |
| `A0_A.HDK` | 10 | `0x3D52` | 15.698 |

Os 30 blocos cobrem integralmente os três arquivos, sem sobreposições,
lacunas ou bytes restantes.

## Validação do conjunto de arquivos

O conjunto em `original-data/` contém 108 arquivos `.HDK`. Os 99 arquivos
com conteúdo passaram na validação estrutural externa com
`tools/inspect_hdk.py`.

Os nove arquivos de `original-data/USER/USER02.HDK` até
`original-data/USER/USER10.HDK` têm zero bytes e foram corretamente
rejeitados pelo leitor com a mensagem `arquivo vazio`.

A validação cobre os limites dos blocos e os nomes das seções. A
integridade dos registros internos e a função dos arquivos `USER`
continuam sem confirmação.

## Hipóteses sobre a serialização interna

- **Contagem e marcadores de registros.** Em `OUT_0`, os dois bytes
  antes do primeiro `FF FF` valem `05 00`, `03 00` e `02 00`, na
  ordem da tabela. Esses valores coincidem com as ocorrências de `COut`.
  O campo pode ser uma contagem de objetos de 16 bits em little-endian.
  `FF FF` também antecede nomes como `CCastMK` e pode marcar o início
  de um registro tipado. Comparar `OUT_0` e `EXIT_0` ajuda a testar se
  a contagem e os limites explicam todo o conteúdo dos blocos.
- **Organização de `CRock`.** Há repetições como `27 D7 F0 E0` em
  `A0.HDK`, `27 D7 EB E0` em `B0.HDK` e `27 C7 E4 E0` em
  `A0_A.HDK`. A associação com células, linhas ou sequências
  compactadas ainda precisa explicar os campos iniciais e o tamanho dos
  dados nas três amostras.
- **Campos e relações entre classes.** Os nomes de atores e controladores
  próximos a `CCastMK`, como `CAPistol` e `CCtrlChase`, podem ser
  sub-registros ou referências. A comparação de registros repetidos,
  começando por `COut`, `CExit` e `CPlate`, pode revelar campos
  fixos, textos variáveis e possíveis coordenadas.
- **Conteúdo de `FLUG_0`.** O bloco termina com `EOF` seguido de byte
  zero nos três arquivos. O significado de nomes como `skey80a`,
  `w40a`, `skey160a` e `def` segue em aberto.

A identificação da engine e a viabilidade do port para PS Vita seguem
em aberto.

## A0.HDK, STAGE_0 (issue #6)

A Etapa 82 depende da conclusão e da revisão técnica da
[Etapa 81](character-identification.md#etapa-81--folhas-locais-dos-candidatos-visuais).
O commit `a1afce2` registra a entrega e a validação local da issue #5.
A aprovação técnica da Etapa 81 ainda precisa ser confirmada. A análise
dos registros de `STAGE_0` fica pendente dessa confirmação.

O ponto `(852, 540)` é uma referência candidata para a janela de estudo.
Ainda faltam evidências sobre sua relação com a posição inicial e com
a colisão. A interpretação de `CEarthFluct` e o valor efetivo do ajuste
selecionado pelo índice 26 também precisam ser verificados antes de
usar essas referências na composição gráfica.

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

Para listar também as classes e localizar os dados de cada seção em uma
cópia local, execute:

```sh
python3 -B tools/inspect_hdk.py --classes original-data/SYSTEM/A0.HDK
```

A coluna `dados` indica o deslocamento absoluto do primeiro byte após
o terminador da classe. Se o bloco terminar no próprio terminador,
esse deslocamento coincide com o fim exclusivo do bloco. A opção exige
um nome de classe em ASCII visível e terminado em zero dentro do bloco.
Sem a opção, o inspetor continua listando o deslocamento e o rótulo.

Para localizar o intervalo de cada seção sem interpretar o campo de
classe, use o modo `--bounds`:

```sh
python3 -B tools/inspect_hdk.py --bounds original-data/USER/USER01.HDK
```

A primeira coluna aponta para o campo de comprimento. A coluna `fim`
indica o primeiro deslocamento fora do bloco, em hexadecimal. A coluna
`comprimento` mostra a diferença entre esses dois deslocamentos, em
bytes decimais. Com apenas `--bounds`, o inspetor aceita blocos sem nome de
classe válido.

Na validação externa do inspetor, o menor bloco aceito tem seis bytes.
São quatro para o comprimento, um para o rótulo e um para o terminador.
Com dados sintéticos, `--bounds` aceita esse mínimo e rejeita um comprimento
de cinco bytes ou um rótulo cujo terminador só apareça no bloco seguinte.
Se um bloco posterior for inválido, o comando encerra com código `1` e
deixa a saída padrão vazia, mesmo após ler um bloco válido.

Para ver o intervalo dos dados de cada seção junto da sua classe, combine
as opções.

```sh
python3 -B tools/inspect_hdk.py --classes --bounds original-data/SYSTEM/A0.HDK
```

Os dados ocupam o intervalo de `dados` (inclusivo) até `fim` (exclusivo).
Se os dois deslocamentos forem iguais, o bloco termina logo após a classe.
O modo combinado exige uma classe válida em cada bloco. Os testes
sintéticos cobrem blocos consecutivos, um bloco sem dados após a classe e
um campo de comprimento truncado.

## Validação do conjunto de arquivos

O conjunto em `original-data/` contém 108 arquivos `.HDK`. Os 99 arquivos
com conteúdo passaram na validação estrutural externa com
`tools/inspect_hdk.py`.

Os nove arquivos de `original-data/USER/USER02.HDK` até
`original-data/USER/USER10.HDK` têm zero bytes e foram corretamente
rejeitados pelo leitor com a mensagem `arquivo vazio`.

A leitura com `--classes` aceitou 98 arquivos, somando 980 blocos.
Todos repetem a sequência de dez pares seção/classe da tabela acima.
Uma segunda leitura percorreu os comprimentos com `struct.unpack_from`
e delimitou os dois campos terminados em zero dentro de cada bloco,
confirmando a sequência e as contagens.

### Exceção em USER01.HDK

O modo padrão lista 17 rótulos, com blocos consecutivos até o fim
exclusivo em `0x1613` (5.651 bytes). No primeiro bloco,
`a0_FLUG_STAGE_0`, o byte imediatamente após o terminador do rótulo,
em `0x0014`, é zero. Por isso, `--classes` retorna
`bloco em 0x0000: nome da classe inválido`.

Ao procurar o primeiro zero após cada rótulo dentro do respectivo bloco,
o trecho anterior a esse byte fica vazio em oito blocos, contém apenas
bytes de `0x21` a `0x7E` em cinco e contém bytes fora desse intervalo
em quatro. Essa contagem descreve os bytes no arquivo. A função desses
campos em `USER01.HDK` e a integridade dos registros internos seguem
em aberto.

Os testes sintéticos verificam que o modo padrão lista um bloco com
campo de classe vazio e que `--classes` o rejeita. Também verificam
se os dois modos detectam um campo de comprimento incompleto após um
bloco válido e apontam seu deslocamento. Na linha de comando, esses
testes conferem o código de saída e a ausência de listagem parcial
em caso de erro. Um arquivo sintético com dois blocos também confirma
que a busca pelo terminador da classe para no fim do primeiro bloco,
mesmo quando há bytes zero no bloco seguinte. Para rodar a suíte na
raiz do worktree, execute `PYTHONPATH=tools python3 -B -m unittest discover -s tests -v`.

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

A investigação da Etapa 82 usa a cópia local de `A0.HDK` e a revisão
da Etapa 81 informada pelo coordenador. A janela de referência é
`[820, 884) × [508, 572)`, escolhida com 32 unidades em cada direção
a partir do ponto candidato `(852, 540)`. A janela serve à análise
gráfica. A posição inicial, a colisão e a física seguem em aberto.

### Limites e classes

`STAGE_0` ocupa `[0x0921, 0x1E41)` (comprimento `0x1520`).
O campo de contagem em `0x093D` vale 60. A busca delimitada por
marcadores de classe encontrou 60 ocorrências, sendo 51 `CEarth`,
um `CEarthFluct`, sete `CPlate` e um `CNumPlate`. Nos 51 registros
`CEarth`, os controles são `0B 0B`. Cada lista começa após os campos
`<IiiIH`, contém elementos `<Iii` de 12 bytes e termina com contagem de
filhos zero imediatamente antes do marcador seguinte. As bases de
referência dos 51 registros são zero.

Dos 284 elementos `CEarth`, 281 apontam para o banco 2 antes do ajuste,
dois para o banco 3 e um para o banco 1. Os três últimos têm posições
de destino com X ≥ 3936, à direita da janela. Os índices brutos cabem
nas respectivas tabelas (245, 715 e 98 entradas para os bancos 1, 2 e
3). No banco 2, todos os recortes brutos cabem no BMP de 640 × 1128.
A tabela do banco 3 começa em `0x48CE08`, endereço passado ao
registrador do descritor `0x4ABDAC`, conforme o
operando imediato no offset `0x50D8C` do PE.

### Recortes que cruzam a janela antes do ajuste

Os offsets são absolutos em `A0.HDK`. O destino foi calculado como
posição do objeto mais deslocamento local; a origem de destino vem
desses campos, enquanto seu tamanho depende da referência efetiva.
`Src` usa `(x, y, largura, altura)`; `Dest` usa
`(esquerda, topo, direita, fundo)` com bordas finais exclusivas.
Os seis resultados usam a tabela bruta do banco 2, equivalente a
simular ajuste zero. Um recorte só entra na lista se houver área de
interseção positiva em ambos os eixos.

| Objeto | Elemento | Referência bruta | Banco/índice | Seletor | Src bruto | Dest com tamanho bruto |
|---|---|---|---|---:|---|---|
| `0x0F13` | `0x0F3C` | `0x08198229` | 2/553 | 25 | (480, 240, 160, 120) | (680, 432, 840, 552) |
| `0x0F13` | `0x0F48` | `0x08198229` | 2/553 | 25 | (480, 240, 160, 120) | (868, 428, 1028, 548) |
| `0x1041` | `0x105E` | `0x08198225` | 2/549 | 25 | (480, 0, 160, 120) | (764, 420, 924, 540) |
| `0x1041` | `0x1076` | `0x081A820E` | 2/526 | 26 | (0, 784, 80, 40) | (816, 516, 896, 556) |
| `0x15E9` | `0x165A` | `0x081A81F6` | 2/502 | 26 | (0, 672, 80, 16) | (776, 532, 856, 548) |
| `0x15E9` | `0x1666` | `0x081A81F6` | 2/502 | 26 | (0, 672, 80, 16) | (856, 532, 936, 548) |

O cruzamento dos destinos brutos com a janela resulta nos seguintes
retângulos, com bordas finais exclusivas. O cálculo usa o maior valor
entre as bordas iniciais e o menor entre as bordas finais em cada eixo.

| Elemento | Interseção com a janela | Largura × altura |
|---|---|---:|
| `0x0F3C` | (820, 508, 840, 552) | 20 × 44 |
| `0x0F48` | (868, 508, 884, 548) | 16 × 40 |
| `0x105E` | (820, 508, 884, 540) | 64 × 32 |
| `0x1076` | (820, 516, 884, 556) | 64 × 40 |
| `0x165A` | (820, 532, 856, 548) | 36 × 16 |
| `0x1666` | (856, 532, 884, 548) | 28 × 16 |

Uma conferência local, a partir da primeira tabela, verificou a extração
de banco, índice e seletor e a igualdade entre as dimensões de origem e
destino nos seis casos. A releitura dos registros e dos bancos depende
da cópia local dos originais.

As origens dos objetos são `(956, 488)` em `0x0F13`,
`(860, 472)` em `0x1041` e `(588, 580)` em `0x15E9`.
Em particular, `0x1076` guarda o deslocamento `(-44, 44)`,
resultando na origem de destino `(816, 516)`.

O [mapa do motor](engine-map.md#associação-com-recursos-gráficos)
documenta dois caminhos. O cálculo de limites consulta banco e índice
após somar a base. No caminho de desenho de `CEarth`, a instrução em
`0x4095E7` (offset `0x89E7` no PE) lê um inteiro de 32 bits em
`0x4ABED4 + 4 × seletor` e `0x4095EE` o soma à referência. O offset
`0x8489` corresponde à leitura equivalente em outra rotina, em
`0x409089`.

Na cópia analisada, a seção `.data` começa em `0x47A000`, com
tamanho gravado de `0x30200` bytes e tamanho virtual de `0x34C2C`
bytes. Os slots 25 e 26, em `0x4ABF38` e `0x4ABF3C`, caem no
trecho preenchido com zeros pelo carregador, entre `0x4AA200` e
`0x4AEC2C` (fim exclusivo). Isso estabelece o valor no carregamento
da imagem. O valor durante o desenho da fase continua em aberto.

A busca pelo endereço-base literal na desmontagem encontrou quatro
leituras. As de `0x409089`, `0x4094F5` e `0x4095E7` somam o
valor à referência gráfica. Em `0x42CCAF`, o código multiplica o
valor lido por 40 e soma o resultado a um campo local de 16 bits.
A busca também incluiu os endereços literais dos dois slots. Escritas
por ponteiros ou por operações sobre intervalos ficam fora dessa
verificação. Uma forma de repetir a busca estática a partir deste worktree é

```sh
objdump -d original-data/HDKWIN.EXE |
    rg -i -A 3 '4abed4|4abf38|4abf3c'
```

Um ajuste durante a fase pode alterar banco, índice, dimensões e chave
de cor. A tabela de recortes acima descreve a geometria antes do ajuste.

### Escrita indireta na tabela de ajustes

A sequência `0x40D17F → 0x40D18B → 0x403B84 → 0x40D1FD`
passa a base fixa `0x4AA398` em ECX. O corpo preserva essa base
em ESI e, ao alcançar `0x40D340` (offset `0xC740`), grava
64 inteiros zero a partir de `ESI+0x1B3C = 0x4ABED4`.
Em seguida grava quatro inteiros com valor 2 a partir de
`ESI+0x1C3C`.

Outra cadeia chega à mesma tabela. Em `0x4530F4`, o código
carrega `0x4AA398` em ECX e chama `0x4023A6 → 0x40DA89`
em `0x4530F9`. A rotina preserva o receptor em ESI. Ela
atualiza os slots 0–15 em quatro grupos e, em `0x40DB00`,
prepara três grupos de 16 slots a partir de `ESI+0x1B7C`.
Para o primeiro desses grupos (slots 16–31), a condição vem
do inteiro em `ESI+0x1C40 = 0x4ABFD8`.

| Valor da condição | Escrita nos slots 25 e 26 |
| --- | --- |
| 0 | `0x40DB74` (offset `0xCF74`) zera o grupo. |
| 1 | `0x40DB63` (offset `0xCF63`) copia, respectivamente, os slots 9 e 10. |
| 2 | `0x40DB49` (offset `0xCF49`) copia, respectivamente, os slots 5 e 6. A primeira cópia desse ramo preenche os slots 16–19. |
| Outros | O laço pula as escritas diretas desse grupo. |

Isso identifica a origem dos ajustes nos ramos observados.
Falta estabelecer se o carregamento ou desenho de A0 alcança esse
chamador e qual condição está ativa nessa sequência. As duas chamadas
após o laço também ficam fora deste recorte.

```bash
exe=/home/ivanderson/Projetos/handkerchief-vita/original-data/HDKWIN.EXE
for intervalo in 40d17f:40d195 403b84:403b89 40d1fd:40d20d \
                 40d305:40d352 4530f4:4530fe 4023a6:4023ab \
                 40da89:40db90; do
    objdump -d -Mintel --start-address="0x${intervalo%:*}" \
        --stop-address="0x${intervalo#*:}" "$exe"
done
```

#### Chamador da atualização da tabela

A chamada direta em `0x453707` (offset `0x52B07`) segue
`0x403E9F → 0x452E6A`. O chamador passa EDI como receptor, um
endereço na pilha para a saída e ESI como segundo argumento. O
ramo em `0x4536F9` exige zero no byte em `[0x4AD228]+0x5C`
para alcançar a chamada.

O método zera a variável local `[EBP-4]` em `0x452E85`.
No caminho que passa por `0x453005`, o retorno da chamada em
`0x452FC3` é positivo e o método marca essa variável com 1.
Nesse ramo, `0x45302E` chama o slot `+0x08` de
`[receptor+0x38]`. Se o fluxo alcançar `0x4530A2` com a
variável marcada, `0x4530E9` chama o slot `+0x04` do mesmo
campo, passando o segundo argumento do método e dois zeros.
A atualização da tabela vem depois, em `0x4530F9` (offset
`0x524F9`). Os desvios em `0x453054` e `0x453063`
também condicionam a chegada a esse bloco.

Esse recorte estabelece a ordem entre os despachos e a escrita
gráfica nesse ramo. A origem do receptor e os destinos virtuais
estão detalhados abaixo.

Com o mesmo `exe` acima, confira os recortes seguintes.

```bash
for intervalo in 4536d8:453719 403e9f:403ea4 452e6a:452ea5 \
                 452fb5:45308c 4530a2:453108; do
    objdump -d -Mintel --start-address="0x${intervalo%:*}" \
        --stop-address="0x${intervalo#*:}" "$exe"
done
```

#### Instância e destinos virtuais do chamador

Em `0x451309`, o código passa o endereço da instância local
`[EBP-0x104]` em ECX a `0x404421 → 0x452514`. O
construtor guarda o receptor em ESI em `0x45252C` e instala
a tabela `0x472470` em `0x452541`. Em `0x45131B`, o mesmo
endereço volta a ECX para a chamada em `0x451321 → 0x401FD7
→ 0x45353A`. A instrução `0x453548` guarda ECX em EDI,
que volta a ECX em `0x453705` para a chamada em `0x453707`.
O método chamado preserva o receptor em ESI em `0x452E80`.

Com alocação bem-sucedida de `0x24` bytes em `0x4525F6`,
o construtor instala `0x46E200` no objeto alocado em
`0x45260D` e guarda seu ponteiro em `[ESI+0x38]` em
`0x452628` (offset `0x51A28`). O ramo de falha grava zero
nesse campo. Na inicialização bem-sucedida, as chamadas em
`0x45284F`, `0x4528CB`, `0x4528D9` e `0x45293A` usam
`0x402275 → 0x459F54` para inserir entradas no vetor desse
objeto. O vetor fica em `+0x04` e sua contagem em `+0x08`.

| Chamada | Entrada na tabela `0x46E200` | Destino e efeito |
| --- | --- | --- |
| `0x45302E`, slot `+0x08` | `0x46E208` (offset `0x6D208`) | `0x404246 → 0x4135E8`. Percorre o vetor e chama o slot `+0x08` de cada entrada não nula em `0x413604`. |
| `0x4530E9`, slot `+0x04` | `0x46E204` (offset `0x6D204`) | `0x4025B3 → 0x4135A0`. Percorre o vetor e encaminha os três argumentos ao slot `+0x04` de cada entrada não nula em `0x4135C8`. |

O caminho de `CMarkRoot` já documentado inicializa outro objeto
com a tabela `0x46EBC8` e grava seu ponteiro em `+0x38` em
`0x422392`. A coincidência do deslocamento não vincula os dois
receptores. No caminho de `0x453707`, a montagem do vetor acima
estabelece os despachos do contêiner. O vínculo desse caminho com
a leitura e o desenho de `STAGE_0` de `A0.HDK` segue em aberto.
Também falta determinar o valor de `0x4ABFD8` no desenho. Uma
escrita direta em `0x440C1D` guarda ali o retorno da chamada
virtual feita em `0x440C19`, pelo slot `+0x10` do subobjeto
em `ESI+0xB8`.

Com o mesmo `exe`, confira a tabela e os trechos citados.

```bash
objdump -s --start-address=0x46e200 --stop-address=0x46e20c "$exe"
for intervalo in 451300:451327 404421:404426 452514:452548 \
                 4525ee:45262b 452845:452854 4528c4:4528de \
                 45292b:45293f 402275:40227a 459f54:459f7b \
                 401fd7:401fdc 45353a:453551 4536f4:45370c \
                 452e6a:452e89 4025b3:4025b8 404246:40424b \
                 4135a0:413614 440c08:440c22; do
    objdump -d -Mintel --start-address="0x${intervalo%:*}" \
        --stop-address="0x${intervalo#*:}" "$exe"
done
```

### Registro CEarthFluct

O marcador em `0x093F` antecede um trecho com controles `0B 0B`
em `0x096C`. Interpretado provisoriamente pela estrutura de
`CEarth`, ele daria posição `(5960, 360)`, base `0x40000000` e
seis elementos em `[0x0980, 0x09C8)`, seguidos de dois bytes zero até
o próximo marcador em `0x09CA`. O primeiro elemento tem referência
bruta `0x081A8202` (banco 2, índice bruto 514, seletor 26).
A estrutura anterior aos controles e o tratamento da posição e da base
pela classe ainda precisam de confirmação. Esse trecho ficou fora da
seleção geométrica de `CEarth`.

### Reprodução local

Na raiz do projeto, o trecho abaixo lê os arquivos originais e imprime
os recortes brutos que cruzam a janela. Os limites das listas são
conferidos usando os marcadores como verificação independente.

```sh
PYTHONPATH=tools python3 -B - <<'MAPA'
from pathlib import Path
import re
import struct

from inspect_hdk import iter_sections
from render_cearth import read_bank, read_bitmap

original = Path("original-data")
data = (original / "SYSTEM/A0.HDK").read_bytes()
sections = {name: offset for offset, name in iter_sections(data)}
start, end = sections["STAGE_0"], sections["FRONT_0"]
assert (start, end) == (0x0921, 0x1E41)
records = list(re.compile(rb"\xff\xff(C[A-Za-z0-9]+)\0").finditer(data, start, end))
assert len(records) == struct.unpack_from("<H", data, start + 28)[0] == 60
exe = (original / "HDKWIN.EXE").read_bytes()
addresses = (0x48AFF8, 0x48B7A8, 0x48CE08, 0x48D120)
banks = {i: read_bank(exe, a, b) for i, (a, b) in
         enumerate(zip(addresses, addresses[1:]), 1)}
_, rows = read_bitmap((original / "BITMAP/HDKMAPA0.BMP").read_bytes())
record_ends = [record.start() for record in records[1:]] + [end]
for record, record_end in zip(records, record_ends):
    if record[1] != b"CEarth":
        continue
    fields = record.end() + 2
    first = fields + struct.calcsize("<IiiIH")
    assert first + 2 <= record_end, f"CEarth em 0x{record.start():04X}: cabeçalho truncado"
    assert data[record.end():fields] == b"\x0b\x0b"
    _, x, y, base, count = struct.unpack_from("<IiiIH", data, fields)
    stop = first + 12 * count
    assert base == 0 and stop + 2 == record_end
    assert data[stop:stop + 2] == b"\0\0"
    for offset in range(first, stop, 12):
        ref, dx, dy = struct.unpack_from("<Iii", data, offset)
        bank, index = (ref >> 26) & 15, ref & 0x7FFF
        assert 0 < ref < 0x40000000 and index < len(banks[bank])
        sx, sy, w, h = banks[bank][index]
        left, top = x + dx, y + dy
        if bank == 2:
            assert 0 <= sx < sx + w <= len(rows[0])
            assert 0 <= sy < sy + h <= len(rows)
        right, bottom = left + w, top + h
        if max(left, 820) < min(right, 884) and max(top, 508) < min(bottom, 572):
            print(f"{record.start():04X} {offset:04X} {ref:08X}",
                  bank, index, (ref >> 16) & 63, (sx, sy, w, h),
                  (left, top, right, bottom))
MAPA
```

Os testes sintéticos do leitor e da geometria podem ser executados a
partir da raiz, mesmo sem os arquivos originais.

Para o BMP indexado, o leitor exige 54 bytes para o cabeçalho de arquivo
e o cabeçalho DIB de 40 bytes. O teste de truncamento verifica todos os
tamanhos de 2 a 53 bytes que preservam a assinatura `BM`.

Um BMP sintético de 3 × 2 pixels verifica que o leitor entrega as
linhas com origem no topo tanto para altura positiva quanto negativa.
Cada linha gravada tem um byte de preenchimento com valor 255, fora da
paleta de duas cores. O teste confirma que esse byte é ignorado, de modo
que as coordenadas de origem dos recortes sejam medidas apenas sobre
os pixels da imagem. Também confere a conversão da paleta de BGR para
RGBA opaco.

Para o banco de retângulos, um PE32 sintético verifica que o leitor
aceita o terminador dentro do limite explícito quando a seção `.data`
está completa. Ao retirar o último byte da seção, o leitor rejeita o
arquivo mesmo quando o registro do banco e o terminador ainda cabem.

```sh
PYTHONPATH=tools python3 -B -m unittest discover -s tests -v
```

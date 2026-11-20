# Candidatos visuais à transformação pelo chapéu

A Etapa 72 compara os bancos gráficos 5 a 8 a partir da pista de que
o personagem controlado pega um chapéu e assume uma forma semelhante à
de um policial azul ou roxo. Essa pista orienta a comparação. A
[estrutura das tabelas e o catálogo](engine-map.md#catálogo-dos-bancos-5-a-8)
permitem localizar os recortes pelo banco e pelo índice, contado desde zero.

## Material local

A única gravação encontrada no repositório e nos dados originais foi
`original-data/XSIVGA~1.AVI`. O vídeo dura 9,6 segundos e mede 320 × 240.
A amostragem a dois quadros por segundo mostra a vinheta da XSIV Games,
sem cenas de jogo. A comparação abaixo usa os atlas opacos e duas folhas
menores, com a paleta de origem e os mesmos índices do catálogo.

## Grupos comparados

Os intervalos são inclusivos e representam amostras para comparação.
A vizinhança na tabela ajuda a reconhecer silhuetas recorrentes.
A ordem de reprodução das poses em jogo continua desconhecida.

| Papel investigado | Banco e índices | Evidência | Confiança na associação |
|---|---|---|---|
| Forma roxa após o chapéu | **6**, 1–20 e 32–44 | Figura roxa com quepe de aba, faixa vermelha e botas escuras. Há poses eretas, inclinadas com pernas em passada, horizontais e com braço ou objeto apontado. É o grupo que mais combina com a descrição de policial roxo. | Alta para o agrupamento visual, baixa para o vínculo com o ator controlado. |
| Alternativas de forma azul | **5**, 129–140 e 157–168. **6**, 193–206 e 388–406. | No banco 5 aparecem figuras azul-clara e azul-escura com poses eretas e horizontais. No banco 6, a primeira amostra é azul-esverdeada e tem um adereço na cabeça. A segunda é azul-escura, com equipamento volumoso nas costas e poses de passada e horizontais. As silhuetas do equipamento e da cabeça dão melhores critérios de comparação do que a cor sozinha. | Média para distinguir os grupos, baixa para a hipótese de que algum corresponda ao policial azul da pista. |
| Forma anterior ao chapéu | **5**, 686–720 | Figura de roupa predominantemente clara, sem o quepe destacado do grupo roxo. Aparece de perfil, em pé, em passada e deitada. É uma amostra para comparar com um futuro quadro antes da coleta. | Baixa. A aparência humana e as poses recorrentes também aparecem em outras figuras. |
| Efeito ou variante clara | **6**, 469–486 | Figuras em amarelo muito claro, com poucos detalhes e várias poses. A silhueta é legível, mas a cor e a sequência sozinhas não estabelecem uma ligação com a coleta do chapéu. | Indeterminada para a transformação. |

O grupo roxo acima está no **banco 6**, conforme os rótulos do atlas.
No banco 7, predominam figuras mecânicas, robôs e veículos. No banco 8,
predominam figuras não humanas, objetos e efeitos. A inspeção desses
dois bancos não revelou uma sequência que combine tão bem com o
uniforme roxo quanto a amostra do banco 6.

## O que falta para identificar o personagem

Nenhuma amostra está ligada à entidade selecionada em A0. Também falta
evidência de que as variantes azul e roxa compartilhem uma forma inicial.
Um registro de gameplay contínuo, com o mesmo personagem antes da
coleta, no contato com o chapéu e após a troca, permitiria comparar
quepe, equipamento dorsal e poses com os recortes acima. Para confirmar
a ligação no motor, falta encontrar a escrita que troca a referência
gráfica do ator nesse evento e resolver seus bits até banco e índice.

As folhas locais são `build/etapa72-banco-5.png` e
`build/etapa72-banco-6.png`. A amostragem da vinheta está em
`build/etapa72-video-amostra.png`. As imagens ficam em `build/`.

## Etapa 80 — rastreamento estático dos itens de A0

A ligação entre a coleta do chapéu e a troca de aparência do ator
controlado ainda não foi encontrada. Em A0, o caminho concreto para
prosseguir começa em `GIMK_0`.

### Registros da fase

Percorrendo os blocos pelo comprimento com `iter_sections`, de
`tools/inspect_hdk.py`, foram encontrados 98 blocos `ITEM_0` nos 108 HDK
locais. Todos têm 30 bytes, classe `CGimkRoot` e os dois últimos bytes
zerados. O `USER01.HDK` tem outros rótulos de seção e os nove arquivos
restantes estão vazios.

Em `A0.HDK`, `ITEM_0` ocupa `0x35F7–0x3615` (fim exclusivo).
O campo de 16 bits em `0x3613` vale zero e termina o bloco. Na seção
`GIMK_0`, da mesma classe, o campo na posição equivalente vale 9 e é
seguido por `FF FF` e `CGimkItem`. O método em `0x42A4FB`, cujo
ponteiro está em `0x46DC18` via `0x403508`, passa o nome de
`CGimkRoot` ao leitor por meio da entrada em `0x46EF00`. Após a
chamada de `0x42A515`, usa o retorno como limite dos laços que
serializam objetos filhos. Em A0, os nomes de classe dos registros
`CGimkItem` de `GIMK_0` começam em `0x336F` (`PowerUp0`) e
`0x33BD` (`Master0`). Esses dois offsets são posições no HDK.

### Resolução do item e caminho de ativação

Os endereços abaixo são virtuais em `HDKWIN.EXE`. Para reproduzir a
leitura nesta cópia, as seções `.text`, `.rdata` e `.data` começam
nos endereços `0x401000`, `0x46E000` e `0x47A000`, nos offsets
`0x400`, `0x6D000` e `0x78A00`, respectivamente.

| Endereço | Evidência estática |
|---|---|
| `0x470DB8–0x470DC0` | Três entradas consecutivas na tabela virtual apontam, via saltos, para o método que retorna a entrada de classe `CGimkItem` (`0x44509F`) e para os métodos em `0x44538F` e `0x445431`, que passam o mesmo nome de classe ao leitor. |
| `0x4453CA–0x4453E0` | No ramo de leitura de `0x44538F`, a chamada em `0x4453D3` recebe a tabela em `0x47FCD8` e o endereço de `objeto+0x54`. Em seguida o método lê o primeiro ponteiro do descritor resolvido e o grava em `objeto+0x44`. |
| `0x47FD68` e `0x47FD78` | Pares de ponteiros (descritor, nome) para `PowerUp0` e `Master0`. Os descritores estão em `0x47FC78` e `0x47FCA8` e começam pelos ponteiros `0x496358` e `0x4963A0`. |
| `0x496358` e `0x4963A0` | As primeiras referências gráficas são `0x1005816A` e `0x10078194`. Extraindo os bits 26–29 e 0–14 das palavras brutas, obtêm-se banco 4, índice 362 para `PowerUp0` e banco 4, índice 404 para `Master0`. Esses valores antecedem a soma da base e os ajustes do desenho descritos no [mapa do motor](engine-map.md#associação-com-recursos-gráficos). |
| `0x470DE4` → `0x401D6B` → `0x445137` | Um método da mesma tabela virtual verifica o byte `objeto+0x58` e chama o slot virtual `+0x54` do argumento recebido. Se o byte estiver zerado e o retorno da chamada virtual for não negativo, grava 1 em `objeto+0x58` (`0x445170`) e chama `0x4268FC` via `0x403FE9` (`0x445195`). Entre os argumentos estão a posição do objeto e o valor em `descritor+0x14`, que vale `0x3A` para `PowerUp0`. |

Esse caminho confirma a configuração gráfica bruta dos dois objetos
de fase e uma mudança de estado no próprio `CGimkItem`. A hipótese de
que `PowerUp0` represente o chapéu ainda depende de identificar o
objeto acionado. O próximo teste estático mínimo é seguir o valor
`0x3A` a partir de `0x4268FC`, identificar o objeto criado ou
selecionado e procurar, no handler correspondente, uma escrita no
estado ou na referência gráfica do ator que recebeu a interação.
A classe do ator controlado, a pose inicial e a ligação com as poses
do banco 6 seguem em aberto.

## Etapa 81 — folhas locais dos candidatos visuais

A investigação da Issue #3, registrada no commit `7594383`, localizou
referências brutas do banco 4 para os objetos de fase `PowerUp0` e
`Master0`. A ligação entre o evento de coleta, o ator controlado e as
poses dos bancos 5 e 6 segue em aberto. Por isso, as folhas abaixo
priorizam o uniforme roxo do banco 6 e usam dois grupos azuis do
banco 5 para comparação visual.

As folhas foram geradas com `python3 -B tools/preview_actor_candidates.py`.
Os rótulos mostram o banco seguido do índice original, contado desde
zero. Os intervalos são inclusivos. Cada recorte está em escala 1:1 e usa
a paleta do BMP de origem.

| Banco | Índices | Recortes por folha | Dimensões | PNGs locais |
|---:|---|---:|---|---|
| 5 | 129–140 e 157–168 | 24 | 1024 × 49 | `build/actors-bank-5-opaco-01.png` e `build/actors-bank-5-chave-zero-01.png` |
| 6 | 1–20 e 32–44 | 33 | 1024 × 110 | `build/actors-bank-6-opaco-01.png` e `build/actors-bank-6-chave-zero-01.png` |

A variante opaca preserva todos os pixels do recorte. A variante
`chave-zero` torna transparentes apenas os pixels cujo índice de
paleta é zero. Essa simulação afeta 16.542 pixels no banco 5 e 37.909
no banco 6. No motor, a escolha da chave depende do bit 15 da
referência efetiva, que as coordenadas do recorte por si só não informam.

A validação local conferiu a sintaxe dos scripts, a rejeição de
índice ausente e de retângulos fora do BMP ou com largura zero, além
dos rótulos, dimensões, CRCs e pixels dos quatro PNGs. As 14 páginas
do catálogo completo foram regeneradas em diretório temporário dentro
de `build/` e coincidiram byte a byte com as referências locais.
As contagens do catálogo também coincidiram, incluindo o descarte
do índice 0 do banco 8. As folhas estabelecem quais recortes podem
ser comparados com gameplay ou com uma referência gráfica rastreada
no motor. A identidade do protagonista e a ordem das poses seguem
indeterminadas.


## Confirmação visual no Vita

Na prévia `HKRC00008`, Ivan reconheceu o protagonista no quarto
grupo, correspondente ao banco 6, índices 32–44. Descreveu o
personagem como um policial de roupa roxa e boina, com uma pistola.
A confirmação estabelece o grupo a priorizar no atlas. A referência
ao ator controlado, a situação em que essa aparência é usada e a
ordem e duração das poses ainda precisam ser confirmadas no motor.
[Controles da prévia](vista-walk.md#prévia-de-personagens-na-rodada).

### Sequência encontrada no executável

A leitura estática de `.data` encontrou em `0x49D8F0` quatro
pares consecutivos de ponteiro e inteiro de 32 bits. Cada ponteiro
leva a uma lista gráfica com um elemento e um terminador zero.

| Par na tabela | Lista gráfica | Referência bruta | Banco e índice | Segundo valor |
|---|---|---|---|---|
| `0x49D8F0` | `0x49D338` | `0x18008029` | 6, 41 | 1 |
| `0x49D8F8` | `0x49D350` | `0x1800802A` | 6, 42 | 1 |
| `0x49D900` | `0x49D368` | `0x1800802B` | 6, 43 | 1 |
| `0x49D908` | `0x49D380` | `0x1800802C` | 6, 44 | 1 |

As listas usam deslocamentos locais `(-20, -39)`. A tabela de
descritores repete o ponteiro inicial `0x49D8F0` em `0x49DA7C`,
`0x49DA8C`, `0x49DA9C` e `0x49DAAC`. A ordem dos registros é
verificável no binário. O estado que seleciona o descritor, o
significado do segundo valor e a duração de cada quadro aguardam
o rastreio da rotina consumidora.

Para reproduzir, use `objdump -s` com
`--start-address=0x49d338 --stop-address=0x49d398`,
`--start-address=0x49d8f0 --stop-address=0x49d910` e
`--start-address=0x49da78 --stop-address=0x49dab0`. A leitura
usa o `HDKWIN.EXE` local em modo somente leitura.

#### Vínculo com o descritor humanoide

O par em `0x47C708` guarda o endereço do descritor `0x4AD268`
e o nome `cgmHUM1a` em `0x47DB24`. A inicialização em
`0x45B902–0x45B916` grava `0x472600` no descritor e
`0x49DB38` no campo seguinte, `0x4AD26C`.
Na tabela que começa em `0x49DB38`, a entrada em `+0x18`
aponta para `0x49DB28`. As quatro palavras desse endereço
apontam para `0x49DA78`, `0x49DA88`, `0x49DA98` e
`0x49DAA8`. Cada um dos quatro registros guarda `0x49D8F0`
na segunda palavra, ligando o descritor nomeado à sequência 41–44.

A leitura estabelece uma rota entre o nome interno e os recortes
reconhecidos no Vita. Falta identificar a rotina que consulta
o campo em `0x4AD26C` por meio de um ponteiro, e verificar
qual estado seleciona a entrada `+0x18`. O significado de `1`
em cada par de quadro continua desconhecido.

Conferir a inicialização com `objdump -d -Mintel` no intervalo
`0x45B902–0x45B917` e os ponteiros com `objdump -s` nos
intervalos `0x47C708–0x47C710`, `0x47DB24–0x47DB2D` e
`0x49DB28–0x49DB58`, sempre em `HDKWIN.EXE` local.

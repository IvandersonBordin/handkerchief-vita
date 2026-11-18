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

## Etapa 81 — prévia local pendente de revisão

A execução local da Issue #3 consta no commit `7594383`. O relatório do
executor registra que a revisão técnica do líder ainda está pendente.
A Issue #5 exige essa revisão e a conclusão da #3 antes de iniciar a
prévia. A geração das folhas e os testes de recorte e PNG da Etapa 81
ficam pendentes até a confirmação do coordenador.

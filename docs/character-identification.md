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

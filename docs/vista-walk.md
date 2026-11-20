# Passeio em VISTA_0

O direcional move o marcador branco e magenta. A câmera acompanha o movimento horizontal. Círculo pausa ou retoma. START sai. A camada de vegetação usa as coordenadas pré-ajuste do leitor existente. O movimento é livre, sem colisões.

Com o VitaSDK configurado no PATH e em VITASDK, execute na raiz:

```sh
python3 -B tools/export_vista_walk.py
cmake -S vita-scenario -B build/vista-walk -DCMAKE_BUILD_TYPE=Release
cmake --build build/vista-walk
python3 -B .agents/skills/vita-port/scripts/check_vpk.py build/vista-walk/vista_walk.vpk
```

Para o teste autorizado, copie o arquivo local build/vista-walk/vista0.rgba para ux0:data/HKVW00001/vista0.rgba e instale o VPK HKVW00001. Tela vermelha indica falha de leitura ou tamanho incorreto. O arquivo de cenário é derivado da cópia do usuário e fica fora do pacote e do repositório.

Teste da câmera no Linux com dados sintéticos:

```sh
cc -std=c11 -Wall -Wextra -Werror -ffunction-sections -fdata-sections -Wl,--gc-sections \
  -idirafter "$VITASDK/arm-vita-eabi/include" vita-scenario/test_camera.c -o /tmp/hkvw-camera
/tmp/hkvw-camera
```

## Encerramento do teste no console

Após capturar o resultado de cada teste físico, encerrar o aplicativo
pelo identificador do título e conferir a confirmação. Com VitaCompanion,
usar `quit <TITLE_ID>`. Preservar os outros aplicativos abertos.

## Rodada híbrida de A0

O protótipo local `HKRC00007` (Handkerchief A0 Playtest) combina o fundo
de A0 e os contornos calculados pela rotina recompilada com o robô e
a física sintética da rodada de Carlos. O percurso acrescenta três
obstáculos azuis, uma linha de piso visível e uma chegada verde.
Direcional esquerda/direita move o robô, X pula e reinicia após a
chegada, e START sai.

No Linux, um teste com UBSan verificou a parada junto aos três
obstáculos, os saltos, a chegada e o reinício. O VPK passou nas
verificações ZIP e LiveArea. No Vita, os seis arquivos instalados
coincidiram com o pacote, e o aplicativo retornou
`HKRC00007 6/6 PASS scene=ready`. O VitaCompanion confirmou o
encerramento. O percurso completo ainda aguarda a primeira
validação manual no console.

O pacote experimental local está em
`build/playable-a0/out/recomp_a0.vpk`, fora do Git.
SHA-256 `3987c46736f35d7a52daeb1569e2a79560e74e7710ba8ca177a7484e437a5d84`.
A rodada lê os dados locais compartilhados em `ux0:data/HKRC00004`.

### Prévia de personagens na rodada

O protótipo local `HKRC00008` (Handkerchief A0 Actor Preview) lê
57 recortes em quatro grupos dos bancos 5 e 6 da cópia local.
O arquivo de pixels separado foi enviado ao Vita e verificado
byte a byte. O VPK contém o visualizador e o código da rodada.
A transparência do índice zero e a ordem por índice de catálogo
são escolhas experimentais. Identidade, ordem e duração das
animações originais seguem em investigação.

Quadrado ativa a prévia e alterna a reprodução automática durante
a caminhada. SELECT passa ao próximo grupo, na ordem banco 5
129–140, banco 5 157–168, banco 6 1–20 e banco 6 32–44. Triângulo
avança um quadro e pausa a reprodução automática. A caixa de
movimento continua sendo a da rodada sintética.

No Linux, o teste com UBSan passou para carregamento, mudanças
de quadro e grupo, recorte na borda e ausência de arquivo.
O pacote passou nos verificadores ZIP e LiveArea. No Vita, os
seis arquivos instalados coincidiram com o VPK e o diagnóstico
registrou `HKRC00008 6/6 PASS scene=ready actors=ready`.
O VitaCompanion confirmou o encerramento. No teste manual, Ivan
confirmou que a prévia funcionou e reconheceu o protagonista no
quarto grupo, banco 6, índices 32–44. A associação com as rotinas
do ator controlado e a ordem original dos quadros seguem em aberto.

O pacote local está em `build/actor-playtest/out/recomp_a0.vpk`,
fora do Git. SHA-256
`82cd3a2337f95cc257e5b79143fce48bf2dd132732c999f9e58e2e2664dac667`.

### Sequência do personagem roxo na rodada

A variante local `HKRC00009` (Handkerchief A0 Sequence) inicia
com o personagem roxo e percorre os índices 41–44 do banco 6
enquanto a rodada registra movimento horizontal. A ordem vem do
[descritor original](character-identification.md#sequência-encontrada-no-executável).
O avanço a cada oito atualizações de caminhada e o retorno ao
índice 41 ao parar são escolhas da demo. O estado e o tempo que
o jogo original atribui à sequência seguem em investigação.
SELECT e Triângulo continuam disponíveis para explorar o catálogo.

Inspirado em comparações por rotina e entradas repetíveis por
quadro, o teste Linux com UBSan verificou quatro imagens distintas,
a repetição do ciclo e o retorno à pose de partida. O VPK passou
nas verificações ZIP e LiveArea. O console estava desligado, então
a instalação e a validação visual desse pacote seguem pendentes.
[I.G.I. Recomp](https://github.com/HenryKun55/igi-recomp),
[PCRecomp](https://github.com/sp00nznet/pcrecomp).

Pacote experimental local `build/actor-sequence/out/recomp_a0.vpk`,
SHA-256 `8536d3671afcb27cec9f085797c196877cdaa9e8caab89cd49189f96da580bec`.
A variante usa os mesmos arquivos locais externos da prévia anterior.

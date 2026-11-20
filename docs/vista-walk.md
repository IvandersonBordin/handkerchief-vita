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

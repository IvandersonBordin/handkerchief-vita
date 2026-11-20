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

## Encerramento e versões no console de teste

Após capturar o resultado de cada teste físico, encerrar o aplicativo
pelo identificador do título e conferir a confirmação. Com VitaCompanion,
usar `quit <TITLE_ID>`. Preservar os outros aplicativos abertos.

A referência de limpeza é manter as três instalações mais recentes
do Handkerchief, usando os registros de teste para ordenar versões
de séries diferentes. Instalar a nova versão quando o teste for
autorizado. Ivan pode remover versões antigas pela LiveArea depois
do teste. Preservar arquivos de dados compartilhados pelas versões
mantidas.

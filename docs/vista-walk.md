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

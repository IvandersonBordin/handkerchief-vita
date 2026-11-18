# Smoke test nativo no PS Vita

O aplicativo em `vita-prototype/` verifica a saída de vídeo e a entrada do controle com
três barras de cor geradas em C. Ele usa as APIs de tela, controle e memória do VitaSDK.
Os dois quadros são preparados na inicialização e ocupam juntos 4,5 MiB de CDRAM,
incluindo o alinhamento. O protótipo usa os padrões de heap e pilha do SDK.
Cada linha tem 960 pixels visíveis e 64 pixels de preenchimento, gravados em
preto opaco na inicialização.
As imagens da LiveArea foram geradas pelo script `make_placeholder_assets.py`
da skill `vita-port`. O aplicativo é independente dos arquivos do jogo.

## Compilar e verificar o pacote

Antes de compilar, execute `python3 vita-prototype/check_livearea.py` na raiz
do worktree. A checagem usa apenas o Python 3 e confere dimensões, formato
indexado, estrutura, CRC e dados comprimidos dos três PNGs sintéticos, além
da ausência de transparência no ícone e das referências de imagem do XML.
Nas demais imagens, um bloco de transparência deve aparecer uma única vez,
entre a paleta e os dados da imagem, com pelo menos uma entrada e sem exceder
o tamanho da paleta.
O limite de descompressão é calculado a
partir das dimensões da imagem e do modo de entrelaçamento.

Com CMake e VitaSDK já disponíveis, aponte `VITASDK` para a instalação e adicione
a pasta `bin` do SDK ao `PATH`. Na raiz do worktree, execute:

```sh
cmake -S vita-prototype -B build/vita-smoke -DCMAKE_BUILD_TYPE=Release
cmake --build build/vita-smoke
python3 vita-prototype/check_livearea.py build/vita-smoke/handkerchief_smoke.vpk
python3 .agents/skills/vita-port/scripts/check_vpk.py build/vita-smoke/handkerchief_smoke.vpk
```

O primeiro comando configura a compilação para o Vita. O segundo gera
`handkerchief_smoke.vpk` com o ID `HKSM00001`. O terceiro exige exatamente
`eboot.bin`, `sce_sys/param.sfo` e os quatro recursos da LiveArea, sem
duplicatas, e compara os recursos com os arquivos locais. O quarto verifica
a estrutura do pacote, incluindo o cabeçalho do executável e os metadados.
O terceiro também rejeita `eboot.bin` e `sce_sys/param.sfo` vazios e lê
as duas entradas até o fim para conferir o CRC do ZIP.
Antes de selecionar o compilador, o CMake confere se os quatro recursos
sintéticos da LiveArea estão presentes. A lista usada nessa checagem também
define os caminhos de origem e destino desses recursos no VPK.
Durante a configuração, o CMake também confere os caminhos de
`vita-elf-create`, `vita-make-fself`, `vita-mksfoex` e `vita-pack-vpk`.
Se faltar um deles, a mensagem informa a variável do VitaSDK e o caminho verificado.

## Roteiro no console

1. Instale o VPK pelo VitaShell e abra **Handkerchief Smoke Test** na LiveArea.
2. Confirme a presença das barras vermelha, verde e azul, nessa ordem.
3. Segure X e verifique se a barra central fica amarela. Solte X e confirme que
   ela volta a ficar verde.
4. Pressione START e confirme o retorno à LiveArea. Abra o aplicativo novamente
   para verificar se ele inicia com a barra central verde.

## Roteiro adicional para a pausa (C03)

O pacote do relógio já validado fica em
`build/vita-smoke-c03-clock/handkerchief_smoke.vpk`. Confira sua identidade
com `sha256sum build/vita-smoke-c03-clock/handkerchief_smoke.vpk`. O SHA-256
esperado para esse arquivo é
`a400b9671e9fbda265dc6a2e0209d8fae11e6335299b72d43559f29255aefa43`.

Se precisar recompilar, os comandos da seção de compilação geram e verificam
`build/vita-smoke/handkerchief_smoke.vpk`. Calcule o SHA-256 desse arquivo
depois que os dois verificadores passarem. Em ambos os casos, registre no
resultado o caminho e o SHA-256 do VPK que efetivamente instalou.

1. Com a barra central verde, pressione Círculo para pausar. Mantenha Círculo
   pressionado e segure X. A barra deve continuar verde.
2. Solte Círculo e, ainda segurando X, pressione Círculo novamente. A barra
   deve ficar amarela. Solte X e confira o retorno ao verde.
3. Solte Círculo e pause novamente com a barra verde. Pressione o botão PS
   para ir à LiveArea e retome o aplicativo sem fechá-lo manualmente.
   Segure X e confirme que a barra continua verde. Isso verifica que a
   pausa permanece ao retomar o aplicativo suspenso.
4. Solte X e pressione START com o aplicativo ainda em pausa. Se voltar à
   LiveArea, abra o aplicativo sem fechá-lo manualmente e segure X, sem
   tocar em Círculo. Registre se a barra fica amarela ou continua verde.
   A barra amarela indica que o estado de pausa foi descartado. A barra
   verde é compatível com a retomada da sessão pausada.
5. Se houver uma ferramenta de diagnóstico já disponível que mostre o
   processo, registre se ele desaparece após START, antes de reabrir.
   Sem essa observação, registre o resultado como evidência do estado
   visual após a reabertura e deixe o encerramento do processo pendente.

O teste básico em console está registrado em `docs/agent-progress.md`.
A pausa, o encerramento completo com START e as medições de memória e FPS
ainda precisam de verificação no console.

Ao pressionar START, o loop escreve `Active loop time: <valor> us` na
saída padrão. O valor soma os intervalos entre atualizações da tela
enquanto o loop está ativo. O intervalo que termina no toque em Círculo
pertence ao estado anterior ao toque. Para conferir o relógio no console,
capture a saída padrão e compare duas sessões de duração total semelhante,
uma com uma pausa prolongada. A sessão com pausa deve registrar menos
tempo ativo. A captura dessa saída no Vita ainda precisa ser verificada.
No executável ARM deste pacote, a inicialização do VitaSDK tenta abrir
`tty0:` para a saída padrão. Se o ambiente de teste não tiver captura
desse canal, registre o tempo ativo como `não observado`. O aplicativo
não grava essa medição em arquivo.

Registre uma linha por observação. Use `não observado` quando o teste ou a
captura não puder ser feito.

| Observação | Resultado no PS Vita |
| --- | --- |
| SHA-256 do VPK instalado | |
| Círculo mantido com X pressionado, depois novo toque em Círculo | Cor durante a pausa e após a retomada |
| Retomada pelo botão PS com o aplicativo pausado | Cor ao segurar X |
| START em pausa | Tela exibida após o toque e cor ao segurar X depois da reabertura |
| Processo entre START e a reabertura | Ferramenta usada e processo presente ou ausente |
| Sessão sem pausa | Duração total aproximada e tempo ativo em µs |
| Sessão com pausa | Duração total aproximada, duração da pausa e tempo ativo em µs |

## Validação local

Um ZIP sintético com as seis entradas esperadas e os recursos atuais passou.
O verificador rejeitou cada uma das duas entradas principais quando vazia
ou com CRC incorreto, além de rejeitar um ícone diferente do arquivo local.
Os recursos locais passaram e o CMake confirmou o diagnóstico de VitaSDK ausente.

O modo de inspeção do pacote aceitou um ZIP temporário com as seis entradas
esperadas e rejeitou entrada ausente, extra, duplicada, recurso alterado e
arquivo que não era um ZIP. Os recursos locais também passaram na checagem
sem argumento. O CMake parou na configuração porque `VITASDK` estava ausente.

Uma simulação no Linux com GCC, avisos tratados como erros e UBSan conferiu
os pixels visíveis e o preenchimento de todas as linhas dos dois quadros.
Ela também confirmou a troca com X, o retorno da cor ao soltar X e a liberação
da memória após START. A checagem da LiveArea passou e o CMake informou que
`VITASDK` precisava ser definido.

Em cópias temporárias, o CMake indicou o caminho de cada recurso da LiveArea
quando removido ou substituído por um diretório. Macros simuladas no Linux
confirmaram os quatro pares de caminhos enviados ao empacotador. Os recursos
atuais passaram em `check_livearea.py`, e a configuração com todos eles
presentes continuou exibindo o diagnóstico esperado se `VITASDK` estivesse ausente.

O verificador exige uma única referência de imagem em cada seção esperada
do XML da LiveArea. O modelo atual e uma cópia com um elemento adicional
`frame` passaram. Cópias com seção duplicada, referência duplicada,
referência ausente, nome de imagem incorreto ou elemento aninhado na
referência foram rejeitadas. O CMake também confirmou o diagnóstico
esperado quando a variável `VITASDK` está ausente.

O verificador de conformidade exige que a terceira letra do nome de cada
bloco PNG seja maiúscula, conforme o bit reservado na
[especificação PNG](https://www.w3.org/TR/png-3/#5Chunk-naming-conventions).
Com CRC válido, uma cópia temporária do ícone com o bloco auxiliar `vpAg`
passou e outra com `vpag` foi rejeitada. Os três PNGs do protótipo passaram.

O verificador exige quatro letras ASCII no nome de cada bloco PNG. Em cópias
temporárias do ícone com CRC válido, rejeitou nomes contendo dígito, pontuação
ou byte fora de ASCII e aceitou um bloco auxiliar desconhecido com nome válido.

O verificador rejeita blocos críticos PNG que ele não reconhece. Com cópias
temporárias do ícone e CRC válido, a regressão confirmou a rejeição de um bloco
crítico desconhecido e a aceitação de um bloco auxiliar desconhecido. Os três
PNGs sintéticos do protótipo continuam passando na checagem.

A checagem aceitou os recursos atuais. Em cópias temporárias com CRC válido,
o verificador rejeitou um ícone com transparência e aceitou a mesma alteração
na imagem de fundo. Também rejeitou blocos de transparência duplicados, vazios,
maiores que a paleta, inseridos antes da paleta ou depois de um bloco de dados
da imagem, inclusive quando esse bloco estava vazio.

A checagem da LiveArea passou com os recursos atuais. Em cópias temporárias,
ela retornou código 1 para PNG ausente, cabeçalho truncado, dimensão incorreta,
XML malformado e referência de imagem incorreta.
A verificação dos blocos também retornou código 1 para PNG truncado após o
cabeçalho, CRC inválido, ausência do bloco final e dados depois desse bloco.
Cinco cópias com CRCs válidos também foram rejeitadas: três com métodos de
compressão, filtro ou entrelaçamento inválidos e duas com blocos duplicados
(`IHDR` e `PLTE`).

O verificador aceitou os três PNGs do protótipo, um PNG com dados divididos
em dois blocos `IDAT` e um PNG entrelaçado Adam7. Uma regressão com cópias
temporárias do ícone confirmou que os blocos `IDAT` consecutivos são aceitos,
inclusive com um bloco de texto após eles. Um bloco de texto entre os
`IDAT` faz o verificador rejeitar a imagem, mesmo com CRCs válidos.
Em seis cópias temporárias
com CRCs válidos, rejeitou compressão inválida, fluxo truncado, bytes após
o fluxo, tamanho descomprimido menor ou maior que o esperado e filtro de
linha inválido.

Uma leitura do controle que retorna erro agora encerra o teste com código 1,
desvincula o quadro da tela e libera a memória após a sincronização. Uma
leitura sem amostras aguarda a próxima atualização da tela. Uma simulação
temporária no Linux, compilada com avisos tratados como erros e UBSan,
confirmou os dois casos, a mudança de cor com X e a saída com START.

O código C foi compilado no Linux com avisos tratados como erros e verificação
de comportamento indefinido. Um teste temporário com chamadas simuladas confirmou
as cores, a troca ao segurar e soltar X, a saída com START, a liberação de memória
e os caminhos de falha na inicialização. Também foram conferidas as dimensões,
a profundidade e o formato indexado dos PNGs e as referências do XML.

A configuração padrão do CMake agora verifica `VITASDK` antes de selecionar
o compilador. Foram exercitados quatro cenários locais: variável ausente,
arquivo da toolchain ausente, macros de empacotamento ausentes e compilador
ARM ausente. Em cada caso o CMake encerrou com o diagnóstico esperado.
A checagem das ferramentas de empacotamento também foi exercitada com uma
configuração simulada no Linux. O CMake identificou cada uma das quatro
ferramentas ausentes e rejeitou um diretório no lugar do empacotador. A
configuração simulada com os quatro caminhos presentes foi aceita, e o
diagnóstico com `VITASDK` ausente continuou funcionando.
Na configuração padrão, o CMake também rejeita diretórios no lugar de
`vita.toolchain.cmake`, `vita.cmake` ou `arm-vita-eabi-gcc`. Em instalações
simuladas, os três caminhos ausentes e os três substituídos por diretórios
produziram o diagnóstico esperado com o caminho correspondente. A checagem
sem `VITASDK` e a validação dos recursos da LiveArea também passaram.

As verificações com toolchain simulada foram feitas antes da disponibilidade
do VitaSDK. A compilação ARM e as verificações do VPK real estão registradas
em `docs/agent-progress.md`.

Com CMake 4.4.4 no Linux, uma toolchain simulada indicada por
`-DCMAKE_TOOLCHAIN_FILE` chegou à etapa de configuração do empacotamento
com o ID `HKSM00001` e os quatro recursos da LiveArea. A configuração
encerrou com mensagens específicas quando a toolchain não definiu
`VITASDK`, quando `share/vita.cmake` estava ausente ou quando havia um
diretório no lugar desse arquivo. A configuração padrão sem `VITASDK`
também emitiu o diagnóstico esperado. A checagem dos recursos passou.

# Auditoria técnica de Gandalf — 19/11/2026

## A. Inventário

| Base inspecionada | Estado observado |
|---|---|
| `main` em `28a713c` | Worktree limpa, alinhada a `origin/main` local. Tem ferramentas Python para HDK, prévias e o protótipo `vita-scenario/`. O registro de 19/11 confirma caminhada animada e reinício da rodada sintética no Vita físico. |
| Bruno em `a3ef39a` | Worktree limpa e alinhada à referência de acompanhamento local. O supervisor aprovou a entrega documental e emitiu recibo ainda sem marcador de consumo para `b02-posicao`. A agenda indicava espera. A proposta local rastreia o vetor usado em `0x445F7B` e o slot `+0x18` até uma possível escrita de X/Y de `CCastMK`. |
| Carlos em `ce516ea` | Worktree limpa e alinhada à referência local. O protótipo da branch tem rodada sintética, animação provisória e cinco testes C no Linux. A agenda indicava espera, sem recibo local para essa revisão. C05 segue bloqueada. |
| Gandalf em `main` | `deploy/gandalf.py` separa parecer da autorização. Valida o snapshot em bwrap antes de chamar o revisor, exige tarefa do catálogo e recibo vinculado a agente, branch publicada e SHA, com consumo único e dependência opcional. As suítes locais cobrem o supervisor e o executor. |
| Runners e skills | `deploy/handkerchief-cycle` usa travas e o marcador compartilhado `usage-limit`, ausente na leitura. Há timer do supervisor a cada cinco minutos e configuração de intervalo de um minuto para sondagem dos executores. A regra do projeto prevê 20 minutos entre chamadas normais, e Gandalf limita suas chamadas a oito em 24 horas. A consulta ao barramento systemd da sessão foi negada, então o estado dos timers instalados ficou sem confirmação. A skill `vita-port` traz referências de recompilação e diagnóstico. As duas worktrees têm instruções e planos próprios. |

O conector GitHub confirmou a liberação da investigação estática de
movimento de Bruno. A aprovação anterior de `STAGE_0` abrange seis
recortes, com limitações documentadas. [Issue #8](https://github.com/IvandersonBordin/handkerchief-vita/issues/8), [issue #6](https://github.com/IvandersonBordin/handkerchief-vita/issues/6).

`file`, `sha256sum` e `objdump -p` confirmaram em leitura o PE32 i386
com base `0x00400000`, SHA-256
`3deeb9b18ac2f6ee7b4af4769d1499fdfe0bde8de44482b57ab4bf6196617b76` e
imports de DirectDraw, DirectSound, DirectPlay, WinMM e outras APIs Win32.
O compilador VitaSDK está instalado localmente. A meta do experimento
inicial é rodar em Linux e avaliar depois a compilação ARMv7.

### HDK e comportamento

- Os leitores existentes reconhecem limites, rótulos e identificadores de
  classes. As evidências de `CEarth` sustentam leitura de coordenadas,
  composição gráfica e seleção de bancos. Os BMPs externos fornecem os
  pixels usados pelas prévias. [Formato HDK](hdk-format.md), [mapa do motor](engine-map.md).
- `CRock`, relações entre registros, eventuais referências a funções,
  condições, ações e lógica interpretável permanecem hipóteses de estudo.
  Para avançar, identificar um campo pela leitura no executável e seguir
  seu valor até o consumidor. Confirmar em mais de uma amostra antes de
  atribuir semântica.
- O relatório de hardware comprova a rodada sintética, incluindo pose
  inicial após reinício. Movimento, colisão, gravidade, IA e eventos do
  jogo original continuam sem equivalência demonstrada.
  [Registro do teste](teste-rodada-2026-11-19.md).

## B. Matriz de ferramentas

A triagem usa fontes dos autores. Os requisitos de upstream e as
declarações de funcionamento ainda exigem reprodução antes de qualquer
adoção. Licença pendente significa que a revisão inicial não confirmou
uma licença para o componente pretendido.

**Última verificação das fontes desta ampliação** 19/11/2026. As
entradas preservadas mantêm as conclusões da auditoria da mesma data.
Os links apontam para os autores. Quando o estado de manutenção não
está explícito, permanece em aberto. Xita declara encerramento no
repositório, e Murk se apresenta como preservação de código histórico.

**CONFIRMADO** indica evidência no código ou na documentação de origem.
**HIPÓTESE** indica aplicação ao Handkerchief ainda por demonstrar.
**TESTADO** exige reprodução local com evidência registrada.
**NÃO APLICÁVEL** indica que a referência está fora do escopo atual.
Nesta rodada, a leitura das fontes confirma o conteúdo pesquisado. O
VitaSDK tem uso local documentado e os novos componentes não passaram
por execução local.

**Consulta por agente**

- **Bruno** consulta as seções de PE32/C++ e física, com prioridade a PCRecomp,
  I.G.I., SH4, SH3, Ghidra e Murk. Os resultados sobre HDK continuam no
  [formato HDK](hdk-format.md) e no [mapa do motor](engine-map.md).
- **Carlos** consulta DirectDraw, renderização e VitaSDK. Prioriza GTA Recomp
  para superfícies e paletas e o fluxo atual de testes para evidência local.
- **Gandalf** consulta todas as seções para comparar evidências, licenças e
  dependências antes de propor uma ação. Registrar novas verificações nesta
  matriz, reaproveitando a entrada quando o repositório já estiver catalogado.

Os cinco projetos individuais de [WolffsRoom](https://github.com/WolffsRoom)
continuam em suas próprias entradas. O perfil e a organização VitaSDK são
fontes de navegação. As notas históricas, a documentação de ferramentas em
`.agents` e os resultados técnicos mantêm suas fontes e escopos originais.

A classificação em negrito indica a utilidade para a estratégia atual.
O estado de evidência ao lado do responsável indica o que foi verificado.
A justificativa de cada classificação consta na mesma linha.

### Recompilação estática x86 → C e engenharia reversa PE32/C++

| Fonte | Componente, dependências e licença | Compatibilidade, limite e decisão |
|---|---|---|
| [PCRecomp](https://github.com/sp00nznet/pcrecomp) | Toolbox MIT de sp00nznet. Python nas ferramentas, também C e Java. `tools/pe/` lê PE e imports, `tools/disasm/` descobre funções, `tools/lift/` emite C, `tools/cpp/` recupera RTTI e vtables. `runtime/recomp32/` usa registradores globais, `runtime/recomp32_cpu/` explicita o estado da CPU e `runtime/win32hle/` usa libc/POSIX/SDL2 para Win32, COM, DirectDraw e DirectSound. Unicorn é opção de teste diferencial. | **EXPERIMENTAL**. **Bruno · muito alta · CONFIRMADO** para componentes descritos. **HIPÓTESE** para a função candidata de HDKWIN e os slots de CCastMK. Começar pelo emissor e pelo teste isolado. Conferir a revisão e o NOTICE antes de reutilizar código. Suporte a Vita por demonstrar. |
| [I.G.I.](https://github.com/HenryKun55/igi-recomp) | `tools/recomp` usa Python e Capstone. `runtime` usa memória convidada de 32 bits e modelo x87. `shim` cobre Win32, DirectDraw/D3D7, DirectSound e DirectInput. MIT na base, exceções LGPL nos patches vitaGL e bibliotecas associadas. Build com CMake/Ninja e VitaSDK no alvo. | **ÚTIL**. **Bruno · alta · CONFIRMADO**. Comparador prioritário de PCRecomp para o tradutor isolado e seus testes entre rotinas nativas e recompiladas. A descrição upstream cobre registradores, memória e x87 em níveis diferentes dos detalhes necessários ao recorte. Isso exige revisão do emissor e dos testes na revisão fixada. Shims e ganchos devem ser tratados como específicos até prova de reutilização. |
| [Silent Hill 4](https://github.com/vallfrr/sh4vita) | Python/Capstone, Ghidra 12/JDK 21 no fluxo de exportação, CMake/VitaSDK e vitaGL. GPL-3.0. | **ÚTIL**. **Bruno · alta · CONFIRMADO**. `x86rec.py` abre o PE32 de SH4 e trabalha com um catálogo de endereços e thunks. Emite uma função C por função convidada, preserva a pilha x86 em CALL/RET e modela x87 em oito doubles. CPU e emissão são referências. Lista de funções, IAT, despachos e runtime precisam dos dados da versão GOG v5. Avaliar seu método diferencial antes de copiar qualquer código. [Emissor](https://raw.githubusercontent.com/vallfrr/sh4vita/main/recomp/x86rec.py) |
| [Silent Hill 3](https://github.com/matfsouza/sh3vita) | `recomp/x86rec.py`, exportação Ghidra e implementação do runtime em `port/`. Python/Capstone, VitaSDK e vitaGL. GPL-3.0. | **REFERÊNCIA**. **Bruno · média · CONFIRMADO**. C por função PE32 e fronteira entre CPU convidada e chamadas externas. Bruno já identificou caminho fixo e patches específicos no emissor. Usar como contraste para imports, ponteiros, vtables e x87. Reaproveitamento de código depende de revisão da licença e das amarras ao executável. |
| [Recomp Toolkit](https://github.com/wraitii/recomp-toolkit) | Tradutor, configuração por jogo e runtime gráfico D3D7–9. A distribuição pede macOS 14 e Command Line Tools. Licença do componente exato pendente. | **REFERÊNCIA**. **Bruno · média · CONFIRMADO**. Apple Silicon é o alvo principal. A lista de jogos exige versões determinadas. Usar como referência para separar descoberta de funções, perfil de versão e substituições nativas. VitaSDK e adequação de Metal/Vulkan ao Vita sem demonstração. [Arquitetura](https://github.com/wraitii/recomp-toolkit/blob/main/docs/architecture.md) |
| [WindowsRecompTool](https://github.com/Zippingrocks/WindowsRecompTool) | PE32/i386, Zydis, emissão C++, CMake e Unicorn opcional para conformidade. MIT. | **ÚTIL**. **Bruno · média · CONFIRMADO**. Referência secundária para rejeitar instruções, imports e saltos indiretos sem suporte, vincular o alvo ao hash e comparar registradores, flags e pilha com referência independente. Saída C++ e Vita sem validação. |
| [Xita](https://github.com/Xita-Project/xita) | `recompiler/`, ferramentas Python para XBE e runtime de Xbox com D3D8 e DirectSound. GPL-3.0. | **REFERÊNCIA**. **Bruno · baixa · CONFIRMADO**. Referência conceitual para tradução por função, substituições e validação. O formato XBE e a interface de kernel do Xbox exigem um carregador distinto de PE32/Win32. |
| [GameReComp](https://github.com/heaven-hm/GameReComp) | heaven-hm, MIT. Pipeline de recuperação e geração de C com matriz de alvos e resultados upstream, incluindo PE32 i386 em macOS ARM64. | **EXPERIMENTAL**. **Bruno · média · CONFIRMADO** como referência de avaliação. Distinguir recuperação de funções, execução e equivalência com referência. |
| [Ghidra](https://github.com/NationalSecurityAgency/ghidra) | NSA, Apache-2.0. Framework Java de análise de binários, desmontagem e decompilação. Os ports SH3 e SH4 exemplificam exportação automatizada de funções. | **ESSENCIAL**. **Bruno · alta · CONFIRMADO** como ferramenta de análise estática. Conferir fronteiras, convenções de chamada e destinos indiretos contra a desmontagem. |

### Física, colisões, lógica e formatos de recursos

| Fonte | Componente, dependências e licença | Compatibilidade, limite e decisão |
|---|---|---|
| [Murk](https://github.com/moonwho101/Murk) | Jogo isométrico de Mark Longo, 1998, preservado por moonwho101. Código C++ MIT em `src/`, solução `src/Murk.sln` para Visual Studio 2022, Win32 e MFC. Usa DirectDraw, DirectSound/MIDI e código de rede da época do DirectPlay. | **ÚTIL**. **Bruno · alta · CONFIRMADO** como fonte histórica. **HIPÓTESE** para padrões de ciclo de atualização, entrada, objetos, colisão, desenho e áudio comparáveis. Priorizar rastrear um objeto do input ao desenho. Vincular campos de Handkerchief a consumidores próprios antes de atribuir semântica. |
| [SpaceCadetPinball](https://github.com/k4zmu2a/SpaceCadetPinball) | k4zmu2a, MIT. Decompilação C++ do 3D Pinball de Windows com SDL. | **REFERÊNCIA**. **Bruno · média · CONFIRMADO** como referência de separação entre recursos, simulação e funções da plataforma. A física do pinball é específica. |
| [isle](https://github.com/isledecomp/isle) | isledecomp, LGPL-3.0. Decompilação C++ de LEGO Island (Windows, 1997). | **ÚTIL**. **Bruno · média · CONFIRMADO** como referência de recuperação de classes e confronto com o binário da época. Classes e recursos pertencem ao jogo. |
| [OpenNox](https://github.com/opennox/opennox) | Repositório principal da equipe OpenNox. Motor de Nox em Go/C com SDL2, OpenAL e GCC segundo o projeto. GPL-3.0. | **REFERÊNCIA**. **Bruno · baixa · CONFIRMADO** como referência de migração incremental. O endereço histórico deu timeout nesta consulta. [Endereço anterior](https://github.com/noxworld-dev/opennox) |
| [fheroes2](https://github.com/ihhub/fheroes2) | ihhub e equipe, reconstrução C++/SDL de Heroes of Might and Magic II. GPL-2.0 e CC-BY-SA-4.0 aparecem no repositório. | **REFERÊNCIA**. **Bruno · média · CONFIRMADO** como referência de separação entre leitor e motor. Conferir escopo de licença por arquivo. |
| [OpenCMR2](https://github.com/pablocpas/OpenCMR2) | pablocpas, GPL-3.0. Port de Colin McRae Rally 2.0 sobre CMR2Decomp, com SDL3 e dados fornecidos pelo usuário. | **REFERÊNCIA**. **Carlos · média · CONFIRMADO** como precedente de adaptação de plataforma. Compatibilidade Vita e formatos HDK sem demonstração. |

### DirectDraw, DirectSound e Win32

| Fonte | Componente, dependências e licença | Compatibilidade, limite e decisão |
|---|---|---|
| [GTA Recomp](https://github.com/sp00nznet/gta) | sp00nznet, MIT no código do projeto. GTA 1997, London 1969/1961 e GTA 2 para Windows x86-64. `src/common/` inclui memória convidada, `src/engine/` cobre carregador e pontes de imports, `src/video/` implementa DirectDraw. `src/sound/` adapta Miles para waveOut. O README descreve slots COM, superfícies, Lock/Unlock, Blt/BltFast, Flip e paletas. | **ÚTIL**. **Carlos · alta · CONFIRMADO** como precedente Windows 2D. **HIPÓTESE** para extrair contratos de superfície e de chamadas COM úteis ao HDKWIN. Conferir pitch, formato de pixels, vida útil e paletas. O upstream marca partes como stubs e não demonstra execução ARM/Vita. |
| [cnc-ddraw](https://github.com/FunkyFr3sh/cnc-ddraw) | FunkyFr3sh, MIT. Reimplementação DirectDraw em C com GDI, OpenGL e D3D9, voltada a Windows e Wine. | **ÚTIL**. **Carlos · média · CONFIRMADO** para comparação de superfícies e escalonamento 2D. Backends e integração Win32 são específicos. |
| [ddraw_emu](https://github.com/dev-zetta/ddraw_emu) | dev-zetta, MIT. Camada DirectDraw 7 para Windows com OpenGL ou Direct3D 9. | **REDUNDANTE**. **Carlos · média · CONFIRMADO** para comparação de DirectDraw 7. O recorte de superfícies, paletas e chamadas COM já é coberto por GTA Recomp e cnc-ddraw. |

### Renderização e otimização no PS Vita

| Fonte | Componente, dependências e licença | Compatibilidade, limite e decisão |
|---|---|---|
| [DeltaruneVita](https://github.com/WolffsRoom/DeltaruneVita) | Butterscotch/vitaGL e adaptação dos recursos GameMaker. MPL-2.0 para arquivos cobertos. | **REDUNDANTE**. **Carlos · média · CONFIRMADO**. O mesmo estudo de cache pode usar o fluxo de VoidStrangerVita, que documenta invalidação pela impressão digital do recurso. O formato GameMaker demanda outro leitor. |
| [UndertaleYellowVita](https://github.com/WolffsRoom/UndertaleYellowVita) | Butterscotch, VitaGL e build com Docker/VitaSDK. Licença do port pendente. | **REFERÊNCIA**. **Carlos · média · CONFIRMADO**. Referência para formatos de textura, telemetria, escala e remapeamento. Adiar até medir custo no protótipo. |
| [VoidStrangerVita](https://github.com/WolffsRoom/VoidStrangerVita) | Butterscotch, VitaGL/OpenAL, dados GameMaker de versão própria, cache PVR. Licença do port pendente. | **ÚTIL**. **Carlos · média · CONFIRMADO**. Ideia reutilizável de invalidar cache pela impressão digital do arquivo de origem. O cache PVR e os plugins são específicos do fluxo upstream. |
| [GoldenIdol-PSVita](https://github.com/WolffsRoom/GoldenIdol-PSVita) | Patcher para PCK de Steam, GDRE Tools e Godot Vita. Licença do patcher pendente. | **REFERÊNCIA**. **Carlos · média · CONFIRMADO**. Referência para preparar dados locais em desktop e adaptar controles, sem migração do motor do projeto. |
| [godot4-psvita](https://github.com/WolffsRoom/godot4-psvita) | Fork do Godot, VitaSDK e ferramentas do build do motor. MIT no motor e nas adições, terceiros com licenças próprias. | **REFERÊNCIA**. **Carlos · média · CONFIRMADO**. Referência de backend e diagnóstico. Sem proposta de adoção de Godot. |
| [vitaGL](https://github.com/Rinnegatamante/vitaGL) | Camada OpenGL sobre GXM, VitaSDK e requisitos de shader conforme configuração. Repositório exibe LGPL-3.0/GPL-3.0. | **EXPERIMENTAL**. **Carlos · média · CONFIRMADO**. Avaliar se o caminho gráfico medido justificar OpenGL. Revisar a licença dos arquivos e das bibliotecas efetivamente ligados. |
| [DMR](https://github.com/GrapheneCt/DMR) | Plugin de kernel para orçamento de RAM, testado pelo autor em 3.65. Licença pendente. | **EXPERIMENTAL**. **Gandalf · baixa · CONFIRMADO**. Reavaliar após medir pressão de memória no hardware. |
| [CapUnlocker](https://github.com/GrapheneCt/CapUnlocker) | Plugin de kernel, VitaSDK/CMake, MIT. Habilita capacidades como o quarto núcleo. | **EXPERIMENTAL**. **Gandalf · baixa · CONFIRMADO**. Reavaliar após medir saturação de CPU e quantificar benefício. |
| [vkvita](https://github.com/DrDecki/vkvita) | Interface experimental semelhante a Vulkan sobre GXM, VitaSDK, GPL-3.0. | **INVIÁVEL**. **Carlos · baixa · CONFIRMADO**. Fora do planejamento ativo. A interface experimental semelhante a Vulkan não resolve os contratos de DirectDraw usados pelo original. |

### Desenvolvimento com VitaSDK, testes, depuração e automação

| Fonte | Componente, dependências e licença | Compatibilidade, limite e decisão |
|---|---|---|
| [VitaSDK](https://github.com/vitasdk) | Toolchain ARM, bibliotecas, stubs e ferramentas VPK. Licenças por componente. | **ESSENCIAL**. **Carlos · alta · TESTADO**. Manter como toolchain principal. Registrar versão e opções ao validar cada build. |
| [psp2-tooling](https://github.com/Grarak/psp2-tooling) | Perfil de CPU/GPU, breakpoints e dumps. GPL-3.0. A rota retail documentada requer firmware 3.65, plugin e módulos Sony de DevKit. | **ÚTIL**. **Gandalf · média · CONFIRMADO**. Priorizar análise offline de logs e dumps disponíveis. O modo de depuração que muda configuração do aparelho fica fora desta rodada. |
| [vita-agent-use](https://github.com/shoui520/vita-agent-use) | CLI Python/JSON, `cryptography`, plugin de kernel e Shell, firmware 3.65 e VitaShell. Build com VitaSDK, CMake e Mbed TLS 3.6.7. GPL-3.0. | **EXPERIMENTAL**. **Gandalf · alta · CONFIRMADO**. Avaliação independente abaixo. A integração seria pela CLI, com compatibilidade local e MCP por verificar. |
| [vita-port-skill](https://github.com/Yannis100/vita-port-skill) | Yannis100, MIT. Guias Markdown, scripts e modelos para VitaSDK, vitaGL, recompilação estática e testes. | **ÚTIL**. **Gandalf · alta · CONFIRMADO** como origem da skill já presente. A revisão importada fica em `.agents/skills/vita-port/UPSTREAM.md`. |
| [psvita-elfs](https://github.com/LiEnby/psvita-elfs) | LiEnby. Coleção de ELFs de firmware Vita para análise. Licença de redistribuição dos binários não identificada. | **INVIÁVEL**. **Gandalf · baixa · NÃO APLICÁVEL** ao experimento atual. Registro bibliográfico. A auditoria usa o PE32 do usuário e os stubs do VitaSDK. A licença e a natureza dos ELFs impedem adotá-los como dependência distribuída. |

### Referências históricas e conceituais

| Fonte | Componente, dependências e licença | Compatibilidade, limite e decisão |
|---|---|---|
| [Reverse Engineering Atlas](https://github.com/jammyfu/reverse-engineering-atlas) | jammyfu, catálogo CC0-1.0 de decompilações, reconstruções, firmware e ferramentas. | **REFERÊNCIA**. **Gandalf · média · CONFIRMADO** como índice de descoberta. Verificar cada achado na fonte original e registrar a licença do projeto específico. |

### Avaliação separada de vita-agent-use

O upstream descreve TLS mútuo com certificado fixado, token breve,
aceite físico de pareamento, interrupção com PS+SELECT e confirmação
local de ações sensíveis. A CLI prevê captura, entrada, abertura e
instalação de app, logs e eventos de crash. A coleta integral de dumps
e seus limites de acesso precisam de confirmação na revisão escolhida.
O protocolo é acessível por CLI, portanto Codex não exige um servidor
MCP para esse desenho. Por solicitação de Ivan, esta é a ferramenta
escolhida para os próximos testes no Vita real. No Manjaro, o cliente
da revisão `ae140b995977829e582885b2a9bfb25b7d286970` está em
`build/vita-agent-use`, fora do Git. O Python local importou
`cryptography` e a CLI exibiu a ajuda geral e a de captura e execução.
A comunicação com o console ainda precisa ser validada.

O upstream declara incompatibilidade com o VitaCompanion, já usado
para os testes da demo. A instalação também exige alteração de
`ur0:tai/config.txt` e reinício. Antes de propor um teste, Gandalf
deve revisar a revisão fixada do cliente e do plugin, delimitar
permissões a operações autorizadas, verificar disponibilidade de
recuperação e definir como voltar ao fluxo atual. Credenciais de
pareamento, screenshots proprietárias e dumps ficam locais.
Configuração, pareamento e operações no aparelho exigem autorização
explícita. [Requisitos e controles do upstream](https://github.com/shoui520/vita-agent-use).

O primeiro teste autorizado deve confirmar o firmware 3.65 e o estado
dos plugins antes da migração. Depois do pareamento, verificar captura
da LiveArea e, com um VPK autorizado, abertura, resposta aos controles,
captura do resultado e encerramento. Registrar a evidência de hardware
no relatório e manter as capturas e credenciais locais.

## C. Sequência e critérios de aprovação

A [decisão de portabilidade](feasibility.md#decisão-provisória) prioriza
a reconstrução nativa incremental e mantém uma prova isolada de
recompilação como ponto de revisão da escolha. Os itens classificados
como **REDUNDANTE** ou **INVIÁVEL** saem do planejamento ativo e
continuam na matriz. O catálogo de ações executáveis segue os recibos
e a ordem já autorizados.

1. **Bruno** segue o escopo da issue #8 e do recibo atual. Procurar
   um escritor da posição do ator pai, verificar de onde vem o valor
   e registrar o endereço e a cadeia de chamada ou o limite da prova.
   Para HDK, usar a mesma disciplina ao seguir campos de geometria,
   classes e possíveis eventos, com endereços e amostras verificáveis.
2. **Gandalf** revisa essa entrega. Para propor a recompilação, fixar
   o SHA do upstream, confirmar licença dos arquivos e dependências,
   revisar o emissor e explicitar as entradas aceitas e a referência
   independente. Manter a proposta fora do catálogo executável
   enquanto os pré-requisitos estiverem pendentes.
3. **Bruno** executa o experimento isolado quando liberado. Primeiro
   entrega resultado reproduzível de uma função. Se houver
   equivalência suficiente, repete com uma função que use ponteiros
   ou estado de objeto, conforme o plano abaixo.
4. **Carlos** mantém a demo nativa e os testes de gráficos,
   animações, controles e recursos. A atividade de catálogo atual é
   confirmar em Linux a pose inicial após o reinício da rodada
   sintética, quando houver recibo. Uma interface para C gerado só
   entra após duas comparações aprovadas e decisão sobre a divisão
   de memória, chamadas externas e ownership. C05 depende da
   evidência requerida para B02/B03.
5. **Gandalf** decide pela evidência. Registrar custo de adaptação,
   instruções cobertas, divergências e impacto observado. Só propor
   uma mudança arquitetural mediante revisão e aprovação do usuário.

## D. Preparação da prova de conceito

**Candidata inicial** `0x40963E–0x4096C5` (inclusive). A desmontagem
local confirma uma rotina com três argumentos na pilha, retorno simples
compatível com cdecl, restauração de ESI/EDI e ponteiro de saída em EAX.
Entradas são endereço de saída, base da referência e ponteiro da lista
terminada por referência zero. A rotina lê descritores de bancos em
`0x4ABD74`, usa largura/altura de 16 bits com sinal e chama
`USER32!UnionRect` por IAT. Escreve quatro valores de 32 bits na saída
e deve lidar com lista nula. O subconjunto `0x409663–0x40969E` permite
primeiro validar memória convidada, aritmética e endereçamento, antes de
cobrir o laço, a união e as cópias na saída. A desmontagem local é
reproduzível com `objdump -d -Mintel --start-address=0x40963e
--stop-address=0x4096c6 original-data/HDKWIN.EXE`. Registrar bytes e
mapa de memória no artefato local excluído do Git.

Bruno relatou 36 comparações manuais contra `resolve_pieces` e três
casos adicionais de extremos. A evidência valida a tradução manual
naquele recorte. Para testar o gerador, usar os mesmos vetores como
check complementar e comparar o estado com um interpretador de
instruções independente, local e isolado, sem iniciar o executável.
Fixar entradas e flags iniciais, comparar saída, EAX, registradores
preservados, pilha, acessos e efeitos fora da faixa esperada. Ao
estender para a rotina completa, incluir lista vazia, união de múltiplos
elementos, alias da saída de `UnionRect`, encaixe da convenção de chamada
e preservação de memória.

**Candidata seguinte** `0x411D9D`, rotina de câmera que atualiza um
contexto por ponteiro com passo limitado. Antes de adotar, confirmar
fronteiras, argumentos, chamadas, offsets e efeitos por desmontagem.
Exercitar as bordas do passo e os limites de câmera usando contexto e
alvo sintéticos. A candidata alternativa é a rotina `0x411C60`, após
mapear os mesmos requisitos.

Para a tradução, comparar primeiro os emissores de PCRecomp e I.G.I.
na mesma candidata e o modelo de SH4 como referência metodológica.
Medir cobertura das instruções, dependências e esforço de adaptação
antes de escolher o emissor. Em paralelo, confrontar a leitura de
RTTI e vtables de PCRecomp com os slots já documentados por Bruno.
Gerar e compilar o C em Linux, guardar no build local as versões,
comandos, vetores e diferenças. Falhar explicitamente em instruções e
chamadas não suportadas. O Python desta sessão não encontrou Capstone
nem Unicorn. A autorização da issue #8 cobre análise estática e exclui
instalações. O andamento da geração e validação depende do acesso às
dependências revisadas e de autorização própria para a prova.

Classificação atual **INCONCLUSIVO**. Usar **VIÁVEL** com
equivalência reproduzível e possibilidade de expandir a cobertura,
**PARCIAL** se só funcionar com adaptações significativas e
**INVIÁVEL NO MOMENTO** se houver evidência de que a adaptação custa
mais que a reconstrução manual. Compilar C já gerado é apenas um passo
da validação.

## E. Integração e publicação

Nesta rodada, este documento e a referência de projeto em `AGENTS.md`
integram a orientação ao fluxo da skill `vita-port`. O diretório
`.agents` está em modo leitura nesta sessão. O catálogo atual admite
a investigação de posição e o teste de reinício, mas carece de uma
ação para recompilação. Se a prova for autorizada, propor uma entrada
fixa para Bruno em `deploy/gandalf.py`, vinculada ao SHA e à
dependência aprovada, para uma função por rodada. Atualizar
`tests/test_gandalf.py` para verificar recusa sem pré-requisito e escopo
da autorização. Restringir o experimento à worktree de Bruno e à
documentação e artefatos locais desse trabalho. Carlos mantém a sua
worktree para o protótipo. Os testes existentes do supervisor e do
executor são a referência de regressão.

## F. Relatório de execução

- **Investigado** — estado das worktrees e recibos, prova anterior de
  Bruno, candidatas, HDK, ferramentas externas e limites do acesso ao console.
- **Implementado** — inventário, matriz, ordem de tarefas, preparação da
  prova e orientação do projeto.
- **Testado** — leitura do PE32, imports, hash e desmontagem do primeiro
  candidato. A leitura de A0 retornou as dez seções e offsets de
  referência. Passaram a verificação de sintaxe de dez arquivos Python,
  os dez testes do supervisor e a suíte Bash do executor.
- **Aprovado anteriormente** — seis recortes gráficos, rodada sintética
  em hardware e entrega documental de Bruno. Recompilação e física
  original seguem sem aprovação técnica.
- **Bloqueado** — geração automática nesta rodada, por dependências
  ausentes e escopo da autorização atual. O estado real dos timers em
  execução ficou sem confirmação pelo barramento systemd.
- **Pendente de autorização** — prova com dependências externas,
  mudanças no catálogo para sua execução, arquitetura, acesso ou
  configuração do console.

## Prioridade da prova de recompilação

A prova com PCRecomp passa à frente de novos recortes exploratórios
do desenho de A0. A base nativa continua sendo usada até a comparação.
Para obter um resultado com menos dependências, começar pela consulta
de sobreposição `0x4096E8–0x409716` (47 bytes). Ela recebe dois
ponteiros para retângulos com quatro inteiros de 32 bits com sinal,
retorna 0 ou 1 em EAX e dispensa imports, tabelas globais e x87.

A referência x86 foi validada no Linux com 514 vetores. O teste usou
apenas o trecho extraído em leitura com `pefile`, inserido em um ELF
temporário com `as --32` e `ld -m elf_i386`. Nenhum executável
Windows foi iniciado. Os resultados coincidiram com as quatro
comparações documentadas por Bruno. A amostra inclui contato pela
borda, contenção e 512 combinações com semente 42 e valores extremos
de inteiros com sinal. Só a referência foi executada. A sandbox padrão
bloqueou o ELF32 com SIGSYS, e a execução autorizada passou.

Antes de liberar a tarefa pelo supervisor, revisar e fixar a revisão do
PCRecomp e disponibilizar suas dependências em ambiente isolado.
O primeiro aceite exige C gerado, compilação Linux e ARMv7 e
equivalência de retorno nos mesmos vetores. Se passar, repetir em
`0x40963E`, incluindo o banco de dimensões e a fronteira
`UnionRect`. A primeira prova mede o custo de traduzir uma rotina
isolada. A segunda mede parte do custo de integração. Limitar a
comparação com I.G.I. ou SH4 a um bloqueio concreto do PCRecomp.

### Resultado da prova com PCRecomp

A revisão MIT `621860a9d22c9b25ae2be4575b956596f4ddaeeb` do
[PCRecomp](https://github.com/sp00nznet/pcrecomp/tree/621860a9d22c9b25ae2be4575b956596f4ddaeeb)
foi usada em diretório temporário com Capstone 5.0.9 e Unicorn 2.1.4.
O emissor `tools/lift/lift32_cpu.py` gerou C diretamente do PE32,
sem alteração manual no corpo gerado. O SHA-256 do jogo é o do
inventário desta auditoria. Os artefatos derivados ficaram locais.

| Função | Comparação executada no Linux | ARMv7 |
|---|---|---|
| `0x4096E8`, sobreposição | 514/514 casos. Igualdade dos oito registradores de uso geral, das flags CF/PF/ZF/SF/OF e dos 4096 bytes da arena. O retorno também coincidiu com o modelo de desigualdades. | Objeto EABI5 compilado com VitaSDK. |
| `0x40963E`, união gráfica | 128/128 casos. Igualdade dos oito registradores, da quantidade de chamadas de importação e dos 24576 bytes da arena. Testes incluem lista nula, lista vazia, até quatro elementos, dimensões negativas e sobreposição do destino com uma fonte da união. | Objeto ARMv7-A compilado com declaração externa de `dispatch`. |

Para reproduzir o primeiro caso, ler 47 bytes a partir do offset de
arquivo `0x8AE8` e chamar `Lifter.lift_function(bytes, 0x4096E8)`.
Incluir `runtime/recomp32_cpu/cpu.h` no C emitido. Montar a arena de
dados em `0x20000000`, com os retângulos nos offsets 0 e 16 e o
retorno e argumentos no offset `0x800`. Usar os dois vetores
`(0,0,4,4,4,0,8,4)`, `(0,0,4,4,1,1,2,2)` e gerar os demais com
`random.Random(42).choice` sobre
`[-2147483648,-100,-1,0,1,100,2147483647]`. Comparar após RET com
Unicorn x86 de 32 bits, uma instrução de retorno por pilha e a mesma
imagem inicial de memória.

Para o segundo caso, emitir os 136 bytes a partir do offset
`0x8A3E`. Mapear a arena em `0x4AB000` e preencher a tabela de
bancos em `0x4ABD74` e o import em `0x4AFA34`. O C exige
`g_image_delta` e um despachante para esse import. O teste usou
`g_image_delta=0`, um adaptador em C no host e um hook Python
independente no Unicorn. Ambos modelaram união de retângulos e a
limpeza de 12 bytes de argumentos do callee. Isso compara a rotina
original e a gerada sob um contrato de teste. Ainda falta comparar
o adaptador com a implementação de Windows em casos reais.

**Decisão após a prova.** A tradução de rotinas delimitadas é viável
nos casos medidos. A estratégia para o jogo completo permanece B
provisória. O runtime testado trata endereços convidados como
ponteiros do host. O Linux permitiu arenas nos endereços de teste.
No Vita falta demonstrar um mapa de memória compatível ou uma
tradução de endereços, o despacho para chamadas externas e a
execução do código ligado. O próximo experimento de maior valor
é ligar a primeira função a um programa VitaSDK com dados
sintéticos, medir sua execução no aparelho e verificar o acesso à
memória antes de integrar código ao protótipo jogável.

### Teste de deslocamento da memória no Linux

A segunda função também passou nos mesmos 128 casos com a arena
nativa movida de `0x4AB000` para `0x20000000` e `0x31000000`,
em duas execuções independentes. Em cada uma, `g_image_delta` recebeu
a nova base menos `0x4AB000`. O código x86 de referência manteve
o banco e a entrada de importação nos endereços originais, mas
recebeu os mesmos ponteiros para as estruturas da arena nova.
Os oito registradores, a contagem de chamadas de `UnionRect`
modelada e todos os bytes da arena coincidiram nas duas bases.

O resultado elimina a necessidade de usar o endereço original
dessas duas referências globais no C do recorte testado. O teste
ainda usa uma arena contígua e ponteiros nativos que cabem em 32
bits. O alcance de outras referências absolutas, o carregamento
completo do PE32 e a execução no PS Vita continuam em aberto.

O VPK de diagnóstico `HKRC00001` com os 514 casos da primeira
função foi compilado com VitaSDK e GCC 15.2.0. O ZIP e o verificador
de VPK da skill passaram. O pacote local está em
`build/recomp-diagnostic/hdk_recomp_test.vpk`, fora do Git.
SHA-256 `a710887a1b8312451dfc35eb8c1a65e43b908edbc2ca7937fb586889c1b6ff66`.
### Execução no PS Vita físico

Em 19/11/2026, o VitaCompanion instalou e abriu `HKRC00001`.
Os seis arquivos instalados foram comparados byte a byte com o VPK.
O aplicativo gravou `HKRC00001 514/514 PASS 423 us`, e o relatório
foi recuperado por FTP. O teste verificou o retorno, a posição da
pilha, os registradores preservados e os dados dos retângulos.
Os 423 microssegundos correspondem à bateria inteira em uma única
execução. A medição não representa tempo por quadro.

A primeira função recompilada executou no Vita com ponteiros para
buffers do próprio aplicativo. A próxima prova física é ligar a
segunda função com dados sintéticos, deslocamento de endereços e
um adaptador de teste para `UnionRect`.

### Segunda rotina no PS Vita físico

Em 19/11/2026, o pacote `HKRC00002` passou no teste de integridade
ZIP e no verificador de VPK. O VitaCompanion instalou e abriu o
aplicativo, e os seis arquivos instalados coincidiram com o pacote.
O relatório recuperado por FTP foi:

```text
HKRC00002 128/128 PASS 1947 us base=810c0a20
```

O diagnóstico executou a rotina gerada em ARMv7 com uma arena de
24 KiB alocada no aplicativo, fora do endereço original. Verificou
o ponteiro retornado, a pilha, ESI/EDI/EBP, a contagem de chamadas
do adaptador e o retângulo produzido em cada vetor sintético.
A chamada `UnionRect` foi atendida pelo adaptador de teste local.
Os 1947 microssegundos incluem a preparação e as verificações
dos 128 vetores em uma execução.

O pacote local está fora do Git em
`build/recomp-second/out/recomp_second.vpk`, SHA-256
`b278910a75e6876bce2fce04a5af85e0cc40bfb3cb0fc1c6bdaf426dc719a98e`.
A prova confirma acesso aos dados deslocados e despacho de um
import modelado neste recorte. A equivalência com a implementação
Windows de `UnionRect`, o despacho entre objetos e o custo do
runtime em uma cena real continuam em aberto. O próximo recorte
deve verificar uma chamada indireta entre funções do jogo antes
de ampliar a integração.

### Chamada indireta entre rotinas originais no Vita

O PCRecomp emitiu a cadeia `0x41C877 → 0x412D29 → 0x4096E8`.
Em `0x41C887`, o primeiro método lê o slot virtual `+0x1C` do
objeto e chama o endereço convidado `0x401843`. O despachante
do diagnóstico redireciona esse endereço ao corpo recompilado
em `0x412D29` e resolve o segundo salto conhecido
`0x40119F → 0x4096E8`. Destinos fora da tabela encerram o teste.

Foram usados os mesmos 514 pares de retângulos da primeira prova.
Unicorn executou os corpos e saltos originais com uma estrutura
sintética de objetos e confirmou retorno e pilha. O código gerado
passou nos mesmos casos no Linux e no Vita. O diagnóstico também
verificou os registradores preservados, a integridade dos retângulos
e uma chamada de cada tipo por caso. No VitaCompanion, os seis
arquivos instalados foram comparados com o VPK e o aplicativo
`HKRC00003` gravou:

```text
HKRC00003 514/514 PASS 1694 us base=81080a20
```

A medição inclui preparação e verificação dos vetores em uma única
execução. O VPK local passou nas verificações de integridade e tem
SHA-256 `a003d601bb2f1757db9cdbf60d5ff98d21d4e6fafcef0442e8c480cf0299991b`.
Os objetos e a tabela de métodos foram montados para o recorte.
O resultado sustenta uma prova híbrida com dados reais de A0.
A relação do chamador com a execução normal de A0, as demais
ramificações de `0x412D29` e a resposta física continuam em aberto.

### Primeira prova híbrida com dados reais de A0

No Linux, `read_vista_objects` leu os seis objetos `CEarth` de
`VISTA_0` em `A0.HDK`, com 32 elementos. `read_bank` leu a tabela
de retângulos do banco 2 do PE32. Todas as referências efetivas
selecionaram esse banco e índices válidos. Os dados originais ficaram
em leitura e os artefatos da comparação foram gravados somente em
`build/recomp-a0/`.

A prova montou para cada objeto a lista terminada por zero, o descritor
de banco e o destino de saída em uma arena de 24 KiB. Com as mesmas
entradas, comparou a execução x86 em Unicorn e o C emitido por PCRecomp
para `0x40963E`. Houve **6/6 resultados equivalentes** nos oito
registradores de uso geral e em todos os bytes da arena. As 32
chamadas ao adaptador `UnionRect` coincidiram em contagem. Os seis
retângulos também coincidiram com o cálculo independente em Python
a partir das coordenadas locais e dimensões originais.

As listas, referências e dimensões vêm do jogo. A arena, o contrato
de `UnionRect` e a entrada da função foram montados no diagnóstico.
A prova mede limites locais dos objetos de fundo. A translação para
o mundo, o ajuste gráfico, a renderização e a ligação com terreno
sólido permanecem fora desse recorte. A rotina já passou no Vita com
vetores sintéticos. Esta comparação com A0 foi executada no Linux.

O próximo recorte útil é ligar essa função ao leitor e à apresentação
nativa, mantendo os dados originais como entrada local separada do VPK.
O critério de integração é preservar as prévias de A0 e demonstrar
os mesmos seis limites com dados carregados pelo aplicativo.

### Protótipo híbrido com dados de A0 no Vita

Uma variante local do passeio nativo executa `0x40963E` antes da
entrada na cena. Os dados de A0 e os seis resultados de referência
do x86 foram exportados para `a0-test.bin`. O exportador existente
regenerou `vista0.rgba`. Os dois arquivos foram enviados separadamente
ao console e conferidos por SHA-256. A variante valida comprimentos,
banco e índices antes de montar a arena e chamar o C recompilado.
A rotina de desenho e os controles vieram do passeio existente.

A primeira instalação, `HKRC00004`, abriu sem retornar relatório.
A revisão passou a gravá-lo pela API de arquivos do VitaSDK. A
reinstalação sobre o primeiro título foi rejeitada pelo VitaCompanion,
então a revisão usou o identificador separado `HKRC00005`, com os
mesmos dados locais. Os seis arquivos instalados coincidiram com
o VPK e o console retornou por FTP:

```text
HKRC00005 6/6 PASS scene=ready
```

O resultado confirma os seis limites gráficos de referência com
dados reais carregados pelo Vita. O campo `scene=ready` confirma a
presença e o tamanho do arquivo de pixels, antes da entrada na
rotina de apresentação. A confirmação visual da tela ainda está
pendente. Os limites calculados são observados pelo diagnóstico
e ainda não são consumidos para desenho nem para colisões.

O VPK experimental local está em `build/recomp-a0/out/recomp_a0.vpk`,
fora do Git. Passou em `unzip -tq` e no verificador da skill.
SHA-256 `2b20d360794c538ab89e4390215560aa914e6b71985654f55d03cf69595c8aa3`.

### Contornos gráficos calculados durante o passeio

A variante local `HKRC00006` usa os seis limites recompilados para
desenhar contornos amarelos sobre o fundo. Para cada objeto, soma a
origem do objeto e subtrai a origem da imagem `(52, 232)`. O desenho
aplica o mesmo deslocamento horizontal da câmera que a imagem e
recorta as linhas às bordas da tela. Os contornos representam limites
gráficos, sem participação na simulação de colisões.

No PC, os seis limites coincidiram com os recortes resolvidos pelo
leitor HDK. Os pixels do contorno em sete posições de câmera
coincidiram com uma referência independente, incluindo as extremidades
do passeio. O VPK passou nas verificações ZIP e LiveArea.
SHA-256 `ce09415a7618be30e185c37db861a1ddad042a25e11d3d1eca71fdd9a75dd3f4`.

O pacote e o arquivo complementar de A0 foram enviados por FTP e
comparados byte a byte. Em seguida, o VitaCompanion instalou
`HKRC00006` com autorização para preservar temporariamente os três
títulos anteriores. Os seis arquivos instalados coincidiram com o
VPK. O relatório por FTP retornou `HKRC00006 6/6 PASS scene=ready`
e o VitaCompanion confirmou o encerramento do aplicativo. Ivan
confirmou visualmente que os contornos acompanharam a rolagem e
ficaram alinhados ao fundo durante o passeio. A pasta de dados `ux0:data/HKRC00004` é compartilhada.

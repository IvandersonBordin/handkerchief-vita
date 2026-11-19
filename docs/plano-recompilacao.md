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

| Fonte | Componente, dependências e licença | Compatibilidade, limite e decisão |
|---|---|---|
| [I.G.I.](https://github.com/HenryKun55/igi-recomp) | `tools/recomp` usa Python e Capstone. `runtime` usa memória convidada de 32 bits e modelo x87. `shim` cobre Win32, DirectDraw/D3D7, DirectSound e DirectInput. MIT na base, exceções LGPL nos patches vitaGL e bibliotecas associadas. Build com CMake/Ninja e VitaSDK no alvo. | Primeira opção para avaliar o tradutor isolado e seus testes entre rotinas nativas e recompiladas. A descrição upstream cobre registradores, memória e x87 em níveis diferentes dos detalhes necessários ao recorte. Isso exige revisão do emissor e dos testes na revisão fixada. Shims e ganchos devem ser tratados como específicos até prova de reutilização. |
| [Silent Hill 4](https://github.com/vallfrr/sh4vita) | Python/Capstone, Ghidra 12/JDK 21 no fluxo de exportação, CMake/VitaSDK e vitaGL. GPL-3.0. | `x86rec.py` abre o PE32 de SH4 e trabalha com um catálogo de endereços e thunks. Emite uma função C por função convidada, preserva a pilha x86 em CALL/RET e modela x87 em oito doubles. CPU e emissão são referências. Lista de funções, IAT, despachos e runtime precisam dos dados da versão GOG v5. Avaliar seu método diferencial antes de copiar qualquer código. [Emissor](https://raw.githubusercontent.com/vallfrr/sh4vita/main/recomp/x86rec.py) |
| [Silent Hill 3](https://github.com/matfsouza/sh3vita) | `recomp/x86rec.py`, exportação Ghidra e implementação do runtime em `port/`. Python/Capstone, VitaSDK e vitaGL. GPL-3.0. | C por função PE32 e fronteira entre CPU convidada e chamadas externas. Bruno já identificou caminho fixo e patches específicos no emissor. Usar como contraste para imports, ponteiros, vtables e x87. Reaproveitamento de código depende de revisão da licença e das amarras ao executável. |
| [Recomp Toolkit](https://github.com/wraitii/recomp-toolkit) | Tradutor, configuração por jogo e runtime gráfico D3D7–9. A distribuição pede macOS 14 e Command Line Tools. Licença do componente exato pendente. | Apple Silicon é o alvo principal. A lista de jogos exige versões determinadas. Usar como referência para separar descoberta de funções, perfil de versão e substituições nativas. VitaSDK e adequação de Metal/Vulkan ao Vita sem demonstração. [Arquitetura](https://github.com/wraitii/recomp-toolkit/blob/main/docs/architecture.md) |
| [WindowsRecompTool](https://github.com/Zippingrocks/WindowsRecompTool) | PE32/i386, Zydis, emissão C++, CMake e Unicorn opcional para conformidade. MIT. | Referência secundária para rejeitar instruções, imports e saltos indiretos sem suporte, vincular o alvo ao hash e comparar registradores, flags e pilha com referência independente. Saída C++ e Vita sem validação. |
| [Xita](https://github.com/Xita-Project/xita) | `recompiler/`, ferramentas Python para XBE e runtime de Xbox com D3D8 e DirectSound. GPL-3.0. | Referência conceitual para tradução por função, substituições e validação. O formato XBE e a interface de kernel do Xbox exigem um carregador distinto de PE32/Win32. |
| [DeltaruneVita](https://github.com/WolffsRoom/DeltaruneVita) | Butterscotch/vitaGL e adaptação dos recursos GameMaker. MPL-2.0 para arquivos cobertos. | Consultar repacotamento e cache condicionado à versão dos recursos. O formato GameMaker demanda outro leitor. |
| [UndertaleYellowVita](https://github.com/WolffsRoom/UndertaleYellowVita) | Butterscotch, VitaGL e build com Docker/VitaSDK. Licença do port pendente. | Referência para formatos de textura, telemetria, escala e remapeamento. Adiar até medir custo no protótipo. |
| [VoidStrangerVita](https://github.com/WolffsRoom/VoidStrangerVita) | Butterscotch, VitaGL/OpenAL, dados GameMaker de versão própria, cache PVR. Licença do port pendente. | Ideia reutilizável de invalidar cache pela impressão digital do arquivo de origem. O cache PVR e os plugins são específicos do fluxo upstream. |
| [GoldenIdol-PSVita](https://github.com/WolffsRoom/GoldenIdol-PSVita) | Patcher para PCK de Steam, GDRE Tools e Godot Vita. Licença do patcher pendente. | Referência para preparar dados locais em desktop e adaptar controles, sem migração do motor do projeto. |
| [godot4-psvita](https://github.com/WolffsRoom/godot4-psvita) | Fork do Godot, VitaSDK e ferramentas do build do motor. MIT no motor e nas adições, terceiros com licenças próprias. | Referência de backend e diagnóstico. Sem proposta de adoção de Godot. |
| [VitaSDK](https://github.com/vitasdk) | Toolchain ARM, bibliotecas, stubs e ferramentas VPK. Licenças por componente. | Manter como toolchain principal. Registrar versão e opções ao validar cada build. |
| [vitaGL](https://github.com/Rinnegatamante/vitaGL) | Camada OpenGL sobre GXM, VitaSDK e requisitos de shader conforme configuração. Repositório exibe LGPL-3.0/GPL-3.0. | Avaliar se o caminho gráfico medido justificar OpenGL. Revisar a licença dos arquivos e das bibliotecas efetivamente ligados. |
| [psp2-tooling](https://github.com/Grarak/psp2-tooling) | Perfil de CPU/GPU, breakpoints e dumps. GPL-3.0. A rota retail documentada requer firmware 3.65, plugin e módulos Sony de DevKit. | Priorizar análise offline de logs e dumps disponíveis. O modo de depuração que muda configuração do aparelho fica fora desta rodada. |
| [vita-agent-use](https://github.com/shoui520/vita-agent-use) | CLI Python/JSON, `cryptography`, plugin de kernel e Shell, firmware 3.65 e VitaShell. Build com VitaSDK, CMake e Mbed TLS 3.6.7. GPL-3.0. | Avaliação independente abaixo. A integração seria pela CLI, com compatibilidade local e MCP por verificar. |
| [DMR](https://github.com/GrapheneCt/DMR) | Plugin de kernel para orçamento de RAM, testado pelo autor em 3.65. Licença pendente. | Reavaliar após medir pressão de memória no hardware. |
| [CapUnlocker](https://github.com/GrapheneCt/CapUnlocker) | Plugin de kernel, VitaSDK/CMake, MIT. Habilita capacidades como o quarto núcleo. | Reavaliar após medir saturação de CPU e quantificar benefício. |
| [vkvita](https://github.com/DrDecki/vkvita) | Interface experimental semelhante a Vulkan sobre GXM, VitaSDK, GPL-3.0. | Baixa prioridade. O executável original importa DirectDraw. |

### Avaliação separada de vita-agent-use

O upstream descreve TLS mútuo com certificado fixado, token breve,
aceite físico de pareamento, interrupção com PS+SELECT e confirmação
local de ações sensíveis. A CLI prevê captura, entrada, abertura e
instalação de app, logs e eventos de crash. A coleta integral de dumps
e seus limites de acesso precisam de confirmação na revisão escolhida.
O protocolo é acessível por CLI, portanto Codex não exige um servidor
MCP para esse desenho. O Python local encontra `cryptography`, mas a
CLI não foi testada no Manjaro.

O upstream declara incompatibilidade com o VitaCompanion, já usado
para os testes da demo. A instalação também exige alteração de
`ur0:tai/config.txt` e reinício. Antes de propor um teste, Gandalf
deve revisar a revisão fixada do cliente e do plugin, delimitar
permissões a operações autorizadas, verificar disponibilidade de
recuperação e definir como voltar ao fluxo atual. Credenciais de
pareamento, screenshots proprietárias e dumps ficam locais.
Configuração, pareamento e operações no aparelho exigem autorização
explícita. [Requisitos e controles do upstream](https://github.com/shoui520/vita-agent-use).

## C. Sequência e critérios de aprovação

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

Para a tradução, testar primeiro a adaptação mínima do emissor de
I.G.I. e usar o modelo de teste de SH4 como comparação metodológica.
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

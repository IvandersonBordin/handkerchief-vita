# Viabilidade do Handkerchief no Vita

Revisão de 18/11/2026. O inventário B01, o mapa do motor e os testes C01–C04 cobriam partes da triagem. Faltava comparar as seis alternativas da [skill local](../.agents/skills/vita-port/references/port-types.md).

| Rota | Ganho possível | Evidência e bloqueio |
|---|---|---|
| 1. Reaproveitar port Vita | Preservaria o jogo inteiro com poucas adaptações. | O [pedido público de 2022](https://github.com/vita-nuova/bounties/issues/108) está fechado em repositório arquivado. A página não apresenta implementação. A busca pública foi limitada. |
| 2. Motor já portado que leia os dados | Aproveitaria simulação, mapas e ferramentas existentes. | O leitor reconhece os limites de 99 HDK com conteúdo. Classes serializadas e tabelas gráficas embutidas em HDKWIN.EXE exigem compatibilidade específica. Nenhum motor compatível identificado. |
| 3. Recompilar fontes originais C/C++ | Preservaria regras, IA e eventos. | O inventário contém PE32 x86 e dados. Falta fonte autorizada. Com ela, seria necessário adaptar pelo menos as interfaces Windows efetivamente usadas. |
| 4. Carregar biblioteca Android ARMv7 | Aproveitaria lógica já compilada para a CPU. | Falta edição Android verificável com biblioteca armeabi-v7a e dependências compatíveis. |
| 5. Reconstruir nativamente | Já existem leitores, prévias e uma camada Vita testada. | A demonstração 71ed7a2 comprova entrada, animação e apresentação. Reconstruir fases, combate, IA, eventos e física com fidelidade segue sendo o maior custo. |
| 6. Adaptar reimplementação portátil | Permitiria acoplar um núcleo existente ao protótipo Vita. | A busca pelo jogo e por reimplementações não revelou núcleo utilizável com licença e cobertura verificáveis. Os leitores Python locais cobrem parte dos dados. |

## Alternativa para preservar a lógica original

A recompilação estática x86 para C poderia preservar rotinas difíceis de reconstruir. O mapa identifica despacho indireto e tabelas virtuais. A leitura de importações com `objdump -p original-data/HDKWIN.EXE` confirma DDRAW, DSOUND, DPLAYX, WINMM, APIs Win32 e sockets. Traduzir instruções deixa por resolver chamadas indiretas, layouts de memória de 32 bits, chamadas Windows e a validação de comportamento. Importação sozinha não prova que toda API seja necessária na campanha. Emulação de Windows/x86 também exigiria um ambiente compatível, desempenho medido e autorização para executar o jogo.

Manter o protótipo e os testes como base. Antes de ampliar a reconstrução, comparar o custo usando uma única rotina determinística já mapeada, com entrada sintética e saída conhecida, em uma prova de tradução estática. Só ampliar se o resultado coincidir e houver caminho verificável para despacho e serviços externos. Reavaliar imediatamente diante de fontes autorizadas, núcleo portátil ou port utilizável. Ainda não há estimativa confiável para o jogo completo.

Evidências locais em [mapa do motor](engine-map.md), [formato HDK](hdk-format.md), branch de Bruno em docs/physics-collision.md e [revisão física da demonstração](https://github.com/IvandersonBordin/handkerchief-vita/issues/4#issuecomment-6994757728).

## Cota dos agentes

The Codex executor and its shared usage-limit marker were retired on 2026-11-20. Agents now run only on request in Claude Code; quota rules are in `CLAUDE.md` and [claude-code-workflow.md](claude-code-workflow.md).

## Auditoria comparativa de 19/11/2026

### Estado auditado

- A base foi `main` em `d292797`, Bruno em `a3ef39a` e Carlos em
  `ce516ea`, com as três worktrees limpas. A issue #8 autoriza o
  rastreio estático de posição. O recibo existente de Bruno aponta
  para o mesmo trabalho. Carlos tem o smoke test e a rodada sintética.
  C05 depende de B02/B03 e segue bloqueada.
- O registro do Vita físico confirma entrada, caminhada animada e
  retorno à pose inicial após reinício da rodada sintética. O cenário
  A0 tem prévia de fundo baseada em coordenadas, referências e bancos
  identificados. A rodada usa lógica e ator provisórios. O jogador
  original, a campanha, o combate, IA, eventos e progressão têm
  cobertura insuficiente. [Registro físico](teste-rodada-2026-11-19.md).
- Bruno rastreou limites do ator e a consulta de sobreposição em
  `0x4096E8`. Retângulos de área positiva que só compartilham uma
  borda retornam zero. O escritor de origem X/Y do ator em atualização,
  a ligação com a geometria sólida e a resposta física seguem em
  aberto, assim como gravidade e salto. O principal impedimento da
  reconstrução é recuperar vínculos entre atualização, dados de fase
  e efeitos observáveis. [Relatório de Bruno](https://github.com/IvandersonBordin/handkerchief-vita/blob/a3ef39a15411cc85b128dc3b7778dd677367c1b4/docs/physics-collision.md).

### Experimentos e resultados

**A — Recompilação.** A inspeção local com `pefile` e `objdump -p`
aceitou o executável i386, com base `0x00400000` e diretório de
relocações presente. A candidata `0x40963E` referencia o banco por
endereço virtual absoluto e chama `USER32!UnionRect` pela tabela de
imports. Uma camada de memória convidada precisa preservar esses
endereços e converter ponteiros de forma explícita. A presença de
relocações, por si só, não mede o custo disso no Vita. Os 39 casos da
tradução manual anterior cobrem um trecho, deixando de fora a tabela
de bancos, a união, os efeitos da função completa e a validação do
emissor.

PCRecomp documenta emissão C e teste diferencial de PE32 com Unicorn.
Os testes upstream podem interromper os dois lados na primeira chamada
externa. Para validar a função completa será preciso cobrir o restante
do laço e controlar `UnionRect`. I.G.I. e SH4 documentam modelos x87 e
camadas Windows para seus jogos, com Vita como alvo. A cobertura de x87,
chamadas indiretas e demais instruções do Handkerchief ficou desconhecida.
A inspeção confirmou o formato PE32, mas o Python local carece de Capstone
e Unicorn. A issue vigente autoriza análise estática. A geração com
ferramenta externa, compilação do C gerado em Linux/ARMv7 e equivalência
ficaram pendentes da preparação e autorização da prova.
[PCRecomp](https://github.com/sp00nznet/pcrecomp),
[I.G.I.](https://github.com/HenryKun55/igi-recomp),
[SH4](https://github.com/vallfrr/sh4vita).

**B — Reconstrução nativa.** O teste de câmera e desenho de `main`
em C passou no Linux com UBSan e pixels sintéticos. O modelo Python da
translação de fundo original, documentado no caminho de `0x42309E`,
passou em oito vetores, incluindo sinais mistos e truncamento para
zero. Exemplos com L igual a zero foram −300 → −200, −4 → −3 e 4 → 3.
A câmera em C centraliza o ator quando possível e limita o recorte às
bordas de 4464 × 384. A origem e a entrada dessa função diferem do
modelo de paralaxe que usa X e L do contexto original. Para reproduzir
o comportamento observado, será preciso expor o deslocamento do mundo
e calcular separadamente a translação de fundo. O teste demonstra
os contratos de câmera, enquanto a movimentação e a resposta à
colisão seguem sem referência equivalente para a demo. VitaSDK com
GCC 15.2.0 compilou `vista_walk` em main e os dois VPKs de Carlos.
Os três passaram em `unzip -tq`, e a rodada anterior é a evidência
física disponível. [Código e cálculo da câmera](engine-map.md#câmera-e-parallax-horizontal).

**C — Arquivos HDK.** A varredura local com `iter_sections` e
`read_vista_objects` leu os limites externos de 99 arquivos com
conteúdo e encontrou nove vazios. Dos 98 com `VISTA_0`, 85 tiveram
listas aceitas integralmente. Entre as 13 rejeições houve cinco por
cabeçalho, quatro por contagem incompatível e quatro por classe sem
suporte. `load_vista_scene` confirmou seis objetos e 32 elementos
resolvidos em A0. A leitura das outras 84 seções prova cobertura
estrutural da camada de fundo, sem comparação visual ou resolução
de bancos caso a caso. Os seis recortes já mapeados de `STAGE_0`
cobrem parte da camada principal. Ainda falta confirmar formatos de
objetos e terreno, ligação a colisores, eventos e se existe lógica
interpretável dentro de HDK. A automatização de fundos é demonstrável
em um subconjunto. A reconstrução automática de fases completas, sem
ajustes manuais por mapa, permanece desconhecida. [Formato HDK](hdk-format.md).

Para reproduzir a contagem, percorra `Path('original-data').rglob('*')`
filtrando `.suffix.lower() == '.hdk'`. Em cada arquivo não vazio,
consuma `list(iter_sections(data))`. Para os que tiverem `VISTA_0`,
conte separadamente sucessos e `ValueError` em
`read_vista_objects(data)`. As funções estão em `tools/`, que deve
estar no `PYTHONPATH`.

### Comparação de estratégias

| Critério | A — Recompilação estática | B — Reconstrução nativa |
|---|---|---|
| Complexidade de implementação | Emissão, catálogo de funções, chamadas indiretas, memória convidada e serviços Windows. DDRAW, DSOUND, DPLAYX e Win32 constam dos imports. A campanha pode usar apenas parte deles, a confirmar. | Leitores, dados de cena e plataforma já existem parcialmente. A complexidade cresce com as regras de física, IA, combate e eventos ainda sem mapa. |
| Tempo até demo funcional | DESCONHECIDO. A próxima entrega mensurável é uma função inteira. | Demo sintética já demonstrada. Tempo até primeira cena fiel jogável é DESCONHECIDO. |
| Tempo até jogo completo | DESCONHECIDO. Depende da cobertura do tradutor, do despacho e dos serviços de plataforma. | DESCONHECIDO. Depende de quanto do conteúdo é reutilizável pelos leitores e de quantos comportamentos exigem recuperação manual. |
| Fidelidade | Pode preservar regras e colisões compiladas se semântica, dados, tempo e chamadas coincidirem. | Pode preservar os comportamentos descritos e testados. Casos não mapeados exigem evidência independente. |
| Reuso local | Reusa mapeamento e vetores, além da plataforma se houver uma interface adequada. | Reusa protótipos, leitores, ferramentas e testes existentes diretamente. |
| VitaSDK | Há precedentes externos. C gerado deste jogo e runtime convidados estão sem teste ARM. | Três VPKs reproduzidos nesta auditoria e evidência física anterior. |
| RAM e desempenho | DESCONHECIDO para HDKWIN. Medir código, arena convidada, buffers e tempo de quadro. | DESCONHECIDO para a campanha. Controla buffers e memória de cada subsistema, mas ainda falta carga representativa. |
| Depuração | Exige comparar registradores, memória, flags e chamadas e separar defeitos do emissor e do runtime. | Testes locais isolam regras recuperadas com mais facilidade, mas o original pode ocultar condições ainda não modeladas. |
| Dependências externas | Emissor, decodificador, referência independente, runtime e implementações de APIs requeridas. | VitaSDK, código local e os recursos do usuário. Usa engenharia reversa para recuperar semântica. |
| Risco de bloqueio | Alto até demonstrar uma função inteira e uma cadeia que cruze objeto e plataforma. | Alto na recuperação de lógica e dados, com caminho incremental já demonstrado para as camadas conhecidas. |
| Manutenção | Depende de fixar versão do executável, configuração do emissor e contratos do runtime. | Código de domínio legível e testes próprios. Cada aproximação introduz uma obrigação de validação. |
| Intervenção humana | Revisão de licença, dependências, casos de divergência e validação física. Esforço total DESCONHECIDO. | Validação do comportamento original e do console, revisão de evidências e lacunas. Esforço total DESCONHECIDO. |

### Decisão provisória

**B — Priorizar engenharia reversa e reconstrução nativa incremental.**
Ela já produz entregas verificáveis com o VitaSDK e aproveita a
cobertura parcial dos fundos sem reescrever cada mapa. O risco é
expandir manualmente regras de jogo ainda desconhecidas. Adiar esse
investimento até medir o custo de traduzir uma função inteira com
estado e pelo menos uma fronteira Win32. A rota híbrida só ganha
prioridade se uma função gerada substituir trabalho real com
equivalência em Linux, viabilidade ARM e custo menor de integração.
A rota A volta à frente se, além disso, a prova avançar para uma cadeia
com ponteiros ou despacho e demonstrar um caminho para as chamadas
efetivamente observadas. Uma função isolada é o primeiro ponto de
decisão, com ampliação condicionada ao seu resultado.

1. **Bruno** conclui o recorte autorizado de B02. Entregar uma escrita
   X/Y com origem e cadeia, ou o limite exato do despacho. Depois
   correlacionar uma variante de `VISTA_0` rejeitada com seu leitor
   no original e seguir um campo de terreno ou evento até um consumidor.
2. **Carlos** mantém os testes do caminho nativo. Com recibo, confirmar
   em Linux a restauração da pose de reinício. Após evidência sobre a
   câmera, propor o teste da translação de fundo. Integração de física
   depende de B02/B03 e de vetores de referência.
3. **Gandalf** revisa dependências e pede a liberação específica da
   prova A. Com a liberação, Bruno traduz `0x40963E` com PCRecomp,
   compara registradores, memória e saída contra a referência x86,
   registra chamadas externas e compila em Linux e ARMv7. Se passar,
   medir uma segunda rotina, a candidata `0x411D9D` ou outra com
   dados de objeto e fronteiras conhecidas. Encerrar a linha se o
   runtime necessário ao recorte superar o custo da implementação
   nativa e isso estiver documentado.
4. **Gandalf e Carlos** validam, com autorização de hardware, uma
   cena representativa, com orçamento de RAM e tempo por quadro.
   Ampliar para uma fase com entrada, movimento, terreno e evento
   após evidência reproduzível. Só então estimar a cobertura exigida
   para a campanha.

O catálogo preserva a classificação e a evidência de todas as
referências. Ghidra serve à inspeção de PE32/C++; VitaSDK é a base do
build. Suspender ddraw_emu e DeltaruneVita como estudos duplicados e
retirar vkvita e psvita-elfs do planejamento ativo. Adiar automação de
console por plugin, otimizações de memória e um runtime Windows
completo até evidências de necessidade. O próximo trabalho dos
executores segue os recibos existentes. Dependências novas, prova
externa, mudança arquitetural e acesso ao Vita exigem as autorizações
correspondentes. [Matriz de ferramentas](plano-recompilacao.md#b-matriz-de-ferramentas).

## Decisão de implementação após as provas físicas

O projeto adota **C — abordagem híbrida** para a próxima fatia
jogável. A decisão anterior de manter B como base provisória fica
no histórico. A engenharia reversa identifica as estruturas e
rotinas responsáveis pelo comportamento. O PCRecomp reaproveita as
rotinas que passam na comparação com x86. A base nativa mantém
controles, apresentação e serviços de plataforma já validados.

A escolha se apoia em três provas físicas: sobreposição de
retângulos, acesso a dados deslocados com um import modelado e
despacho indireto entre rotinas originais. O protótipo também
carregou seis objetos reais de A0 no Vita e desenhou os limites
calculados durante a rolagem. A reconstrução nativa permanece
adequada para a plataforma. Traduzir o executável inteiro ainda
exigiria demonstrar cobertura de imports, memória e lógica de
fase. [Resultados das provas](plano-recompilacao.md#resultado-da-prova-com-pcrecomp).

Para a próxima fatia, Bruno compara o terreno e as animações
visíveis na gravação com o HDK e o executável. Carlos mantém a
rodada e prepara comparações repetíveis de câmera, apresentação
e controles dentro das tarefas liberadas. O primeiro critério
para ampliar a recompilação é reproduzir uma consulta original
ao terreno de A0 com dados reais e verificar seu efeito no
movimento. A gravação indica situações de teste. As regras e as
coordenadas vêm dos dados e das rotinas do jogo.

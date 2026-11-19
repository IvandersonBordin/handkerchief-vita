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

O executor instalado em ~/.local/bin/handkerchief-cycle usa um marcador compartilhado em ~/.local/state/handkerchief/usage-limit (ou XDG_STATE_HOME). Um erro de cota do CLI grava a mensagem. Os próximos ciclos retornam antes de chamar o modelo. Chamadas já em curso podem terminar. Após a renovação confirmada, remover apenas esse marcador libera os dois agentes. Essa retomada manual evita sondagens pagas.

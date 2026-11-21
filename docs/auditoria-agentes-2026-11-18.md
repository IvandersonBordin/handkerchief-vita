# Auditoria noturna de Bruno e Carlos — 18/11/2026

> Historical record. The Codex executor described here was retired on 2026-11-20; see [claude-code-workflow.md](claude-code-workflow.md).

## Evidências e correções

Conferidos ExecStart e TimersMonotonic das quatro unidades de usuário, o launcher real, o publicador, os estados persistentes e os logs do journal. O executor instalado era idêntico ao arquivo versionado antes da edição. A substituição foi atômica e a nova cópia também foi comparada com o repositório. Os serviços e timers seguiram sem reinício.

| Pergunta | Resultado |
|---|---|
| Intervalo efetivo | O systemd acorda um minuto após a inatividade. A sondagem lê arquivos locais. O executor agenda o ciclo normal para pelo menos 1.200 segundos após a publicação. A regra vigente permite uma continuação rápida por ciclo normal quando houve progresso além do diário e existe ação concreta. Após a continuação, volta o intervalo normal. |
| Cota | A mensagem real do journal contém “ERROR: You’ve hit your usage limit”. O executor reconhece essa forma e mantém um marcador compartilhado antes de novas chamadas. Uma chamada já iniciada pelo outro agente pode terminar. A liberação após renovação continua manual. |
| Falta de trabalho | Carlos estava em espera após declarar dependência de hardware. Agora ausência de decisão válida também implica espera. A decisão vem da última linha não vazia da resposta em stdout, antes do rodapé do launcher. Diagnósticos em stderr não autorizam continuação. |
| Falhas | Antes, uma falha deixava um horário normal agendado e podia resultar em tentativas indefinidas, inclusive com arquivos já staged após falha de publicação. Agora o executor grava espera antes de iniciar. Falhas ordinárias registram fase e código no journal; uma interrupção abrupta mantém a espera. Arquivos e índice ficam disponíveis para revisão. |
| Diário | O publicador aceita docs/, e houve commits exclusivamente de agent-progress.md, por exemplo 733abc7 de Bruno. O ciclo agora verifica alterações antes de invocar o publicador. Se apenas o diário mudou, preserva o arquivo localmente e suspende novas chamadas. Isso também cobre ciclos sem alterações. A regra vale para publicações pelo executor dos timers. |

## Reprodução e recuperação

Executar `bash tests/test_handkerchief_cycle.sh` na raiz. O teste usa Git temporário e executores simulados. Abrange continuação rápida limitada, janela de 1.200 segundos, espera sem chamada, diário isolado, falha do agente, falha do publicador com índice preservado, decisão ausente, erro de cota e retomada. O executor anterior falhou na regressão do diário. Os testes não acessam a conta Codex nem o Vita.

Para recuperar uma falha, consultar o journal da unidade, conferir status, staged, HEAD e referência remota da branch. Se o push falhou após commit, resolver a publicação desse commit antes de gastar outra chamada do modelo. Após sanar a causa ou registrar tarefa nova, a coordenação libera apenas o arquivo de agenda do agente. O marcador compartilhado de cota tem precedência.

## Entregas e continuidade

- Carlos publicou a lógica e o aplicativo sintético da rodada até 4931ff2. O teste Linux do módulo passou novamente com UBSan e avisos tratados como erros. O VPK HKRD00001 local teve SHA-256 069297981f6a87e836cdcedfe622e2e0298198933a29ee07c8d6cae8cd3f0687. Integração de VISTA_0 e validação física dessa rodada seguem pendentes. A nova entrega pode avançar no PC. [Tarefa de Carlos](https://github.com/IvandersonBordin/handkerchief-vita/issues/4#issuecomment-6997924705)
- Bruno publicou em ae21aca a identificação de CRobot0 como receptor da origem em um ramo específico. O próximo aceite é uma escrita na posição do próprio ator ou um bloqueio delimitado. [Tarefa de Bruno](https://github.com/IvandersonBordin/handkerchief-vita/issues/8#issuecomment-6997925921)

O console não foi acessado durante a auditoria.

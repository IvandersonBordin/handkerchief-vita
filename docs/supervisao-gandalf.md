# Supervisão técnica local

O timer de usuário consulta as referências Git locais nos minutos 00, 05, 10 e assim por diante. Os timers de Bruno e Carlos continuam independentes. O executor grava a proposta CONTINUAR em espera após o push. A revisão exige coincidência entre a branch do agente e sua referência de acompanhamento local. Na primeira sondagem, a revisão começa pela última entrega de cada branch. O supervisor não consulta o GitHub a cada sondagem.

O serviço testa um arquivo Git do SHA em bwrap, sem rede, com fontes somente para leitura e build temporária. Bruno usa unittest. Carlos usa os cinco programas de teste Linux, compilação ARM, integridade dos dois VPKs e verificação da LiveArea. Após os testes, Gandalf usa GPT-6.2 Sol com raciocínio Light (`low`) para revisar o diff e a proposta de tarefa. O launcher local de Bruno usa GPT-6.2 Astra com o mesmo nível. A resposta contém decisão técnica, justificativa e autorização separada para a proposta. O ciclo do agente consome a autorização uma vez, sob trava, se o SHA ainda for o mesmo. A instalação do supervisor não altera uma execução já iniciada pelo executor anterior.

O intervalo mínimo entre chamadas de Gandalf é 20 minutos e o teto é dezesseis chamadas por janela móvel de 24 horas. O limite é local a Gandalf. Todos respeitam o arquivo usage-limit. O desbloqueio de cota continua manual após confirmar a renovação. O estado inclui o SHA, a identidade da proposta e a decisão. Um teste reprovado, interrupção após a reserva ou erro do revisor estaciona o evento. Uma alteração técnica publicada permite nova avaliação. Falha de cota permite retomar após desbloqueio e respeita o intervalo mínimo. Alterações exclusivamente no diário e sondagens ociosas dispensam o modelo.

Para uma tarefa já autorizada que dependa do outro agente, o coordenador pode preparar no diretório de estado um arquivo bruno.pending ou carlos.pending. As linhas são o SHA completo da entrega do próprio agente, a ação técnica em uma linha e, opcionalmente, o identificador do pré-requisito no formato agente/SHA-completo. Grave por renomeação atômica enquanto o agente estiver em espera. A aprovação registrada do pré-requisito habilita a revisão da proposta. O revisor pode escolher uma ação do catálogo mesmo com proposta ausente. Bloqueios que exigem decisão humana permanecem em espera.

## Continuação e migração

O campo `decision` registra o parecer sobre a entrega. O campo `next_action` registra espera ou uma ação do catálogo mantido em `deploy/gandalf.py`. A mesma chamada decide ambos. O executor recebe exclusivamente a instrução fixa correspondente ao agente. Os pareceres e os textos de propostas ficam fora dessa instrução. C05 permanece bloqueada.

Cada autorização em `grants` inclui agente, SHA, identificador da tarefa e dependência explícita. Sob as travas existentes, o executor valida o recibo, a branch e sua referência publicada e cria um marcador exclusivo antes de iniciar o agente. Se houver interrupção após o consumo, o marcador preserva a execução única. Autorizações antigas em arquivo .approved exigem inspeção. Uma proposta com dependência ausente ou inválida aguarda correção ou evidência.

Para Bruno no SHA `80375220230ce2a4c7276d4ed5d7363dee84b7de`, o parecer anterior já registrou testes aprovados e hold com proposta vazia. A migração acrescenta uma autorização para a investigação estática de EDI, +0x38 e slots virtuais quando esse SHA ainda é a ponta publicada. Preserva o parecer e o histórico de chamadas.

A validação local da alteração passou em dez testes Python e na suíte Bash do executor com Git temporário. Foram exercitados hold com e sem continuação, proposta vazia, escopo, dependência, consumo único, SHA obsoleto, repetição, falha de teste, migração, cota e sondagem ociosa.

## Operação

- Ver agenda e próxima execução com `systemctl --user list-timers 'handkerchief*'`.
- Acompanhar a validação com `journalctl --user -u handkerchief-gandalf.service -f`.
- Consultar decisões com `python3 -m json.tool ~/.local/state/handkerchief/gandalf.json`. O estado pode ainda não existir antes da primeira entrega.
- Suspender novas sondagens com `systemctl --user disable --now handkerchief-gandalf.timer`. Uma revisão em andamento pode terminar. Para interrompê-la também, use `systemctl --user stop handkerchief-gandalf.service`. Uma revisão interrompida após a reserva requer inspeção antes de repetir.
- Retomar com `systemctl --user enable --now handkerchief-gandalf.timer`. Os timers dos agentes permanecem independentes. A execução depende de o gerenciador systemd do usuário estar em funcionamento.
- Reproduzir os testes com `python3 -B -m unittest discover -s tests -p test_gandalf.py` e `bash tests/test_handkerchief_cycle.sh`. As duas suítes usam repositórios temporários e dispensam acesso ao Codex e ao Vita.

## Alternancia de tarefas autorizadas

O catalogo inclui a investigacao estatica b04-indices (enderecos 0x4AD16C e 0x4AD228) para Bruno e os testes Linux de controles-a0 para Carlos. Ambas as tarefas sao limitadas ao escopo tecnico e nao autorizam testes no hardware, instalacoes ou uso de dados proprietarios no Git.

Quando as tres ultimas concessoes de um agente sao da mesma acao, Gandalf remove temporariamente essa acao das opcoes da revisao seguinte. O modelo escolhe entre as outras tarefas previamente autorizadas ou hold. O historico e os recibos antigos permanecem intactos. A regra nao gera novas revisoes sem entrega, nao reabre eventos reservados e nao contorna o intervalo de 20 minutos nem a cota.

Depois de atualizar o checkout local, valide com `python3 -B -m unittest discover -s tests -p test_gandalf.py` antes de reiniciar os servicos. O codigo publicado no GitHub nao atualiza automaticamente o processo systemd local.

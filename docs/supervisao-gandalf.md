# Supervisão técnica local

O timer de usuário consulta as referências Git locais nos minutos 00, 05, 10 e assim por diante. Os timers de Bruno e Carlos continuam independentes. O executor grava a proposta CONTINUAR em espera após o push. A revisão exige coincidência entre a branch do agente e sua referência de acompanhamento local. Na primeira sondagem, a revisão começa pela última entrega de cada branch. O supervisor não consulta o GitHub a cada sondagem.

O serviço testa um arquivo Git do SHA em bwrap, sem rede, com fontes somente para leitura e build temporária. Bruno usa unittest. Carlos usa os cinco programas de teste Linux, compilação ARM, integridade dos dois VPKs e verificação da LiveArea. Apenas após sucesso o Astra recebe o diff e a proposta de tarefa. A resposta contém decisão técnica, justificativa e autorização separada para a proposta. O ciclo do agente consome a autorização uma vez, sob trava, se o SHA ainda for o mesmo. A instalação do supervisor não altera uma execução já iniciada pelo executor anterior.

O intervalo mínimo entre chamadas de Gandalf é 20 minutos e o teto é oito chamadas por janela móvel de 24 horas. O limite é local a Gandalf. Todos respeitam o arquivo usage-limit. O desbloqueio de cota continua manual após confirmar a renovação. O estado inclui o SHA, a identidade da proposta e a decisão. Um teste reprovado, interrupção após a reserva ou erro do revisor estaciona o evento. Uma alteração técnica publicada permite nova avaliação. Falha de cota permite retomar após desbloqueio e respeita o intervalo mínimo. Alterações exclusivamente no diário e sondagens ociosas dispensam o modelo.

Para uma tarefa já autorizada que dependa do outro agente, o coordenador pode preparar no diretório de estado um arquivo bruno.pending ou carlos.pending. As linhas são o SHA completo da entrega do próprio agente, a ação técnica em uma linha e, opcionalmente, o identificador do pré-requisito no formato agente/SHA-completo. Grave por renomeação atômica enquanto o agente estiver em espera. A aprovação registrada do pré-requisito habilita a revisão da proposta. O revisor só libera a ação fornecida. Bloqueios que exigem decisão humana permanecem em espera.

## Operação

- Ver agenda e próxima execução com `systemctl --user list-timers 'handkerchief*'`.
- Acompanhar a validação com `journalctl --user -u handkerchief-gandalf.service -f`.
- Consultar decisões com `python3 -m json.tool ~/.local/state/handkerchief/gandalf.json`. O estado pode ainda não existir antes da primeira entrega.
- Suspender novas sondagens com `systemctl --user disable --now handkerchief-gandalf.timer`. Uma revisão em andamento pode terminar. Para interrompê-la também, use `systemctl --user stop handkerchief-gandalf.service`. Uma revisão interrompida após a reserva requer inspeção antes de repetir.
- Retomar com `systemctl --user enable --now handkerchief-gandalf.timer`. Os timers dos agentes permanecem independentes. A execução depende de o gerenciador systemd do usuário estar em funcionamento.
- Reproduzir os testes com `python3 -B -m unittest discover -s tests -p test_gandalf.py` e `bash tests/test_handkerchief_cycle.sh`. As duas suítes usam repositórios temporários e dispensam acesso ao Codex e ao Vita.

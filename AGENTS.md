
# Handkerchief — Native PS Vita Port

## Project Goal
Develop a native PlayStation Vita port of Handkerchief.

The project's technical feasibility is not yet established.
Do not assume the original game engine, source code,
graphics API, or asset formats.

## Developer Environment
- OS: Manjaro KDE Linux
- Shell: Zsh
- Editor: Visual Studio Code
- AI Agent: OpenAI Codex
- Version Control: Git
- Target Platform: PlayStation Vita
- Porting Skill: vita-port (.agents/skills/vita-port/SKILL.md)

## Communication
- Explain decisions in Brazilian Portuguese.
- The user is a programming beginner.
- Explain commands before executing them.
- Prefer small, verifiable steps.

## Safety
- Do not run sudo or install packages without approval.
- Do not execute downloaded scripts without reviewing them.
- Do not modify system or console firmware settings.
- Treat original game files as read-only.
- Keep proprietary game assets out of Git.
- Never commit credentials or secrets.

## Engineering
- Analyze the original game before choosing an architecture.
- Prefer existing, proven tools over unnecessary rewrites.
- Avoid unnecessary dependencies and abstractions.
- Make memory ownership explicit.
- Avoid unnecessary allocations in performance-critical loops.
- Measure performance and memory usage on real hardware.
- Do not claim a build works without testing it.

## Workflow
1. Inspect the relevant files.
2. Explain the proposed change.
3. Implement the smallest useful change.
4. Run appropriate tests.
5. Report results and remaining uncertainties.
6. Recommend the next step.

## Project Documentation
Record important findings and decisions in docs/.

## Regra prioritária de economia de tokens

- Os timers verificam localmente a cada minuto se há trabalho liberado.
  O intervalo normal entre chamadas ao modelo é de 20 minutos. Após uma
  publicação com alterações além do diário, permitir uma continuação no
  próximo disparo se houver uma próxima ação concreta e já autorizada.
  Após essa continuação, aplicar novamente o intervalo normal.
- O agente encerra o resumo com `CONTINUAR: <ação concreta>` ou
  `AGUARDAR: <dependência ou conclusão>`. Espera explícita ou ciclo sem
  novo commit suspende chamadas até a coordenação registrar uma nova
  tarefa ou evidência e remover o arquivo de estado daquele agente
  (`bruno.schedule` ou `carlos.schedule` no diretório do bloqueio de cota).
- Antes de chamar o Codex, o executor deve verificar localmente o bloqueio
  de cota compartilhado. Se qualquer agente receber `usage limit`, bloquear
  novas chamadas dos dois. Retomar manualmente após confirmar a renovação
  da cota e receber o pedido de Ivan.
- Reaproveite as evidências já registradas. Se a tarefa depende de hardware,
  de uma decisão ou de um dado que continua ausente, registre o bloqueio
  uma vez e aguarde uma mudança. Evite repetir investigações, testes ou
  commits apenas para registrar mais um ciclo de espera.

## Validação e publicação automática

Após cada tarefa de implementação ou documentação neste projeto:

1. Execute as verificações aplicáveis. Para Python, verifique a sintaxe,
   execute os testes existentes e compare com as saídas de referência
   quando disponíveis.
2. Execute `git diff --check`, revise o diff e confira os arquivos
   criados, modificados e excluídos. Confira também o conteúdo staged
   com `git diff --cached` e `git diff --cached --check` antes do commit.
3. Adicione explicitamente apenas os arquivos da tarefa. Evite
   `git add .`. Exclua da publicação `original-data/`, `build/`,
   executáveis comerciais, temporários, credenciais e segredos.
4. Se houver erros, alterações inesperadas ou dúvidas de segurança,
   interrompa a publicação e informe o usuário.
5. Com as verificações aprovadas, crie um commit com mensagem descritiva
   e execute `git push origin main`. A autorização cobre commits e
   pushes normais das tarefas solicitadas neste projeto.
6. Preserve as alterações do usuário e o histórico. A autorização
   exclui ações destrutivas, `push --force`, `reset --hard` e
   reescrita de histórico. Se o push for rejeitado, pare e informe o
   conflito.
7. Ao finalizar, informe o SHA do commit, as verificações executadas,
   o resultado do push e o estado do Git.

## Scope Control
Do not start unrelated tasks.
Do not create multiple agents unless explicitly requested.
Do not perform large refactors without approval.

## Coordenação entre ChatGPT e Codex local

- O ChatGPT no navegador atua como líder do projeto: define objetivos,
  prioridades, critérios de aceite e revisa entregas usando GitHub Issues.
- O Codex no VS Code atua como executor local: lê a issue atribuída à tarefa,
  inspeciona os arquivos do computador, implementa, executa testes e relata evidências.
- Antes de iniciar trabalho novo, verifique a issue indicada no GitHub e siga
  os critérios de aceite. Não invente requisitos além dos descritos.
- Mantenha um resumo da execução na própria issue: evidências, testes,
  limitações, commit e resultado do push. Não publique binários, screenshots
  proprietárias ou dados de `original-data/` na issue.
- Para cada tarefa com mudanças, respeite a rotina de validação e publicação
  automática acima. Se houver bloqueio ou ambiguidade material, registre na
  issue e peça decisão ao usuário; não substitua prova por suposições.
- O ChatGPT não controla nem inicia automaticamente esta sessão local do Codex.
  A execução requer uma sessão ativa no VS Code e autorização apropriada.

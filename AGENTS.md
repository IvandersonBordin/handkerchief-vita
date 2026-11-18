
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

## Validação e publicação — worktree isolado

- Execute testes e revise todas as alterações.
- Não execute git push, merge, rebase ou reset.
- Não altere a branch main.
- Não publique arquivos comerciais ou credenciais.
- Não modifique arquivos fora do escopo do agente.
- Não faça commits automáticos.
- Entregue as alterações para revisão do líder.

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

## Instruções específicas deste worktree

Leia também AGENT_ROLE.md antes de iniciar qualquer tarefa.

As restrições de AGENT_ROLE.md prevalecem quando forem
mais rigorosas que as regras gerais deste arquivo.

Neste worktree:
- Não faça push, merge ou alterações na main.
- Não execute tarefas fora do escopo do agente.
- Não instale dependências sem autorização.
- Não publique arquivos comerciais.
- Deixe commits e integração para revisão do líder.

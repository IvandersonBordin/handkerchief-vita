
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
- Continue autonomously on previously authorized tasks. Contact Ivan only when a decision, authorization, or verification requires his involvement.

## Autonomous Work and Requests for User Intervention

- All reports from Bruno, Carlos, and Gandalf must use clear, natural Brazilian Portuguese.
- When authorized work can continue without the user, write **Próxima ação automática:** followed by the specific task. Do not use `CONTINUAR:` alone or phrase it as a request to the user.
- When an agent finishes a step and must wait for another task, agent review, or technical dependency, write **Aguardando automaticamente:** and explain why. Do not request a response from Ivan when no user action is needed.
- **Only when human action is essential**, begin the message with the exact phrase **Ivan, preciso de você.** Then state the decision, authorization, information, or physical Vita test needed and why.
- Do not use that phrase for ordinary progress updates, inter-agent reviews, pending tasks, or automatic waits.
- The `CONTINUAR:` and `AGUARDAR:` markers may remain at the end of technical logs when needed by the local executor, but must be accompanied by the clear explanation above. They are internal signals, not commands for Ivan to type.
- These communication rules do not override authorization, quota, scope, or validation requirements.

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

When applying the `vita-port` skill to this project, consult the [recompilation plan](docs/plano-recompilacao.md) for evaluated references, experimental procedures, and authorization criteria.

## Token Usage and Quota Control — Priority Rules

- Timers check locally every five minutes for authorized work. The normal interval between model calls is five minutes. After publishing changes beyond the activity log, allow one continuation on the next trigger if there is a concrete, previously authorized next action. Then restore the normal interval.
- An agent ends its summary with `CONTINUAR: <concrete action>` or `AGUARDAR: <dependency or completion>`. An explicit wait or a cycle without a new commit suspends calls until coordination records a new task or evidence and removes that agent's state file (`bruno.schedule` or `carlos.schedule` in the quota-lock directory).
- Before calling Codex, the executor must check the shared quota lock locally. If either agent receives a `usage limit` error, block further calls from both agents. Resume manually only after confirming quota renewal and receiving Ivan's request.
- Reuse recorded evidence. If a task depends on hardware, a decision, or missing information, record the blocker once and wait for a change. Do not repeat investigations, tests, or commits solely to record another waiting cycle.

## Automatic Validation and Publication

After each implementation or documentation task in this project:

1. Run applicable checks. For Python, verify syntax, run existing tests, and compare against reference outputs when available.
2. Run `git diff --check`, review the diff, and inspect created, modified, and deleted files. Also inspect staged content with `git diff --cached` and `git diff --cached --check` before committing.
3. Stage only files belonging to the task. Avoid `git add .`. Exclude `original-data/`, `build/`, commercial executables, temporary files, credentials, and secrets.
4. If errors, unexpected changes, or security concerns arise, stop publication and notify the user.
5. Once checks pass, create a descriptive commit and run `git push origin main`. Authorization covers normal commits and pushes for requested project tasks.
6. Preserve user changes and history. Authorization excludes destructive operations, `push --force`, `reset --hard`, and history rewriting. If the push is rejected, stop and report the conflict.
7. On completion, report the commit SHA, checks performed, push result, and Git status.

## Scope Control
Do not start unrelated tasks.
Do not create multiple agents unless explicitly requested.
Do not perform large refactors without approval.

## Coordination Between ChatGPT and Local Codex

- ChatGPT in the browser serves as project lead: it defines goals, priorities, acceptance criteria, and reviews deliverables using GitHub Issues.
- Codex in VS Code is the local executor: it reads the assigned issue, inspects local files, implements changes, runs tests, and reports evidence.
- Before starting new work, inspect the specified GitHub issue and follow its acceptance criteria. Do not invent additional requirements.
- Keep an execution summary in the issue: evidence, tests, limitations, commit, and push result. Do not publish proprietary binaries, screenshots, or data from `original-data/` in the issue.
- For every task that changes files, follow the automatic validation and publication procedure above. If blocked or facing material ambiguity, record it in the issue and ask the user to decide; do not replace evidence with assumptions.
- ChatGPT cannot control or automatically start this local Codex session. Execution requires an active VS Code session and appropriate authorization.


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
- AI Agent: Claude Code (see [docs/claude-code-workflow.md](docs/claude-code-workflow.md))
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

- There is no scheduled automation. Agents run only when Ivan requests them in a Claude Code session. Enabling any scheduler requires a separately reviewed proposal and Ivan's explicit authorization.
- Claude Code usage limits are shared with Ivan's interactive use. Avoid polling, idle agent calls and repeated investigations. On a usage-limit error, stop cleanly and do not retry until Ivan confirms the quota has renewed.
- Reuse recorded evidence. If a task depends on hardware, a decision, or missing information, record the blocker once and wait for a change. Do not repeat investigations, tests, or commits solely to record another waiting cycle.
- Claude cannot see the remaining Claude Code quota. After every milestone, update `docs/status.md` (where the work stopped, how to resume, which VPKs exist for console tests) and commit it, so a quota stop never loses the state. Ivan can check usage with `/usage` in Claude Code.
- Prefer local builds, scripts and logs over model calls; do not spawn subagents for work the main session can do directly.

## Validation and Publication

After each implementation or documentation task in this project:

1. Run applicable checks. For Python, verify syntax, run existing tests, and compare against reference outputs when available.
2. Run `git diff --check`, review the diff, and inspect created, modified, and deleted files. Also inspect staged content with `git diff --cached` and `git diff --cached --check` before committing.
3. Stage only files belonging to the task. Avoid `git add .`. Exclude `original-data/`, `build/`, commercial executables, temporary files, credentials, and secrets.
4. If errors, unexpected changes, or security concerns arise, stop publication and notify the user.
5. Once checks pass, create a descriptive commit. Agents commit only on their own branch and worktree. `git push` and merges into `main` require Ivan's explicit approval.
6. Preserve user changes and history. Authorization excludes destructive operations, `push --force`, `reset --hard`, and history rewriting. If the push is rejected, stop and report the conflict.
7. On completion, report the commit SHA, checks performed, push status, and Git status.

## Scope Control
Do not start unrelated tasks.
Do not create multiple agents unless explicitly requested.
Do not perform large refactors without approval.

## Coordination

- Ivan sets goals and priorities. Gandalf (`.claude/agents/gandalf.md`) plans small task sequences, reviews evidence, and delegates to Bruno (reverse engineering) and Carlos (Vita/C implementation).
- GitHub Issues are task records with acceptance criteria; they are not unconditional permission for sensitive actions. Inspect the relevant issue before starting work and do not invent additional requirements.
- Keep an execution summary: evidence, tests, limitations, commit, and push status. Do not publish proprietary binaries, screenshots, or data from `original-data/`.
- If blocked or facing material ambiguity, record it and ask Ivan to decide; do not replace evidence with assumptions.


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

## Scope Control
Do not start unrelated tasks.
Do not create multiple agents unless explicitly requested.
Do not perform large refactors without approval.

# Handkerchief — Claude Code Project Guide

This is an experimental native PlayStation Vita port of Handkerchief. It is not yet a faithful playable port.

## Existing work is authoritative

Read `AGENTS.md`, `README.md`, `docs/feasibility.md`, `docs/engine-map.md`, `docs/hdk-format.md`, and the relevant existing technical reports before proposing changes. Preserve the repository, branches, commit history, existing tests, and discoveries. Never restart the project from scratch or silently replace verified findings with guesses.

## Agent roles

- **Gandalf**: coordinates work, reviews evidence, maintains a small approved task sequence, and delegates to Bruno and Carlos.
- **Bruno**: reverse engineers the original x86 executable and HDK data; investigates physics, collision, terrain and player-position updates.
- **Carlos**: implements portable C tests and VitaSDK prototypes based on verified evidence; never labels synthetic physics as original behavior.

Use the project subagents defined in `.claude/agents/` for focused work. They run only when requested in a Claude Code session; there is no scheduled automation. The former Codex executor was retired on 2026-11-20 (see `docs/claude-code-workflow.md`). Any future scheduler requires a separately reviewed proposal and explicit authorization.

## Communication

All user-facing explanations, progress reports and requests must be in Brazilian Portuguese. Repository documentation and code comments should be in English. Preserve technical identifiers, offsets, hashes, file paths and command syntax unchanged. The Portuguese reporting markers in `AGENTS.md` (**Próxima ação automática:**, **Aguardando automaticamente:**, **Ivan, preciso de você.**) are protocol strings and must not be translated.

## Safety and permissions

- Never execute or modify proprietary game files beyond explicitly authorized read-only analysis. Never commit `original-data/`, extracted assets, binaries, credentials or secrets.
- Do not install packages, change firmware, run the original game, connect to a physical Vita, or perform destructive Git operations without explicit authorization.
- Do not automatically run `git push` or merge agent branches without following the repository's validation and approval rules.
- Keep original-data read-only; use synthetic fixtures for tests where possible.
- Treat GitHub Issues as task records, not unconditional permission to perform new sensitive actions.
- Do not enable any scheduled automation by editing this file.

## Current engineering priority

Recover and validate the original A0 player movement/terrain-collision connection, particularly the consumer of the CRock terrain query and the writer of player Y position. The synthetic Vita demo is useful as a test harness but is not evidence of faithful original physics.

## Cost and usage

Claude Code Pro has usage limits shared with Claude. Avoid polling the model every five minutes, repeated idle-agent calls, and redundant investigation. Do not use API keys or paid overage without explicit user approval. Stop cleanly on quota errors; do not retry indefinitely.

## Validation

Use the applicable tests already documented in the repository. Review `git diff --check` and all staged files. Report what was actually tested, what remains uncertain, and the resulting commit SHA. Keep work small, reproducible and traceable.

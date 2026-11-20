# Controlled Local Automation — Handkerchief PS Vita

This automation is **opt-in** and is not enabled by a remote commit. It executes one GitHub issue at a time using the Codex CLI on the user's computer. ChatGPT in the browser **does not** continuously monitor the process or automatically start new tasks. GitHub stores tasks and reports.

## Prerequisites

- Git and GitHub CLI (`gh`) authenticated as `IvandersonBordin`.
- Codex **CLI** available via `command -v codex` and authenticated. The VS Code extension alone does not guarantee that the command-line executable is installed.
- Python 3, systemd --user, and a repository clone at `~/Projetos/handkerchief-vita`. Do not install packages automatically.
- Clean working tree and `main` synchronized with `origin/main` before execution. The executor does not automatically pull pending changes.

## Preparation — Does Not Enable Automation

In a Manjaro terminal, inside the repository:

```bash
git pull --ff-only origin main
command -v codex
codex --version
python3 tools/issue_worker.py
python3 -m py_compile tools/issue_worker.py
```

If the Codex CLI is unavailable, **do not enable the timer**. Installation and authentication require a separate step and consent.

The executor uses Bubblewrap to mount `original-data/` and `.git/` read-only inside a writable project area. The `~/.codex/` directory has restricted write access for client runtime state; when present, `~/.codex/auth.json` is mounted separately as read-only. Other directories outside the project remain read-only. Before starting Codex, the executor checks that `HDKWIN.EXE` is readable but cannot be opened for writing. Codex uses `--ephemeral` to reduce writes outside the project. This isolation configuration still requires local testing with Codex CLI.

The executor selects only open issues created by the repository owner and labeled `codex-ready`. To create the labels once:

```bash
for label in codex-ready codex-running codex-blocked codex-done; do
  gh label create "$label" --repo IvandersonBordin/handkerchief-vita --color D4C5F9 --force
done
```

To authorize issue #1 after reviewing its contents:

```bash
gh issue edit 1 --repo IvandersonBordin/handkerchief-vita --add-label codex-ready
```

Before execution, perform the read-only isolation test with Bubblewrap according to the setup procedure. The safe mode above does not validate GitHub credentials or the entire Codex integration.

To **run a single manual test**, without scheduling:

```bash
python3 tools/issue_worker.py --run
```

Logs are stored in `build/codex-issue-N.log`, outside Git. Check issue status using `gh issue view 1 --repo IvandersonBordin/handkerchief-vita`.

## Enable Periodic Execution (Optional; After a Successful Manual Test)

```bash
mkdir -p ~/.config/systemd/user
cp deploy/handkerchief-worker.service deploy/handkerchief-worker.timer ~/.config/systemd/user/
systemctl --user daemon-reload
systemctl --user enable --now handkerchief-worker.timer
systemctl --user list-timers handkerchief-worker.timer
```

The timer checks for a task approximately every 15 minutes while the user session and computer are active. VS Code does not need to remain open, but Codex CLI and authentication must be available to the service. **Do not enable linger** by default. Execution cannot continue when the computer is powered off or suspended.

To pause immediately:

```bash
systemctl --user disable --now handkerchief-worker.timer
```

To inspect logs:

```bash
journalctl --user -u handkerchief-worker.service -n 100 --no-pager
```

## Security and Known Limitations

- The executor requires `--run` and uses a local lock to prevent concurrent execution.
- It selects only owner-created issues labeled `codex-ready`, reserves them as `codex-running`, removes `codex-ready`, and requires manual reauthorization for blocked attempts.
- It executes `codex exec --ephemeral` with `workspace-write` inside Bubblewrap, with a 45-minute limit; it does not use `danger-full-access`.
- It prevents publication of prohibited files, executables, deletions, oversized files, or detected changes to original-game metadata; it protects `AGENTS.md` and the executor itself from automatic changes.
- It checks `git diff --check`, the AST of modified Python files, available unittest suites, and the staged file list before publishing.
- This system **is not an absolute security boundary**. Codex can write to `~/.codex/` (except `auth.json`, when present) to maintain runtime state, so isolation does not guarantee protection from every process side effect. Bubblewrap prevents writes to the originals during the isolated session, but the external process controls commits and publication. Metadata checks cannot detect every possible modification; maintain backups of the original game files.
- Automated checks are limited: image-specific and visual-fidelity tests may still require human review. A published commit does not imply technical approval by ChatGPT.
- Issues are instruction inputs: authorize only tasks written or reviewed by you and the project lead. Do not use this automation for sensitive operations, installations, firmware changes, or publishing commercial assets.
- On Git divergence, Codex CLI failure, authentication problems, validation errors, or rejected pushes, execution stops and records a blocker. It does not force-push or reset history.

### Reference Gameplay Recording in Local Executors

The installed launcher makes the user-supplied MP4 available to Bruno and Carlos at `/opt/handkerchief-reference/gameplay.mp4` when the file exists in Downloads. Only that file is mounted, read-only. The configuration was checked with `bash -n` and a Bubblewrap mount test. The video and extracted frames remain outside Git. The recording guides authorized tasks and does not change scheduling or execution limits.

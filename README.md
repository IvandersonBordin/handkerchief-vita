# Handkerchief — PS Vita Port (Research Project)

> **Work in progress.** An independent, non-commercial project that runs the original Windows game *Handkerchief* (OPUS / ASK, 1999-2000) natively on the PlayStation Vita, by statically recompiling its x86 executable to C. You must supply your own copy of the game.

[Project status](docs/status.md) · [Static recompilation](docs/static-recomp.md) · [Save and stages](docs/save-and-stages.md)

## Current status (2026-11-21)

The recompiled game runs on Linux and on a real PS Vita (PCH-1010, firmware 3.65) at its native 25 frames per second.

| Area | State |
| --- | --- |
| Intro, title screen, stage A0 | Playable on Linux and on the Vita, verified on hardware |
| Graphics | DirectDraw 640x480x8 emulated in software; GDI text (speech balloons) with a bundled font; 4:3 or stretched 16:9 |
| Sound | DirectSound/PlaySound over an SDL mixer; music and effects play. Mixing in loud scenes is still being tuned |
| Input | Keyboard on Linux; pad on the Vita (Cross shoot/confirm, Square jump, Start pause, Select game menu) |
| Menus | The game's Win32 menu bar and dialogs (exit, game and key configuration, save/load slots, text entry with the Vita keyboard) are drawn over the game and driven by the pad |
| Save / continue | Works on both platforms, verified on the console |
| Other stages | Territories A-J load (one area, e2, returns to the intro); a full playthrough has not been tested |
| Port extras | Pause screen with FPS counter, aspect toggle and cheats (the game's own "Unrivaled Mode") |

Known issues: the mix saturates in some heavy scenes, scene transitions pause briefly, a white line appears at one screen edge in A0, and native widescreen is not implemented yet.

## Technical approach

- `recomp/x86rec.py` (derived from [sh3vita](https://github.com/matfsouza/sh3vita), GPL-3.0) translates the whole `HDKWIN.EXE` to C. The generated code runs unchanged; the port supplies the Windows it expects.
- `port/` implements that environment: a guest memory and CPU runtime (`port/rt`), Win32 (`port/win32`), and the game-specific DirectDraw, GDI, DirectSound and dialog layers (`port/hk`). `port/pc` and `port/vita` are the platform entry points (SDL2).
- Reverse engineering of the executable and the HDK data (docs/engine-map.md, docs/hdk-format.md) guides fixes; findings are kept with addresses and evidence.

## Building

Generated C and the resulting binaries are derived from the commercial executable: they stay local and are never published. With your own game files:

```sh
build/venv/bin/python -I recomp/x86rec.py && build/venv/bin/python -I recomp/gen_import_stubs.py
make -f port/Makefile.pc -j8                       # Linux: build/hk-pc/handkerchief <game folder>
cmake -S port/vita -B build/hk-vita -DCMAKE_BUILD_TYPE=Release && cmake --build build/hk-vita -j8   # Vita VPK
```

On the Vita, copy the game files (`HDKWIN.EXE`, `BITMAP/`, `SYSTEM/`, `USER/`, `WAV/`) to `ux0:data/handkerchief/`.

## Development workflow: from Codex to Claude Code

The project was originally developed with an automated **OpenAI Codex CLI** executor (a scheduled cycle of scripts, systemd units and issue workers). On **2026-11-20** that executor was retired and development moved to **[Claude Code](https://claude.com/claude-code)**, used interactively from Visual Studio Code.

The switch brought a substantial productivity gain for this project in the author's experience. Investigations and prototype iterations take fewer round-trips. Reverse-engineering findings are easier to review and keep traceable. Work no longer stalls on failed automated cycles or usage-limit interruptions in the middle of a review.

How the project works with Claude Code today:

- **Gandalf** (coordination and evidence review) is the main Claude Code session the author talks to. It delegates long, well-bounded tasks to the subagents in [`.claude/agents/`](.claude/agents/): **Bruno** (x86/HDK reverse engineering and the recompiler, on Opus) and **Carlos** (runtime, Win32/DirectDraw emulation and VitaSDK builds, on Sonnet), and a read-only **tester** on Haiku for builds, test runs and screenshots.
- Agents run **only when requested** in a Claude Code session; there is no scheduled automation.
- Pushes and merges into `main` still require the author's explicit approval, and the existing validation rules still apply.

The retired Codex executor remains available in Git history (up to commit `306d7ba`). See [Claude Code workflow](docs/claude-code-workflow.md) for details.

## Reproducing the data inspection (earlier research tools)

Requirements: Python 3 and your **own legally obtained copy** of the original game files. On Linux, place the following files locally (paths are case-sensitive):

```text
original-data/
├── HDKWIN.EXE
├── SYSTEM/A0.HDK
└── BITMAP/HDKMAPA0.BMP
```

From the repository root:

```sh
python3 -B tools/inspect_hdk.py original-data/SYSTEM/A0.HDK
python3 -B tools/render_cearth.py
python3 -B tools/render_vista0.py
```

These tools inspect HDK sections and render background previews under `build/`. They read the original files without modifying them. The original game files, derived assets and local build outputs are **not distributed in this repository**.

For the VitaSDK demonstration build and its requirements, see [VISTA_0 prototype instructions](docs/vista-walk.md). Other experimental prototypes described there may use local build artifacts or assets that are not published.

## Contributions

Contributions and technical discussions can be proposed through [GitHub Issues](../../issues).

## Legal and project boundaries

This is an unofficial, non-commercial research effort, licensed under GPL-3.0 (see LICENSE and THIRD_PARTY_NOTICES.md). No original commercial game executable or proprietary game assets are provided. You must supply your own original data. Reverse-engineering findings and prototype behavior should not be interpreted as proof of compatibility with the complete game.


## Technical documentation

- [Project status and handoff](docs/status.md)
- [Static recompilation](docs/static-recomp.md)
- [Save and stages](docs/save-and-stages.md)
- [Feasibility and strategy](docs/feasibility.md)
- [Engine map](docs/engine-map.md)
- [HDK format](docs/hdk-format.md)
- [Character investigation](docs/character-identification.md)
- [Vita prototypes and tests](docs/vista-walk.md)
- [Claude Code workflow](docs/claude-code-workflow.md)
- [Development and validation guidelines](AGENTS.md)

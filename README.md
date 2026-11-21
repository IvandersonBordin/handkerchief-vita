# Handkerchief — PS Vita Port (Research Project)

> **Work in progress.** This is an independent research project exploring a native PlayStation Vita port of *Handkerchief*. It is **not a complete or faithful playable port**.

[Technical documentation](docs/feasibility.md)

## Current status

The project combines reverse engineering of the original Windows x86 game with native C/VitaSDK prototypes and Linux-based tests.

| Area | Current evidence | Limitations |
| --- | --- | --- |
| Original game data | Python readers for HDK sections, backgrounds and selected sprite banks; previews of A0 | Not all objects, events or game assets are decoded |
| PS Vita prototype | Camera, input, rendering and experimental character animation demonstrated on hardware | Prototypes are not a full game |
| Movement | A synthetic test course supports walking, jumping, obstacles, finish and restart | Its physics are **not** the original game's physics |
| Terrain investigation | ROCK_0 terrain grids decoded; an original vertical terrain query compared against an independent model in 432 test cases | The connection between terrain queries and player movement/collision is not yet established |
| Original player behavior | x86 position records and related routines are being traced | Gravity, jumping and faithful ground collision remain unverified |

The most important milestone is **a character moving and colliding against A0's actual terrain**, with behavior validated against the original game. The current prototypes do not yet achieve this.

## Technical approach

- Inspect original HDK data and selected x86 routines to understand formats and game behavior.
- Build portable, testable C components for game logic, with VitaSDK-specific platform code.
- Use small isolated differential tests to check recovered behavior instead of assuming that a plausible implementation is faithful.
- Evaluate translation/recompilation of individual routines only when it provides a measurable benefit; **full executable recompilation is not the current plan**.

See [feasibility and strategy](docs/feasibility.md), [engine map](docs/engine-map.md), [HDK format](docs/hdk-format.md), and [prototype test notes](docs/vista-walk.md).

## Development workflow: from Codex to Claude Code

The project was originally developed with an automated **OpenAI Codex CLI** executor (a scheduled cycle of scripts, systemd units and issue workers). On **2026-11-20** that executor was retired and development moved to **[Claude Code](https://claude.com/claude-code)**, used interactively from Visual Studio Code.

The switch brought a substantial productivity gain for this project in the author's experience. Investigations and prototype iterations take fewer round-trips. Reverse-engineering findings are easier to review and keep traceable. Work no longer stalls on failed automated cycles or usage-limit interruptions in the middle of a review.

How the project works with Claude Code today:

- **Gandalf** (coordination and evidence review) is the main Claude Code session the author talks to. It delegates long, well-bounded tasks to the subagents in [`.claude/agents/`](.claude/agents/): **Bruno** (x86/HDK reverse engineering and the recompiler) and **Carlos** (runtime, Win32/DirectDraw emulation and VitaSDK builds) on Opus, and a read-only **tester** on Haiku for builds, test runs and screenshots.
- Agents run **only when requested** in a Claude Code session; there is no scheduled automation.
- Pushes and merges into `main` still require the author's explicit approval, and the existing validation rules still apply.

The retired Codex executor remains available in Git history (up to commit `306d7ba`). See [Claude Code workflow](docs/claude-code-workflow.md) for details.

## Reproducing the data inspection

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

This is an unofficial, non-commercial research effort. No original commercial game executable or proprietary game assets are provided. You must supply your own original data. Reverse-engineering findings and prototype behavior should not be interpreted as proof of compatibility with the complete game.


## Technical documentation

- [Feasibility and strategy](docs/feasibility.md)
- [Engine map](docs/engine-map.md)
- [HDK format](docs/hdk-format.md)
- [Character investigation](docs/character-identification.md)
- [Vita prototypes and tests](docs/vista-walk.md)
- [Claude Code workflow](docs/claude-code-workflow.md)
- [Development and validation guidelines](AGENTS.md)

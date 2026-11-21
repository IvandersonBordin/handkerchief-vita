# Claude Code Workflow

Since 2026-11-20 the project is driven interactively from Claude Code. The previous
Codex CLI executor (`deploy/gandalf.py`, `deploy/handkerchief-cycle`,
`tools/issue_worker.py`, the `handkerchief-*` systemd units and the
`~/.local/bin/handkerchief-{agent,cycle,publish}` launchers) was retired. It remains
available in Git history up to commit `306d7ba`, and a local copy with SHA-256 sums was
kept outside the repository in `~/.local/share/handkerchief-codex-arquivo/`.

There is **no scheduled automation**. Agents run only when Ivan asks for them in a
Claude Code session. Any future scheduler requires a separate, reviewed proposal and
explicit authorization before it is enabled (see `CLAUDE.md`, "Cost and usage").

## Agents

Defined in `.claude/agents/`:

Gandalf is the main session Ivan talks to (its model is the session's, chosen with `/model`).
Subagents it can delegate to:

| Agent | Role | Model |
|---|---|---|
| `bruno` | Reverse engineering and the x86 -> C recompiler | opus |
| `carlos` | Runtime, Win32/DirectDraw emulation, Linux and Vita builds | opus |
| `tester` | Builds, test runs, screenshots, log summaries (no edits) | haiku |

Since the 2026-11-20 pivot to static recompilation (docs/static-recomp.md), delegated work uses a
worktree branched from `recomp/static-pipeline`. The older worktrees `../handkerchief-vita-bruno`
(`agent/reverse-engineering`) and `../handkerchief-vita-carlos` (`agent/vita-smoke-test`) keep the
earlier findings and prototypes. Short tasks are done directly in the main session: each subagent
starts without context and spends quota re-reading it.

Each agent works only in its own worktree. Agents may commit locally on their own branch
after validation; `git push` and merges into `main` require Ivan's approval.

## Reporting markers

The Brazilian Portuguese markers defined in `AGENTS.md` are kept unchanged:
**Próxima ação automática:**, **Aguardando automaticamente:** and
**Ivan, preciso de você.** The former executor-only markers `CONTINUAR:` and
`AGUARDAR:` are no longer parsed by any program and should not be used.

## Validation commands

Bruno (`../handkerchief-vita-bruno`):

```sh
PYTHONPATH="$PWD/tools:$PWD" python3 -B -m unittest discover -s tests -v
```

Carlos (`../handkerchief-vita-carlos/vita-prototype`), as previously run by the retired
review gate:

```sh
flags="-std=c11 -Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all"
cc $flags sprite.c test_sprite.c -o /tmp/test_sprite && /tmp/test_sprite /tmp/synthetic.rgba
cc $flags sprite.c test_sprite_animation.c -o /tmp/test_sprite_animation && /tmp/test_sprite_animation /tmp/synthetic.rgba
cc $flags demo_round.c test_demo_round.c -o /tmp/test_demo_round && /tmp/test_demo_round /tmp/synthetic.rgba
cc $flags demo_round.c round_scene.c test_round_scene.c -o /tmp/test_round_scene && /tmp/test_round_scene /tmp/synthetic.rgba
cmake -S . -B /tmp/hk-build -DCMAKE_BUILD_TYPE=Release && cmake --build /tmp/hk-build -j2
python3 -B check_livearea.py
```

Also run these two (all passed on 2026-11-20, `sdk_include=$HOME/vitasdk/arm-vita-eabi/include`):

```sh
cc $flags test_crock.c -o /tmp/test_crock && /tmp/test_crock
cc $flags -idirafter "$sdk_include" -Dmain=smoke_main -c main.c -o /tmp/main.o &&
cc $flags -idirafter "$sdk_include" test_loop.c sprite.c /tmp/main.o -o /tmp/test_loop && /tmp/test_loop
```

`test_loop` must print `Active loop time: 1200000 us` twice (see `docs/vita-smoke-test.md`
on Carlos's branch). On `main`, the camera regression needs the link flags from
`docs/vista-walk.md` (`-ffunction-sections -fdata-sections -Wl,--gc-sections`).

## Reference sources

Local-only, never committed:

- Original data: `original-data/` (read-only; Claude Code deny rules block writes).
- Ghidra project: `~/Projetos/handkerchief-ghidra.gpr` / `.rep` (`analyzeHeadless` is not
  on `PATH`; use the Ghidra install under `/usr/bin/ghidra`'s tree).
- Gameplay video (51 min, 360p): `~/Downloads/YTDown.com_YouTube_Media_u65GuE901bg_Handkerchief-PC-Game-Gameplay_002_360p.mp4`.
  Earlier reports cite the A0 opening at 01:00.
- Recompilation proofs and Vita diagnostics: `build/recomp-*`, `build/gameplay-reference/`.
- Vita tooling: `~/Projetos/ferramentas/vdpm`, `~/Projetos/ferramentas/vitacompanion`.

External projects already evaluated (PCRecomp, sh4vita/sh3vita `x86rec.py`, I.G.I. recomp,
cnc-ddraw, soloader, vitaGL and others) are catalogued with verdicts in
[plano-recompilacao.md](plano-recompilacao.md), section B.

The `vita-port` skill lives in `.agents/skills/vita-port` and is exposed to Claude Code
through the symlink `.claude/skills/vita-port`.

## State carried over from the Codex executor (2026-11-20)

- Last Gandalf-approved tips: Bruno `0da4c45`, Carlos `fe4c5c4`.
- Bruno `2c9c97f` (callers of `0x418E47`: `0x418E2F`, `0x42DBD0`) is published but
  **not reviewed**; the Codex review failed on a usage limit.
- Proposed next Bruno task (not yet approved): trace the value saved in `[esp+0x14]`
  from `0x42DBDB` and check whether it takes part in a write to the Y field (`+0x1C`).
- Carlos last waited on evidence that the CRock query Y reaches the player.
- Neither agent branch has been merged into `main` (merge-base `bd1a8bc`). Both branches
  still carry pre-migration copies of `AGENTS.md`, `docs/local-automation.md` and, for
  Bruno, `AGENT_ROLE.md`. Integrate findings by selecting the technical files
  (`docs/physics-collision.md`, `docs/hdk-format.md`, `tests/`, `tools/`,
  `vita-prototype/`) rather than merging those legacy files.

Task catalog used by the former supervisor (IDs kept for traceability):
`altura-a0`, `terreno-a0`, `b04-indices`, `b03-edi`, `b02-posicao` (Bruno);
`teste-percurso-a0`, `sonda-terreno-a0`, `controles-a0`, `rodada-reinicio` (Carlos).
C05 remains blocked until original movement/collision evidence exists.

## GitHub labels

The `codex-ready`, `codex-running`, `codex-blocked` and `codex-done` labels were used only by
the retired issue worker. No program reads them now.

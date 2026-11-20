# Handkerchief — PS Vita Port (Research Project)

> **Work in progress.** This is an independent research project exploring a native PlayStation Vita port of *Handkerchief*. It is **not a complete or faithful playable port**. Feedback and technical review are welcome.

[Português](#português) · [Technical documentation](docs/feasibility.md)

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

## Feedback wanted

I am learning game porting, reverse engineering and C development, and I use Codex to assist with investigation and implementation. I would particularly appreciate review of:

- The division between portable game logic and Vita-specific rendering/input.
- The terrain and player-physics investigation, including missing evidence and better test strategies.
- The HDK readers, data structures, memory usage and potential performance issues on PS Vita.
- The build/test setup and whether the project is becoming unnecessarily complex.

Please feel free to open an [issue](../../issues) with constructive suggestions. **No one is expected to do the work for me**; the goal is to learn and improve the implementation.

## Legal and project boundaries

This is an unofficial, non-commercial research effort. No original commercial game executable or proprietary game assets are provided. You must supply your own original data. Reverse-engineering findings and prototype behavior should not be interpreted as proof of compatibility with the complete game.

---

## Português

Este é um projeto independente de pesquisa para portar *Handkerchief* nativamente ao PS Vita. **Ainda não é um port completo nem fiel ao jogo original.** O objetivo é estudar os formatos HDK, compreender partes do motor Windows x86 e implementar componentes testáveis em C com VitaSDK.

**Já existe:** leitura parcial dos dados, prévias de cenário, protótipos no Vita com controles/animação e testes de uma rodada com física sintética. Uma consulta vertical do terreno original foi validada em 432 casos isolados.

**Ainda falta:** vincular as consultas do terreno à movimentação real do protagonista, reproduzir gravidade, saltos e colisões, e implementar o restante das regras, eventos e fases.

**Ajuda bem-vinda:** revisão da arquitetura, da engenharia reversa, dos testes e da separação entre lógica portátil e código específico do Vita. Sugestões podem ser enviadas pelas [issues](../../issues).

### Documentação técnica

- [Viabilidade e estratégia](docs/feasibility.md)
- [Mapa do motor](docs/engine-map.md)
- [Formato HDK](docs/hdk-format.md)
- [Investigação de personagens](docs/character-identification.md)
- [Protótipos e testes no Vita](docs/vista-walk.md)
- [Orientações de desenvolvimento e validação](AGENTS.md)

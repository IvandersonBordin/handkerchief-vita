# Handkerchief for PlayStation Vita

![Handkerchief for PlayStation Vita](docs/media/cover.png)

An unofficial, non-commercial port of the Windows game *Handkerchief* (OPUS Corp. / ASK Co., Ltd., 1999-2000) to the PlayStation Vita. The original executable is statically recompiled from x86 to C, so the game's own code runs unchanged; the port supplies the Windows environment it expects. **You need your own copy of the game.**

<p align="center"><img src="docs/media/gameplay.gif" alt="Title screen and the first stage running in the port" width="480"></p>

Screenshots are captured from the port running the original game data. The game's artwork belongs to its rights holders.

> **25 FPS is the original game's speed, not a performance limit.** *Handkerchief* was designed to update and draw exactly 25 frames per second, and all of its movement, animation and timing depend on that rate.

## Status

Work in progress. Tested on a PS Vita 1000 (firmware 3.65, HENkaku) and on Linux.

| Area | State |
| --- | --- |
| Intro, title, stages | Running at the native 25 FPS in 4:3 and native widescreen on the console. All 10 territories, their bosses' RESULTS screens and the ending/credits run through on the port (verified on PC by triggering each boss's own death path); play-testing on the Vita is ongoing |
| Graphics | DirectDraw 8-bit emulated in software; **native widescreen** by default (848x480, more of the world visible), or 4:3 and stretched 16:9 |
| Sound | Music and effects through a software mixer following DirectSound semantics, fixed volumes, separate music and effects levels. A 32-bit allocation wrap that turned missing WAVs into full-scale noise on the Vita is fixed |
| Menus | The game's own menus and dialogs (save/load slots, game and key configuration, exit) drawn over the game and driven by the pad; on-screen keyboard for save names |
| Save / continue | Works |
| Extras | Pause menu in the style of the game's title screen: save, load (also inside a stage), RESULTS, cheats, settings, button remapping, language. Select shows a tips & controls card |
| Memory and loading | Guest memory stays flat over 20 stage restarts on the console (a 32-bit allocator bug that leaked memory is fixed); stage files load from RAM, so area changes and saves no longer stall |

## Installing on the PS Vita

1. Build the VPK (below) and install it with VitaShell.
2. Copy your game files to `ux0:data/handkerchief/`: `HDKWIN.EXE`, `BITMAP/`, `SYSTEM/`, `USER/` and `WAV/`.
3. Start *Handkerchief* from the LiveArea. A short log (startup and errors) is written to `ux0:data/handkerchief/hklog.txt`; the previous launch's is kept as `hklog.prev.txt`. Attach both when reporting a bug.

## Controls (PS Vita)

| Button | Action |
| --- | --- |
| D-pad / left stick | Move. Down: lie down (press again or another direction to get up) |
| Cross | Jump; always confirms in menus, dialogs and the title screen |
| Square | Shoot |
| Circle | Switch and rotation (board or leave vehicles); always cancels in menus and dialogs |
| Triangle | Turncoat bomb |
| R (hold) | Rapid fire |
| L | Turncoat bomb (next to Shoot on R) |
| Select (hold) | Tips & controls card during a stage: your current button map and the game's own hints, one at a time (Left/Right: next tip; the game keeps running). It is a button action, so it can be remapped |
| Start | Pause screen: save, load, display options, widescreen, FPS counter, volume, cheats, controls, Results, and the game's menu (Game menu: save, load, stop, exit, configuration) |

The gameplay buttons can be remapped in Start > Settings > Controls (choose an action, press Cross, then the new button; the previous owner of that button swaps in). The D-pad, left stick and Start are fixed, and Cross/Circle keep confirm/cancel in menus whatever the map. The map is saved in `hkport.cfg` as `map_<button>=<action>` lines. `hkport.cfg` also holds the options of the pause screen (language, aspect, native widescreen, FPS counter, volume, music); a fresh install starts with the FPS counter off and volume 7. The load-time improvements (`preopen`, `userfast`, `preload_se`, `savefast`) and the heap fix (`heapfix`) are on by default; setting one of them to 0 in `hkport.cfg` (for example `savefast=0`) switches it off if it ever causes trouble. The game's menu bar can also be put on a button there (action "Game menu"); an older `hkport.cfg` that had Select on the game menu is moved to the tips card once. The RESULTS panel (time, shots, bombs, expenditure, kills, destruction, injuries) is an item of the Start menu, right below Settings (greyed outside a stage); Circle or Cross returns to the menu.

## Language (Brazilian Portuguese)

The game is in English. A Brazilian Portuguese fan translation exists, made by **Patryck Padilha de Oliveira**
([patryckpo.com](https://patryckpo.com)); it is not part of this repository and is not redistributed here. Download it from
<https://patryckpo.com/traducoes/windows/handkerchief/downloads/> (a WinRAR self-extracting archive; its contents are
`HDKWINBR.EXE` and a `BITMAP/` folder with `HDKCON00.BMP` and `HDKSYS00.BMP`).

1. Extract it and copy the files to `ux0:data/handkerchief/lang/ptbr/` (so you have `lang/ptbr/HDKWINBR.EXE`,
   `lang/ptbr/BITMAP/HDKCON00.BMP` and `lang/ptbr/BITMAP/HDKSYS00.BMP`). Your original game files stay untouched.
2. In the pause screen choose Start > Settings > Language > Português (BR) and restart the game. The port's own menus
   switch to Portuguese too. If the files are not found, the game stays in English and the menu says so.

On Linux, run with `HK_LANG=pt` and the same folder layout inside the game folder (`<game>/lang/ptbr/`).

## Building your own VPK (easy way)

One command builds the VPK on your own computer from your own copy of the game; nothing from the game is distributed. It needs Linux x86-64, about 3 GB of free disk space and an internet connection on the first run (it downloads a pinned [VitaSDK](https://vitasdk.org), about 110 MB, into `~/.cache/handkerchief-builder`). The first build takes a few minutes; re-running is nearly instant. Only the original `HDKWIN.EXE` is supported (its SHA-256 is checked: `3deeb9b18ac2f6ee7b4af4769d1499fdfe0bde8de44482b57ab4bf6196617b76`).

**Linux**

```sh
git clone <this repository> handkerchief-vita && cd handkerchief-vita
curl -LsSf https://astral.sh/uv/install.sh | sh        # once; or just use python3, see below
uv run build.py --game "/path/to/folder/with/HDKWIN.EXE"
```

Without uv: `python3 build.py --game "/path/to/folder"` (it creates a private venv in the cache folder). It also needs `curl`, `tar` and `bzip2`. If you run it from inside the game folder (or leave `--game` out and the game is in a common place) the folder is detected automatically.

**Windows**: the build runs inside WSL (Windows Subsystem for Linux); native Windows is not supported.

1. In PowerShell as administrator: `wsl --install -d Ubuntu`, reboot, open Ubuntu.
2. In Ubuntu: `sudo apt update && sudo apt install -y git curl bzip2 python3 python3-venv`.
3. Clone the repository and run it, pointing to the game through `/mnt/c`:
   `python3 build.py --game "/mnt/c/Games/Handkerchief"` (use the folder where `HDKWIN.EXE` is).
4. The result is in the `out` folder (`\\wsl$\Ubuntu\home\<you>\handkerchief-vita\out` in Explorer).

**Result and installing**: `out/handkerchief.vpk` (about 1.9 MB) and `out/ux0_data_handkerchief/` (your game files laid out for the console). Install the VPK with VitaShell, then copy the contents of `ux0_data_handkerchief` to `ux0:data/handkerchief/`.

Optional Portuguese (BR) fan translation (see below): extract it, then add `--ptbr "/path/to/extracted/folder"`; the files are placed in `lang/ptbr/` for you. `--clean` forgets the cached build, `--cache DIR` and `--out DIR` change the folders.

## Building (developers)

Requirements: Python 3 with `capstone`, a C compiler and SDL2 for the Linux build, [VitaSDK](https://vitasdk.org) with SDL2 for the Vita build. The function list (addresses and symbol names only) is committed in `recomp/data/`, so Ghidra is not needed. The generated C and every binary built from it are derived from the commercial executable: keep them local and never redistribute them.

```sh
# 1. C generation from your HDKWIN.EXE in original-data/ (set HK_EXE and HK_GEN to use other paths)
build/venv/bin/python -I recomp/x86rec.py && build/venv/bin/python -I recomp/gen_import_stubs.py
# 2. Linux build: run it with the game folder as argument
make -f port/Makefile.pc -j8 HK_DEVTOOLS=0 OUT=build/hk-pc && build/hk-pc/handkerchief /path/to/game
# 3. PS Vita VPK (player build: Release, no developer tools; `python3 build.py` does this for you)
cmake -S port/vita -B build/hk-vita -DCMAKE_BUILD_TYPE=Release -DHK_DEVTOOLS=OFF && cmake --build build/hk-vita -j8
# Developers: -DHK_DEVTOOLS=ON (Vita) / HK_DEVTOOLS=1 (PC, the Makefile default) adds the test aids
# (prof, hitch log, memlog, stress driver, heap trace, audio recorder, HK_TRACE_*).
```

See [docs/static-recomp.md](docs/static-recomp.md) for the recompiler and the runtime design.

## How it works

- `recomp/`: the x86 to C static recompiler (derived from [sh3vita](https://github.com/matfsouza/sh3vita)) and the data patches and hooks the port needs.
- `port/rt`: guest memory, CPU state and the native runtime.
- `port/win32`: the Win32 API subset the game uses (files, timers, messages, registry, threads).
- `port/hk`: DirectDraw, GDI text, DirectSound, the dialog and menu system, and the widescreen hooks.
- `port/pc` and `port/vita`: the platform entry points (SDL2).

Technical documentation: [static recompilation](docs/static-recomp.md), [engine map](docs/engine-map.md), [HDK data format](docs/hdk-format.md), [save and stages](docs/save-and-stages.md), [widescreen](docs/widescreen-plan.md).

## How this port was made

This project was built with the help of AI. The recompiler, the runtime and the Windows/DirectX emulation were written with an AI coding assistant ([Claude Code](https://claude.com/claude-code) by Anthropic). The project owner directed the work, decided what to build and tested every build on a real PS Vita. AI-written changes were checked against the original game's behaviour with logs, screenshots and measurements on PC and on the console before being kept.

## License

GPL-3.0 (see [LICENSE](LICENSE)). Third-party components and their licences are listed in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). *Handkerchief* is a trademark of its respective owners; this project is not affiliated with them and distributes no game data.

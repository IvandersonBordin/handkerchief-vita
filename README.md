<h1 align="center">Handkerchief for PlayStation Vita</h1>

<p align="center">
  <img src="docs/media/cover.png" alt="Handkerchief for PlayStation Vita" width="720">
</p>

<p align="center">
  <a href="../../releases/latest"><img alt="Latest release" src="https://img.shields.io/github/v/release/IvandersonBordin/handkerchief-vita?include_prereleases&label=download&color=6a5acd"></a>
  <img alt="Platform" src="https://img.shields.io/badge/platform-PS%20Vita-003791">
  <img alt="Status" src="https://img.shields.io/badge/status-beta-orange">
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/badge/license-GPL--3.0-blue"></a>
</p>

<p align="center">
  <b>The 1999 Japanese PC run-and-gun <i>Handkerchief</i>, running natively on the PS Vita.</b><br>
  The game's own code, recompiled from x86 to ARM: no emulator, the original gameplay unchanged.<br>
  <a href="GUIA-PT.md">🇧🇷 Guia em português</a>
</p>

> [!NOTE]
> **Public beta.** The whole game runs from start to finish, but it has not been played on every console setup yet. If something goes wrong, please [report it](#-reporting-bugs) (English or Portuguese). Every report helps.

<p align="center"><img src="docs/media/gameplay.gif" alt="Title screen and the first stage running on the port" width="480"></p>

## ✨ Features

- **The complete game**: intro, all 10 territories, the bosses, RESULTS screens, ending and credits.
- **Native widescreen** (848x480, more of the world visible), plus the original 4:3 or stretched 16:9.
- **Full sound**: music and effects through a mixer that follows the original DirectSound behaviour.
- **The original look everywhere**: the port's pause and settings menus, dialogs and save screens are drawn with the game's own art and lettering.
- **Pause menu**: save and load anywhere, RESULTS, cheats, display and volume options, button remapping, language.
- **Continue after GAME OVER** from your last save.
- **Tips & controls card** on Select, with the game's own hints.
- **Brazilian Portuguese** with the fan translation (optional, see [Language](#-language-brazilian-portuguese)).
- **Fast loading** and multi-core presentation on the console.

> **25 FPS is the game's real speed.** *Handkerchief* was designed to run at exactly 25 frames per second; all of its movement and timing depend on that rate.

## 🚀 Quick start

You need **your own copy of the PC game** and a PS Vita with **HENkaku/Ensō** (custom firmware) and VitaShell. Nothing from the game is distributed here: you build the Vita version yourself, on your computer, with one click.

### 1. Get the game

Download it from one of these pages (the builder also has a button for each):

1. **[My Abandonware](https://www.myabandonware.com/game/handkerchief-h0j)**
2. **[archive.org](https://archive.org/details/Handkerchief)**

Both the original release and the one with the community fix are supported.

### 2. Build the Vita version

Download the builder for your computer from the **[latest release](../../releases/latest)**:

| Your computer | Download | Then |
| --- | --- | --- |
| 🪟 **Windows** 10/11 (64-bit) | `HandkerchiefVitaBuilder-WINDOWS.zip` | extract it and double-click `HandkerchiefVitaBuilder.exe` |
| 🐧 **Linux** (64-bit) | `HandkerchiefVitaBuilder-LINUX.tar.gz` | extract it and double-click (or run) `HandkerchiefVitaBuilder` |

A window opens in Portuguese or English with three steps: **choose the game folder**, optionally choose the Portuguese translation, and click **Build my Vita game**.

> [!TIP]
> Windows may say it "protected your PC" because the program is not code-signed: click **More info**, then **Run anyway**.
> The first build takes 5 to 20 minutes and downloads about 250 MB of free tools; later builds take about a minute.

### 3. Install on the Vita

When the builder says **Done**, it opens a folder with two things:

1. **`handkerchief.vpk`**: copy it to the Vita and install it with VitaShell (open the file, press ✕).
2. **`handkerchief/`**: copy the whole folder to `ux0:data/`, so you get `ux0:data/handkerchief/HDKWIN.EXE`.

Open the **Handkerchief** bubble on the LiveArea and play. Updating later? Copy the new folder over the old one: your saves (`USER/`) are kept.

## 🎮 Controls

| Button | Action |
| :-: | --- |
| D-pad / left stick | Move (Down: lie down) |
| ✕ | Jump · confirm in menus |
| □ | Shoot |
| ○ | Switch / rotate, board or leave vehicles · cancel in menus |
| △ or L | Turncoat bomb |
| R (hold) | Rapid fire |
| Select (hold) | Tips & controls card (Left/Right: next tip) |
| Start | Pause menu |

Gameplay buttons can be remapped in **Start → OPTIONS → Controls** (*AJUSTES* in Portuguese): choose an action, press ✕, then the new button.

<details>
<summary><b>More about the pause menu and settings</b></summary>

- **Start** opens the pause menu: save, load, RESULTS, cheats, OPTIONS and the game's own menu (save, load, stop, exit, configuration). The title screen has its own OPTIONS entry with the same settings.
- **RESULTS** (time, shots, bombs, expenditure, kills, destruction, injuries) sits right below OPTIONS and is greyed out outside a stage.
- During **GAME OVER** the port offers to continue from your last save (✕ = Yes, ○ = No).
- The D-pad, left stick and Start are fixed, and ✕/○ always confirm/cancel in menus whatever the map.
- Settings are saved in `ux0:data/handkerchief/hkport.cfg` (language, aspect, widescreen, FPS counter, volume, music, button map as `map_<button>=<action>`). A fresh install starts with the FPS counter off and volume 7.
- The loading and memory improvements (`preopen`, `userfast`, `preload_se`, `savefast`, `heapfix`) are on by default; setting one to `0` in `hkport.cfg` switches it off if it ever causes trouble.

</details>

## 🇧🇷 Language (Brazilian Portuguese)

The game is in English. A Brazilian Portuguese fan translation was made by **Patryck Padilha de Oliveira** ([patryckpo.com](https://patryckpo.com)). It is not part of this repository and is not redistributed here.

- **Easy way:** download it from <https://patryckpo.com/traducoes/windows/handkerchief/downloads/>, extract it, and pick the extracted folder in step 2 of the builder.
- **By hand:** copy `HDKWINBR.EXE` and its `BITMAP/` folder (`HDKCON00.BMP`, `HDKSYS00.BMP`) to `ux0:data/handkerchief/lang/ptbr/`.

Then choose **Start → OPTIONS → Language → Português (BR)** and restart the game. The port's own menus switch to Portuguese too.

## 📊 Status

Tested on a PS Vita 1000 (firmware 3.65, HENkaku) and on Linux.

| Area | State |
| --- | --- |
| Stages | ✅ All 10 territories, bosses, RESULTS, ending and credits, at the native 25 FPS in 4:3 and widescreen |
| Graphics | ✅ 8-bit DirectDraw emulated in software; native widescreen, 4:3 or stretched 16:9 |
| Sound | ✅ Music and effects, separate volume levels |
| Menus and dialogs | ✅ The game's own menus, driven by the pad, with an on-screen keyboard for save names |
| Save / load / continue | ✅ |
| Memory | ✅ Flat over long sessions (tested with 20 stage, save and menu cycles) |
| Network mode | ❌ Not supported (replaced by OPTIONS on the title screen); online play is a goal for a future version |

## 🐞 Reporting bugs

If the game crashes, freezes, sounds wrong, or something does not behave like the original:

1. Open a new issue with the **[Bug report](../../issues/new/choose)** template. English or Portuguese are both fine.
2. Say what happened, where in the game, and how to make it happen again.
3. Attach `ux0:data/handkerchief/hklog.txt` and `hklog.prev.txt` (copy them with VitaShell) and, if you can, a screenshot (PS + Start).

We read every report and fix what we can as soon as possible.

## 🛠️ For developers

<details>
<summary><b>Builder details and command line</b></summary>

The builder checks that your files are complete and that `HDKWIN.EXE` is one of the two supported editions (SHA-256 `3deeb9b18ac2f6ee7b4af4769d1499fdfe0bde8de44482b57ab4bf6196617b76` with the community fix, or the original archive.org release). It downloads a pinned [VitaSDK](https://vitasdk.org) with SDL2, CMake and Ninja, checked by SHA-256, into `%LOCALAPPDATA%\HandkerchiefBuilder` (Windows) or `~/.cache/handkerchief-builder` (Linux). It needs about 3 GB of free disk space. If something fails, the window shows what to do and keeps a log; finished downloads are kept, so pressing the build button again resumes.

Command line, without the window, on Linux and native Windows (Python 3.9+; `uv run build.py ...` also works):

```sh
python3 build.py --game "/path/to/folder/with/HDKWIN.EXE" [--ptbr "/path/to/translation"] [--out DIR] [--cache DIR] [--clean]
```

It writes `out/handkerchief.vpk` and `out/handkerchief/`.

The two downloads are produced by the GitHub Actions workflow [`builder.yml`](.github/workflows/builder.yml) (PyInstaller) when a version tag is pushed. They contain only this repository's code (GPL-3.0), never the game or anything derived from it.

</details>

<details>
<summary><b>Building from source</b></summary>

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

On Linux, `HK_LANG=pt` with `<game>/lang/ptbr/` selects the Portuguese translation.

</details>

<details>
<summary><b>How it works</b></summary>

The original executable is statically recompiled from x86 to C, so the game's own code runs unchanged; the port supplies the Windows environment it expects.

- `recomp/`: the x86 to C static recompiler (derived from [sh3vita](https://github.com/matfsouza/sh3vita)) and the data patches and hooks the port needs.
- `port/rt`: guest memory, CPU state and the native runtime.
- `port/win32`: the Win32 API subset the game uses (files, timers, messages, registry, threads).
- `port/hk`: DirectDraw, GDI text, DirectSound, the dialog and menu system, and the widescreen hooks.
- `port/pc` and `port/vita`: the platform entry points (SDL2).

Technical documentation: [static recompilation](docs/static-recomp.md), [engine map](docs/engine-map.md), [HDK data format](docs/hdk-format.md), [save and stages](docs/save-and-stages.md), [widescreen](docs/widescreen-plan.md).

</details>

## 🤖 How this port was made

This project was built with the help of AI. The recompiler, the runtime and the Windows/DirectX emulation were written with an AI coding assistant ([Claude Code](https://claude.com/claude-code) by Anthropic). The project owner directed the work, decided what to build and tested every build on a real PS Vita. AI-written changes were checked against the original game's behaviour with logs, screenshots and measurements on PC and on the console before being kept.

## 🙏 Credits

- **OPUS Corp. / ASK Co., Ltd.** for *Handkerchief* (1999-2000).
- **Patryck Padilha de Oliveira** for the Brazilian Portuguese fan translation.
- **[sh3vita](https://github.com/matfsouza/sh3vita)**, the recompiler this project's recompiler is derived from.
- **[VitaSDK](https://vitasdk.org)** and the PS Vita homebrew community.

## 📜 License

GPL-3.0 (see [LICENSE](LICENSE)). Third-party components and their licences are listed in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

*Handkerchief* is a trademark of its respective owners. This project is unofficial and non-commercial, is not affiliated with them, and distributes no game data. Screenshots are captured from the port running the original game data; the game's artwork belongs to its rights holders.

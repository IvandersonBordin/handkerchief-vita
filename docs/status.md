# Project status (handoff)

Updated at every milestone so work can resume after a quota stop or a new session.
Last update: 2026-11-20, branch `recomp/static-pipeline`.

## Where the project stopped

Main path: full static recompilation of `HDKWIN.EXE` (see [static-recomp.md](static-recomp.md)).

- `recomp/x86rec.py` translates the whole executable to C with no untranslatable instructions.
- The Linux build (`make -f port/Makefile.pc -j8`, run in `build/hk-game`, a copy of the game data)
  boots the original code: CRT, window, DirectDraw 640x480x8, palette, every BMP bank, the title
  bitmaps and `System/t0.HDK`. The actor update chain studied by Bruno (`0x418E3C`, `0x4196E5`,
  `0x42DBD5`) runs.
- The intro plays correctly on Linux: XSIV Games logo, fade, OPUS Corp. logo, with the right palette
  (screenshots with `HK_SHOT=30,120,300,600`), then the ASK logo, then the full title screen
  (frame 1500): logo, background, three characters, menu NEW GAME / CONTINUE / NETWORK / EXIT.
  The copyright line is GDI text, logged but not drawn yet.
- Next step: input. HDKWIN.EXE imports no DirectInput; keys arrive as window messages
  (PeekMessageA/DispatchMessageA to the window procedure) plus GetKeyState. Map SDL keys (PC) and the
  Vita pad to WM_KEYDOWN/WM_KEYUP, then start a NEW GAME and check stage A0.
- CRT `memcpy`/`memmove` (`0x45E4B0`/`0x45F6C0`) are native; guest memory is 256 MB on PC (64 MB ran out
  when a 13.9 MB block was requested). Two double frees by the game are logged and ignored, as Windows does.
- Not done yet: text rendering (ExtTextOutA is only logged), keyboard/pad input, sound (DirectSound
  reports no driver), dialogs (DialogBoxParamA), Vita build of the recompiled game.

## How to resume

```sh
build/venv/bin/python -I recomp/x86rec.py && build/venv/bin/python -I recomp/gen_import_stubs.py
make -f port/Makefile.pc -j8
cd build/hk-game && HK_HEADLESS=1 HK_SHOT=30,300 timeout -s KILL 15 ../hk-pc/handkerchief . > ../hk-run.log 2>&1; tail ../hk-run.log
```

The last `FATAL:` line of the log names the next missing Win32 import or DirectDraw method.
Without `HK_HEADLESS` a window shows the game's primary surface.

## VPKs available for console tests

None of the recompiled game yet. Earlier prototypes (local, outside Git): `build/vista-walk/vista_walk.vpk`,
`build/recomp-*/` diagnostics (HKRC00001-00006, tested on the Vita on 2026-11-19).
A Vita build of the recompiled game needs SDL2 for VitaSDK (`vdpm sdl2`, requires Ivan's approval)
and a Vita platform layer (`port/vita/`).

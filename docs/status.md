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
- Input works: GetKeyState answers from the keyboard (PC) or pad (Vita) and key changes are posted as
  WM_KEYDOWN/WM_KEYUP (the game peeks with PM_NOREMOVE and takes messages with GetMessageA at 0x40EA33).
  With Enter on the title (HK_KEYS=1300:0d:6) NEW GAME starts: "TERRITORY 'GOLD FISH'" card, then
  stage A0 from System/a0.HDK with HUD, enemies and explosions (screenshots 1350-2200).
  Key roles found by scripted key tests in A0 (2026-11-20, build/keytest): X shoots, A and Up jump,
  S crouches; Z, C, V, D, space, Shift, Ctrl, Alt and the mouse buttons showed no visible effect at that
  point. Menus confirm with Enter/space and go back with Z/X/C/Shift/Ctrl (0x4117D4).
  Vita mapping (port/vita/main.c): Cross = X + space (shoot / confirm), Circle = A (jump), down = arrow + S
  (crouch), Square = Z, Triangle = C, L = Shift, R = Ctrl, Start = Enter, Select = Esc.
- Ivan's console test (2026-11-20): the game plays correctly on the Vita, colours look right, Start enters
  the stage, Select quits; requests were a jump button and shooting on Cross (done in the mapping above).
- Missing: GDI text (speech bubbles are empty), sound, save/continue not checked.
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

- `build/hk-vita/handkerchief.vpk` (local only, derived from HDKWIN.EXE: never publish), title `HKRC00010`,
  SHA-256 `9a28d273850441f91f3ed3870cfbb0cdef42aa1985fb335d56421ea24519aec3`. Built 2026-11-20 with
  `cmake -S port/vita -B build/hk-vita -DCMAKE_BUILD_TYPE=Release && cmake --build build/hk-vita -j8`;
  the skill's check_vpk.py reports 0 errors. **Installed and run on the console on 2026-11-20** (PCH-1010,
  firmware 3.65, via vita-agent-use): it boots and stays up at about 26 FPS (one CPU core at 100%,
  233 MB of 383 MB USER_RW used); Start on the title loads HdkSKY00.BMP (NEW GAME begins). Stage A0
  was not reached within 15 s before the app was closed; vita-agent-use screen capture returns
  frame_unavailable for this app, so the screen was not observed remotely. Expected: intro, title and
  stage A0 with the provisional pad mapping. Needs the player's game files in `ux0:data/handkerchief/` (HDKWIN.EXE,
  BITMAP/, SYSTEM/, USER/, WAV/); log in `ux0:data/handkerchief/hklog.txt`.
- Console tools: vita-agent-use (Ivan authorized the identity in build/vita-agent-identity on 2026-11-20 to
  connect, upload, install HKRC00010 and capture the screen). Game data is in ux0:data/handkerchief/
  (all files verified except XSIVGA~1.AVI: the Vita refuses '~' in the name; no import plays video).
  Upload tip: give `--transfer-state` a fresh file outside the identity folder after an interrupted upload.
- Earlier prototypes: `build/vista-walk/vista_walk.vpk`, `build/recomp-*/` diagnostics (HKRC00001-00006).
- SDL2 2.32.8 for VitaSDK was installed into ~/vitasdk from the official vitasdk-autobuild package
  (SHA-256 checked) because the SDK's vdpm has no pacman client.

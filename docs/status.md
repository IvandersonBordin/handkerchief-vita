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
  The game's Key Configuration dialog (resource 1492) names the actions: Move Up/Down/Left/Right, Shoot & Bomb,
  Turncoat Bomb, Switch & Rotation, Jump, Crouch (option "Move Up to Jump").
  Vita mapping (port/vita/main.c): Cross = X + space (shoot / confirm), Square = A (jump), down = arrow + S
  (crouch), Circle = Z and Triangle = C (probably Turncoat Bomb and Switch & Rotation, not verified),
  L = Shift, R = Ctrl, Select = Esc. Start is the port's own pause (darkened frame with a pause sign; the
  game's clocks skip the paused time through g_pause_qpc). On the title, Cross confirms.
- Game menus are Win32 dialogs and are not implemented yet: Esc opens resource 1498 "Exit Handkerchief ?"
  (CreateDialogParamA at 0x40E9D1) and F2 opens a configuration dialog; both first call
  IDirectDraw::GetGDISurface (slot 14), which stops the run. Dialog IDs used by the game: 0x5D0-0x5DC
  (1490 secret options, 1491 Game Configuration, 1492 Key Configuration, 1496 network lobby).
- Ivan's console test (2026-11-20): the game plays correctly on the Vita, colours look right, Start enters
  the stage, Select quits; requests were a jump button and shooting on Cross, then jump on Square and pause on Start (all done in
  the mapping above). What looked like "Select quits" is the run stopping at GetGDISurface.
- GDI text is drawn (2026-11-20): the game asks for MS Sans Serif with LOGFONT heights 14 and 18; 1-bit fonts
  rendered from Liberation Sans (tools/make_font.py -> port/hk/font_data.h) are used, with TA_CENTER/RIGHT,
  TA_BOTTOM/BASELINE, ETO_OPAQUE/CLIPPED and opaque/transparent backgrounds. Speech bubbles in A0 show their
  text ("Z Z Z z z z . . .").
- Screen aspect: on the Vita pause screen (Start), Triangle toggles 4:3 letterboxed / stretched 16:9, saved in
  ux0:data/handkerchief/hkport.cfg. True widescreen (more of the world visible) would need changes to the
  game's drawing and camera; not attempted.
- Missing: sound, Win32 dialogs (Esc exit prompt, F2 configuration), save/continue not checked.
- Old test apps HKRC00005-00009 were uninstalled by Ivan from the LiveArea (vita-agent-use `content preview`
  fails with firmware error 0xFFFEFFF2 on this console).
- Ivan's feedback (2026-11-20): the game runs smoothly on the Vita; stretched 16:9 makes text easier to read
  but distorts the image, so he asked for a native widescreen mode.

## Planned work (approved order)

1. Sound: works on PC since 2026-11-21. port/hk/dsound.c implements DirectSound (DirectX 5 interfaces) over an
   SDL2 mixer (from sh3vita, soft limiter above 0.75 full scale) and PlaySoundA; SetTimer/WM_TIMER drive the
   game's 40 ms sound timer (proc 0x45CF00). HK_NOSOUND=1 disables it. Root cause of the earlier crash: the
   CRT small-block heap (__sbh_alloc_block 0x4626D0, used by malloc 0x45E2E0 below the threshold [0x4A88EC]
   = 0x1E0) handed out a live sound object a second time (0x45CB29, then CPtrArray::SetSize at 0x459D13),
   with no free in between (gdb + SH3_VTIME=1). Why its bookkeeping goes wrong is still unknown; the fix is a
   load-time patch (hk_patch_data, port/hk/glue.c) that sets the threshold to 0, so every allocation goes to
   HeapAlloc and the port's heap. Verified: 3 min of A0 with walking, shooting and jumping, sound continuous,
   no crash, no clipped samples (SDL disk audio driver). Console test pending.
2. Win32 dialogs used by the game: 1498 "Exit Handkerchief ?" (Esc), 1491 Game Configuration and 1492 Key
   Configuration (F2); both start with IDirectDraw::GetGDISurface. The 4:3/16:9 option can move there.
3. Native widescreen (requested by Ivan): a wider view of the world instead of stretching. Start from the
   camera and horizontal parallax mapped in docs/engine-map.md ("Câmera e parallax horizontal"); find where
   the 640-pixel view width is assumed (surfaces, camera clamp, actor culling, HUD) before changing it.
   Good candidate for a Bruno task (reverse engineering, opus).
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

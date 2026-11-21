# Project status (handoff)

Updated at every milestone so work can resume after a quota stop or a new session.
Last update: 2026-11-21, branch `recomp/static-pipeline`.

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
  Turncoat Bomb, Switch & Rotation, Jump, Crouch (option "Move Up to Jump"). Its defaults (seen in the dialog,
  2026-11-21): arrows move, Z = Switch & Rotation, X = Shoot & Bomb, C = Turncoat Bomb, A = Jump, S = Crouch;
  joystick buttons 1-5.
  Vita mapping (port/vita/main.c): Cross = X + space (shoot / confirm), Square = A (jump), down = arrow + S
  (crouch), Circle = Z (Switch & Rotation), Triangle = C (Turncoat Bomb), L = Shift, R = Ctrl, Select = Esc. Start is the port's own pause (darkened frame with a pause sign; the
  game's clocks skip the paused time through g_pause_qpc). On the title, Cross confirms.
- Win32 dialogs and the menu bar work (port/hk/dialog.c, 2026-11-21): the port draws the game's DIALOG/DIALOGEX
  templates over the frame and runs the game's own dialog procedures. The game's options live in its menu bar
  (MENU 1505 title, 1504 stage, 1502 with Network), which in full screen the game shows on a right click
  (window procedure 0x40ED30 -> SetMenu). F10 on PC, or Select on the Vita pause screen, sends that right click;
  the port draws the bar, the pad picks an item and the port posts WM_COMMAND: File (Save This Game = F2 ->
  slot dialog 1500 + name 1499; Stop This Game = F4; Continue -> 1500 load; Exit = Esc -> 1498), Option (Key
  Configuration 1492, Game Configuration 1491; Toggle Display Mode and Network are greyed: windowed mode lost the
  palette, no DirectPlay) and About (1516). Keys in dialogs: arrows move the focus spatially, space/X/Cross press
  the focused control (a combo box opens its list), Enter/C/Triangle press OK, Esc/Z/Circle/Select cancel.
  Verified on PC with scripted keys and screenshots: Music On off stops the title music (#433), Sound Effects
  On off leaves 0-1 voices while shooting (12-20 with it on), Reset restores the defaults, a key remap shows
  up again after a restart, Continue loads USER01, Save writes User02.HDK. F2 with no soldiers shows dialog 1493
  "Can't save because you have no men.".
  Settings: HDKWIN.EXE keeps them in a CConfig object built with defaults at every start (0x414CF6) and never
  stores them; the port saves CConfig +0x04..+0x9B to hkconfig.txt in the game folder after OK in 1491/1492 and
  loads it through a recompiler hook at 0x414D0B (recomp/image_patches.py HOOKS: regenerate build/hk-recomp/gen).
  Text in EDIT controls (save name, dialog 1499): on PC typed while the control has the focus (SDL text input,
  Backspace erases, EM_LIMITTEXT respected; HK_TEXT=frame:text types in headless tests, '~' = Backspace); verified:
  "Ivan's save 1" typed, saved and found in User02.HDK. On the Vita, Cross on the control opens the system
  keyboard (sceImeDialog, drawn by SDL's GXM renderer through sceCommonDialogUpdate) with the current text;
  compiles and links, **not tested on the console**. Not done: the 1490 secret options (editor menu only). Remapping a key to one the pad cannot send makes that action unusable on the Vita until
  Reset.
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
- The black diagonal strip in the A0 mound under the player's house is original: Ivan's recording of the game
  (build/gameplay-reference/overview.jpg, first row) shows it too. HK_TRACE_PIX=300,305,1530 shows the game
  drawing it with an opaque BltFast (call at 0x408D59, element bit 15 clear) of black 40x20 tiles from
  HdkMAPb0.BMP (surface #9); the surface has its colour key, the game just does not ask for it there.
- Save/continue and other stages (2026-11-21, Bruno, details in [save-and-stages.md](save-and-stages.md)): CONTINUE
  (dialog 1500 GAME DATA, a DIALOGEX template) loads a slot, F2 in a stage saves into a slot, and the round trip
  was verified headless. Fixes: DIALOGEX parsing, PostMessageA to dialogs delivered, no port navigation on 1500,
  all letters/F1/F4 posted (H D K in a stage opens Secret Option 1490). Test aid `HK_STAGE=<label>` starts NEW GAME
  in any stage file (hook in recomp/image_patches.py: regenerate the C); 32 stages of territories A-J load with no
  FATAL, `e2` returns to the intro. On the Vita, Save/Stop are reached through the port's menu bar.
  12 WAV files are missing from the player's data and the game retries opening them on every play.
- Old test apps HKRC00005-00009 were uninstalled by Ivan from the LiveArea (vita-agent-use `content preview`
  fails with firmware error 0xFFFEFFF2 on this console).
- Ivan's feedback (2026-11-20): the game runs smoothly on the Vita; stretched 16:9 makes text easier to read
  but distorts the image, so he asked for a native widescreen mode.

## Planned work (approved order)

- Ivan's console test (2026-11-21, b702239): menu bar, save and continue work on the Vita. Requests: Select opens
  the menu directly (done), sound distorted like a speaker at maximum (master gain 0.7 + block limiter replace the
  per-sample soft clipper; PC: house explosion peaks 0.9 full scale, no clipped samples), and later a Vita-style
  friendly menu instead of the Windows menu bar and dialogs (future work).


1. Sound: works on PC since 2026-11-21. port/hk/dsound.c implements DirectSound (DirectX 5 interfaces) over an
   SDL2 mixer (from sh3vita, soft limiter above 0.75 full scale) and PlaySoundA; SetTimer/WM_TIMER drive the
   game's 40 ms sound timer (proc 0x45CF00). HK_NOSOUND=1 disables it. Root cause of the earlier crash: the
   CRT small-block heap (__sbh_alloc_block 0x4626D0, used by malloc 0x45E2E0 below the threshold [0x4A88EC]
   = 0x1E0) handed out a live sound object a second time (0x45CB29, then CPtrArray::SetSize at 0x459D13),
   with no free in between (gdb + SH3_VTIME=1). Why its bookkeeping goes wrong is still unknown; the fix is a
   load-time patch (hk_patch_data, port/hk/glue.c) that sets the threshold to 0, so every allocation goes to
   HeapAlloc and the port's heap. Verified: 3 min of A0 with walking, shooting and jumping, sound continuous,
   no crash, no clipped samples (SDL disk audio driver). Installed on the console on 2026-11-21 (the first
   attempt shipped without dsound.c: CMake's file(GLOB) had not re-run; now CONFIGURE_DEPENDS); it runs with
   sound. Ivan's listening test (2026-11-21): music and effects play everywhere, but sound drops briefly when
   the fish robot shoots the crate and crackles when crouching, picking up glucose and shooting at once;
   the game slowed in tunnels (also before sound).
   Measured on PC: up to 20 voices at once; the game flips 25 times/s and calls Palette::SetEntries every
   frame, and each SetEntries also presented the frame (50 presents/s, each waiting for vblank). Fixes:
   palette changes are shown by the next Flip (or after 60 ms without one), the mixer uses fixed-point
   positions and direct sample access, and the SDL buffer is 2048 frames. The Vita now shows 25 presents/s
   (one per game frame; the "50 FPS" measured before was the doubled presents).
   Ivan's second listening test (2026-11-21, commit 01b62f5): sound still drops out, worse than before, in the
   long tunnel, when the bazooka shot destroys the player's house, and when crouching; FPS counter shows a
   steady 25. Findings: on PC (SDL disk driver, HK_TRACE_DS, scratchpad sil.py) the house is destroyed around
   frame 1900 with no silent stretch, but the mix saturates the soft limiter (peaks at full scale for ~5 s)
   and the background loop is quiet afterwards (peak ~2000). On the Vita, main thread is now on core 0 and the
   audio thread on core 1 (confirmed in hklog), and the late-callback check logged **no** late callback, so
   the drops are not CPU starvation. Vita logs also show ~10 "[heap] free of non-allocated" per session.
   Next: log on the console what happens at each audible->silent transition (playing voices, Stop/Release
   callers, sample memory still non-zero?) to tell game logic, limiter, and overwritten sample memory apart.
   FPS above 30 is not possible without changing game speed: the game logic runs at 25 frames/s.
2. Win32 dialogs used by the game: done on PC (see above); needs a console test of the menu (Start, then
   Select) and of the configuration dialogs with the pad.
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

- 2026-11-21 (dialogs, menu and system keyboard, not tested on the console): `build/hk-vita/handkerchief.vpk`,
  SHA-256 `3b942f51c80b853f909d9949c00101470b5259aa8b250d92b6c9ba468ac4a97a`, check_vpk.py 0 errors. To test:
  Start, then Select opens the game's menu; Option > Game / Key Configuration; Cross selects, Triangle = OK,
  Circle cancels; File > Save This Game, then Cross on the name field should open the system keyboard.

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

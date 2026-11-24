# Save/continue and stage loading (recompiled HDKWIN.EXE)

Findings of 2026-11-21. Addresses are HDKWIN.EXE virtual addresses. "Verified" means observed in a
headless run of the Linux build (`SH3_VTIME=1`, scripted keys, screenshots); everything else is marked as
static reading or hypothesis.

## Save files

- `User/User00.HDK` is the working copy of the current game; `User/User01.HDK` .. `User10.HDK` are the ten
  slots shown in the GAME DATA dialog. The title screen deletes `User00.HDK` (DeleteFileA fails with 0 when it is
  absent, as on Windows).
- Record format (read from the player's `USER01.HDK`, 5651 bytes): a sequence of
  `[u32 size of the whole record][name, NUL-terminated][MFC-serialized body]`. Per visited area:
  `<label>_FLUG_STAGE_0`, `<label>_MARK_ENEMY_0`, `<label>_GIMK_STAGE_0`, `<label>_objects_0`; global:
  `_CGG_0` (the current area label, e.g. `a0_b`, at body offset 17), `_MAIN_0` (includes the key bindings
  0x26 0x28 0x25 0x27 Z X C A S), `_FLUG_0`, `_objects_0`, and `info` (`Data N [m/d/yyyy day h:mm:ss]`, format
  string at 0x47DDBC). Record names come from the table at 0x47BC40.
- Writing a record (class with constructor 0x40A084, destructor 0x40A1C9): the record goes to `NAME.ser` through
  CRT `fopen`; on close the game copies `NAME` to `NAME.bak` (CopyFileA), merges `NAME.ser` into `NAME` with
  0x4081EA (records with the same name are replaced through a rewrite into `NAME.mck`, then DeleteFileA(NAME) and
  MoveFileA(NAME.mck, NAME); new records are appended with `rb+`), restores `NAME.bak` if the merge failed, and
  deletes `NAME.bak` and `NAME.ser`.

## When the game saves and loads (verified)

- **CONTINUE** on the title opens dialog 1500 (DIALOGEX "GAME DATA", modeless, proc 0x40423C -> 0x432587) in LOAD
  mode. Load copies the slot to `User00.HDK` (`CopyFileA(User0N, User00)`) and starts the area named in `_CGG_0`.
  The player's 2016 save loads into `a0_b` with 670 money.
- **In a stage, F2** opens the same dialog in SAVE mode (key handler 0x451EA3: GetKeyState(0x71) when bit 0 of
  `[[scene+0x28]+0x58]` is set; menu command 40063 does the same). The function 0x452200 refuses with string 0x88
  "Can't save because you have no men." when the party is empty; the dialog warns "Your soldiers will disappear if
  you save now." Saving merges the current state into `User00.HDK` (several .ser/.mck rounds), then copies it to
  the chosen slot. Verified: slot 2 written, then CONTINUE lists "Data 2 [11/21/2026 sat 9:32:33]" and loads it.
- **F4** in a stage asks "Stop this game ?" (string 0x76, dialog 1497); menu command 40061 does the same.
- The GAME DATA dialog is driven by the game itself: its loop at 0x4328CD polls the input object (0x4117D1:
  arrows, Enter/space, Shift/Ctrl/Z/X/C, Esc), moves the slot radio buttons (1420-1429) with 0x401726, and posts
  WM_COMMAND 0x9CA4 (confirm) / 0x9CA5 (cancel) to the dialog at 0x4329F8 / 0x432A15. Buttons 40072 Rename and
  40024 Delete exist in the template; the key loop does not reach them (mouse only on Windows).
  The port clicks them with R / D (Vita: Square / L). Delete asks "Delete this game ?" with MB_OKCANCEL (0x40F2A0,
  answer == IDOK); the Vita answers MessageBoxA with its own box (Cross = OK, Circle = Cancel). In this loop
  Z/X/C/Shift/Ctrl are confirm and only Esc cancels, so on the Vita Circle sends Esc (not Z) while a dialog is up.
- Console bug (2026-11-21, wide_native=1 only): the 1500 loop runs game frames through 0x40E9F9, whose message loop
  (0x40EA18-0x40EA6D) runs a frame only when PeekMessageA finds the queue empty. The port's dialog refresh presented
  from inside PeekMessageA every 33 ms counted from the start of the previous present; at 848x480 a Vita present
  takes longer than the 40 ms window timer, so every PeekMessageA presented again and found a new WM_TIMER: no frame,
  no key poll, the pad did nothing. The refresh now counts from the end of the last present of any kind. PC
  reproduction: `SDL_AUDIODRIVER=dummy HK_PRESENT_MS=50 HK_WIDE=848` with sound on (the timer exists only once
  DirectSound works) and `HK_KEYS=1300:28:3,1320:20:6,1400:28:3,1440:20:4` (no load before the fix, slot 2 after).

## Other dialogs

- 1497: generic OK/Cancel confirmation; 0x4104FB loads a string resource, formats it and shows it (callers:
  "Stop this game ?" 0x76 at 0x451F36, "Delete this game ?" 0x78 from the Delete button of 1500).
- 1499: generic text input (0x410654: edit 1328 gets the current text, group box 1245 the caption); only caller
  0x432428, the Rename button of 1500 (renames a save slot). Not a name prompt for a new game.
- 1490: Secret Option, opened by typing H, D, K in a stage (state machine at 0x451F5E on the last WM_KEYDOWN
  key, then 0x4155F0). Unrivaled Mode makes 'u' toggle invincibility and 'm' add money (verified: HUD shows
  "INVINCIBLE [ON]", money 1000 -> 3100 after m m).
- 1484 "Stage select." and the editor dialogs 1443-1480: no code references them (no `push 0x5CC`); leftovers of
  the editor build.
- DIALOGEX templates: 1486, 1500, 1514, 1516 (`tools/dump_dialogs.py` now dumps them).

## Port fixes 

1. `port/hk/dialog.c` parses DLGTEMPLATEEX (CONTINUE failed with "extended DIALOGEX template not supported").
2. `PostMessageA` to a dialog is queued and `DispatchMessageA` hands it to the dialog procedure (before, every
   PostMessageA except WM_CLOSE/WM_QUIT was dropped, so the 1500 confirm/cancel never arrived). The only other
   PostMessageA in the game (0x40ECE8) is unchanged.
3. `dialog.c` adds no key navigation of its own to dialog 1500 (the port focus moved to Cancel while the game moved
   the slot, so Enter cancelled).
4. All letters, F1 and F4 are posted as WM_KEYDOWN/WM_KEYUP (typed codes); the PC build maps F1 and F4.
5. CopyFileA, MoveFileA and DeleteFileA log guest and host paths.

File semantics checked against Windows: CopyFileA/MoveFileA/DeleteFileA return values and case-insensitive names
(the slot written as `User02.HDK` replaces `USER02.HDK`, because the game deletes it first). The port's safe
write (`safe_write.h`: CREATE_ALWAYS goes to `NAME.tmp`, swapped in on CloseHandle through `NAME.bak`) uses the
same `.bak` suffix as the game. In the flows above they do not meet: the game only creates `NAME` with
CREATE_ALWAYS (`wb+`) when `NAME` is missing, so no game `NAME.bak` exists then, and nothing opens `NAME` between
the game's DeleteFileA and MoveFileA. Open risk, not observed.

On the Vita, Save and Stop are reached through the port's menu bar (81433f5); the Secret Option needs
letters, which the pad does not send.

## Stages

- The game scene (0x454341) passes the label table at 0x4899A0 (84 entries: `a0`, `a0_a` .. `a0_e`, `a1`,
  `a1_a`, `a2`, `b0` .. `j0_a`, NULL) to 0x4255BD with flags 1, which builds the first stage from entry 0; the
  title uses the table at 0x489AD8 (`t0`). Stage files are `System/<label>.HDK` (format string 0x46E258).
  Territories A-J; `x0` is the territory's main stage with sub-areas `x0_a`.., `x1` the sky stage, `x2` the boss.
- Test aid `HK_STAGE=<label>` (port only, `port/hk/glue.c`): points entry 0 at the requested label, so NEW GAME
  loads that file through the game's own loader. A hook before 0x4542C0 (`recomp/image_patches.py`, needs
  `recomp/x86rec.py` to be re-run) marks territories B-J as entered: the scene constructor marks only A
  (0x4542BC), and starting directly in another territory ran its first-entry event (0x45525D -> 0x455414) before
  the player exists, crashing on a null pointer at 0x45717F ("indirect call to 0x00000000"). That crash is an
  artifact of the shortcut, not of the port: normal play enters B-J with a player.
- Results (NEW GAME with `HK_STAGE`, ~30 s idle, screenshots at frames 1400/1700/2200, 2026-11-21): all of `a0`,
  `a0_a`, `a1`, `a2`, `b0`, `b0_a`, `b1`, `b2`, `c0`, `c0_a`, `c1`, `c2`, `d0`, `d0_a`, `d1`, `d2`, `e0`, `e1`, `f0`,
  `f1`, `f2`, `g0`, `g1`, `g2`, `h0`, `h1`, `h2`, `i0`, `i1`, `i2`, `j0`, `j0_a` load and draw their own
  graphics (territory emblem in the HUD, rain in `b0`, night sky with red moon in `j0`) with no FATAL and no new
  missing import. Only idle loading was tested, not play through the stages.
- `e2` "returning to the intro" is the game's own GAME OVER (verified 2026-11-21): the idle player is shot by the
  enemy on the billboard tower, frames 1800-2000 show the grey "GAME OVER" screen, then the game restarts the
  startup sequence 0x4540C2 (XSIV logo, `HdkOPUS.BMP` with sound 0x5E `X_NEXT`, `HdkASK.BMP`, title). `X_NEXT`
  belongs to that OPUS logo, not to `e2`. With Unrivaled Mode ticked the same run stays in `e2` past frame 3200.
  Neither the `HK_STAGE` shortcut nor the port is the cause; normal play reaches `e2` from `e1`/`e1_a`.
- Invincibility: Unrivaled Mode (checkbox 1417 -> 0x4AC84C, read at 0x4154FC) enables it at once, because the
  invincibility flag 0x47BCD8 is initialised to 1 in `.data`; 'u' (0x455126) XORs 0x47BCD8, so the first 'u'
  turns invincibility OFF (HUD "INVINCIBLE [OFF]"). The checks at 0x435A1F/0x435AA4 need both flags.
- Area exits (static reading of the `EXIT_0` block of each stage file, `CExit` records): each record holds a target
  label pattern and an entry point name, e.g. `a0_b`: `??_-`>`r0` at x=16 and `??_+`>`l0` at x=1424 (left and right
  edges). In the pattern `?` keeps the current character and `+`/`-` step it, so the chain is linear: `a0` ->
  `a0_a`..`a0_e` -> `a1` (sky) -> `a1_a`, whose right edge has two exits (upper y=280 -> `b0` `l0`, lower y=880 ->
  `a2`); `a2` (boss) has `CExitFlg` records (flag-gated, `def`/`exit0`) and two `b0`>`0` exits at y=864. `b0`'s left
  edge leads back to `a1_a` `r0`. Each territory repeats this up to `j0`; `e2` exits to `f0` (`+0`>`0`).
- Transitions verified in the port (2026-11-21, headless, scripted keys, Unrivaled Mode): CONTINUE slot 1 (`a0_b`)
  + hold Right -> `a0_c` loads (working state merged into `User00.HDK`); `HK_STAGE=a1_a` + Right -> `a2` (boss
  "KeyMan" in the HUD); `HK_STAGE=b0` + Left -> `a1_a` (territory emblem changes from B to A). No FATAL, hang or
  missing import. Not reached: the boss victory in `a2` and the forward `a2`/`a1_a` -> `b0` exit (a 12000-frame
  scripted fight with fire and jumps earned money but did not win). Holding Right stops at the bunker in `a0_c`.
- Missing sounds in the player's data: `A_BOMB_1`, `A_MGUN_0`, `A_MGUN_1`, `B_LSER_0`, `C_BUND_0`, `F_TANK`,
  `I_NOT`, `I_USE`, `J_SCRM_0`, `J_SCRM_1`, `L_INVI`, `Y_LAST` (.wav, neither in `WAV/` nor `WAV/8bit/`). The
  game retries the open every time it plays one (the log shows about as many failed `malloc(0xFFFFFFFF)`): ~50 failed opens/s in A0
  and ~1000/s in `a1` (enemy machine guns). Windows would fail the same way; on the Vita the repeated failing
  file opens may cost time. Hypothesis to check: related to sound drops heard on the console when enemies fire.
- Observation, not investigated: a black band in the ground under the player's house in A0 (frames 1800-2100).

## Results counters

The RESULTS screen (TIME, SHOT, BOMB, TURNCOAT BOMB, EXPENDITURE, KILL(S), DESTRUCTION, INJURIES) reads 13 dwords
of the game object (vtable 0x472240, 0x768 bytes). Static evidence (HDKWIN.EXE):

- Object: created by the stage scene ctor (0x454316 `push 0x768`, ctor 0x44C721 via 0x40283D at 0x454333, stored
  at [scene+0x60] at 0x45434B) and by 0x44CD80 (same ctor at 0x44CDAA). 0x455286 reads the territory flags
  [obj+0x45C+t*4] through [scene+0x60]. Stage scene vtable 0x4724B8.
- Counters `c[0..12]` at obj+0x4AC: zeroed only by the ctor (0x44C95D -> 0x4015D7 -> 0x44D38B, memset 0x34),
  saved / loaded with the game (0x44CE3F / 0x44CE66, 0x34 bytes), increased by the virtual AddStat(idx, n) at
  vtable +0x28 (0x40433B -> 0x44D36C: `add [ecx+eax*4+0x4AC],edx`), usually through the wrapper 0x426B79 (thunk
  0x40333C: [this+0x50]->AddStat). Calls with a constant index (call address): 2 shots fired (0x41DC90, 0x41DFB2 n = edi, 0x41E0FA),
  4 bombs thrown (0x41E47E), 7 and 5 (0x4286C6, 0x428746, 0x4289E0; hits of turncoat bomb / bomb by the
  display order), 9 EXPENDITURE (0x42EF87, after the money [this+0x118] -= edx), 8 (0x42F0C2, item value; not
  shown), 11 DESTRUCTION (0x428955, 0x4336CC, 0x434501, n = 1), 12 INJURIES (0x4522DC, n = 1), 1 (0x452319, after
  the injury loop; not shown). c[3], c[6] and c[10] were not found at a call with a constant index (0x41999F passes the index in eax); they
  are identified by the draw code and the runs below.
- RESULTS screen: ctor 0x4570B6 (`[this+0x8C] = [arg1+0x50] + 0x4AC`, 0x45711C..0x457126), draw 0x45735B. TIME =
  c[0] / 25 split as h:m:s with format 0x48A2B0 `TIME    %.2d:%.2d:%.2d` (0x45749E..0x4574D4); the values use format
  0x48A280 `%d/%d\n\n%d/%d\n\n%d/%d\n\n%c%d\n\n%d\n\n%d\n\n%d\n\n` with arguments pushed at 0x457609..0x457639: SHOT
  c[3]/c[2], BOMB c[5]/c[4], TURNCOAT BOMB c[7]/c[6], EXPENDITURE `$`c[9] (`\` yen when [0x4AA3A0] != 0),
  KILL(S) c[10], DESTRUCTION c[11], INJURIES c[12]. The labels are string resource 7 (LoadString at 0x4575DF):
  `SHOT\n\nBOMB\n\nTURNCOAT BOMB\n\nEXPENDITURE\n\nKILL(S)\n\nDESTRUCTION\n\nINJURIES`; the PT-BR HDKWINBR.EXE has
  `TIROS / BOMBAS / TURNCOAT BOMBS / GASTOS / MORTE(S) / DESTRUIÇÃO / PERDAS` there and keeps `TIME`.
- The animal name on that screen ("GOLD FISH") is the territory's emblem sprite (table 0x489C20 indexed by
  [this+0x88] = territory), not a rank.

Runtime (PC, headless, `HK_RESULTS_LOG=1` logs the counters when one other than the time changes, 2026-11-22):
- `HK_STAGE=a0`, Enter, X pressed every 8 frames: c[0] counts game frames (2202 at frame 3592, 25 per second),
  c[2] goes 1, 2, ... 213 with each shot; everything else 0 (no target in reach).
- `HK_STAGE=e2`: the enemy on the tower hits the idle player: c[12] 0 -> 1 at frame 1733 (GAME OVER follows, the
  next game object starts from 0). In a later run c[3] = 2 of c[2] = 12 shots.
- `HK_STAGE=a1_a`, Right + X: c[3] = 6 hits of 58 shots, c[10] = 1 kill at frame 1908; after the area change to `a2`
  (KeyMan) the same object (0x014FE52C) keeps counting (TIME 00:00:56 at frame 2880).
- So the counters are valid mid-stage and cumulative for the current game (new game / continue), not per territory:
  nothing resets them when an area or territory changes. Not observed at run time: c[4..7] (bombs), c[9]
  (expenditure), c[11] (destruction); their meaning rests on the call sites above and the draw code.

Port use: `port/hk/results.c` (scene pointer recorded in hk_ws_bar_x, checks both vtables) feeds the RESULTS panel
shown while Select is held in a stage on the Vita (`pm_results`, `port/vita/pause_menu.h`); PC: Tab, or
`HK_RESULTS_AT=frame[:frames]` for screenshots.

## How to reproduce

```sh
# private generated C with the hook (the shared build/hk-recomp/gen has no hook until it is regenerated)
HK_GEN=build/hk-recomp/gen-x build/venv/bin/python -I recomp/x86rec.py
build/venv/bin/python -I recomp/gen_import_stubs.py build/hk-recomp/gen-x
make -f port/Makefile.pc -j8 GEN=build/hk-recomp/gen-x OUT=build/hk-pc-x
cd <copy of the game data>
HK_HEADLESS=1 SH3_VTIME=1 HK_STAGE=c0 HK_KEYS=1300:0d:6 HK_SHOT=1400,2200 timeout -s KILL 100 <OUT>/handkerchief .
# CONTINUE + load slot 1:  HK_KEYS=1300:28:3,1320:0d:6,1380:0d:4
# save into slot 2 after loading: add ,1700:71:4,1780:28:3,1820:0d:4
# Secret Option in a stage: HK_KEYS=1300:0d:6,1700:48:3,1710:44:3,1720:4b:3
```

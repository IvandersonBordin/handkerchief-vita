"""Port-specific changes to HDKWIN.EXE behaviour, applied at recompile time.

Same interface as sh3vita's recomp/image_patches.py (GPL-3.0), which x86rec.py expects:
- IMAGE_PATCHES: bytes replaced in the image before translation (old bytes are checked first).
- ADDR_OVERRIDES: {va: native} the whole function is replaced by a native C function.
- ADDR_WRAPS: {va: native} the recompiled body becomes F_xxx_orig; F_xxx calls the native.
- HOOKS: {va: native} the native runs just before the instruction at va.

Handkerchief starts with no patches: every entry must be justified by a measured need.
The D3DX constants only exist for Silent Hill 3; the values below never match any HDKWIN.EXE code,
so the D3DX fast path in x86rec.py stays disabled.
"""
from dataclasses import dataclass

SPILL = "c->eax=eax;c->ecx=ecx;c->edx=edx;c->ebx=ebx;c->esp=esp;c->ebp=ebp;c->esi=esi;c->edi=edi;"
RELOAD = "eax=c->eax;ecx=c->ecx;edx=c->edx;ebx=c->ebx;esp=c->esp;ebp=c->ebp;esi=c->esi;edi=c->edi;"


@dataclass(frozen=True)
class Patch:
    va: int
    old: bytes
    new: bytes
    why: str


def apply_patches(pe, patches):
    """Replaces the bytes of each patch in pe.data (bytearray); all old bytes are checked before anything changes."""
    plan = []
    for p in patches:
        if len(p.old) != len(p.new):
            raise ValueError(f"patch {p.va:#x}: size change ({p.why})")
        o = pe.off(p.va)
        if o is None:
            raise ValueError(f"patch {p.va:#x}: address not in the image ({p.why})")
        if bytes(pe.data[o:o + len(p.old)]) != p.old:
            raise ValueError(f"patch {p.va:#x}: unexpected bytes (unsupported HDKWIN.EXE?) ({p.why})")
        plan.append((o, p.new))
    for o, new in plan:
        pe.data[o:o + len(new)] = new


def hook_code(native):
    """C code that runs native(c) with the recompiled function's registers, then takes back what it changed."""
    return f"{{ void {native}(CPU *c); {SPILL} {native}(c); {RELOAD} }}"


IMAGE_PATCHES = []
# MSVC CRT memcpy (0x45E4B0) and memmove (0x45F6C0): same prologue (overlap test at +0x14), then copies through
# computed jump tables the recompiler cannot follow (first run stopped on an indirect jump to 0x45F97C).
# Ghidra leaves them unnamed (FUN_...), so they are matched by address. Native versions: port/rt/natives.c.
ADDR_OVERRIDES = {0x45E4B0: "nat_memcpy", 0x45F6C0: "nat_memmove"}
ADDR_WRAPS = {}
# 0x414D0B: in the CConfig constructor, right after the defaults (Reset 0x414D1E, key table 0x414D49); esi = this.
# The port loads the settings saved by the Game / Key Configuration dialogs (hkconfig.txt, port/hk/dialog.c):
# the game itself never stores them.
# Test aid HK_STAGE (port/hk/glue.c): the game scene constructor marks only territory A's first-entry event as done
# (0x4542BC: mov byte [esi+0x70],1; bytes [esi+0x71..0x7A] were zeroed at 0x454296). Starting NEW GAME in another
# territory then runs its first-entry event (0x45525D -> 0x455414) before the player exists ([scene+0x4C] = 0) and
# calls through a null vtable at 0x45717F. The hook runs before 0x4542C0 and, only when HK_STAGE is set, marks
# territories B-J as entered too. Without HK_STAGE it does nothing.
HOOKS = {0x414D0B: "hk_config_loaded", 0x4542C0: "hk_stage_scene_hook"}
ABI_NATIVES = set()

FTOL2 = None   # MSVC __ftol is found by its Ghidra name in x86rec.py when present

# Silent Hill 3 only (D3DX dispatch thunks): disabled for Handkerchief.
D3DX_CPU_CHECK = 0xFFFFFFFF
D3DX_STATE = 0
D3DX_TABLE = 0
D3DX_TABLE_DEFAULT = 0
D3DX_TABLE_SLOTS = 0
D3DX_X87 = {}

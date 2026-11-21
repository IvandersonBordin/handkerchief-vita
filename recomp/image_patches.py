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
HOOKS = {}
ABI_NATIVES = set()

FTOL2 = None   # MSVC __ftol is found by its Ghidra name in x86rec.py when present

# Silent Hill 3 only (D3DX dispatch thunks): disabled for Handkerchief.
D3DX_CPU_CHECK = 0xFFFFFFFF
D3DX_STATE = 0
D3DX_TABLE = 0
D3DX_TABLE_DEFAULT = 0
D3DX_TABLE_SLOTS = 0
D3DX_X87 = {}

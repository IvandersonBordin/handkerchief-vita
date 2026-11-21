"""Read-only dump of the DIALOG resources of HDKWIN.EXE (structure only: ids, styles, rectangles, captions).
Usage: build/venv/bin/python -I tools/dump_dialogs.py original-data/HDKWIN.EXE"""
import struct, sys
import pefile

CLS = {0x80: "BUTTON", 0x81: "EDIT", 0x82: "STATIC", 0x83: "LISTBOX", 0x84: "SCROLLBAR", 0x85: "COMBOBOX"}

def sz_or_ord(d, o):
    if struct.unpack_from("<H", d, o)[0] == 0xFFFF:
        return struct.unpack_from("<H", d, o + 2)[0], o + 4
    e = o
    while struct.unpack_from("<H", d, e)[0]: e += 2
    return d[o:e].decode("utf-16-le"), e + 2

def dump(d):
    style, exst, n, x, y, cx, cy = struct.unpack_from("<IIHhhhh", d, 0)
    o = 18
    menu, o = sz_or_ord(d, o); cls, o = sz_or_ord(d, o); title, o = sz_or_ord(d, o)
    font = ""
    if style & 0x40:
        pt = struct.unpack_from("<H", d, o)[0]; font, o = sz_or_ord(d, o + 2); font = f"{font} {pt}pt"
    print(f"  style {style:08x} rect {x},{y} {cx}x{cy} title {title!r} font {font}")
    for i in range(n):
        o = (o + 3) & ~3
        st, ex, x, y, cx, cy, cid = struct.unpack_from("<IIhhhhH", d, o)
        o += 18
        c, o = sz_or_ord(d, o); t, o = sz_or_ord(d, o)
        extra = struct.unpack_from("<H", d, o)[0]; o += 2 + extra
        c = CLS.get(c, c) if isinstance(c, int) else c
        print(f"    id {cid:5d} {c:9s} style {st:08x} {x:4d},{y:4d} {cx:4d}x{cy:3d} {t!r}")

pe = pefile.PE(sys.argv[1])
for t in pe.DIRECTORY_ENTRY_RESOURCE.entries:
    if t.id != 5: continue
    for e in t.directory.entries:
        for l in e.directory.entries:
            r = l.data.struct
            print(f"DIALOG {e.id} lang {l.id}")
            dump(pe.get_data(r.OffsetToData, r.Size))

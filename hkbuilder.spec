# PyInstaller spec for the one-file builder (same file for Windows and Linux; build it on the target OS).
# Contains only this repository's own code: the recompiler sources, the C runtime sources the VPK build needs, the
# function-address list, and the GUI. The game and the toolchains are never included (downloaded at run time).
import sys
name = "HandkerchiefVitaBuilder"
a = Analysis(["HandkerchiefVitaBuilder.py"],
             pathex=["."],
             datas=[("recomp", "recomp"), ("port", "port")],
             hiddenimports=["capstone", "json", "gzip", "struct", "collections", "dataclasses", "re", "pathlib", "hashlib", "zlib", "bz2", "lzma", "build", "hkbuild.core", "hkbuild.gui", "hkbuild.strings"],
             excludes=["numpy", "PIL", "matplotlib"])
pyz = PYZ(a.pure)
exe = EXE(pyz, a.scripts, a.binaries, a.datas, [], name=name, console=False, upx=False)

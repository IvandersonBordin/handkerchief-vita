#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.9"
# dependencies = ["capstone>=5"]
# ///
"""Build your own Handkerchief PS Vita VPK from your own copy of the game (command line).

    python3 build.py --game "/path/to/folder/with/HDKWIN.EXE" [--ptbr "/path/to/translation"]

Most players should use the double-click program instead (HandkerchiefVitaBuilder, see README.md). This is the same
pipeline for advanced users. Nothing from the game is distributed: the executable is recompiled to C on your machine,
compiled with a pinned VitaSDK downloaded into a cache folder, and the result is written to ./out (handkerchief.vpk
plus the folder 'handkerchief' to copy to ux0:data/ on the Vita). Re-running is cheap: finished steps are skipped.
Works on Linux x86-64 and native Windows (64-bit); needs about 3 GB of free disk space and internet on the first run.
Without uv, plain python3 works too: capstone is then installed into a private venv in the cache folder.
"""
import argparse, os, re, pathlib, subprocess, sys, time

ROOT = pathlib.Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
from hkbuild import core, strings   # noqa: E402
from hkbuild.strings import t        # noqa: E402


def ensure_python_deps(cache):
    """Under `uv run` (or the packaged exe) the dependencies exist; with plain python3 re-run inside a private venv."""
    if getattr(sys, "frozen", False):
        return
    try:
        import capstone  # noqa: F401
        return
    except ImportError:
        pass
    venv = cache / "venv"
    py = venv / ("Scripts" if os.name == "nt" else "bin") / ("python.exe" if os.name == "nt" else "python")
    if not py.exists():
        print("Creating a private Python environment (capstone)...", flush=True)
        subprocess.check_call([sys.executable, "-m", "venv", str(venv)])
        subprocess.check_call([str(py), "-m", "pip", "install", "--quiet", "capstone>=5"])
    sys.exit(subprocess.call([str(py), str(ROOT / "build.py")] + sys.argv[1:]))


def main():
    strings.detect()
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--game", help="folder that contains HDKWIN.EXE (auto-detected if omitted)")
    ap.add_argument("--ptbr", help="folder with the extracted PT-BR fan translation (optional)")
    ap.add_argument("--out", default=str(ROOT / "out"), help="output folder (default: ./out)")
    ap.add_argument("--cache", default=str(core.default_cache()), help="cache folder")
    ap.add_argument("--clean", action="store_true", help="forget the cached recompilation and Vita build")
    a = ap.parse_args()
    cache = pathlib.Path(a.cache).resolve()
    cache.mkdir(parents=True, exist_ok=True)
    ensure_python_deps(cache)
    game = a.game or core.autodetect_game()
    t0 = time.time()
    last = [""]

    def cb(ev, **d):
        if ev == "status" and d["text"] != last[0] and not re.search(r"\((?!\d*0/)\d+/\d+\)", d["text"]):
            last[0] = d["text"]
            print(f"[{time.time() - t0:6.1f}s] {d['text']}", flush=True)
    try:
        if not game:
            raise core.BuildError("err_nogame")
        if a.ptbr and not pathlib.Path(a.ptbr).is_dir():
            raise core.BuildError("err_tr")
        out = core.Builder(cache, a.out, cb).build(game, a.ptbr, a.clean)
    except core.BuildError as e:
        print(f"\nERROR: {e}", file=sys.stderr)
        sys.exit(1)
    print(f"{t('cli_done')}\n  {out}\nTotal time: {time.time() - t0:.0f} s")


if __name__ == "__main__":
    main()

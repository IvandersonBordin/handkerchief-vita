#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.9"
# dependencies = ["capstone>=5", "cmake", "ninja"]
# ///
"""Build your own Handkerchief PS Vita VPK from your own copy of the game.

    uv run build.py --game "/path/to/folder/with/HDKWIN.EXE" [--ptbr "/path/to/translation"]

Nothing from the game is distributed: the executable is recompiled to C on your machine, compiled with a VitaSDK that
is downloaded into a cache folder, and the result is written to ./out (the VPK plus the game files laid out for
ux0:data/handkerchief/). Re-running is cheap: finished steps are skipped (use --clean to redo everything).
Without uv, plain `python3 build.py ...` works too: the Python packages are then installed into a private venv in the
cache folder. Needs: Linux x86-64 (Windows: use WSL), about 3 GB of free disk space, an internet connection for the
first run (VitaSDK, ~110 MB), and the `tar`, `bzip2` and `curl` commands that the VitaSDK package tool itself needs.
"""
import argparse, hashlib, os, pathlib, platform, shutil, subprocess, sys, tarfile, time, urllib.request

ROOT = pathlib.Path(__file__).resolve().parent

# The one supported executable (the original 1999-2000 release, 'HDKWIN.EXE').
EXE_SHA256 = "3deeb9b18ac2f6ee7b4af4769d1499fdfe0bde8de44482b57ab4bf6196617b76"
# Pinned VitaSDK (classic 'master' autobuild, Linux x86-64). SDL2 comes from the VitaSDK package manager (vdpm).
SDK_URL = ("https://github.com/vitasdk/autobuilds/releases/download/master-linux-v2.540/"
           "vitasdk-x86_64-linux-gnu-2026-05-24_12-05-02.tar.bz2")
SDK_SHA256 = "b1a8f4d6e41460c9ecf2c3c4590eb7eb3ea6b690767f508bd5696dbc77a4272e"
SDK_PACKAGES = ["sdl2"]
FUNCS = ROOT / "recomp" / "data" / "hdkwin-functions.json.gz"
GAME_ITEMS = ["HDKWIN.EXE", "BITMAP", "SYSTEM", "USER", "WAV"]
T0 = time.time()


def say(msg):
    print(f"[{time.time() - T0:6.1f}s] {msg}", flush=True)


def die(msg):
    print(f"\nERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def find_ci(folder, name):
    """Case-insensitive child lookup (Windows copies are often upper case, Linux is case sensitive)."""
    for c in folder.iterdir():
        if c.name.lower() == name.lower():
            return c
    return None


def find_game(arg):
    cands = [pathlib.Path(arg)] if arg else []
    if not arg:
        home = pathlib.Path.home()
        cands += [pathlib.Path.cwd(), ROOT, ROOT / "game", home / "Handkerchief", home / "Games" / "Handkerchief",
                  home / ".steam/steam/steamapps/common/Handkerchief"]
        cands += [pathlib.Path(p) for p in ("/mnt/c/Program Files (x86)/Handkerchief", "/mnt/c/Games/Handkerchief",
                                            "/mnt/c/Program Files (x86)/ASK/Handkerchief")]
    for c in cands:
        if c.is_dir() and find_ci(c, "HDKWIN.EXE"):
            return c
    if arg:
        die(f"no HDKWIN.EXE in {arg}. Pass the folder that contains the game's HDKWIN.EXE.")
    die("could not find the game. Run: uv run build.py --game \"/path/to/folder/with/HDKWIN.EXE\"")


def run(cmd, env=None, cwd=None, log=None):
    say("$ " + " ".join(str(c) for c in cmd))
    if log:
        with open(log, "w") as lf:
            r = subprocess.run(cmd, env=env, cwd=cwd, stdout=lf, stderr=subprocess.STDOUT)
        if r.returncode:
            tail = "".join(open(log, errors="replace").readlines()[-25:])
            die(f"command failed ({r.returncode}); see {log}\n{tail}")
    else:
        r = subprocess.run(cmd, env=env, cwd=cwd)
        if r.returncode:
            die(f"command failed ({r.returncode}): {cmd[0]}")


def ensure_python_deps(cache):
    """Under `uv run` the dependencies already exist; with plain python3 re-run inside a private venv."""
    try:
        import capstone  # noqa: F401
        return
    except ImportError:
        pass
    venv = cache / "venv"
    py = venv / "bin" / "python"
    if not py.exists():
        say("Creating a private Python environment (capstone, cmake, ninja)...")
        subprocess.check_call([sys.executable, "-m", "venv", str(venv)])
        subprocess.check_call([str(py), "-m", "pip", "install", "--quiet", "capstone>=5", "cmake", "ninja"])
    os.execv(str(py), [str(py), str(ROOT / "build.py")] + sys.argv[1:])


def tool(name):
    """A build tool from the active Python environment (pip wheels) or the system."""
    p = pathlib.Path(sys.prefix) / "bin" / name
    if p.exists():
        return str(p)
    w = shutil.which(name)
    if not w:
        die(f"{name} not found; install it (pip install {name}) or use uv.")
    return w


def ensure_sdk(cache):
    sdk = cache / "vitasdk"
    stamp = sdk / ".hk-ready"
    if stamp.exists():
        say("VitaSDK: cached")
        return sdk
    arc = cache / "downloads" / SDK_URL.rsplit("/", 1)[1]
    arc.parent.mkdir(parents=True, exist_ok=True)
    if not (arc.exists() and sha256(arc) == SDK_SHA256):
        say(f"VitaSDK: downloading {SDK_URL.rsplit('/', 1)[1]} (about 110 MB)...")
        part = arc.with_suffix(".part")
        with urllib.request.urlopen(SDK_URL) as r, open(part, "wb") as f:
            shutil.copyfileobj(r, f, 1 << 20)
        if sha256(part) != SDK_SHA256:
            part.unlink()
            die("the downloaded VitaSDK does not match the pinned SHA-256; try again or report this.")
        part.rename(arc)
    if sdk.exists():
        shutil.rmtree(sdk)
    say("VitaSDK: extracting...")
    tmp = cache / "sdk-extract"
    if tmp.exists():
        shutil.rmtree(tmp)
    tmp.mkdir()
    with tarfile.open(arc, "r:bz2") as t:
        t.extractall(tmp)
    (tmp / "vitasdk").rename(sdk)
    tmp.rmdir()
    env = dict(os.environ, VITASDK=str(sdk), PATH=f"{sdk}/bin:{os.environ['PATH']}")
    say("VitaSDK: installing SDL2 with vdpm...")
    run([str(sdk / "bin" / "vdpm")] + SDK_PACKAGES, env=env)
    stamp.write_text(SDK_SHA256 + "\n")
    return sdk


def recompile(cache, exe, exe_hash):
    gen = cache / "gen"
    stamp = gen / ".hk-ready"
    srcs = sorted((ROOT / "recomp").glob("*.py")) + [FUNCS]
    key = hashlib.sha256((exe_hash + "".join(sha256(p) for p in srcs)).encode()).hexdigest()
    if stamp.exists() and stamp.read_text().strip() == key:
        say("Recompiler: cached")
        return gen
    if gen.exists():
        shutil.rmtree(gen)
    gen.mkdir(parents=True)
    env = dict(os.environ, HK_EXE=str(exe), HK_FUNCS=str(FUNCS), HK_GEN=str(gen))
    say("Recompiler: translating HDKWIN.EXE to C (about 1 minute)...")
    run([sys.executable, "-I", str(ROOT / "recomp" / "x86rec.py")], env=env, log=cache / "x86rec.log")
    run([sys.executable, "-I", str(ROOT / "recomp" / "gen_import_stubs.py"), str(gen)], env=env,
        log=cache / "import-stubs.log")
    stamp.write_text(key + "\n")
    return gen


def build_vpk(cache, sdk, gen):
    bld = cache / "build-vita"
    env = dict(os.environ, VITASDK=str(sdk), PATH=f"{sdk}/bin:{os.environ['PATH']}")
    cmake, ninja = tool("cmake"), tool("ninja")
    say("Vita build: configuring (Release, player build: no developer tools)...")
    run([cmake, "-S", str(ROOT / "port" / "vita"), "-B", str(bld), "-G", "Ninja", f"-DCMAKE_MAKE_PROGRAM={ninja}",
         "-DCMAKE_BUILD_TYPE=Release", "-DHK_DEVTOOLS=OFF", f"-DHK_GEN={gen}"], env=env, log=cache / "cmake.log")
    say("Vita build: compiling (a few minutes)...")
    run([cmake, "--build", str(bld), "-j", str(os.cpu_count() or 4)], env=env, log=cache / "compile.log")
    vpk = bld / "handkerchief.vpk"
    if not vpk.exists():
        die("the build finished without a VPK; see the compile log in the cache folder.")
    return vpk


def copy_game(game, dest, ptbr):
    dest.mkdir(parents=True, exist_ok=True)
    for item in GAME_ITEMS:
        src = find_ci(game, item)
        if not src:
            say(f"  note: {item} not found in the game folder (skipped)")
            continue
        tgt = dest / item
        if src.is_dir():
            shutil.copytree(src, tgt, dirs_exist_ok=True)
        else:
            shutil.copy2(src, tgt)
    say(f"Game files copied to {dest}")
    if ptbr:
        found = {}
        for p in ptbr.rglob("*"):
            if p.is_file() and p.name.upper() in ("HDKWINBR.EXE", "HDKCON00.BMP", "HDKSYS00.BMP"):
                found[p.name.upper()] = p
        if "HDKWINBR.EXE" not in found:
            die(f"HDKWINBR.EXE not found under {ptbr} (extract the fan translation archive first).")
        lang = dest / "lang" / "ptbr"
        (lang / "BITMAP").mkdir(parents=True, exist_ok=True)
        shutil.copy2(found["HDKWINBR.EXE"], lang / "HDKWINBR.EXE")
        for n in ("HDKCON00.BMP", "HDKSYS00.BMP"):
            if n in found:
                shutil.copy2(found[n], lang / "BITMAP" / n)
        say(f"Portuguese (BR) translation placed in {lang}")


def main():
    if sys.platform.startswith("win"):
        die("native Windows is not supported. Install WSL (wsl --install, Ubuntu) and run this script inside it; "
            "see 'Building your own VPK (easy way)' in README.md.")
    if platform.machine() not in ("x86_64", "AMD64"):
        die("the pinned VitaSDK is for Linux x86-64 only.")
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--game", help="folder that contains HDKWIN.EXE (auto-detected if omitted)")
    ap.add_argument("--ptbr", help="folder with the extracted PT-BR fan translation (optional)")
    ap.add_argument("--out", default=str(ROOT / "out"), help="output folder (default: ./out)")
    ap.add_argument("--cache", default=os.environ.get("HK_BUILDER_CACHE",
                    str(pathlib.Path.home() / ".cache" / "handkerchief-builder")), help="cache folder")
    ap.add_argument("--clean", action="store_true", help="forget the cached recompilation and Vita build")
    a = ap.parse_args()
    cache = pathlib.Path(a.cache).resolve()
    cache.mkdir(parents=True, exist_ok=True)
    ensure_python_deps(cache)

    game = find_game(a.game)
    exe = find_ci(game, "HDKWIN.EXE")
    say(f"Game folder: {game}")
    h = sha256(exe)
    if h != EXE_SHA256:
        die(f"{exe.name} is not the supported version.\n  expected SHA-256 {EXE_SHA256}\n  found    SHA-256 {h}\n"
            "Only the original Handkerchief release is supported. Patched, translated or cracked executables will "
            "not work; use your untouched copy (the Portuguese translation is added with --ptbr).")
    say("HDKWIN.EXE verified.")
    ptbr = pathlib.Path(a.ptbr) if a.ptbr else None
    if ptbr and not ptbr.is_dir():
        die(f"--ptbr folder not found: {ptbr}")
    if a.clean:
        for d in ("gen", "build-vita"):
            shutil.rmtree(cache / d, ignore_errors=True)

    sdk = ensure_sdk(cache)
    gen = recompile(cache, exe, h)
    vpk = build_vpk(cache, sdk, gen)
    out = pathlib.Path(a.out).resolve()
    out.mkdir(parents=True, exist_ok=True)
    shutil.copy2(vpk, out / "handkerchief.vpk")
    size = (out / "handkerchief.vpk").stat().st_size
    say(f"VPK: {out / 'handkerchief.vpk'} ({size / 1e6:.2f} MB)")
    if size > 2.6e6:
        say("  warning: the VPK is much larger than the expected ~1.9 MB; was it built in Release mode?")
    copy_game(game, out / "ux0_data_handkerchief", ptbr)
    say("Done. Install handkerchief.vpk with VitaShell, then copy the folder ux0_data_handkerchief to "
        "ux0:/data/handkerchief on the Vita.")
    print(f"Total time: {time.time() - T0:.0f} s")


if __name__ == "__main__":
    main()

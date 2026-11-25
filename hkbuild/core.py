"""Build pipeline shared by the GUI and the command line: verify the game, fetch pinned public tools, recompile
HDKWIN.EXE to C on this machine, build the VPK, lay out the game files. Pure standard library (+ capstone)."""
import contextlib, gzip, hashlib, os, pathlib, platform, re, runpy, shutil, subprocess, sys, tarfile, threading, time
import urllib.request, zipfile

from .strings import t

FROZEN = getattr(sys, "frozen", False)
ROOT = pathlib.Path(getattr(sys, "_MEIPASS", None) or pathlib.Path(__file__).resolve().parent.parent)
IS_WIN = os.name == "nt"
PLAT = "windows" if IS_WIN else "linux"

# Supported HDKWIN.EXE builds: the original release (archive.org) and the community-fix exe; same code, only the
# .data path strings differ (the fix drops the Hdkwin/ prefix).
EXE_EDITIONS = {
    "3deeb9b18ac2f6ee7b4af4769d1499fdfe0bde8de44482b57ab4bf6196617b76": "community fix",
    "da76ca668b49d0754379416a6f4eb324eb1126b4cb079bfb4d93f4e2dbcdc08d": "original release",
}
EXE_SHA256 = "3deeb9b18ac2f6ee7b4af4769d1499fdfe0bde8de44482b57ab4bf6196617b76"   # recompilation cache key (code is identical)
ARCHIVE_URL = "https://archive.org/details/Handkerchief"
GAME_ITEMS = ["HDKWIN.EXE", "BITMAP", "SYSTEM", "WAV"]   # USER is not shipped: updating must never overwrite the saves (the port creates it at boot)
REQUIRED_DIRS = ["BITMAP", "SYSTEM", "WAV"]
FUNCS = ROOT / "recomp" / "data" / "hdkwin-functions.json.gz"
TRANSLATION_URL = "https://patryckpo.com/traducoes/windows/handkerchief/downloads/"

# Pinned public downloads (url, sha256, size in MB for the progress text). Same VitaSDK commit (autobuild v2.540) on both.
_GH = "https://github.com/vitasdk/autobuilds/releases/download/"
SDK = {
    "linux": (_GH + "master-linux-v2.540/vitasdk-x86_64-linux-gnu-2026-05-24_12-05-02.tar.bz2",
              "b1a8f4d6e41460c9ecf2c3c4590eb7eb3ea6b690767f508bd5696dbc77a4272e", 110),
    "windows": (_GH + "master-win-v2.540/vitasdk-x86_64-w64-mingw32-2026-05-24_12-05-17.tar.bz2",
                "dc2b8a5fb72e162e9ebc3890897d4ab57ab8ed0aff729a6aca6fe94079155b0e", 95),
}
# VitaSDK package 'sdl2' (what `vdpm sdl2` downloads); extracted into <sdk>/arm-vita-eabi.
SDL2 = ("https://github.com/vitasdk/packages/releases/download/master/sdl2.tar.xz",
        "f7318878817e5268fd991a2796d83a544c7688d0c98f510e4d6c9d3b07725e5b", 1)
_CM = "https://github.com/Kitware/CMake/releases/download/v3.31.9/"
CMAKE = {
    "linux": (_CM + "cmake-3.31.9-linux-x86_64.tar.gz",
              "312d78e0b7c5c9b4a97a87a46c3e525aa84895fe8135c238c4616bba73dc9518", 50),
    "windows": (_CM + "cmake-3.31.9-windows-x86_64.zip",
                "dc16867d0c0ffb35026976aff119335d5b47def44e76ae82ffafdc1f98b3d59d", 40),
}
_NJ = "https://github.com/ninja-build/ninja/releases/download/v1.13.2/"
NINJA = {
    "linux": (_NJ + "ninja-linux.zip", "5749cbc4e668273514150a80e387a957f933c6ed3f5f11e03fb30955e2bbead6", 1),
    "windows": (_NJ + "ninja-win.zip", "07fc8261b42b20e71d1720b39068c2e14ffcee6396b76fb7a795fb460b78dc65", 1),
}
# Step weights for the overall progress bar (fractions of 1.0).
WEIGHTS = [("verify", 0.01), ("sdk", 0.30), ("tools", 0.04), ("recomp", 0.15), ("config", 0.02), ("compile", 0.42),
           ("copy", 0.06)]


class BuildError(Exception):
    def __init__(self, key, **kw):
        self.key, self.kw = key, kw
        super().__init__(t(key, **kw))


class Cancelled(Exception):
    pass


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def find_ci(folder, name):
    """Case-insensitive child lookup (Windows copies are often upper case, Linux is case sensitive)."""
    try:
        for c in folder.iterdir():
            if c.name.lower() == name.lower():
                return c
    except OSError:
        pass
    return None


def check_platform():
    m = platform.machine().lower()
    if m not in ("x86_64", "amd64"):
        raise BuildError("err_platform", want=t("plat_" + PLAT), have=f"{platform.system()} {platform.machine()}")
    if sys.platform not in ("win32", "linux"):
        raise BuildError("err_platform", want=t("plat_" + PLAT), have=platform.system())


def default_cache():
    if os.environ.get("HK_BUILDER_CACHE"):
        return pathlib.Path(os.environ["HK_BUILDER_CACHE"])
    if IS_WIN:   # keep it short: the VitaSDK tree has long paths
        return pathlib.Path(os.environ.get("LOCALAPPDATA", str(pathlib.Path.home()))) / "HandkerchiefBuilder"
    return pathlib.Path.home() / ".cache" / "handkerchief-builder"


def default_out():
    d = pathlib.Path.home() / "Desktop"
    return (d if d.is_dir() else pathlib.Path.home()) / "Handkerchief-Vita"


# ---- game folder -------------------------------------------------------------------------------------------------

def check_game(folder, want_hash=True):
    """Return the folder that really holds HDKWIN.EXE (the chosen one or a direct child) or raise BuildError."""
    folder = pathlib.Path(folder)
    if not folder.is_dir() or not find_ci(folder, "HDKWIN.EXE"):
        sub = None
        if folder.is_dir():
            for c in sorted(folder.iterdir()):
                if c.is_dir() and find_ci(c, "HDKWIN.EXE"):
                    sub = c
                    break
        if not sub:
            raise BuildError("err_no_exe")
        folder = sub
    missing = [d for d in REQUIRED_DIRS if not (find_ci(folder, d) and find_ci(folder, d).is_dir())]
    if missing:
        raise BuildError("err_missing", names=", ".join(missing))
    if want_hash:
        h = sha256(find_ci(folder, "HDKWIN.EXE"))
        if h not in EXE_EDITIONS:
            raise BuildError("err_hash", found=h)
    return folder


def game_edition(folder):
    """Friendly edition name of the HDKWIN.EXE in folder, or None if unknown."""
    return EXE_EDITIONS.get(sha256(find_ci(pathlib.Path(folder), "HDKWIN.EXE")))


def autodetect_game():
    home = pathlib.Path.home()
    roots = [home / "Handkerchief", home / "Games" / "Handkerchief", home / "Desktop", home / "Downloads",
             home / "Documents", pathlib.Path.cwd(), ROOT]
    if IS_WIN:
        for env in ("ProgramFiles(x86)", "ProgramFiles"):
            if os.environ.get(env):
                p = pathlib.Path(os.environ[env])
                roots += [p / "Handkerchief", p / "ASK" / "Handkerchief", p / "ASK", p]
        roots += [pathlib.Path(d + ":/" + s) for d in "CDE" for s in ("Games/Handkerchief", "Handkerchief")]
    else:
        roots += [home / ".steam/steam/steamapps/common/Handkerchief", home / ".wine/drive_c/Games/Handkerchief",
                  home / ".wine/drive_c/Program Files (x86)/Handkerchief"]
    for r in roots:
        try:
            if r.is_dir() and find_ci(r, "HDKWIN.EXE"):
                return r
            if r.is_dir() and r.name.lower() in ("desktop", "downloads", "documents", "ask", "program files",
                                                 "program files (x86)"):
                for c in sorted(r.iterdir())[:300]:
                    if c.is_dir() and find_ci(c, "HDKWIN.EXE"):
                        return c
        except OSError:
            continue
    return None


def find_translation_files(folder):
    found = {}
    for p in pathlib.Path(folder).rglob("*"):
        if p.is_file() and p.name.upper() in ("HDKWINBR.EXE", "HDKCON00.BMP", "HDKSYS00.BMP"):
            found[p.name.upper()] = p
    if "HDKWINBR.EXE" not in found:
        raise BuildError("err_tr")
    return found


# ---- the builder -------------------------------------------------------------------------------------------------

class Builder:
    """cb(event, **data) receives: status(text), progress(frac 0..1), log(line). Safe to call from a worker thread."""

    def __init__(self, cache, out, cb=None):
        self.cache = pathlib.Path(cache).resolve()
        self.out = pathlib.Path(out).resolve()
        self.cb = cb or (lambda *a, **k: None)
        self.cancel_flag = threading.Event()
        self.proc = None
        self._base = 0.0
        self._span = 0.0
        self.logs = self.cache / "logs"

    # progress helpers
    def status(self, text):
        self.cb("status", text=text)

    def frac(self, f):
        self.cb("progress", value=min(1.0, self._base + self._span * max(0.0, min(1.0, f))))

    def begin(self, name):
        base = 0.0
        for n, w in WEIGHTS:
            if n == name:
                self._base, self._span = base, w
                break
            base += w
        self.frac(0)

    def cancel(self):
        self.cancel_flag.set()
        p = self.proc
        if p and p.poll() is None:
            with contextlib.suppress(Exception):
                p.kill()

    def check_cancel(self):
        if self.cancel_flag.is_set():
            raise Cancelled()

    # downloading / unpacking
    def download(self, spec, name):
        url, sha, mb = spec
        d = self.cache / "downloads"
        d.mkdir(parents=True, exist_ok=True)
        arc = d / url.rsplit("/", 1)[1]
        if arc.exists() and sha256(arc) == sha:
            return arc
        part = arc.with_suffix(arc.suffix + ".part")
        last = None
        for attempt in range(3):
            self.check_cancel()
            try:
                req = urllib.request.Request(url, headers={"User-Agent": "HandkerchiefVitaBuilder"})
                with urllib.request.urlopen(req, timeout=60) as r, open(part, "wb") as f:
                    total = int(r.headers.get("Content-Length") or mb * 1e6)
                    got = 0
                    while True:
                        self.check_cancel()
                        b = r.read(1 << 18)
                        if not b:
                            break
                        f.write(b)
                        got += len(b)
                        self.frac(got / total)
                last = None
                break
            except Cancelled:
                raise
            except Exception as e:   # network errors: retry, then explain
                last = e
                time.sleep(2)
        if last is not None:
            raise BuildError("err_download", name=name, why=str(last))
        if sha256(part) != sha:
            part.unlink()
            raise BuildError("err_sha", name=name)
        part.replace(arc)
        return arc

    def unpack(self, arc, dest, strip_top=False):
        """Extract a .tar.* or .zip safely into dest (streaming, with progress)."""
        dest = pathlib.Path(dest)
        dest.mkdir(parents=True, exist_ok=True)
        base = dest.resolve()
        size = max(1, arc.stat().st_size)
        if arc.name.endswith(".zip"):
            with zipfile.ZipFile(arc) as z:
                infos = z.infolist()
                for i, m in enumerate(infos):
                    self.check_cancel()
                    target = (base / m.filename).resolve()
                    if base not in target.parents and target != base:
                        raise BuildError("err_sha", name=arc.name)
                    z.extract(m, base)
                    if not IS_WIN and not m.is_dir():
                        os.chmod(target, 0o755 if (m.external_attr >> 16) & 0o111 else 0o644)
                    self.frac((i + 1) / len(infos))
            return
        with open(arc, "rb") as raw, tarfile.open(fileobj=raw, mode="r|*") as tf:
            for m in tf:
                self.check_cancel()
                target = (base / m.name).resolve()
                if base not in target.parents and target != base:
                    raise BuildError("err_sha", name=arc.name)
                if hasattr(tarfile, "data_filter"):
                    tf.extract(m, base, filter="data")
                else:
                    tf.extract(m, base)
                self.frac(raw.tell() / size)

    # steps
    def ensure_sdk(self):
        sdk = self.cache / "vitasdk"
        stamp = sdk / ".hk-ready"
        key = SDK[PLAT][1] + SDL2[1]
        if stamp.exists() and stamp.read_text().strip() == key:
            return sdk
        self.begin("sdk")
        name = "VitaSDK"
        self._span = WEIGHTS[1][1] * 0.45
        self.status(t("st_dl", size=f"~{SDK[PLAT][2]} MB", name=name))
        arc = self.download(SDK[PLAT], name)
        self._base += self._span
        self._span = WEIGHTS[1][1] * 0.5
        self.status(t("st_extract", name=name))
        if sdk.exists():
            shutil.rmtree(sdk)
        tmp = self.cache / "sdk-extract"
        shutil.rmtree(tmp, ignore_errors=True)
        self.unpack(arc, tmp)
        (tmp / "vitasdk").rename(sdk)
        shutil.rmtree(tmp, ignore_errors=True)
        self._base += self._span
        self._span = WEIGHTS[1][1] * 0.05
        self.status(t("st_dl", size="~1 MB", name="SDL2"))
        a2 = self.download(SDL2, "SDL2")
        self.unpack(a2, sdk / "arm-vita-eabi")      # what `vdpm sdl2` does, without bash/curl/tar
        stamp.write_text(key + "\n")
        return sdk

    def ensure_tools(self):
        tools = self.cache / "tools"
        exe = ".exe" if IS_WIN else ""
        cm = tools / "cmake" / ("cmake-3.31.9-windows-x86_64" if IS_WIN else "cmake-3.31.9-linux-x86_64") / "bin"
        nj = tools / "ninja"
        if not ((cm / f"cmake{exe}").exists() and (nj / f"ninja{exe}").exists()):
            self.begin("tools")
            self.status(t("st_dl", size=f"~{CMAKE[PLAT][2]} MB", name="CMake + Ninja"))
            a = self.download(CMAKE[PLAT], "CMake")
            b = self.download(NINJA[PLAT], "Ninja")
            self.status(t("st_extract", name="CMake + Ninja"))
            shutil.rmtree(tools, ignore_errors=True)
            self.unpack(a, tools / "cmake")
            self.unpack(b, nj)
            if not IS_WIN:
                os.chmod(nj / "ninja", 0o755)
        return cm, nj

    def recompile(self, exe, exe_hash):
        gen = self.cache / "gen"
        stamp = gen / ".hk-ready"
        srcs = sorted((ROOT / "recomp").glob("*.py")) + [FUNCS]
        key = hashlib.sha256((exe_hash + "".join(sha256(p) for p in srcs)).encode()).hexdigest()
        if stamp.exists() and stamp.read_text().strip() == key:
            return gen
        self.begin("recomp")
        self.status(t("st_recomp"))
        shutil.rmtree(gen, ignore_errors=True)
        gen.mkdir(parents=True)
        self.logs.mkdir(parents=True, exist_ok=True)
        env = {"HK_EXE": str(exe), "HK_FUNCS": str(FUNCS), "HK_GEN": str(gen)}
        done = threading.Event()
        err = []

        def work():
            old_env = {k: os.environ.get(k) for k in env}
            os.environ.update(env)
            log = self.logs / "recompile.log"
            try:
                with open(log, "w", encoding="utf-8") as lf, contextlib.redirect_stdout(lf), \
                        contextlib.redirect_stderr(lf):
                    for script, args in (("x86rec.py", []), ("gen_import_stubs.py", [str(gen)])):
                        old_argv = sys.argv
                        sys.argv = [script] + args
                        try:
                            runpy.run_path(str(ROOT / "recomp" / script), run_name="__main__")
                        except SystemExit as e:
                            if e.code not in (None, 0):
                                raise RuntimeError(f"{script} exited with {e.code}")
                        finally:
                            sys.argv = old_argv
            except BaseException as e:   # reported in the caller
                err.append(e)
            finally:
                for k, v in old_env.items():
                    if v is None:
                        os.environ.pop(k, None)
                    else:
                        os.environ[k] = v
                done.set()

        th = threading.Thread(target=work, daemon=True)
        th.start()
        t0 = time.time()
        while not done.wait(0.5):
            self.check_cancel()   # (the translation itself cannot be interrupted; it ends within ~2 minutes)
            self.frac(1 - 1 / (1 + (time.time() - t0) / 60))
        if err:
            raise BuildError("err_step", step=t("st_recomp"), log=self.logs / "recompile.log",
                             tail=f"{type(err[0]).__name__}: {err[0]}")
        stamp.write_text(key + "\n")
        return gen

    def run(self, cmd, env, logname, step, on_line=None):
        self.logs.mkdir(parents=True, exist_ok=True)
        log = self.logs / logname
        flags = 0x08000000 if IS_WIN else 0   # CREATE_NO_WINDOW: no console flashes in the windowed .exe
        with open(log, "w", encoding="utf-8", errors="replace") as lf:
            lf.write("$ " + " ".join(str(c) for c in cmd) + "\n")
            self.proc = subprocess.Popen([str(c) for c in cmd], env=env, stdout=subprocess.PIPE,
                                         stderr=subprocess.STDOUT, text=True, encoding="utf-8", errors="replace",
                                         creationflags=flags)
            for line in self.proc.stdout:
                lf.write(line)
                if on_line:
                    on_line(line)
            rc = self.proc.wait()
        self.check_cancel()
        if rc:
            tail = "".join(open(log, encoding="utf-8", errors="replace").readlines()[-15:])
            raise BuildError("err_step", step=step, log=log, tail=tail)

    def build_vpk(self, sdk, gen, cm, nj):
        sd = sdk.as_posix()
        exe = ".exe" if IS_WIN else ""
        env = dict(os.environ, VITASDK=sd,
                   PATH=os.pathsep.join([str(sdk / "bin"), str(cm), str(nj), os.environ.get("PATH", "")]))
        bld = self.cache / "build-vita"
        self.begin("config")
        self.status(t("st_config"))
        self.run([cm / f"cmake{exe}", "-S", ROOT / "port" / "vita", "-B", bld, "-G", "Ninja",
                  f"-DCMAKE_MAKE_PROGRAM={(nj / ('ninja' + exe)).as_posix()}", "-DCMAKE_BUILD_TYPE=Release",
                  "-DHK_DEVTOOLS=OFF", f"-DHK_GEN={gen.as_posix()}"], env, "cmake.log", t("st_config"))
        self.begin("compile")
        self.status(t("st_compile0"))

        def line(s):
            m = re.match(r"\[(\d+)/(\d+)\]", s)
            if m:
                d, n = int(m.group(1)), int(m.group(2))
                self.status(t("st_compile", done=d, total=n))
                self.frac(d / n)
        self.run([cm / f"cmake{exe}", "--build", bld, "-j", str(os.cpu_count() or 4)], env, "compile.log",
                 t("st_compile0"), on_line=line)
        vpk = bld / "handkerchief.vpk"
        if not vpk.exists():
            raise BuildError("err_novpk", log=self.logs / "compile.log")
        return vpk

    def copy_game(self, game, dest, tr):
        self.begin("copy")
        self.status(t("st_copy"))
        dest.mkdir(parents=True, exist_ok=True)
        files = []
        for item in GAME_ITEMS:
            src = find_ci(game, item)
            if src:
                files += [(p, dest / item / p.relative_to(src)) for p in src.rglob("*") if p.is_file()] \
                    if src.is_dir() else [(src, dest / item)]
        for i, (s, d) in enumerate(files):
            self.check_cancel()
            d.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(s, d)
            self.frac((i + 1) / max(1, len(files)))
        if tr:
            found = find_translation_files(tr)
            lang = dest / "lang" / "ptbr"
            (lang / "BITMAP").mkdir(parents=True, exist_ok=True)
            shutil.copy2(found["HDKWINBR.EXE"], lang / "HDKWINBR.EXE")
            for n in ("HDKCON00.BMP", "HDKSYS00.BMP"):
                if n in found:
                    shutil.copy2(found[n], lang / "BITMAP" / n)

    def build(self, game, translation=None, clean=False):
        """Full pipeline. Returns the output folder. Raises BuildError / Cancelled."""
        check_platform()
        self.begin("verify")
        self.status(t("st_verify"))
        game = check_game(game)
        exe = find_ci(game, "HDKWIN.EXE")
        if translation:
            find_translation_files(translation)
        self.cache.mkdir(parents=True, exist_ok=True)
        free = shutil.disk_usage(self.cache).free / 1e9
        if free < 3:
            raise BuildError("err_space", need=3, path=self.cache, have=f"{free:.1f}")
        if clean:
            for d in ("gen", "build-vita"):
                shutil.rmtree(self.cache / d, ignore_errors=True)
        sdk = self.ensure_sdk()
        cm, nj = self.ensure_tools()
        gen = self.recompile(exe, sha256(exe))   # the exe actually in use (check_game pins it, but the key must not depend on that)
        vpk = self.build_vpk(sdk, gen, cm, nj)
        self.out.mkdir(parents=True, exist_ok=True)
        shutil.copy2(vpk, self.out / "handkerchief.vpk")
        self.copy_game(game, self.out / "handkerchief", translation)
        self._base, self._span = 0.0, 1.0
        self.frac(1.0)
        return self.out

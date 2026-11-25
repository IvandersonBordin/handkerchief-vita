"""Tk window in the look of the port's menus: three plain steps, a progress screen and a 'what to do next' screen.

Colours are the port's own (port/hk/theme.h); no game art, fonts or text are used or read."""
import os, pathlib, subprocess, sys, threading, time, webbrowser
import tkinter as tk
from tkinter import filedialog, font as tkfont, messagebox, ttk

from . import core, strings
from .strings import t


def host_env():
    """Environment for launching the desktop's own programs (browser, file manager). A PyInstaller bundle on Linux
    points LD_LIBRARY_PATH at its private libraries, which breaks xdg-open and the browser; restore the original."""
    env = dict(os.environ)
    if getattr(sys, "frozen", False) and not core.IS_WIN:
        orig = env.get("LD_LIBRARY_PATH_ORIG")
        if orig is not None:
            env["LD_LIBRARY_PATH"] = orig
        else:
            env.pop("LD_LIBRARY_PATH", None)
    return env


def open_path(target):
    """Open a folder or a web address with the desktop's default program."""
    try:
        if core.IS_WIN:
            os.startfile(str(target))   # noqa: S606
        else:
            subprocess.Popen(["xdg-open", str(target)], env=host_env(), stdout=subprocess.DEVNULL,
                             stderr=subprocess.DEVNULL, start_new_session=True)
        return True
    except Exception:
        try:
            return webbrowser.open(str(target))
        except Exception:
            return False


def open_folder(p):
    open_path(p)


def open_url(url):
    if not open_path(url):
        messagebox.showinfo("Handkerchief Vita Builder", url)   # last resort: show the address so it can be copied



# Palette of the port's UI theme (port/hk/theme.h, docs/menus.md); GOLD is our own heading colour.
BG, PANEL, INK = "#0c0a16", "#1c1838", "#0a0814"
BORDER, BORDER_IN, BAR = "#6c5bc5", "#433a89", "#3a2f7a"
GOLD, CREAM, TEXT, DIM, RED = "#f2c94c", "#fbfdda", "#dcd6c6", "#a09585", "#f6261c"
OKC, ERRC = "#7fe3a8", "#ff7a7a"
# Pixel sizes (negative = pixels in Tk), so the text is the same size on every screen and DPI setting.
FONT_SUBST = {"big": ("bold", -19), "h1": ("bold", -23), "h2": ("bold", -28), "mid": ("normal", -16), "norm": ("normal", -16)}
# A clean sans-serif: the bundled Tk on Linux otherwise falls back to a small fixed-width font.
FONT_FAMILIES = ("Segoe UI", "Noto Sans", "DejaVu Sans", "Liberation Sans", "Cantarell", "Ubuntu", "Arial", "Helvetica")


class Btn(tk.Frame):
    """Big flat button: purple border, red dot on the focused / hovered one, Return / Space / click to press."""

    def __init__(self, parent, text, command, font, width=None, pad=(18, 9), primary=False):
        super().__init__(parent, bg=BORDER, highlightthickness=0, bd=0, takefocus=1)
        self.cmd, self.primary = command, primary
        self.inner = tk.Frame(self, bg=BAR if primary else PANEL, bd=0)
        self.inner.pack(padx=2, pady=2, fill="both", expand=True)
        self.dot = tk.Canvas(self.inner, width=14, height=14, bg=self.inner["bg"], highlightthickness=0)
        self.dot.pack(side="left", padx=(10, 0))
        self.lbl = tk.Label(self.inner, text=text, font=font, bg=self.inner["bg"], fg=CREAM, width=width,
                            padx=pad[0], pady=pad[1], cursor="hand2")
        self.lbl.pack(side="left", fill="x", expand=True)
        self.dot.pack_configure(padx=(10, 0))
        self.hover = False
        for w in (self, self.inner, self.lbl, self.dot):
            w.bind("<Button-1>", lambda e: self.press())
            w.bind("<Enter>", lambda e: self.set_hover(True))
            w.bind("<Leave>", lambda e: self.set_hover(False))
        self.bind("<FocusIn>", lambda e: self.refresh())
        self.bind("<FocusOut>", lambda e: self.refresh())
        self.bind("<Return>", lambda e: self.press())
        self.bind("<space>", lambda e: self.press())
        self.refresh()

    def invoke(self):
        self.press()

    def press(self):
        self.focus_set()
        self.cmd()

    def set_hover(self, v):
        self.hover = v
        self.refresh()

    def refresh(self):
        on = self.hover or self.focus_get() is self
        base = BAR if self.primary else PANEL
        bg = BORDER_IN if on else base
        for w in (self.inner, self.lbl, self.dot):
            w.config(bg=bg)
        self.config(bg=GOLD if self.focus_get() is self else BORDER)
        self.dot.delete("all")
        if on:
            self.dot.create_oval(2, 2, 12, 12, fill=RED, outline="#8a1410")


class App:
    def __init__(self, root):
        self.root = root
        strings.detect()
        root.title(t("title", plat=t("plat_" + core.PLAT)))
        root.geometry("880x760")
        root.minsize(800, 700)
        root.configure(bg=BG)
        self.game = tk.StringVar()
        self.tr = tk.StringVar()
        self.out = pathlib.Path(core.default_out())
        have = set(tkfont.families(root))
        family = next((f for f in FONT_FAMILIES if f in have), tkfont.nametofont("TkDefaultFont").cget("family"))
        for named in ("TkDefaultFont", "TkTextFont", "TkFixedFont", "TkMenuFont"):
            try:
                tkfont.nametofont(named).configure(family=family, size=-16)
            except tk.TclError:
                pass
        self.fonts = {n: tkfont.Font(family=family, size=sz, weight=w)
                      for n, (w, sz) in FONT_SUBST.items()}
        self.big = self.fonts["big"]
        self.builder = None
        self.queue = []
        self.lock = threading.Lock()
        self.frame = None
        self.hdr = None
        st = ttk.Style(root)
        try:
            st.theme_use("clam")
        except tk.TclError:
            pass
        st.configure("Hk.Horizontal.TProgressbar", troughcolor=INK, background=GOLD, bordercolor=BORDER,
                     lightcolor=GOLD, darkcolor=GOLD, thickness=22)
        self.root.after(100, self.pump)
        self.lang_screen()

    # --- small widget helpers -------------------------------------------------------------------------------
    def L(self, parent, text, kind="text", **kw):
        fg = {"text": TEXT, "head": GOLD, "dim": DIM, "sel": CREAM, "ok": OKC, "err": ERRC}[kind]
        font = {"head": self.fonts["big"], "sel": self.fonts["big"]}.get(kind, self.fonts["norm"])
        kw.setdefault("font", font)
        kw.setdefault("justify", "left")
        kw.setdefault("anchor", "w")
        return tk.Label(parent, text=text, bg=parent["bg"], fg=fg, **kw)

    def sep(self, parent):
        tk.Frame(parent, bg=BORDER_IN, height=2).pack(fill="x", pady=10)

    def card(self, parent, **kw):
        outer = tk.Frame(parent, bg=BORDER_IN)
        inner = tk.Frame(outer, bg=PANEL, **kw)
        inner.pack(padx=2, pady=2, fill="both", expand=True)
        return outer, inner

    def render_header(self):
        """Text header with a purple rule (our own drawing; no game art)."""
        for w in self.hdr.winfo_children():
            w.destroy()
        tk.Label(self.hdr, text="HANDKERCHIEF \u2014 Vita Builder", font=self.fonts["h2"], bg=BG,
                 fg=GOLD).pack(pady=(10, 6))
        tk.Frame(self.hdr, bg=BORDER, height=3).pack(fill="x")
        tk.Frame(self.hdr, bg=BORDER_IN, height=2).pack(fill="x", pady=(1, 0))

    # --- language choice (first screen, every start; the last choice is preselected) ------------------------
    def lang_file(self):
        return pathlib.Path(core.default_cache()) / "language.txt"

    def lang_screen(self):
        try:
            last = self.lang_file().read_text().strip()
            if last in ("en", "pt"):
                strings.LANG = last
        except Exception:
            pass
        f = self.new_frame()
        self.hdr = tk.Frame(f, bg=BG)
        self.hdr.pack(fill="x", pady=(30, 8))
        self.render_header()
        self.L(f, "Choose your language  /  Escolha o idioma", "sel", anchor="center").pack(fill="x", pady=(0, 26))
        first = None
        for code, label in (("pt", "Portugu\u00eas (Brasil)"), ("en", "English")):
            b = Btn(f, label, lambda c=code: self.choose_lang(c), self.fonts["h1"], width=20, pad=(18, 12), primary=True)
            b.pack(pady=9)
            if code == strings.LANG:
                first = b
        if first:
            first.focus_set()
            first.refresh()

    def choose_lang(self, code):
        strings.LANG = code
        try:
            self.lang_file().parent.mkdir(parents=True, exist_ok=True)
            self.lang_file().write_text(code)
        except Exception:
            pass
        self.root.title(t("title", plat=t("plat_" + core.PLAT)))
        self.main_screen()
        g = core.autodetect_game()
        if g:
            self.set_game(g, auto=True)

    # --- plumbing -------------------------------------------------------------------------------------------
    def new_frame(self):
        if self.frame:
            self.frame.destroy()
        self.frame = tk.Frame(self.root, bg=BG, padx=24, pady=14)
        self.frame.pack(fill="both", expand=True)
        return self.frame

    def pump(self):
        with self.lock:
            items, self.queue = self.queue, []
        for fn in items:
            fn()
        self.root.after(100, self.pump)

    def post(self, fn):
        with self.lock:
            self.queue.append(fn)

    def header(self, f):
        self.hdr = tk.Frame(f, bg=BG)
        self.hdr.pack(fill="x")
        self.render_header()

    # --- screen 1 -------------------------------------------------------------------------------------------
    def main_screen(self):
        f = self.new_frame()
        top = tk.Frame(f, bg=BG)
        top.pack(fill="x")
        self.L(top, t("title", plat=t("plat_" + core.PLAT)), "dim", font=self.fonts["mid"]).pack(side="left")
        Btn(top, "Idioma / Language", self.lang_screen, self.fonts["mid"], pad=(8, 3)).pack(side="right")
        self.header(f)
        self.L(f, t("intro"), wraplength=700).pack(fill="x", pady=(6, 8))
        self.L(f, t("step1"), "head").pack(fill="x")
        self.L(f, t("step1_hint"), "dim", wraplength=700).pack(fill="x")
        r = tk.Frame(f, bg=BG)
        r.pack(fill="x", pady=5)
        Btn(r, t("choose"), self.pick_game, self.big, pad=(10, 4)).pack(side="left")
        self.game_lbl = self.L(r, self.game.get() or t("none"), "sel", wraplength=500, font=self.fonts["norm"])
        self.game_lbl.pack(side="left", padx=10)
        self.game_msg = self.L(f, "", wraplength=700)
        self.game_msg.pack(fill="x")
        r = tk.Frame(f, bg=BG)
        r.pack(fill="x", pady=2)
        self.L(r, t("no_game"), "dim").pack(side="left")
        Btn(r, t("open_archive"), lambda: open_url(core.ARCHIVE_URL), self.fonts["norm"], pad=(8, 3)).pack(side="left", padx=8)
        self.sep(f)
        self.L(f, t("step2"), "head").pack(fill="x")
        self.L(f, t("step2_hint"), "dim", wraplength=700).pack(fill="x")
        r = tk.Frame(f, bg=BG)
        r.pack(fill="x", pady=5)
        Btn(r, t("choose"), self.pick_tr, self.big, pad=(10, 4)).pack(side="left")
        Btn(r, t("clear"), lambda: (self.tr.set(""), self.tr_lbl.config(text=t("none"))), self.big,
            pad=(10, 4)).pack(side="left", padx=6)
        Btn(r, t("step2_link"), lambda: open_url(core.TRANSLATION_URL), self.big, pad=(10, 4)).pack(side="left")
        self.tr_lbl = self.L(f, self.tr.get() or t("none"), "sel", wraplength=700, font=self.fonts["norm"])
        self.tr_lbl.pack(fill="x", pady=(4, 0))
        self.sep(f)
        self.L(f, t("step3"), "head").pack(fill="x")
        self.L(f, t("first_time"), "dim", wraplength=700).pack(fill="x")
        self.build_btn = Btn(f, t("build_btn"), self.start, self.fonts["h1"], width=24, pad=(20, 10), primary=True)
        self.build_btn.pack(pady=14)
        if self.game.get():
            self.set_game(self.game.get())

    def pick_game(self):
        d = filedialog.askdirectory(title=t("pick_game_title"), mustexist=True)
        if d:
            self.set_game(d)

    def set_game(self, d, auto=False):
        try:
            g = core.check_game(d)
        except core.BuildError as e:
            self.game.set("")
            self.game_lbl.config(text=str(d))
            self.game_msg.config(text=str(e), fg=ERRC)
            return
        self.game.set(str(g))
        self.game_lbl.config(text=str(g))
        self.game_msg.config(text=(t("autodetected", path=g) + "  " if auto else "") + t("game_ok", edition=t("ed_archive" if core.game_edition(g) == "original release" else "ed_cd")), fg=OKC)

    def pick_tr(self):
        d = filedialog.askdirectory(title=t("pick_tr_title"), mustexist=True)
        if not d:
            return
        try:
            core.find_translation_files(d)
        except core.BuildError as e:
            messagebox.showerror(t("step2"), str(e))
            return
        self.tr.set(d)
        self.tr_lbl.config(text=d)

    # --- screen 2 -------------------------------------------------------------------------------------------
    def start(self):
        if not self.game.get():
            messagebox.showwarning(t("step1"), t("err_nogame"))
            return
        try:
            core.check_platform()
        except core.BuildError as e:
            messagebox.showerror(t("title", plat=t("plat_" + core.PLAT)), str(e))
            return
        game, tr = self.game.get(), self.tr.get() or None
        f = self.new_frame()
        self.header(f)
        self.L(f, t("step3"), "head", font=self.fonts["h1"]).pack(fill="x", pady=(16, 14))
        self.st_lbl = self.L(f, "", "sel", wraplength=700)
        self.st_lbl.pack(fill="x", pady=8)
        self.bar = ttk.Progressbar(f, maximum=1000, length=700, style="Hk.Horizontal.TProgressbar")
        self.bar.pack(fill="x", pady=8)
        self.time_lbl = self.L(f, "", "dim")
        self.time_lbl.pack(fill="x")
        self.L(f, t("first_time"), wraplength=700).pack(fill="x", pady=14)
        Btn(f, t("cancel"), lambda: self.builder.cancel(), self.big, pad=(20, 6)).pack(pady=10)
        self.t0 = time.time()
        self.running = True
        self.tick()
        self.builder = core.Builder(core.default_cache(), self.out, self.on_event)
        threading.Thread(target=self.work, args=(game, tr), daemon=True).start()

    def on_event(self, ev, **d):
        if ev == "status":
            self.post(lambda: self.running and self.st_lbl.config(text=d["text"]))
        elif ev == "progress":
            self.post(lambda: self.running and self.bar.config(value=d["value"] * 1000))

    def tick(self):
        if self.running:
            e = int(time.time() - self.t0)
            self.time_lbl.config(text=t("elapsed", m=e // 60, s=e % 60))
            self.root.after(1000, self.tick)

    def work(self, game, tr):
        try:
            out = self.builder.build(game, tr)
            self.post(lambda: self.done(out))
        except core.Cancelled:
            self.post(lambda: (setattr(self, "running", False), self.main_screen()))
        except core.BuildError as e:
            self.post(lambda: self.failed(str(e)))
        except Exception as e:   # never leave the user with a frozen window
            self.post(lambda: self.failed(t("err_generic", why=f"{type(e).__name__}: {e}")))

    # --- screen 3 -------------------------------------------------------------------------------------------
    def done(self, out):
        self.running = False
        f = self.new_frame()
        self.header(f)
        self.L(f, t("done_title"), "head", font=self.fonts["h2"]).pack(fill="x", pady=(12, 0))
        self.L(f, t("done_folder", path=out), wraplength=700).pack(fill="x", pady=(4, 10))
        for k in ("done1", "done2", "done3"):
            outer, box = self.card(f, padx=12, pady=8)
            outer.pack(fill="x", pady=5)
            self.L(box, t(k), "sel", wraplength=660, font=self.fonts["mid"]).pack(fill="x")
        self.L(f, t("done_note"), "dim", wraplength=700).pack(fill="x", pady=8)
        r = tk.Frame(f, bg=BG)
        r.pack(pady=6)
        Btn(r, t("open_out"), lambda: open_folder(out), self.big, pad=(16, 7), primary=True).pack(side="left", padx=6)
        Btn(r, t("close"), self.root.destroy, self.big, pad=(16, 7)).pack(side="left", padx=6)
        open_folder(out)

    def failed(self, msg):
        self.running = False
        f = self.new_frame()
        self.L(f, t("failed_title"), "err", font=self.fonts["h2"]).pack(fill="x")
        outer, _ = self.card(f)
        outer.pack(fill="both", expand=True, pady=10)
        txt = tk.Text(_, wrap="word", height=14, bg=INK, fg=TEXT, insertbackground=CREAM, bd=0, padx=10, pady=8,
                      font=self.fonts["norm"], selectbackground=BAR)
        txt.insert("1.0", msg)
        txt.config(state="disabled")
        txt.pack(fill="both", expand=True)
        r = tk.Frame(f, bg=BG)
        r.pack(pady=4)
        Btn(r, t("open_logs"), lambda: open_folder(core.default_cache() / "logs"), self.big, pad=(14, 6)).pack(side="left", padx=6)
        Btn(r, t("again"), self.main_screen, self.big, pad=(14, 6), primary=True).pack(side="left", padx=6)


def main():
    try:
        core.check_platform()
    except core.BuildError as e:
        strings.detect()
        r = tk.Tk()
        r.withdraw()
        messagebox.showerror(t("title", plat=t("plat_" + core.PLAT)), str(e))
        return 1
    if core.IS_WIN:   # crisp text on high-DPI screens
        try:
            import ctypes
            ctypes.windll.shcore.SetProcessDpiAwareness(1)
        except Exception:
            pass
    root = tk.Tk()
    App(root)
    root.mainloop()
    return 0

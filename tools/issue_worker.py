#!/usr/bin/env python3
"""Executor local e conservador de uma GitHub Issue aprovada por execução."""
import argparse
import ast
import fcntl
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent
OWNER = "IvandersonBordin"
REPO = "IvandersonBordin/handkerchief-vita"
ALLOWED_SUFFIXES = {".py", ".md", ".txt", ".json", ".toml", ".yml", ".yaml", ".sh", ".service", ".timer"}
FORBIDDEN = {"original-data", "build", ".git", ".env", "__pycache__", "dist"}
PROTECTED = {"AGENTS.md", "tools/issue_worker.py"}
MAX_FILES = 20
MAX_BYTES = 300_000


def cmd(args, *, check=True, input_text=None, timeout=120):
    return subprocess.run(args, cwd=ROOT, text=True, input=input_text,
                          capture_output=True, check=check, timeout=timeout)


def gh(*args):
    return cmd(["gh", *args]).stdout


def git(*args):
    return cmd(["git", *args]).stdout.strip()


def label(issue, name, *, remove=False):
    gh("issue", "edit", str(issue), "--remove-label" if remove else "--add-label", name)


def comment(issue, message):
    gh("issue", "comment", str(issue), "--body", message)


def fail(issue, message):
    print("BLOQUEADO:", message, file=sys.stderr)
    if issue:
        try:
            label(issue, "codex-ready", remove=True)
            label(issue, "codex-blocked")
            comment(issue, "Execução interrompida: " + message +
                    "\nVerifique os arquivos locais e o log em `build/` antes de reativar.")
        except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as exc:
            print("Não consegui atualizar a issue:", exc, file=sys.stderr)
    return 1


def validate_path(raw):
    path = Path(raw)
    if (path.is_absolute() or ".." in path.parts or
            any(part in FORBIDDEN or part.startswith(".env") for part in path.parts) or
            path.suffix.lower() not in ALLOWED_SUFFIXES):
        raise ValueError("Caminho não permitido: " + raw)
    candidate = ROOT / path
    resolved = candidate.resolve()
    if raw in PROTECTED or candidate.is_symlink() or not resolved.is_relative_to(ROOT):
        raise ValueError("Caminho fora da raiz: " + raw)
    if resolved.exists() and (not resolved.is_file() or resolved.stat().st_size > MAX_BYTES):
        raise ValueError("Arquivo grande ou especial: " + raw)


def collect_changes():
    tracked = git("diff", "--name-only").splitlines()
    untracked = git("ls-files", "--others", "--exclude-standard").splitlines()
    deleted = git("diff", "--name-only", "--diff-filter=D").splitlines()
    if deleted:
        raise ValueError("Exclusões não são publicadas automaticamente: " + ", ".join(deleted))
    paths = sorted(set(tracked + untracked))
    if len(paths) > MAX_FILES:
        raise ValueError("Excesso de arquivos modificados (" + str(len(paths)) + ")")
    for path in paths:
        validate_path(path)
    return paths


def original_snapshot():
    folder = ROOT / "original-data"
    if not folder.exists():
        return {}
    return {str(p.relative_to(folder)): (p.stat().st_size, p.stat().st_mtime_ns)
            for p in folder.rglob("*") if p.is_file()}


def process():
    # Falha fechada: o Codex ainda seria executado dentro da mesma raiz
    # gravável que contém os arquivos comerciais. Exigir isolamento real
    # antes de permitir uma execução desassistida.
    originals = ROOT / "original-data"
    if originals.exists():
        print("BLOQUEADO: original-data/ está dentro da área de trabalho do Codex. "
              "Configure um worktree isolado antes de executar --run.", file=sys.stderr)
        return 2
    for binary in ("gh", "git", "codex"):
        if not shutil.which(binary):
            print("Falta ferramenta:", binary, file=sys.stderr)
            return 2
    if not (ROOT / "AGENTS.md").is_file():
        print("Execute a partir do projeto com AGENTS.md", file=sys.stderr)
        return 2
    if git("branch", "--show-current") != "main":
        print("Esperada a branch main", file=sys.stderr)
        return 2
    # Proteção: não iniciar com alterações locais ou commits pendentes.
    if git("status", "--porcelain"):
        print("Há alterações locais. Resolva-as antes de ativar o executor.", file=sys.stderr)
        return 2
    gh("auth", "status")
    git("fetch", "origin", "main")
    if git("rev-parse", "HEAD") != git("rev-parse", "origin/main"):
        print("Git local e remoto divergentes ou sem sincronização. Não vou alterar.", file=sys.stderr)
        return 2
    raw = gh("issue", "list", "--repo", REPO, "--state", "open",
             "--label", "codex-ready", "--json", "number,title,author", "--limit", "10")
    issues = json.loads(raw)
    issues = [i for i in issues if i["author"]["login"].lower() == OWNER.lower()]
    if not issues:
        print("Nenhuma issue codex-ready criada pelo proprietário.")
        return 0
    issue = min(issues, key=lambda i: i["number"])["number"]
    info = json.loads(gh("issue", "view", str(issue), "--repo", REPO,
                         "--json", "title,body,author,labels"))
    labels = {x["name"] for x in info["labels"]}
    if "codex-ready" not in labels or "codex-running" in labels or "codex-done" in labels:
        print("Issue não elegível:", issue)
        return 0
    body = info["body"]
    if len(body) > 20_000:
        return fail(issue, "Descrição da issue excessivamente longa")
    try:
        label(issue, "codex-running")
        label(issue, "codex-ready", remove=True)
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired):
        print("Não consegui reservar a issue #", issue, file=sys.stderr)
        return 2
    logdir = ROOT / "build"
    logdir.mkdir(exist_ok=True)
    logfile = logdir / f"codex-issue-{issue}.log"
    head = git("rev-parse", "HEAD")
    originals_before = original_snapshot()
    prompt = (
        "Você é o executor local. Leia AGENTS.md e cumpra SOMENTE a issue abaixo. "
        "Trabalhe com Python stdlib e dados originais em modo somente leitura. "
        "NÃO execute HDKWIN.EXE, não instale pacotes, não altere original-data/, "
        "não faça commit, push, fetch, curl nem publique dados do jogo. "
        "Não execute comandos fora do projeto. Faça mudanças pequenas e testes locais. "
        "Se faltar um recurso, registre a limitação em documentação sem inventar resultados. "
        "Ao terminar, deixe os arquivos modificados para validação externa.\n\n"
        f"ISSUE #{issue}: {info['title']}\n\n{body}"
    )
    try:
        result = cmd(["codex", "exec", "--sandbox", "workspace-write", "-"],
                     input_text=prompt, check=False, timeout=2700)
        logfile.write_text(result.stdout + "\n\nSTDERR:\n" + result.stderr,
                           encoding="utf-8")
        if result.returncode:
            return fail(issue, f"Codex retornou código {result.returncode}")
        if git("rev-parse", "HEAD") != head:
            return fail(issue, "O agente alterou o histórico do Git")
        if original_snapshot() != originals_before:
            return fail(issue, "Arquivos em original-data/ foram alterados")
        paths = collect_changes()
        if not paths:
            label(issue, "codex-done")
            comment(issue, "Execução concluída sem alterações de código. Log local: " +
                    str(logfile.relative_to(ROOT)) + ".")
            return 0
        git("diff", "--check")
        for path in paths:
            if path.endswith(".py"):
                ast.parse((ROOT / path).read_text(encoding="utf-8"), filename=path)
        if (ROOT / "tests").is_dir():
            t = cmd([sys.executable, "-m", "unittest", "discover", "-s", "tests"],
                    check=False, timeout=300)
            if t.returncode:
                return fail(issue, "Testes unittest falharam; verifique log local")
        cmd(["git", "add", "--", *paths])
        git("diff", "--cached", "--check")
        staged = git("diff", "--cached", "--name-only").splitlines()
        if sorted(staged) != paths:
            return fail(issue, "Arquivos staged divergentes")
        # Commit automático somente depois dos limites e checagens objetivas.
        git("commit", "-m", f"chore: executa issue #{issue} com validação local")
        push = cmd(["git", "push", "origin", "main"], check=False, timeout=120)
        if push.returncode:
            return fail(issue, "Commit local feito, mas push falhou; intervenção necessária")
        label(issue, "codex-done")
        comment(issue, f"Executor concluído. Commit: `{git('rev-parse', '--short', 'HEAD')}`. "
                f"Arquivos: {', '.join(paths)}. Verificações: diff --check, sintaxe Python "
                "e unittest (quando disponível). Push concluído. "
                "Revisão técnica pelo líder ainda pendente.")
        print("Issue concluída:", issue)
        return 0
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired, ValueError,
            SyntaxError, OSError) as exc:
        return fail(issue, f"Validação/execução falhou: {type(exc).__name__}: {str(exc)[:250]}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", action="store_true",
                        help="autoriza a execução de uma issue pronta (padrão: só informação)")
    args = parser.parse_args()
    if not args.run:
        print("Modo seguro: nenhuma ação. Use --run para processar uma issue codex-ready.")
        return 0
    lock = ROOT / ".git" / "codex-issue-worker.lock"
    with lock.open("w") as fd:
        try:
            fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            print("Já existe uma execução em andamento.")
            return 0
        return process()


if __name__ == "__main__":
    raise SystemExit(main())

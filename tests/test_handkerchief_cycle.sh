#!/usr/bin/env bash
# O teste usa um Git temporário e executores simulados, sem modelo nem rede.
set -eu
repo=$(git rev-parse --show-toplevel)
fixture=$(mktemp -d)
trap 'python3 -c "import shutil,sys; shutil.rmtree(sys.argv[1])" "$fixture"' EXIT
mkdir -p "$fixture/bin" "$fixture/work/docs"
export XDG_STATE_HOME="$fixture/state" FIXTURE="$fixture"
cycle="$fixture/cycle"
sed -e "s|\$HOME/.local/bin/|$fixture/bin/|g" \
    -e "s|\$HOME/Projetos/handkerchief-vita-\$agent|$fixture/work|g" \
    "${1:-$repo/deploy/handkerchief-cycle}" > "$cycle"
cd "$fixture/work"
git init -q
git config user.name teste
git config user.email teste@localhost
git commit -qm inicio --allow-empty
cat > "$fixture/bin/handkerchief-agent" <<'AGENT'
#!/usr/bin/env bash
echo "$1" >> "$FIXTURE/calls"
if [[ -n "$CHANGE" ]]; then echo alterado >> "$FIXTURE/work/$CHANGE"; fi
echo "$REPLY"
echo "CONTINUAR: exemplo da transcrição" >&2
echo "=== FINALIZADO $1 ==="
date -Is
exit "$CODE"
AGENT
cat > "$fixture/bin/handkerchief-publish" <<'PUBLISH'
#!/usr/bin/env bash
set -eu
echo publicado >> "$FIXTURE/publications"
cd "$FIXTURE/work"
git add --all
if [[ "$PUBCODE" != 0 ]]; then exit "$PUBCODE"; fi
git diff --cached --quiet || git commit -qm teste
PUBLISH
chmod +x "$fixture/bin/"*
state="$XDG_STATE_HOME/handkerchief"
schedule="$state/bruno.schedule"
export CHANGE=code REPLY='CONTINUAR: próxima ação' CODE=0 PUBCODE=0
run() { bash "$cycle" bruno; }
calls() { wc -l < "$fixture/calls"; }
run
test "$(cat "$schedule")" = 'espera 0'
test "$(head -1 "$state/bruno.pending")" = "$(git rev-parse HEAD)"
run
test "$(calls)" = 1
cp "$state/bruno.pending" "$state/bruno.approved"
run
test "$(calls)" = 2
run
test "$(calls)" = 2
echo 'normal 0' > "$schedule"
CHANGE=docs/agent-progress.md run
run
test "$(calls)" = 3
test -s docs/agent-progress.md
test "$(wc -l < "$fixture/publications")" = 2
echo 'normal 0' > "$schedule"
CHANGE='' CODE=7 run && exit 1
run
test "$(calls)" = 4
echo 'normal 0' > "$schedule"
PUBCODE=8 run && exit 1
run
test "$(calls)" = 5
test -n "$(git diff --cached --name-only)"
echo 'normal 0' > "$schedule"
REPLY="ERROR: You've hit your usage limit." CODE=1 run
bash "$cycle" carlos
run
test "$(calls)" = 6
unlink "$state/usage-limit"
REPLY='AGUARDAR: teste físico' run
run
test "$(calls)" = 7
echo 'normal 0' > "$schedule"
REPLY='sem decisão' run
run
test "$(calls)" = 8
echo 'Agenda, espera, diário, falhas, cota compartilhada e retomada aprovados.'

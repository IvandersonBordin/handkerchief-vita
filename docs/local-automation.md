# Automação local controlada — Handkerchief PS Vita

Esta automação é **opt-in** e não é habilitada por um commit remoto. Ela
executa uma issue por vez por meio do Codex CLI no computador do usuário.
O ChatGPT no navegador **não** acompanha o processo continuamente nem
inicia novas tarefas automaticamente. O GitHub armazena as tarefas e os relatórios.

## Pré-requisitos

- Git e GitHub CLI (`gh`) autenticados para `IvandersonBordin`.
- Codex **CLI** disponível por `command -v codex` e autenticado. A extensão
  do VS Code sozinha não garante a instalação do executável de linha de comando.
- Python 3, systemd --user e repositório clonado em
  `~/Projetos/handkerchief-vita`. Não instale pacotes automaticamente.
- Working tree limpa e `main` sincronizada com `origin/main` antes de executar.
  O executor não faz pull automático de mudanças pendentes.

## Preparar, sem ativar automaticamente

No terminal do Manjaro, dentro do repositório:

```bash
git pull --ff-only origin main
command -v codex
codex --version
python3 tools/issue_worker.py
python3 -m py_compile tools/issue_worker.py
```

Se o Codex CLI não existir, **não ative o timer**. A instalação e autenticação
exigem uma etapa separada e consentimento.

O executor usa Bubblewrap para montar `original-data/` e `.git/` em modo somente leitura dentro de uma área de projeto gravável. Antes de executar o Codex, verifica se `HDKWIN.EXE` pode ser lido e não pode ser aberto para escrita. O Codex usa `--ephemeral` para reduzir gravações fora do projeto. Esse isolamento ainda precisa ser testado localmente com o Codex CLI nesta configuração.\n\nO executor só escolhe issues abertas do proprietário com o rótulo
`codex-ready`. Para criar os rótulos uma única vez:

```bash
for label in codex-ready codex-running codex-blocked codex-done; do
  gh label create "$label" --repo IvandersonBordin/handkerchief-vita --color D4C5F9 --force
done
```

Para autorizar a issue #1, após verificar seu conteúdo:

```bash
gh issue edit 1 --repo IvandersonBordin/handkerchief-vita --add-label codex-ready
```

Antes da execução, faça o teste de isolamento read-only com o Bubblewrap conforme a etapa de configuração. O modo seguro acima não valida credenciais do GitHub nem toda a integração com Codex.\n\nPara **testar manualmente apenas uma execução**, sem agendar:

```bash
python3 tools/issue_worker.py --run
```

Logs ficam em `build/codex-issue-N.log` e fora do Git. É possível verificar
o estado em `gh issue view 1 --repo IvandersonBordin/handkerchief-vita`.

## Ativar execução periódica (opcional; após teste manual aprovado)

```bash
mkdir -p ~/.config/systemd/user
cp deploy/handkerchief-worker.service deploy/handkerchief-worker.timer ~/.config/systemd/user/
systemctl --user daemon-reload
systemctl --user enable --now handkerchief-worker.timer
systemctl --user list-timers handkerchief-worker.timer
```

O timer verifica uma tarefa a cada ~15 minutos enquanto a sessão do usuário
e o computador estiverem ativos. Não é necessário deixar o VS Code aberto,
mas é necessário que o Codex CLI e a autenticação estejam disponíveis ao
serviço. **Não habilite linger** por padrão. A execução não continua com o
computador desligado ou suspenso.

Para pausar imediatamente:

```bash
systemctl --user disable --now handkerchief-worker.timer
```

Para acompanhar:

```bash
journalctl --user -u handkerchief-worker.service -n 100 --no-pager
```

## Segurança e limites conhecidos

- O executor exige `--run` e usa um lock local para impedir concorrência.
- Seleciona somente issues do proprietário marcadas `codex-ready`;
  reserva como `codex-running`, remove `codex-ready` e exige reautorização
  manual para tentativas bloqueadas.
- Executa `codex exec --ephemeral` com `workspace-write` dentro do Bubblewrap, limitado\n  a 45 minutos; não usa `danger-full-access`.
- Impede publicações com arquivos proibidos, executáveis, exclusões,
  arquivos grandes ou mudanças nos metadados dos originais detectadas;
  protege `AGENTS.md` e o próprio executor de alterações automáticas.
- Valida `git diff --check`, AST dos Python modificados, testes unittest
  quando disponíveis e revisa a lista de arquivos staged antes de publicar.
- O sistema **não é uma barreira de segurança absoluta**. O Bubblewrap protege\n  os originais de escrita durante a sessão isolada, mas o processo externo\n  controla os commits e a publicação. Metadados não detectam todas as possíveis\n  alterações; mantenha backup dos arquivos originais.
- A verificação automática é limitada: testes específicos de imagens e
  fidelidade visual ainda podem exigir revisão humana. Um commit publicado
  não significa aprovação técnica do ChatGPT.
- Issues são entradas de instrução: autorize apenas tarefas escritas/revisadas
  por você e pelo líder. Não use a automação para operações sensíveis,
  instalações, mudanças de firmware nem publicação de recursos comerciais.
- Se houver divergência Git, falha de Codex CLI, problema de autenticação,
  erro de validação ou push rejeitado, o processo para e registra bloqueio.
  Não faz force push nem reset de histórico.

# PLANO DE TRABALHO — CARLOS

## Missão
Desenvolver o protótipo nativo do Handkerchief para PS Vita,
com testes reproduzíveis e integração gradual.

## Ordem obrigatória
- C01: VitaSDK, toolchain, compilação e VPK válido.
- C02: teste no PS Vita físico, dependente de confirmação humana.
- C03: loop principal, tempo, entrada e estados do motor.
- C04: renderização de sprites, animações e cenários.
- C05: movimentação e colisões, dependente de B02 e B03.
- C06: personagens, combate, inimigos e progressão.
- C07: memória, desempenho e estabilidade.
- C08: versão experimental jogável e testes finais.

## Regras
1. Leia este plano e docs/agent-progress.md no início.
2. Retome a primeira tarefa não concluída.
3. Execute apenas uma tarefa pequena e verificável por ciclo.
4. Não repita validações já aprovadas sem motivo técnico.
5. Não declare compilação real com toolchain simulada.
6. Não declare teste físico sem confirmação do usuário.
7. Se faltar VitaSDK, documente o bloqueio e prepare
   somente tarefas independentes que sejam seguras.
8. Não invente física ou colisões do jogo original.
9. Não faça commit, push, merge, rebase ou reset.
10. Ao terminar, atualize docs/agent-progress.md.
11. Só marque CONCLUIDA com critérios verificáveis.
12. Não inclua recursos comerciais no repositório.

## Evidência de compilação real — 18/11/2026

O VitaSDK 2026.08 foi instalado em:
/home/ivanderson/vitasdk

A compilação manual ARM foi concluída com sucesso.

Diretório de build:
/home/ivanderson/Projetos/handkerchief-vita-build-manual

VPK gerado:
handkerchief_smoke.vpk

O pacote foi validado como ZIP íntegro, contendo eboot.bin,
param.sfo e recursos da LiveArea.

Próximas ações:
1. Reproduzir a compilação usando o VitaSDK real.
2. Executar os verificadores técnicos do VPK.
3. Registrar C01 como concluída somente após essas verificações.
4. Preparar C02 para teste no console físico.
5. Não declarar C02 concluída sem confirmação do usuário.

Nunca publicar arquivos comerciais do jogo.

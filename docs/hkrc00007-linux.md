# Teste Linux da rodada híbrida HKRC00007

A [issue #4](https://github.com/IvandersonBordin/handkerchief-vita/issues/4#issuecomment-7043183524) registra uma execução anterior com GCC, avisos como erros e UBSan. O escopo é a rodada HKRC00007, com três obstáculos e movimentação sintética. A fidelidade à física original de A0 fica fora deste teste.

## Pré-verificação

No worktree de Carlos, execute o bloco abaixo. Ele identifica o compilador, lista apenas nomes de arquivos candidatos e confere a disponibilidade da árvore local apontada para o teste. A primeira linha permite indicar outro caminho de leitura com a variável `HKRC_A0_SRC`.

```sh
(
  src=${HKRC_A0_SRC:-../handkerchief-vita/build/playable-a0}
  if [ ! -d "$src" ] || [ ! -r "$src" ] || [ ! -x "$src" ]; then
    printf 'Fonte local do HKRC00007 indisponível em %s\n' "$src" >&2
    exit 2
  fi
  gcc --version || exit 3
  find "$src" -type f \( -name '*.c' -o -name '*.h' -o \
    -name 'CMakeLists.txt' -o -name 'Makefile' -o -iname 'README*' \) -print
)
```

Confirme na árvore listada quais fontes implementam a rodada e qual é o ponto de entrada do teste Linux. Use a configuração de compilação do próprio teste, com `-Wall -Wextra -Werror -fsanitize=undefined -fno-sanitize-recover=all`. Direcione o executável e eventuais dados sintéticos para um diretório criado com `mktemp -d`. Registre o comando completo após confirmá-lo nos fontes. Uma listagem vazia, um VPK sozinho ou a ausência de um ponto de entrada para o teste deixam a reprodução pendente.

## Critérios para o teste

Use a lógica da própria rodada e entradas controladas por passos. Gere no teste qualquer dado necessário para isolar a lógica. Registre o estado inicial antes da primeira entrada.

| Caso | Evidência esperada |
| --- | --- |
| Direita | A coordenada horizontal aumenta em um trecho livre. |
| Esquerda | Após avançar em trecho livre, inverter a entrada diminui a coordenada horizontal. |
| Obstáculos e salto | Para cada um dos três obstáculos, avançar no chão bloqueia a passagem. X inicia a subida e, com avanço, o robô cruza o obstáculo e aterrissa. |
| Chegada | Cruzar a marca de saída muda o estado da rodada para concluída. |
| Reinício | Após a chegada, soltar e pressionar X restaura a posição inicial e limpa o estado de conclusão. |

O teste deve indicar qual caso falhou e retornar código diferente de zero se uma checagem falhar. Para registrar aprovação, exija evidência dos cinco casos, código de saída zero e execução sem diagnóstico do UBSan.

## Marcos de apresentação

Na amostra de `gameplay.mp4` entre 00:20 e 03:50, a casa serve de referência para o enquadramento inicial (cerca de 00:20–00:50). A partir de cerca de 01:05–01:20, o terreno elevado dá lugar ao desnível. Por volta de 03:20–03:35, a visão passa do túnel para uma área aberta. O HUD permanece na faixa inferior. Na comparação visual futura, observe o que sai pelas bordas do quadro ao avançar e voltar, a leitura do piso durante o salto e o retorno do enquadramento ao reiniciar. Na descrição do HKRC00007, os obstáculos azuis, a linha do piso e a chegada verde são as referências de apresentação da rodada.

Nesta sessão, faltou acesso a `../handkerchief-vita/build/playable-a0`. O teste Linux da rodada e a comparação visual com o protótipo seguem pendentes.

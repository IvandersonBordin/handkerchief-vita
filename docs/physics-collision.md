# Coordenadas de CCastMK

Análise estática da cópia local de `HDKWIN.EXE` (PE32 x86, SHA-256
`3deeb9b18ac2f6ee7b4af4769d1499fdfe0bde8de44482b57ab4bf6196617b76`).
Os endereços das instruções são virtuais. Os campos com prefixo `+`
são deslocamentos em memória.

## Origem e retângulo em memória

1. O descritor em `0x4718C8` aponta para o nome `CCastMK` em
   `0x488C98`. A rotina em `0x445BB6` retorna esse descritor.
   O caminho de inicialização em `0x445B82` instala a tabela de métodos
   em `0x471EB0` e, em `0x445B88`, chama
   `0x401BDB → 0x445CD2`.
2. Em `0x445CF4`, o inicializador passa `ator+0x18` como base de uma
   estrutura de coordenadas. Em `0x445D5E`, ele chama
   `0x40321F → 0x410E16` com quatro valores locais
   `(-256, -64, 256, 64)`. A rotina guarda os dois primeiros
   como deslocamentos e subtrai cada limite inferior do superior
   para obter as dimensões 512 × 128, nas unidades internas.
3. `0x410E36` chama `0x401D7F → 0x410E48`. Essa rotina soma
   os deslocamentos locais ao par de origem e escreve os quatro
   limites na estrutura. Convertidos para offsets relativos ao ator,
   os campos ficam assim.

| Campos do ator | Operação no caminho observado |
| --- | --- |
| `+0x18`, `+0x1C` | Origem X e Y usada como entrada. |
| `+0x20`, `+0x24` | Dimensões calculadas pela diferença entre os limites locais. |
| `+0x28`, `+0x2C` | Deslocamentos locais da esquerda e do topo. |
| `+0x30`, `+0x34` | Esquerda e topo, somando origem e deslocamento. |
| `+0x38`, `+0x3C` | Direita e base, somando as dimensões à esquerda e ao topo. |

## Leitura da origem

A chamada em `0x43DD6F` passa o ator e um endereço na pilha para
`0x401320 → 0x445FEA`. No caminho em `0x446014`, a rotina
copia `+0x18` e `+0x1C` ao par de saída. Se `+0x78` contém um
ponteiro e o bit `0x20` de `+0x80` está ativo, o X ainda vem de
`+0x18` e o Y vem da chamada indireta em `0x445FFF` com os dois
bits inferiores limpos.

O uso do retângulo em colisões, as unidades das coordenadas, a origem
do Y alternativo e a ligação com `STAGE_0` ou `CRock` seguem em
aberto. O próximo recorte de B02 é identificar um escritor de
`+0x18` ou `+0x1C` fora da inicialização e verificar de onde vem
o valor escrito.

## Reprodução local

Execute na raiz do worktree. `objdump` lê as seções em disco e
`sha256sum` identifica a versão analisada.

```bash
exe=/home/ivanderson/Projetos/handkerchief-vita/original-data/HDKWIN.EXE
sha256sum "$exe"
objdump -s --start-address=0x4718c8 --stop-address=0x4718d0 "$exe"
objdump -s --start-address=0x488c98 --stop-address=0x488ca0 "$exe"
for intervalo in 445b82:445bbc 401bdb:401be0 445cd2:445d67 \
                 40321f:403224 410e16:410e3e 401d7f:401d84 \
                 410e48:410e6e 43dd53:43dd74 401320:401325 \
                 445fea:446027; do
    objdump -d -Mintel --start-address="0x${intervalo%:*}" \
        --stop-address="0x${intervalo#*:}" "$exe"
done
```

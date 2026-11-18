# Mapa técnico do motor de Handkerchief

O objetivo do projeto é desenvolver um port nativo de Handkerchief para
PlayStation Vita. Este mapa reúne resultados da análise estática de
`HDKWIN.EXE` e dos arquivos originais. A viabilidade do port segue em aberto.

## Arquivos HDK

Os HDK contêm blocos consecutivos. Cada bloco começa com um comprimento de
32 bits em little-endian que inclui o próprio campo, seguido pelo rótulo
da seção e pelo nome de classe, terminados em zero. A
[documentação do formato](hdk-format.md) descreve as dez seções das amostras
e a validação estrutural dos 99 arquivos com conteúdo. A estrutura dos
registros internos foi investigada apenas em parte.

## Funções identificadas em HDKWIN.EXE

Os endereços são virtuais e se referem à cópia analisada do executável
PE32 x86. Os nomes `fread` e `fseek` descrevem o comportamento observado
na desmontagem. São identificações funcionais de rotinas sem símbolos.

| Endereço | Responsabilidade observada |
|---|---|
| `0x408159` | Busca uma seção pelo rótulo. Lê o comprimento do bloco e o soma ao deslocamento atual para avançar. |
| `0x45D930` | Leitura do fluxo em elementos de tamanho e quantidade informados, compatível com `fread`. |
| `0x45DB00` | Reposicionamento do fluxo, compatível com `fseek`. |
| `0x42B439` | Método de serialização de `CEarth`. Passa o nome da classe ao leitor comum e delega o conteúdo à rotina compartilhada. |
| `0x4125E2` | Serialização compartilhada. O ramo de leitura em `0x41271C` consome os campos conforme a máscara presente no fluxo. |
| `0x40AC63` | Lê a lista com 12 bytes por elemento e acrescenta uma entrada zerada em memória. |
| `0x412F27` | Passa a lista e a base da referência a `0x40963E` e desloca o retângulo obtido pela posição do objeto. |
| `0x40963E` | Consulta as dimensões e calcula a união dos retângulos dos elementos. |
| `0x451629` | Inicializa os bancos globais com endereços de tabelas embutidas no executável. |
| `0x4120F9` | Grava o ponteiro da tabela em `descritor+0x04`. |

## Estrutura parcial de CEarth

### Campos verificados

Os deslocamentos abaixo são relativos ao objeto em memória. No arquivo,
os grupos são escritos em sequência quando seus bits estão presentes.
A rotina também testa uma máscara do chamador para decidir se guarda ou
apenas consome os dados.

| Condição no fluxo | Dados consumidos | Destino ou efeito observado |
|---|---|---|
| Campo inicial | Um valor de 32 bits | `+0x14`. |
| Bit `0x02` | Dois valores de 32 bits | `+0x18` e `+0x1C` quando armazenados. O par é usado para transladar o retângulo do objeto. |
| Bit `0x04` | Quatro valores de 32 bits | `+0x20` a `+0x2C` quando armazenados. O cálculo em `0x410E48` usa esses valores como dimensões e deslocamentos locais. |
| Bit `0x08` | Base de 32 bits, contagem de 16 bits e elementos de 12 bytes | Base em `+0x40` e ponteiro da lista em `+0x44` no caminho que mantém os dados. |
| Bit `0x01` | Contagem de 16 bits seguida da leitura de objetos | Controla um laço que pode anexar os objetos lidos à coleção. |

Em `A0.HDK`, o primeiro `CEarth` de `VISTA_0` tem posição `(524, 556)`,
base de referência zero e cinco elementos entre `0x0704` e `0x0740`
(fim exclusivo). Esses offsets são posições no arquivo HDK.

### Interpretações e lacunas

O uso na aritmética de retângulos confirma o papel geométrico dos pares
de posição e deslocamento. As unidades e o papel desses limites na
colisão continuam em aberto. O significado do campo `+0x14`, a semântica
completa das máscaras e a função da coleção de objetos ainda precisam
ser estabelecidos.

## Lista de elementos e bancos de dimensões

Cada elemento tem três valores de 32 bits em little-endian.

| Offset no elemento | Uso observado |
|---|---|
| `+0x00` | Referência compactada a uma entrada de um banco de dimensões. O consumidor encerra a lista em memória quando encontra uma referência bruta zero. |
| `+0x04` | Deslocamento local X, interpretado com sinal. |
| `+0x08` | Deslocamento local Y, interpretado com sinal. |

`0x40963E` soma a base de `CEarth+0x40` à referência antes de extrair
o **banco nos bits 26–29** e o **índice nos bits 0–14**. A consulta usa
o ponteiro em `0x4ABD74 + banco × 0x14`. Cada entrada ocupa 8 bytes,
com largura e altura de 16 bits com sinal em `+0x04` e `+0x06`.

| Banco | Endereço da tabela em `.data` | Endereço do ponteiro global |
|---:|---|---|
| 0 | `0x48A3D8` | `0x4ABD74` |
| 1 | `0x48AFF8` | `0x4ABD88` |
| 2 | `0x48B7A8` | `0x4ABD9C` |

As dimensões abaixo foram lidas diretamente do banco 2 no executável.
O índice 556 é usado duas vezes em posições diferentes no primeiro
`CEarth` de `A0.HDK`.

| Índice | Largura | Altura |
|---:|---:|---:|
| 564 | 184 | 40 |
| 556 | 216 | 176 |
| 555 | 216 | 88 |
| 558 | 184 | 176 |

### Associação com recursos gráficos

As dimensões dos bancos verificados já estão presentes no executável.
Uma hipótese é que as entradas descrevam regiões de imagens BMP. Para
confirmá-la, falta seguir a referência do banco e do índice até uma
operação de desenho que identifique o bitmap e a região de origem.
Também falta determinar o significado dos primeiros quatro bytes da
entrada e dos bits que o cálculo dos limites ignora, incluindo o bit 15.

## Próximos marcos técnicos

1. Rastrear o banco 2 até uma chamada de desenho e verificar, com uma
   entrada conhecida, qual imagem e qual região são usadas.
2. Validar de ponta a ponta a leitura de registros `CEarth` em mais de um
   HDK, cobrindo o grupo opcional `0x04`, listas de elementos e objetos
   aninhados. Usar os limites das seções como verificação independente.
3. Demonstrar em desktop a composição de um trecho de fase a partir dos
   dados do usuário e conferir as posições e os retângulos calculados.
   Usar o resultado e a dependência das rotinas x86 de Win32 para decidir
   entre reaproveitar código autorizado, traduzir rotinas ou
   reimplementar o motor.
4. Com a rota técnica definida, validar uma cena mínima no PS Vita
   e medir memória e tempo por quadro no aparelho antes de ampliar o
   escopo para jogabilidade.

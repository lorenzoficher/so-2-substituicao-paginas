# Simulador de Substituição de Páginas

Trabalho de Sistemas Operacionais (UNIPAMPA). Mede o número de falhas de página das
políticas **FIFO**, **OPT** e **LRU aproximado** (bit de referência + N bits de
histórico, envelhecidos a cada I acessos) sobre traces reais de acesso à memória,
variando o número de frames. Também conta as escritas de volta causadas por vítimas
sujas.

> Em construção.

## Pré-requisitos (WSL Ubuntu)

```bash
sudo apt install -y g++ make python3-matplotlib
```

## Traces

Os traces não estão no repositório. Baixe-os para `traces/` (use **HTTPS** — por
HTTP o endereço é bloqueado):

```bash
mkdir -p traces
for t in bzip gcc sixpack swim bigone; do
    wget -P traces "https://www.inf.unioeste.br/~marcio/SO/trace/$t.trace"
done
```

### O bigone é a concatenação dos outros quatro

Os quatro traces menores têm 1 000 000 de acessos cada; o `bigone` tem 4 000 000. Cada
bloco de 1M de linhas do `bigone` é idêntico, byte a byte, a um dos outros traces, na
ordem `bzip`, `gcc`, `sixpack`, `swim` (todos com fim de linha LF, sem CRLF):

```bash
cd traces
wc -l *.trace
md5sum bzip.trace gcc.trace sixpack.trace swim.trace bigone.trace
for i in 0 1 2 3; do
    sed -n "$((i*1000000+1)),$(((i+1)*1000000))p" bigone.trace | md5sum
done
cat bzip.trace gcc.trace sixpack.trace swim.trace | md5sum   # igual ao md5 do bigone.trace acima
```

| Bloco do bigone | Linhas              | md5                                | Trace igual |
|-----------------|---------------------|------------------------------------|-------------|
| 1               | 1 – 1 000 000       | `599135f701c07b88c01053c4806f001e` | `bzip`      |
| 2               | 1 000 001 – 2M      | `c11811d97de4fc16f816d4dcddb1bcba` | `gcc`       |
| 3               | 2 000 001 – 3M      | `149a73f7fab839e378a408f43a7dc589` | `sixpack`   |
| 4               | 3 000 001 – 4M      | `4fa8d14c991c1c5408c949f9b2fa823b` | `swim`      |

O arquivo inteiro (`386b03adda7ee1bcf2a00138f6ce0a21`) também bate com o md5 da
concatenação dos quatro.

Consequência: o `bigone` **não é um quinto programa independente**, e sim uma carga
com **trocas de fase** — a cada 1M de acessos o conjunto de páginas em uso muda de
programa. Por isso ele fica **fora da média da varredura exploratória** (como já
previa o ADR 0002) e a análise deve tratá-lo à parte, como carga com trocas de fase.

## Uso

```bash
make          # compila → build/sim
make test     # roda os testes
make grid     # simula a grade de experimentos/grid.conf → results/<trace>.csv
make plots    # gráficos → results/figuras/
```

Simulação avulsa:

```bash
./build/sim traces/gcc.trace fifo --frames 4,8,16
./build/sim traces/gcc.trace lru-approx --bits 8 --interval 1000 --frames 4,8,16
```

## Resultados e análise

_A preencher após rodar a grade._

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

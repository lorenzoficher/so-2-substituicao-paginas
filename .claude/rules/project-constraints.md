# Restrições deste projeto

O que **não pode mudar** e não se descobre lendo o código. Modelo de domínio em
[`CONTEXT.md`](../../CONTEXT.md); decisões e porquês em [`docs/adr/`](../../docs/adr/).

## Stack

- **Simulador em C++17 puro** (só a STL). Compila com `g++ -std=c++17 -O2` via
  Makefile no **WSL Ubuntu** — o professor precisa compilar sem instalar nada além de
  g++ e make.
- **Testes com doctest** (header único versionado em `tests/`), escritos antes da
  implementação (TDD).
- **Python só para gráficos** (matplotlib), lendo os CSV. Nenhuma lógica de
  simulação em Python.

## Interface e saída

- **CLI**: trace, política, lista de números de frames, N e I entram por argumento.
- **Saída em CSV** no stdout, uma linha por simulação; `make grid` grava
  `results/<trace>.csv`.
- Valores da grade **só** em `experiments/grid.conf`.

## Semântica da simulação

- **Determinismo:** mesma entrada → mesmo resultado. Sem aleatoriedade.
- Página de 4096 bytes e endereços de 32 bits, fixos.
- Falhas compulsórias contam. Páginas sujas ao fim do trace não geram escrita de
  volta.
- **Só o OPT enxerga o futuro** do trace.
- Nenhuma política usa o estado sujo/limpo para escolher a vítima.

## Entrega

- Só o programa — **sem artigo**. A interpretação dos resultados vai no `README.md`.
- Trabalho **individual**: o repositório de referência
  (`PPrauchner/SO-Trabalho-2-Paginacao`) serve só para estrutura e decisões de
  domínio. Não copiar código, resultados nem texto dele.

## Repositório

- Traces (`traces/`) ficam **fora do git**. Resultados (`results/`) são versionados.

# CLAUDE.md — SO Trabalho 2: Substituição de Páginas

Trabalho de Sistemas Operacionais (UNIPAMPA), individual. Simulador que mede o
número de falhas de página de três políticas de substituição — **FIFO**, **OPT** e
**LRU aproximado** (bit de referência + N bits de histórico, envelhecidos a cada I
acessos) — sobre traces de acesso à memória, variando o número de frames. Métrica
secundária: escritas de volta (vítimas sujas).

Entrega: **só o programa** (sem artigo). A interpretação dos resultados fica na
seção "Resultados e análise" do `README.md`.

Enunciado: `trabalho_memoria.md` (baseado em `https://www.inf.unioeste.br/~marcio/SO/Trabalho2Bim.pdf`,
que pede só OPT e LRU aproximado — o FIFO é a terceira política escolhida aqui).

## Stack

- **Simulador:** C++17 (só STL), g++ + Makefile, no **WSL Ubuntu**. Emite CSV.
- **Testes:** doctest (header único em `tests/`), escritos antes do código (TDD).
- **Gráficos:** Python 3 + matplotlib (`scripts/`), também no WSL. Só lê CSV —
  nenhuma lógica de simulação em Python. Ver `docs/adr/0001`.

## Trace

Uma linha por acesso: `<endereço hex 32 bits> <R|W>` (ex.: `31348900 W`).
Página de 4096 bytes → número da página = `endereço >> 12`.

## Estrutura

```
src/
  trace.{hpp,cpp}       lê o arquivo → vector<Access>{page, is_write}
  policy.hpp            interface Policy: access(page) → falha? vítima?
  fifo.{hpp,cpp}        class Fifo
  opt.{hpp,cpp}         class Opt (recebe o trace inteiro; próximo uso pré-computado)
  lru_approx.{hpp,cpp}  class LruApprox (recebe N e I)
  simulator.{hpp,cpp}   run(trace, policy) → {falhas, escritas de volta}
  main.cpp              CLI + CSV no stdout
tests/                  doctest
experiments/grid.conf   ÚNICO lugar com os valores da grade (traces, frames, pares N:I)
scripts/                run_grid.sh (make grid) e plot.py (make plots)
results/                CSV por trace + figuras/ (versionados)
traces/                 traces de entrada — NÃO versionados (ver README)
docs/                   adr/, agents/
```

Regras de desenho:
- Cada política é dona dos seus frames e metadados e só decide substituição.
- O **simulator** concentra a contagem: falhas e escritas de volta. O conjunto de
  páginas sujas vive nele, não nas políticas.
- Só o OPT enxerga o futuro do trace.

## CLI e CSV

```bash
./build/sim <trace> fifo|opt --frames 4,8,16
./build/sim <trace> lru-approx --bits 8 --interval 1000 --frames 4,8,16
```

```
trace,policy,frames,history_bits,aging_interval,accesses,page_faults,writebacks
```

`history_bits` e `aging_interval` vazios fora do LRU aproximado.

## Comandos (no WSL)

```bash
make          # compila → build/sim
make test     # compila e roda os testes
make grid     # roda a grade de experimentos → results/<trace>.csv
make plots    # gera results/figuras/*.png a partir dos CSV
```

## Semântica (não negociável)

- **Determinismo:** mesma entrada → mesmo resultado. Sem aleatoriedade.
- Página de 4096 bytes, endereços de 32 bits, fixos.
- Falhas compulsórias entram na contagem. Páginas sujas ao fim do trace não geram
  escrita de volta.
- Desempates do LRU aproximado conforme o `CONTEXT.md`.

## Documentação

- `CONTEXT.md` — glossário de domínio. Usar os termos dele em código, issues e testes.
- `docs/adr/` — decisões e porquês.
- `.claude/rules/` — restrições e calibragem deste projeto.

## Agent skills

### Issue tracker

Issues no GitHub (repositório privado, via `gh`). See `docs/agents/issue-tracker.md`.

### Triage labels

Vocabulário padrão (needs-triage, needs-info, ready-for-agent, ready-for-human, wontfix). See `docs/agents/triage-labels.md`.

### Domain docs

Single-context: `CONTEXT.md` + `docs/adr/` na raiz. See `docs/agents/domain.md`.

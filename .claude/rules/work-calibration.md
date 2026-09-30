# Calibragem de Trabalho

## Quebra de trabalho

A unidade é a **área**: `src/` (simulador), `scripts/` + `experiments/` (grade e
gráficos), `README.md` (análise). Dentro de `src/`, cada política conta como módulo
próprio.

- Implementar **uma** política junto com seus testes = implementação direta.
- Issue que toca **duas ou mais políticas**, ou cruza áreas = plano de sub-tarefas.

## Camadas deste projeto

Este projeto **não é organizado em camadas** — vale "um domínio por commit".
Domínios: leitor de trace, simulator (contagem), cada política (FIFO, OPT, LRU
aproximado), CLI/CSV, grade, gráficos, análise.

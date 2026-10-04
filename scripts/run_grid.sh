#!/usr/bin/env bash
# Roda a grade de experimentos (`make grid`): para cada trace de
# experiments/grid.conf, simula FIFO e OPT em todos os números de frames e o
# LRU aproximado com cada par N:I, gravando results/<trace>.csv.
#
# Os valores da grade vivem só em experiments/grid.conf. As variáveis GRID_CONF,
# TRACES_DIR, RESULTS_DIR e SIM trocam os caminhos padrão (usadas pelos testes).

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
GRID_CONF="${GRID_CONF:-$ROOT/experiments/grid.conf}"
TRACES_DIR="${TRACES_DIR:-$ROOT/traces}"
RESULTS_DIR="${RESULTS_DIR:-$ROOT/results}"
SIM="${SIM:-$ROOT/build/sim}"

# shellcheck source=../experiments/grid.conf
source "$GRID_CONF"

frames_list="$(echo $FRAMES | tr ' ' ',')"

mkdir -p "$RESULTS_DIR"

# O CSV é montado num temporário e só vira results/<trace>.csv quando todas as
# simulações do trace terminam bem; se alguma falhar, o trap apaga o temporário.
tmp=""
trap 'rm -f "$tmp"' EXIT

for trace in $TRACES; do
    trace_file="$TRACES_DIR/$trace.trace"
    csv="$RESULTS_DIR/$trace.csv"
    if [[ ! -f "$trace_file" ]]; then
        echo "aviso: $trace_file não encontrado — trace '$trace' pulado (ver README)" >&2
        continue
    fi
    tmp="$(mktemp "$RESULTS_DIR/.$trace.csv.XXXXXX")"
    chmod 644 "$tmp"
    {
        "$SIM" "$trace_file" fifo --frames "$frames_list"
        "$SIM" "$trace_file" opt --frames "$frames_list" | tail -n +2
        for pair in $LRU_PAIRS; do
            "$SIM" "$trace_file" lru-approx --bits "${pair%%:*}" --interval "${pair##*:}" \
                --frames "$frames_list" | tail -n +2
        done
    } > "$tmp"
    mv "$tmp" "$csv"
    echo "$csv" >&2
done

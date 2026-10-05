#!/usr/bin/env bash
# Testes de ponta a ponta de scripts/run_grid.sh (rodados por `make test`).
#
# Cada teste monta uma grade pequena num diretório temporário — grid.conf, traces/
# e results/ próprios, apontados pelas variáveis GRID_CONF, TRACES_DIR e
# RESULTS_DIR — e confere os CSV gerados.

set -u

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RUN_GRID="$ROOT/scripts/run_grid.sh"
HEADER="trace,policy,frames,history_bits,aging_interval,accesses,page_faults,writebacks"

failures=0

fail() {
    echo "FALHOU: $1" >&2
    failures=$((failures + 1))
}

# Cria um diretório de trabalho com grid.conf, traces/belady.trace e results/.
setup() {
    WORK="$(mktemp -d)"
    mkdir -p "$WORK/traces" "$WORK/results"
    for page in 1 2 3 4 1 2 5 1 2 3 4 5; do
        printf '%x R\n' $((page << 12))
    done > "$WORK/traces/belady.trace"
    cat > "$WORK/grid.conf" <<'EOF'
TRACES="belady"
FRAMES="3 4"
LRU_PAIRS="8:2 2:3"
EOF
    export GRID_CONF="$WORK/grid.conf" TRACES_DIR="$WORK/traces" RESULTS_DIR="$WORK/results"
}

teardown() {
    rm -rf "$WORK"
    unset GRID_CONF TRACES_DIR RESULTS_DIR SIM
}

test_grade_gera_um_csv_por_trace_com_um_cabecalho() {
    setup
    bash "$RUN_GRID" 2>/dev/null || fail "run_grid.sh saiu com erro"
    local csv="$WORK/results/belady.csv"
    if [[ ! -f "$csv" ]]; then
        fail "results/belady.csv não foi gerado"
    else
        [[ "$(grep -c '^trace,' "$csv")" == 1 ]] || fail "CSV deve ter um único cabeçalho"
        [[ "$(head -n 1 "$csv")" == "$HEADER" ]] || fail "primeira linha deve ser o cabeçalho"
        # 2 frames × (fifo + opt + 2 pares N:I) = 8 linhas de dados.
        [[ "$(wc -l < "$csv")" == 9 ]] || fail "esperadas 8 linhas de dados"
        grep -qx 'belady,fifo,3,,,12,9,0' "$csv" || fail "falta a linha do FIFO com 3 frames"
        grep -q '^belady,opt,4,' "$csv" || fail "falta a linha do OPT com 4 frames"
        grep -q '^belady,lru-approx,3,2,3,' "$csv" || fail "falta a linha do par N:I 2:3"
    fi
    teardown
}

test_trace_ausente_gera_aviso_e_e_pulado() {
    setup
    sed -i 's/^TRACES=.*/TRACES="ausente belady"/' "$GRID_CONF"
    local err
    err="$(bash "$RUN_GRID" 2>&1 >/dev/null)" || fail "trace ausente não deve interromper a grade"
    [[ "$err" == *ausente* ]] || fail "trace ausente deve gerar aviso no stderr"
    [[ ! -e "$WORK/results/ausente.csv" ]] || fail "trace ausente não deve gerar CSV"
    [[ -f "$WORK/results/belady.csv" ]] || fail "os demais traces devem seguir"
    teardown
}

test_falha_do_simulador_nao_deixa_csv_pela_metade_nem_tmp() {
    setup
    # Simulador que funciona para FIFO e OPT e falha no LRU aproximado, depois de
    # já ter impresso parte da saída.
    cat > "$WORK/sim" <<EOF
#!/usr/bin/env bash
if [[ "\$2" == lru-approx ]]; then echo "$HEADER"; exit 1; fi
exec "$ROOT/build/sim" "\$@"
EOF
    chmod +x "$WORK/sim"
    export SIM="$WORK/sim"
    echo "csv anterior" > "$WORK/results/belady.csv"

    if bash "$RUN_GRID" 2>/dev/null; then
        fail "falha do simulador deve fazer run_grid.sh sair com erro"
    fi
    [[ "$(cat "$WORK/results/belady.csv")" == "csv anterior" ]] \
        || fail "CSV anterior não deve ser trocado por um CSV pela metade"
    [[ -z "$(ls -A "$WORK/results" | grep -v '^belady.csv$')" ]] \
        || fail "não deve sobrar temporário em results/"
    teardown
}

test_rodar_duas_vezes_produz_csv_identicos() {
    setup
    bash "$RUN_GRID" 2>/dev/null
    cp "$WORK/results/belady.csv" "$WORK/primeira.csv"
    bash "$RUN_GRID" 2>/dev/null
    cmp -s "$WORK/primeira.csv" "$WORK/results/belady.csv" || fail "duas rodadas devem dar CSV idênticos"
    teardown
}

test_funciona_a_partir_de_outro_diretorio() {
    setup
    # SIM fica no padrão: o script deve achar build/sim pela raiz do repositório,
    # e não pelo diretório corrente.
    (cd "$WORK" && bash "$RUN_GRID" 2>/dev/null) \
        || fail "run_grid.sh deve funcionar fora da raiz do repositório"
    [[ -f "$WORK/results/belady.csv" ]] || fail "CSV deve ser gerado rodando de outro diretório"
    teardown
}

test_grade_gera_um_csv_por_trace_com_um_cabecalho
test_rodar_duas_vezes_produz_csv_identicos
test_funciona_a_partir_de_outro_diretorio
test_falha_do_simulador_nao_deixa_csv_pela_metade_nem_tmp
test_trace_ausente_gera_aviso_e_e_pulado

if ((failures > 0)); then
    echo "test_grid.sh: $failures falha(s)" >&2
    exit 1
fi
echo "test_grid.sh: ok"

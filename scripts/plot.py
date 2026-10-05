"""
Gera os gráficos da análise (`make plots`) a partir dos CSV de results/.

Responsabilidades:
- Ler de experiments/grid.conf os traces e o par de referência (primeiro par de
  LRU_PAIRS); os demais valores vêm dos CSV.
- Validar results/<trace>.csv: ausente, vazio ou sem o par de referência é erro.
- Gerar, por trace, em results/figuras/: falhas de página × frames, experimento de
  sensibilidade (N e intervalo de envelhecimento) e escritas de volta × frames.

Nenhuma simulação acontece aqui — o script só lê CSV (ver docs/adr/0001).
"""

from __future__ import annotations

import csv
import shlex
import sys
from dataclasses import dataclass
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
from matplotlib.ticker import NullLocator  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent

POLICY_LABELS = {"fifo": "FIFO", "opt": "OPT", "lru-approx": "LRU aproximado"}


class PlotError(Exception):
    """Entrada inválida para os gráficos (grid.conf ou CSV)."""


@dataclass(frozen=True)
class Row:
    """Uma linha do CSV: uma simulação.

    Attributes:
        policy: Política de substituição (fifo, opt, lru-approx).
        frames: Número de frames.
        bits: Bits de histórico (N); None fora do LRU aproximado.
        interval: Intervalo de envelhecimento (I); None fora do LRU aproximado.
        faults: Falhas de página.
        writebacks: Escritas de volta.
    """

    policy: str
    frames: int
    bits: int | None
    interval: int | None
    faults: int
    writebacks: int


@dataclass(frozen=True)
class GridConf:
    """O que os gráficos usam de experiments/grid.conf.

    Attributes:
        traces: Nomes dos traces.
        reference: Par de referência (N, I) — primeiro par de LRU_PAIRS.
    """

    traces: list[str]
    reference: tuple[int, int]


def read_grid_conf(path: Path) -> GridConf:
    """Lê TRACES e o primeiro par de LRU_PAIRS de um grid.conf.

    Args:
        path: Caminho do grid.conf (atribuições no formato do shell).

    Returns:
        Os traces e o par de referência.

    Raises:
        PlotError: Arquivo ausente, ou TRACES / LRU_PAIRS ausentes ou malformados.
    """
    if not path.is_file():
        raise PlotError(f"{path} não encontrado")
    values: dict[str, str] = {}
    for line in path.read_text().splitlines():
        tokens = shlex.split(line, comments=True)
        if len(tokens) == 1 and "=" in tokens[0]:
            key, value = tokens[0].split("=", 1)
            values[key] = value
    traces = values.get("TRACES", "").split()
    pairs = values.get("LRU_PAIRS", "").split()
    if not traces:
        raise PlotError(f"{path}: TRACES vazio ou ausente")
    if not pairs:
        raise PlotError(f"{path}: LRU_PAIRS vazio ou ausente — sem par de referência")
    try:
        bits, interval = (int(v) for v in pairs[0].split(":"))
    except ValueError:
        raise PlotError(f"{path}: par de referência malformado '{pairs[0]}' (esperado N:I)")
    return GridConf(traces, (bits, interval))


def _optional_int(text: str) -> int | None:
    return int(text) if text else None


def load_results(path: Path) -> list[Row]:
    """Lê o CSV de resultados de um trace.

    Args:
        path: Caminho de results/<trace>.csv.

    Returns:
        As simulações do arquivo, na ordem em que aparecem.

    Raises:
        PlotError: Arquivo ausente, vazio, sem linhas de dados ou malformado.
    """
    if not path.is_file():
        raise PlotError(f"{path} não encontrado — rode `make grid` antes")
    with path.open(newline="") as f:
        reader = csv.DictReader(f)
        try:
            rows = [
                Row(
                    policy=r["policy"],
                    frames=int(r["frames"]),
                    bits=_optional_int(r["history_bits"]),
                    interval=_optional_int(r["aging_interval"]),
                    faults=int(r["page_faults"]),
                    writebacks=int(r["writebacks"]),
                )
                for r in reader
            ]
        except (KeyError, TypeError, ValueError) as e:
            raise PlotError(f"{path}: CSV malformado ({e})")
    if not rows:
        raise PlotError(f"{path} está vazio — rode `make grid` antes")
    return rows


def _series(rows: list[Row], policy: str, bits: int | None = None,
            interval: int | None = None) -> tuple[list[int], list[int], list[int]]:
    """Filtra uma curva e a ordena por número de frames.

    Args:
        rows: Simulações de um trace.
        policy: Política da curva.
        bits: N exigido (só LRU aproximado).
        interval: I exigido (só LRU aproximado).

    Returns:
        Frames, falhas de página e escritas de volta, em ordem crescente de frames.
    """
    chosen = sorted(
        (r for r in rows if r.policy == policy
         and (bits is None or r.bits == bits)
         and (interval is None or r.interval == interval)),
        key=lambda r: r.frames,
    )
    return ([r.frames for r in chosen], [r.faults for r in chosen],
            [r.writebacks for r in chosen])


def _frames_axis(ax: plt.Axes, frames: list[int]) -> None:
    """Eixo x em log₂ com um rótulo por número de frames da grade (ADR 0002)."""
    ax.set_xscale("log", base=2)
    ax.set_xticks(frames)
    ax.set_xticklabels([str(f) for f in frames], rotation=90, fontsize=8)
    ax.xaxis.set_minor_locator(NullLocator())
    ax.set_xlabel("Número de frames")
    ax.grid(True, which="major", alpha=0.3)


def _save(fig: plt.Figure, path: Path) -> Path:
    fig.tight_layout()
    fig.savefig(path, dpi=120)
    plt.close(fig)
    return path


def main_curves(trace: str, rows: list[Row], reference: tuple[int, int]
                ) -> dict[str, tuple[list[int], list[int], list[int]]]:
    """Curvas do experimento principal: FIFO, OPT e o par de referência.

    Args:
        trace: Nome do trace (só para a mensagem de erro).
        rows: Simulações do trace.
        reference: Par de referência (N, I) do LRU aproximado.

    Returns:
        Por política, frames, falhas de página e escritas de volta.

    Raises:
        PlotError: Falta FIFO, OPT ou o par de referência no CSV.
    """
    ref_bits, ref_interval = reference
    curves = {
        "fifo": _series(rows, "fifo"),
        "opt": _series(rows, "opt"),
        "lru-approx": _series(rows, "lru-approx", ref_bits, ref_interval),
    }
    for policy, (frames, _, _) in curves.items():
        if not frames:
            what = (f"par de referência N={ref_bits}, I={ref_interval}"
                    if policy == "lru-approx" else POLICY_LABELS[policy])
            raise PlotError(f"trace '{trace}': {what} ausente no CSV — rode `make grid`")
    return curves


def plot_trace(trace: str, rows: list[Row], reference: tuple[int, int],
               out_dir: Path) -> list[Path]:
    """Gera as três figuras de um trace.

    Args:
        trace: Nome do trace (prefixo dos arquivos e título).
        rows: Simulações do trace.
        reference: Par de referência (N, I) do LRU aproximado.
        out_dir: Diretório de saída (results/figuras/).

    Returns:
        Caminhos das figuras geradas.

    Raises:
        PlotError: Falta FIFO, OPT ou o par de referência no CSV.
    """
    ref_bits, ref_interval = reference
    curves = main_curves(trace, rows, reference)
    all_frames = sorted({r.frames for r in rows})
    ref_label = f"{POLICY_LABELS['lru-approx']} (N={ref_bits}, I={ref_interval})"
    out_dir.mkdir(parents=True, exist_ok=True)
    paths = []

    # Falhas nunca chegam a zero (as compulsórias contam); escritas de volta chegam,
    # e o eixo log descartaria esses pontos — symlog é linear perto do zero.
    for index, ylabel, suffix, yscale in (
        (1, "Falhas de página", "falhas", {"value": "log"}),
        (2, "Escritas de volta", "escritas", {"value": "symlog", "linthresh": 1}),
    ):
        fig, ax = plt.subplots(figsize=(8, 5))
        for policy, series in curves.items():
            label = ref_label if policy == "lru-approx" else POLICY_LABELS[policy]
            ax.plot(series[0], series[index], marker="o", markersize=4, label=label)
        _frames_axis(ax, all_frames)
        ax.set_yscale(**yscale)
        if suffix == "escritas":
            ax.set_ylim(bottom=0)
        ax.set_ylabel(ylabel)
        ax.set_title(f"{trace}: {ylabel.lower()} × número de frames")
        ax.legend()
        paths.append(_save(fig, out_dir / f"{trace}-{suffix}.png"))

    fig, (ax_n, ax_i) = plt.subplots(1, 2, figsize=(13, 5), sharey=True)
    for ax, fixed, values, label_of in (
        (ax_n, {"interval": ref_interval},
         sorted({r.bits for r in rows if r.policy == "lru-approx"
                 and r.interval == ref_interval}),
         lambda v: f"N={v}"),
        (ax_i, {"bits": ref_bits},
         sorted({r.interval for r in rows if r.policy == "lru-approx"
                 and r.bits == ref_bits}),
         lambda v: f"I={v}"),
    ):
        varying = "bits" if "interval" in fixed else "interval"
        for value in values:
            frames, faults, _ = _series(rows, "lru-approx", **fixed, **{varying: value})
            ax.plot(frames, faults, marker="o", markersize=3, label=label_of(value))
        fifo, opt = curves["fifo"], curves["opt"]
        ax.plot(fifo[0], fifo[1], "k--", linewidth=1, label="FIFO")
        ax.plot(opt[0], opt[1], "k:", linewidth=1, label="OPT")
        _frames_axis(ax, all_frames)
        ax.set_yscale("log")
        ax.legend(fontsize=8)
    ax_n.set_ylabel("Falhas de página")
    ax_n.set_title(f"Bits de histórico N variando (I={ref_interval})")
    ax_i.set_title(f"Intervalo de envelhecimento I variando (N={ref_bits})")
    fig.suptitle(f"{trace}: experimento de sensibilidade do LRU aproximado")
    paths.append(_save(fig, out_dir / f"{trace}-sensibilidade.png"))
    return paths


def main(grid_conf: Path = ROOT / "experiments" / "grid.conf",
         results_dir: Path = ROOT / "results") -> int:
    """Gera as figuras de todos os traces do grid.conf.

    Args:
        grid_conf: Caminho do grid.conf.
        results_dir: Diretório com os CSV; as figuras vão para <results_dir>/figuras.

    Returns:
        0 em sucesso, 1 se alguma entrada for inválida (mensagem no stderr).
    """
    try:
        conf = read_grid_conf(grid_conf)
        data = {t: load_results(results_dir / f"{t}.csv") for t in conf.traces}
        # Valida todos os traces antes de gravar qualquer figura: um erro no meio
        # não pode deixar results/figuras/ com metade nova e metade antiga.
        for trace, rows in data.items():
            main_curves(trace, rows, conf.reference)
        for trace, rows in data.items():
            for path in plot_trace(trace, rows, conf.reference, results_dir / "figuras"):
                print(path, file=sys.stderr)
    except PlotError as e:
        print(f"erro: {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

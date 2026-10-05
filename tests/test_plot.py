"""
Testes de scripts/plot.py (rodados por `make test-plots`).

Responsabilidades:
- Montar grid.conf e results/ pequenos num diretório temporário.
- Conferir as figuras geradas e as mensagens de erro de `main`.
"""

import io
import sys
import tempfile
import unittest
from contextlib import redirect_stderr
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent / "scripts"))

import plot  # noqa: E402

HEADER = "trace,policy,frames,history_bits,aging_interval,accesses,page_faults,writebacks"

CSV_TOY = "\n".join([
    HEADER,
    "toy,fifo,4,,,100,50,10",
    "toy,fifo,8,,,100,30,6",
    "toy,opt,4,,,100,40,8",
    "toy,opt,8,,,100,20,4",
    "toy,lru-approx,4,8,100,100,45,9",
    "toy,lru-approx,8,8,100,100,25,5",
    "toy,lru-approx,4,2,100,100,47,9",
    "toy,lru-approx,8,2,100,100,27,5",
    "toy,lru-approx,4,8,10,100,46,9",
    "toy,lru-approx,8,8,10,100,26,5",
]) + "\n"

GRID_CONF = 'TRACES="toy"\nFRAMES="4 8"\nLRU_PAIRS="8:100 2:100 8:10"\n'


class PlotTestCase(unittest.TestCase):
    """Base com um diretório temporário contendo grid.conf e results/."""

    def setUp(self) -> None:
        self._tmp = tempfile.TemporaryDirectory()
        self.root = Path(self._tmp.name)
        self.results = self.root / "results"
        self.results.mkdir()
        self.grid_conf = self.root / "grid.conf"
        self.grid_conf.write_text(GRID_CONF)

    def tearDown(self) -> None:
        self._tmp.cleanup()

    def run_main(self) -> tuple[int, str]:
        """Roda `plot.main` sobre o diretório temporário.

        Returns:
            O código de saída e o que foi escrito no stderr.
        """
        err = io.StringIO()
        with redirect_stderr(err):
            code = plot.main(self.grid_conf, self.results)
        return code, err.getvalue()


class TestFigures(PlotTestCase):
    def test_gera_as_tres_figuras_do_trace(self) -> None:
        (self.results / "toy.csv").write_text(CSV_TOY)
        code, err = self.run_main()
        self.assertEqual(code, 0, err)
        figures = self.results / "figuras"
        for name in ("toy-falhas.png", "toy-sensibilidade.png", "toy-escritas.png"):
            path = figures / name
            self.assertTrue(path.is_file(), f"{name} não foi gerada")
            self.assertEqual(path.read_bytes()[:8], b"\x89PNG\r\n\x1a\n")


class TestErrors(PlotTestCase):
    def assert_fails_with(self, *fragments: str) -> None:
        """Confere que `main` falha, cita os fragmentos e não gera figuras."""
        code, err = self.run_main()
        self.assertEqual(code, 1)
        self.assertIn("erro:", err)
        for fragment in fragments:
            self.assertIn(fragment, err)
        self.assertFalse((self.results / "figuras").exists())

    def test_csv_ausente_gera_erro_claro(self) -> None:
        self.assert_fails_with("toy.csv", "não encontrado")

    def test_csv_vazio_gera_erro_claro(self) -> None:
        (self.results / "toy.csv").write_text("")
        self.assert_fails_with("toy.csv", "vazio")

    def test_csv_so_com_cabecalho_gera_erro_claro(self) -> None:
        (self.results / "toy.csv").write_text(HEADER + "\n")
        self.assert_fails_with("toy.csv", "vazio")

    def test_par_de_referencia_ausente_no_csv_gera_erro_claro(self) -> None:
        (self.results / "toy.csv").write_text(CSV_TOY)
        self.grid_conf.write_text(GRID_CONF.replace("8:100 2:100", "16:100 2:100"))
        self.assert_fails_with("par de referência", "N=16", "I=100")

    def test_par_ausente_em_um_trace_nao_gera_figura_de_nenhum(self) -> None:
        (self.results / "toy.csv").write_text(CSV_TOY)
        sem_referencia = [line.replace("toy,", "outro,") for line in CSV_TOY.splitlines()
                          if ",8,100," not in line]
        (self.results / "outro.csv").write_text("\n".join(sem_referencia) + "\n")
        self.grid_conf.write_text(GRID_CONF.replace('"toy"', '"toy outro"'))
        self.assert_fails_with("outro", "par de referência")


if __name__ == "__main__":
    unittest.main()

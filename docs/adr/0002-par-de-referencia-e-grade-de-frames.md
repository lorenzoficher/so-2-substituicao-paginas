# Par de referência 8:100 e grade de frames densa na faixa baixa

**Status:** Proposta — aguardando aprovação do aluno. Nada aqui foi aplicado ao
`experiments/grid.conf` nem aos `results/` (issue #9).

Os valores provisórios da grade (par de referência 8:1000, frames 4..256 dobrando)
foram fixados sem dados. Uma varredura exploratória sobre os quatro traces
independentes (bzip, gcc, sixpack, swim; bigone fica de fora) mostrou que o
**intervalo de envelhecimento** é o parâmetro que mais pesa: com I = 1000 o LRU
aproximado fica **pior que o FIFO** em 8 pontos da grade (bzip com 12–16 frames, gcc
com 4–12), enquanto com I = 100 ele fica entre o OPT e o FIFO em todos os pontos, para
qualquer N. Por isso propomos **8:100** como par de referência, uma grade de frames
densa até 32 e estendida até 512, e pares de sensibilidade que variam N com I = 100 e
I com N = 8.

## Varredura

- N ∈ {1, 2, 4, 8, 16, 32} × I ∈ {10, 30, 100, 300, 1000, 3000, 10⁴, 3·10⁴, 10⁵}
  (54 pares), em 27 números de frames: 2, 3, 4, 5, 6, 8, 10, 12, 14, 16, 20, 24, 28,
  32, 40, 48, 56, 64, 80, 96, 128, 160, 192, 256, 320, 384, 512.
- FIFO e OPT nos mesmos números de frames. Tudo via `build/sim`, uma simulação por
  (trace, política, N, I) com a lista de frames separada por vírgulas.

## Métrica

Para cada trace e número de frames, a **posição** do LRU aproximado entre o OPT e o
FIFO:

    posição = (falhas_LRU − falhas_OPT) / (falhas_FIFO − falhas_OPT)

0 é o OPT (limite inferior), 1 é o FIFO, acima de 1 é pior que o FIFO. Pontos em que
FIFO e OPT diferem em até 1000 falhas de página (0,1% dos acessos) são descartados —
ali a razão é ruído (ex.: bzip com ≥ 320 frames, onde as três políticas só têm as
317 falhas compulsórias). A posição é promediada nos números de frames de cada trace
e depois entre os quatro traces, para que nenhum trace pese mais. Também contamos os
pontos com posição > 1.

## Páginas distintas e joelho das curvas

| Trace   | Páginas distintas | Joelho (OPT) | Joelho (FIFO) | Observação |
|---------|------------------:|-------------:|--------------:|------------|
| bzip    |   317 | 12 | 12 | Queda abrupta de 10 → 12 frames (FIFO 40 385 → 4 581). A grade que dobra pula de 8 para 16 e não a vê. Saturação (só compulsórias) a partir de ~320 frames. |
| gcc     | 2 852 | 20 | 20 | Curva suave; ainda cai em 512 frames. |
| sixpack | 3 890 | 20 | 24 | Curva suave; ainda cai em 512 frames. |
| swim    | 2 543 | 24 | 40 | Queda forte entre 14 e 32 frames (FIFO 242 630 → 81 638). |
| bigone  | 8 254 | — | — | Fora da varredura; só contado para dimensionar a grade. |

Joelho = ponto de maior distância à corda da curva normalizada (falhas × log₂ frames,
4..512 frames), o critério do *Kneedle*. Em todos os traces o joelho cai entre 12 e
40 frames — é onde a grade precisa de resolução. Nenhum trace exceto o bzip satura
até 512 frames.

## Resultado da varredura

Posição média (4 traces) na grade de frames proposta:

| N \ I |   10  |   30  |  100  |  300  | 1000  | 3000  | 10⁴   | 3·10⁴ | 10⁵   |
|------:|------:|------:|------:|------:|------:|------:|------:|------:|------:|
|  1    | 0,866 | 0,836 | 0,807 | 0,829 | 0,934 | 1,043 | 1,147 | 1,326 | 1,935 |
|  2    | 0,827 | 0,766 | 0,731 | 0,758 | 0,854 | 0,963 | 1,024 | 1,226 | 1,884 |
|  4    | 0,791 | 0,721 | 0,705 | 0,734 | 0,832 | 0,943 | 1,027 | 1,228 | 1,879 |
|  8    | 0,740 | 0,696 | **0,679** | 0,711 | 0,817 | 0,925 | 1,021 | 1,226 | 1,879 |
| 16    | 0,708 | 0,671 | 0,656 | 0,693 | 0,798 | 0,928 | 1,029 | 1,225 | 1,879 |
| 32    | 0,686 | 0,653 | 0,637 | 0,680 | 0,787 | 0,937 | 1,029 | 1,225 | 1,879 |

Por trace, na mesma grade:

| Par    | bzip  | gcc   | sixpack | swim  | Média | Pontos pior que FIFO |
|--------|------:|------:|--------:|------:|------:|---------------------:|
| 8:100  | 0,598 | 0,779 | 0,673 | 0,668 | 0,679 | 0 |
| 32:100 | 0,582 | 0,736 | 0,616 | 0,613 | 0,637 | 0 |
| 8:300  | 0,728 | 0,827 | 0,663 | 0,625 | 0,711 | 4 |
| 8:1000 | 1,044 | 0,885 | 0,692 | 0,647 | 0,817 | 8 (bzip 12 frames: 3,32) |

Leitura:

- **I domina.** Para todo N o mínimo está em I = 100; de I ≥ 10⁴ em diante o LRU
  aproximado é, em média, pior que o FIFO. Com intervalos longos o bit de referência
  fica ligado em quase todas as páginas e a escolha da vítima passa a depender de
  históricos velhos e do desempate pela carregada há mais tempo — a política perde a
  informação de recência que a distinguiria do FIFO (hipótese; a varredura mede o
  efeito, não a causa).
- **N rende pouco depois de 8.** Com I = 100, ir de 8 para 32 bits melhora a média em
  0,04; de 1 para 8, em 0,13.
- Na grade antiga (4..256 dobrando) o 8:1000 parecia razoável (0,749) — os pontos em
  que ele perde para o FIFO estão justamente nos frames que a grade pulava.

## Proposta

- **Par de referência: 8:100.** Fica a 0,04 do melhor par varrido (32:100), nunca
  perde para o FIFO e mantém o histórico de 8 bits. 32:100 seria a alternativa se a
  meta for só o mínimo da métrica.
- **Grade de frames:** 4 6 8 10 12 14 16 20 24 32 48 64 96 128 192 256 384 512
  (18 valores). Densa até 32 para pegar os joelhos (12–40) e o salto do bzip em
  10 → 12; até 512 porque gcc, sixpack, swim e bigone ainda não saturaram em 256.
- **Pares de sensibilidade:** variar N com I fixo — 2:100, 4:100, 16:100, 32:100 — e
  variar I com N fixo — 8:10, 8:1000, 8:10000, 8:100000. O 8:1000 e o 8:100000 ficam
  de propósito: mostram o LRU aproximado degradando até ficar pior que o FIFO.

## Considered Options

- **Manter 8:1000** — era o valor provisório, mas fica pior que o FIFO no bzip
  (12–16 frames) e no gcc (4–12 frames).
- **32:100** — melhor média (0,637), mas o ganho sobre 8:100 é pequeno e se mede numa
  escala de N pouco usual.
- **Grade dobrando até 256** — 7 pontos, mas pula o joelho de todos os traces e
  esconde as derrotas do 8:1000 para o FIFO.

## Ressalvas

- O par foi escolhido nos mesmos quatro traces em que será avaliado; a varredura é
  exploratória, não uma validação independente.
- O bigone não entrou na varredura (só a contagem de páginas distintas).

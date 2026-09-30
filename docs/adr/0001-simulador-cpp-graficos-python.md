# Simulador em C++17 no WSL, Python só para gráficos

O simulador é escrito em C++17 (só a STL), compilado com g++ e Makefile no WSL Ubuntu, e grava os resultados em CSV; Python + matplotlib apenas lê esses CSV e gera os gráficos do artigo, sem nenhuma lógica de simulação. A grade de experimentos (5 traces, até 4M de acessos cada, ~245 simulações) leva segundos em C++ contra dezenas de minutos em Python puro, e as estruturas da STL (`unordered_map`, `priority_queue`, `vector`) evitam reimplementar hash e heap como seria preciso em C. O WSL garante que o professor compila com um único `make`.

## Considered Options

- **Python puro** — uma linguagem só, mas o OPT e o LRU aproximado exigiriam otimizações cuidadosas para caber em tempo razoável com traces de 4M de acessos.
- **C11** — mais "raiz" para SO, mas o esforço iria para estruturas de dados, não para as políticas.

# Simulador de Substituição de Páginas

Simulador que mede o número de falhas de página produzidas por diferentes políticas de substituição ao processar traces de acesso à memória, variando a quantidade de frames disponíveis.

## Language

### Memória

**Página**:
Bloco de 4096 bytes do espaço de endereçamento virtual de 32 bits; identificada pelo número de página (endereço >> 12).
_Avoid_: Bloco, segmento

**Frame**:
Espaço na memória física capaz de conter exatamente uma página.
_Avoid_: Quadro, moldura, slot

**Número de frames**:
Quantidade de frames disponíveis durante uma simulação; fixa do início ao fim, todos começando vazios.
_Avoid_: Tamanho da memória, frames livres

**Falha de página**:
Acesso a uma página que não está em nenhum frame no momento do acesso, incluindo as falhas compulsórias do início. É a métrica principal comparada entre políticas.
_Avoid_: Page miss, cache miss, page fault (em texto em português)

**Página suja**:
Página em frame que recebeu ao menos uma escrita desde que foi carregada.
_Avoid_: Página modificada, dirty page

**Escrita de volta**:
Gravação de uma vítima suja no disco no momento em que ela deixa o frame. É a métrica secundária; páginas ainda sujas ao fim do trace não contam.
_Avoid_: Write-back, flush, page-out

### Trace

**Trace**:
Sequência ordenada de acessos à memória lida de um arquivo, uma linha por acesso.
_Avoid_: Log, arquivo de entrada

**Acesso**:
Uma linha do trace: um endereço de 32 bits em hexadecimal e uma operação (R para leitura, W para escrita).
_Avoid_: Referência, requisição, instrução

**Troca de fase**:
Ponto do trace em que o conjunto de páginas em uso muda de forma abrupta. O `bigone` é a concatenação exata `bzip + gcc + sixpack + swim` (blocos de 1M de acessos, md5 conferido — ver README), então tem três trocas de fase: é uma carga com trocas de fase, não um quinto programa independente, e fica fora da média da varredura exploratória.
_Avoid_: Mudança de contexto, troca de programa

### Políticas

**Política de substituição**:
Regra que escolhe qual página sai de um frame quando ocorre uma falha de página e todos os frames estão ocupados. É o que o enunciado chama de "abordagem de distribuição das páginas". Nenhuma política considera se a vítima é página suja.
_Avoid_: Algoritmo de distribuição, estratégia de alocação

**Vítima**:
Página escolhida pela política de substituição para deixar seu frame e dar lugar à página que faltou.
_Avoid_: Página expulsa, página removida

**Próximo uso**:
Posição, no trace, do próximo acesso a uma página a partir do acesso atual.
_Avoid_: Distância futura

**OPT**:
Política de substituição que remove a página cujo próximo acesso está mais distante no futuro; serve como limite inferior de falhas.
_Avoid_: Ótimo, Belady, MIN

**FIFO**:
Política de substituição que remove a página que está há mais tempo na memória, independentemente do uso.
_Avoid_: Fila

**LRU aproximado**:
Política de substituição cuja vítima é a página de bit de referência desligado, desempatando pelo menor histórico e, depois, pela carregada há mais tempo. O bit vem antes do histórico por ser a informação mais recente: na ordem inversa, a página carregada desde o último envelhecimento (histórico zerado) seria sempre a próxima vítima.
_Avoid_: Aging, LRU (é o exato, fora da comparação), clock, segunda chance

**Bit de referência**:
Bit de uma página em frame ligado a cada acesso a ela (inclusive o que a carregou) e desligado a cada envelhecimento.
_Avoid_: Bit R, bit de uso

**Histórico**:
Registro de N bits de uma página em frame; o bit mais significativo representa o intervalo de envelhecimento mais recente. Página recém-carregada começa com histórico zerado.
_Avoid_: Contador, idade

**Bits de histórico** (N):
Largura do histórico; parâmetro do LRU aproximado.
_Avoid_: Tamanho do contador, bits adicionais

**Envelhecimento**:
Operação aplicada a todas as páginas em frame: o histórico desloca um bit à direita, o bit de referência entra como bit mais significativo e é então zerado.
_Avoid_: Deslocamento, shift, tick

**Intervalo de envelhecimento**:
Número de acessos entre dois deslocamentos consecutivos do histórico de todas as páginas. Cada acesso do trace conta como uma unidade de tempo (o enunciado chama de "instruções").
_Avoid_: Tick, período, quantum

### Experimentos

**Simulação**:
Uma execução de um trace sob uma política de substituição e um número de frames (e, no LRU aproximado, um par N/intervalo), produzindo um único número de falhas de página.
_Avoid_: Rodada, teste

**Experimento principal**:
Execução das três políticas de substituição sobre um trace para cada quantidade de frames escolhida, com N e intervalo de envelhecimento nos valores padrão.
_Avoid_: Benchmark, teste

**Experimento de sensibilidade**:
Execução apenas do LRU aproximado com quantidade de frames fixa, variando N e o intervalo de envelhecimento.
_Avoid_: Tuning, calibração

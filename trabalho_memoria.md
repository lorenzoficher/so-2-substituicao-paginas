# Avaliação do impacto da política de substituição no número de falhas de página

Avaliar o impacto da política de substituição no número de falhas de página de acordo com os algoritmos previstos na seção do gerenciamento de memória. O trabalho deverá avaliar o número de falhas de páginas com diferentes tamanhos de frames. Escolha alguns valores de número de frames que achar conveniente para avaliar. Faça um programa para fazer essa avaliação, comparando 3 abordagens de distribuição das páginas.

No caso do LRU aproximado escolha um valor de bits que poderá ser setado pelo programa. Pense também em um valor de instruções (de quanto em quanto o tempo os bits são deslocados/envelhecidos).

Os traces a serem utilizados estão disponibilizados no moodle.

## Formato do trace

O formato utilizado pelo trace é:

31348900 W Endereço em hexadecimal | Escrita ou leitura

Os endereços são de uma arquitetura de 32 bits, sendo que deve ser considerado um uma página de tamanho 4096 bytes.

## Observações

- O trabalho é individual.
- Baseado em: https://www.inf.unioeste.br/~marcio/SO/Trabalho2Bim.pdf

## Objetivo

Analisar como diferentes políticas de substituição de páginas afetam a taxa de falhas de página e comparar o comportamento de diferentes estratégias de distribuição das páginas em memória.

## Requisitos esperados

- Avaliar o número de falhas de página para diferentes quantidades de frames.
- Comparar 3 abordagens de distribuição das páginas.
- Implementar um programa para simular o comportamento das políticas.
- Considerar o algoritmo LRU aproximado com parâmetro configurável de bits e intervalo de envelhecimento.
- Interpretar os resultados em termos de desempenho de memória.

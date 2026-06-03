# Trabalho — Multiplicação de Matrizes: Sequencial vs. Paralelo (pthreads)

**Problema escolhido:** P1 — Multiplicação de matrizes grandes  
**Disciplina:** Laboratório de Programação II — LPII-261-T001
**Aluno:**: João Victor Oliveira
**Matrícula**: 20240008468
---

## Estrutura do repositório

```
matmul/
├── src/
│   ├── matmul_seq.c   # versão sequencial (implementação do enunciado + cronômetro)
│   └── matmul_par.c   # versão paralela com pthreads
├── results/           # CSVs gerados pelos binários (criado automaticamente)
├── bin/               # binários compilados (criado automaticamente)
├── Makefile
├── benchmark.sh       # script de benchmark com tabela de speedup
└── README.md
```

---

## Como compilar e executar

```bash
# Compilar os dois binários
make

# Executar manualmente
./bin/matmul_seq <n>           # ex: ./bin/matmul_seq 1000
./bin/matmul_par <n> <threads> # ex: ./bin/matmul_par 1000 4

# Benchmark completo (tabela automática de speedup)
make bench
```

---

## Estratégia de paralelização

A multiplicação `C = A × B` com matrizes *n×n* tem custo **O(n³)** e é
fortemente *CPU-bound*. O ponto-chave é que cada elemento `C[i][j]` depende
apenas de dados de `A` e `B` — nunca escreve onde outra thread lê ou escreve.

**Divisão de trabalho:** o conjunto de linhas de `C` é particionado em *T*
blocos contíguos. A thread `t` calcula exclusivamente as linhas
`[linha_ini, linha_fim)` de `C`:

```
Thread 0 → linhas  0 .. (n/T)-1
Thread 1 → linhas  n/T .. 2*(n/T)-1
...
Thread T-1 → últimas linhas (+ resto quando n % T ≠ 0)
```

Consequências:
- **Escrita em posições disjuntas** → zero *race conditions*, zero mutex.
- **Sem etapa de merge** → cada thread escreve diretamente no array final `C`.
- **Balanceamento**: as primeiras `n % T` threads recebem 1 linha extra,
  distribuindo o resto igualmente.

---

## Cronômetro

O enunciado exige que apenas a **computação** seja medida. O código usa
`clock_gettime(CLOCK_MONOTONIC, ...)` ao redor exclusivamente da chamada a
`multiplicar()` (sequencial) ou do bloco `pthread_create` / `pthread_join`
(paralelo). O preenchimento das matrizes fica **fora** do cronômetro.

---

## Resultados medidos

> Ambiente de execução (sandbox CI): **1 CPU lógica** (x86-64, Linux).  
> Os números abaixo são os valores reais medidos neste ambiente.

| n    | versão     | threads | tempo (s) | speedup |
|------|------------|---------|-----------|---------|
| 500  | sequencial | —       | 0.1573    | 1.00×   |
| 500  | paralelo   | 1       | 0.1554    | 1.01×   |
| 500  | paralelo   | 2       | 0.1520    | 1.03×   |
| 500  | paralelo   | 4       | 0.1531    | 1.03×   |
| 1000 | sequencial | —       | 1.7151    | 1.00×   |
| 1000 | paralelo   | 1       | 1.5422    | 1.11×   |
| 1000 | paralelo   | 2       | 1.5309    | 1.12×   |
| 1000 | paralelo   | 4       | 1.5352    | 1.12×   |
| 1200 | sequencial | —       | 3.7928    | 1.00×   |
| 1200 | paralelo   | 1       | 3.6403    | 1.04×   |
| 1200 | paralelo   | 2       | 3.7170    | 1.02×   |
| 1200 | paralelo   | 4       | 4.0893    | 0.93×   |

**Checksum de validação:** todos os `C[i][j]` são idênticos entre as versões
sequencial e paralela para o mesmo `n` (mesmas sementes `42` e `99`):

| n    | checksum           |
|------|--------------------|
| 500  | 3.064237e+09       |
| 1000 | 2.445671e+10       |
| 1200 | 4.228056e+10       |

---

## Análise — Por que o speedup não foi linear?

### 1. Ambiente de 1 CPU lógica

O ambiente de teste (sandbox) disponibiliza **apenas 1 núcleo de CPU**. Neste
caso, múltiplas threads concorrem pelo mesmo núcleo e o sistema operacional faz
*time-slicing*. O resultado esperado nesse cenário é:

- Nenhuma aceleração real — o trabalho é o mesmo, só dividido em fatias
  menores que o SO precisa escalonar.
- Leve overhead de `pthread_create`/`pthread_join` e possível degradação de
  cache (cada thread "esquenta" uma parte diferente do L1/L2 que a outra
  thread vai precisar logo depois).

Isso explica por que com `T=4` e `n=1200` o tempo ficou *pior* (0.93× de
speedup).

### 2. O que acontece em uma máquina com múltiplos núcleos

Em um sistema com, por exemplo, 4 ou 8 núcleos físicos, o speedup esperado
para este problema segue a **Lei de Amdahl**:

```
S(T) = 1 / ( (1 - p) + p/T )
```

onde `p ≈ 1.0` (a multiplicação é 100% paralelizável) e `(1-p)` é a fração
serial residual (setup de threads, join, checksum). Exemplos típicos relatados
na literatura para multiplicação de matrizes:

| Threads | Speedup típico (4-core) | Eficiência |
|---------|------------------------|------------|
| 1       | 1.00×                  | 100%       |
| 2       | ~1.90×                 | 95%        |
| 4       | ~3.60×                 | 90%        |
| 8       | ~5.50×                 | 69%        |

O speedup diminui com mais threads por:

- **Overhead de criação/destruição de threads** — `pthread_create` é caro.
  Pode ser mitigado com *thread pool* (reuso de threads).
- **Contenção de memória / cache thrashing** — `A` e `B` são lidas por todas
  as threads. Com matrizes grandes (n=1500, ~18 MB cada), elas não cabem nos
  caches L1/L2 e as threads disputam a largura de banda do barramento de memória.
- **False sharing** — embora as escritas em `C` sejam disjuntas por linha, o
  último elemento de uma linha e o primeiro da próxima podem cair na mesma
  *cache line* (64 bytes = 8 doubles). Isso é raro aqui pois as linhas têm
  `n ≥ 500` doubles.
- **Fração serial** — checksum final, malloc, join, acesso a `results/` são
  todos sequenciais.

### 3. Acesso a `B` não é *cache-friendly*

A multiplicação percorre `B` coluna a coluna (acesso com stride `n`), o que
causa muitos *cache misses*. Uma otimização clássica é transpor `B` antes da
multiplicação, tornando todos os acessos sequenciais — mas isso está fora do
escopo deste trabalho (seria uma otimização de algoritmo, não de paralelismo).

---

## Conclusão

- A paralelização por blocos de linhas está **correta**: o checksum garante
  que `C` é idêntico ao resultado sequencial.
- Em ambiente de **1 CPU** o paralelismo não ajuda — é esperado e esperável.
- Em uma máquina **multi-core** (laboratório, computador pessoal), o mesmo
  código deve mostrar speedup próximo de `T` para `T ≤ número de núcleos`,
  decaindo suavemente após esse ponto por causa dos fatores listados acima.
- O exercício demonstra na prática que *threads corretas* e *threads rápidas*
  são objetivos diferentes: a correção (escrita disjunta, sem mutex) é o
  primeiro passo; o speedup real depende da arquitetura de memória, da
  topologia de cache e da Amdahl.
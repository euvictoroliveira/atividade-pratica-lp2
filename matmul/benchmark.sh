#!/usr/bin/env bash
# benchmark.sh — executa sequencial e paralelo para vários N e T,
# coletando tempos e imprimindo uma tabela de speedup.

set -e

SEQ=./bin/matmul_seq
PAR=./bin/matmul_par
RESDIR=results

mkdir -p "$RESDIR"
rm -f "$RESDIR"/seq.csv "$RESDIR"/par.csv

# Detecta número de CPUs lógicas
NCPUS=$(nproc 2>/dev/null || sysctl -n hw.logicalcpu 2>/dev/null || echo 4)
echo "CPUs detectadas: $NCPUS"
echo ""

# Tamanhos de matriz a testar
NS=(500 1000 1200)

# Contagens de threads a testar
TS=(1 2 4 $NCPUS)
# Remove duplicatas e ordena
TS=($(echo "${TS[@]}" | tr ' ' '\n' | sort -nu))

RUNS=2  # repetições por configuração (usa a menor)

echo "============================================================"
printf "%-6s %-8s %-8s %-12s %-10s\n" "n" "threads" "runs" "tempo(s)" "speedup"
echo "------------------------------------------------------------"

for N in "${NS[@]}"; do
    # --- Sequencial ---
    best_seq=999999
    for ((r=0; r<RUNS; r++)); do
        $SEQ "$N" > /dev/null
        t=$(tail -1 "$RESDIR/seq.csv" | cut -d',' -f4)
        # guarda o menor
        if (( $(echo "$t < $best_seq" | bc -l) )); then
            best_seq=$t
        fi
    done
    printf "%-6s %-8s %-8s %-12s %-10s\n" "$N" "seq(1)" "$RUNS" "$best_seq" "1.00x"

    # --- Paralelo para cada T ---
    for T in "${TS[@]}"; do
        best_par=999999
        for ((r=0; r<RUNS; r++)); do
            $PAR "$N" "$T" > /dev/null
            t=$(tail -1 "$RESDIR/par.csv" | cut -d',' -f4)
            if (( $(echo "$t < $best_par" | bc -l) )); then
                best_par=$t
            fi
        done
        speedup=$(echo "scale=2; $best_seq / $best_par" | bc -l)
        printf "%-6s %-8s %-8s %-12s %-10s\n" "$N" "par($T)" "$RUNS" "$best_par" "${speedup}x"
    done
    echo ""
done
echo "============================================================"
echo "Arquivos CSV em $RESDIR/"

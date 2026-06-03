/*
 * matmul_par.c — Multiplicação de matrizes paralela com pthreads
 *
 * Estratégia: cada thread recebe um bloco contíguo de linhas de C para calcular.
 * As escritas são em posições disjuntas → sem race condition, sem mutex, sem merge.
 *
 * Divisão de carga:
 *   - linhas_base = n / T  (cada thread recebe pelo menos esse número)
 *   - As primeiras (n % T) threads recebem uma linha extra para cobrir o resto
 *   Isso garante balanceamento ótimo quando n não é múltiplo de T.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

// Define de estrutura com os argumentos de cada thread
typedef struct {
    const double *A;
    const double *B;
    double       *C;
    int           n;
    int           linha_ini;   // primeira linha (inclusiva) 
    int           linha_fim;   // última linha (exclusiva)  
} ThreadArgs;

// Define rotinas para cada thread calcular seu bloco de linhas de c
static void *calcular_bloco(void *arg) {
    ThreadArgs *ta = (ThreadArgs *)arg;
    const double *A = ta->A;
    const double *B = ta->B;
    double       *C = ta->C;
    int           n = ta->n;

    for (int i = ta->linha_ini; i < ta->linha_fim; i++) {
        for (int j = 0; j < n; j++) {
            double soma = 0.0;
            for (int k = 0; k < n; k++)
                soma += A[i*n + k] * B[k*n + j];
            C[i*n + j] = soma;
        }
    }
    return NULL;
}

/* Preenche a matriz com valores pseudoaleatórios determinísticos       
  (mesma semente do sequencial → mesmo A e B → mesmo C esperado)      */
static void preencher(double *M, int n, unsigned int seed) {
    srand(seed);
    for (int i = 0; i < n * n; i++)
        M[i] = (double)(rand() % 100) / 10.0;
}

// função main
int main(int argc, char *argv[]) {
    int n = (argc > 1) ? atoi(argv[1]) : 1000;
    int T = (argc > 2) ? atoi(argv[2]) : 4;
    if (T < 1) T = 1;
    if (T > n) T = n;  // garante que não haja mais threads do que linhas

    printf("Paralelo  — n = %d, threads = %d\n", n, T);

    double *A = malloc(n * n * sizeof(double));
    double *B = malloc(n * n * sizeof(double));
    double *C = malloc(n * n * sizeof(double));
    if (!A || !B || !C) { perror("malloc"); return 1; }

    /* Setup fora do cronômetro */
    preencher(A, n, 42);
    preencher(B, n, 99);

    pthread_t   *threads = malloc(T * sizeof(pthread_t));
    ThreadArgs  *args    = malloc(T * sizeof(ThreadArgs));
    if (!threads || !args) { perror("malloc threads"); return 1; }

    /* Pré-calcula intervalos de linhas para cada thread */
    int linhas_base  = n / T;
    int linhas_extra = n % T;   /* as primeiras 'linhas_extra' threads pegam +1 */

    int linha_atual = 0;
    for (int t = 0; t < T; t++) {
        int fatia = linhas_base + (t < linhas_extra ? 1 : 0);
        args[t].A         = A;
        args[t].B         = B;
        args[t].C         = C;
        args[t].n         = n;
        args[t].linha_ini = linha_atual;
        args[t].linha_fim = linha_atual + fatia;
        linha_atual += fatia;
    }

    // Inicia cronomômetro
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (int t = 0; t < T; t++)
        pthread_create(&threads[t], NULL, calcular_bloco, &args[t]);

    for (int t = 0; t < T; t++)
        pthread_join(threads[t], NULL);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    // Finaliza cronômetro e calcula tempo decorrido 

    double elapsed = (t1.tv_sec - t0.tv_sec) +
                     (t1.tv_nsec - t0.tv_nsec) * 1e-9;

    // Checksum para validação cruzada
    double checksum = 0.0;
    for (int i = 0; i < n * n; i++) checksum += C[i];

    printf("Tempo    : %.4f s\n", elapsed);
    printf("Checksum : %.6e\n", checksum);

    // Saída CSV para o script de benchmark
    FILE *f = fopen("results/par.csv", "a");
    if (f) { fprintf(f, "%d,par,%d,%.6f,%.6e\n", n, T, elapsed, checksum); fclose(f); }

    free(threads); free(args);
    free(A); free(B); free(C);
    return 0;
}

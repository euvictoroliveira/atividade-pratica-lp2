#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Preenche a matriz com valores pseudoaleatórios determinísticos
static void preencher(double *M, int n, unsigned int seed) {
    srand(seed);
    for (int i = 0; i < n * n; i++)
        M[i] = (double)(rand() % 100) / 10.0;
}

// Implementação sequencial fornecida no enunciado 
void multiplicar(const double *A, const double *B, double *C, int n) {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            double soma = 0.0;
            for (int k = 0; k < n; k++)
                soma += A[i*n + k] * B[k*n + j];
            C[i*n + j] = soma;
        }
}

int main(int argc, char *argv[]) {
    int n = (argc > 1) ? atoi(argv[1]) : 1000;
    printf("Sequencial — n = %d\n", n);

    double *A = malloc(n * n * sizeof(double));
    double *B = malloc(n * n * sizeof(double));
    double *C = malloc(n * n * sizeof(double));
    if (!A || !B || !C) { perror("malloc"); return 1; }

    // Setup fora do cronômetro 
    preencher(A, n, 42);
    preencher(B, n, 99);

    // Inicia cronomômetro
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    multiplicar(A, B, C, n);

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
    FILE *f = fopen("results/seq.csv", "a");
    if (f) { fprintf(f, "%d,seq,1,%.6f,%.6e\n", n, elapsed, checksum); fclose(f); }

    free(A); free(B); free(C);
    return 0;
}

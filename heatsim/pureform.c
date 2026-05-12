#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>

#define INDEX(i, j, N) ((i) * (N) + (j))

void initialize(double* matrix, int N) {
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            matrix[INDEX(i, j, N)] = 0.0;

    matrix[INDEX(N/2, N/2, N)] = 100.0;
}

void jacobi_iteration(double* current, double* next, int N,
                      int iterations, int threads,
                      omp_sched_t sched, int chunk_size) {
    omp_set_num_threads(threads);
    omp_set_schedule(sched, chunk_size);

    for (int iter = 0; iter < iterations; ++iter) {
        #pragma omp parallel for schedule(runtime)
        for (int i = 1; i < N - 1; ++i) {
            for (int j = 1; j < N - 1; ++j) {
                next[INDEX(i, j, N)] = 0.25 * (
                    current[INDEX(i - 1, j, N)] +
                    current[INDEX(i + 1, j, N)] +
                    current[INDEX(i, j - 1, N)] +
                    current[INDEX(i, j + 1, N)]
                );
            }
        }

        double* temp = current;
        current = next;
        next = temp;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        printf("Használat: %s <matrix_meret> <iteraciok> <szalak> <schedule_tipus> <chunk_meret>\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    int iterations = atoi(argv[2]);
    int threads = atoi(argv[3]);
    char* sched_type_str = argv[4];
    int chunk_size = atoi(argv[5]);

    omp_sched_t schedule;
    if (strcmp(sched_type_str, "static") == 0) schedule = omp_sched_static;
    else if (strcmp(sched_type_str, "dynamic") == 0) schedule = omp_sched_dynamic;
    else if (strcmp(sched_type_str, "guided") == 0) schedule = omp_sched_guided;
    else {
        return 1;
    }

    double* current = (double*)malloc(N * N * sizeof(double));
    double* next = (double*)malloc(N * N * sizeof(double));
    if (!current || !next) {
        fprintf(stderr, "Nem sikerült memóriát lefoglalni.\n");
        return 1;
    }

    initialize(current, N);
    initialize(next, N);

    double start = omp_get_wtime();
    jacobi_iteration(current, next, N, iterations, threads, schedule, chunk_size);
    double end = omp_get_wtime();
    double elapsed = end - start;
    printf("Time taken with %d threads (%s): %.6f seconds\n", threads, sched_type_str, elapsed);

    FILE* f = fopen("results.csv", "a");
    if (f) {
        fprintf(f, "%dx%d,%d,%d,%s,%d,%.6f\n", N, N, iterations, threads,
                sched_type_str, chunk_size, elapsed);
        fclose(f);
    }

    free(current);
    free(next);
    return 0;
}
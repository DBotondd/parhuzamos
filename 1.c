// Generálj egy tömböt és számold ki az összegét:

/*sorosan
OpenMP-vel
mérj időt */

#include <stdio.h>
#include <omp.h>
#include <stdlib.h>

//CSOMAGOT ADD FEL
#define N 10000

int main () {

    int tomb[N];
    
    for (int i = 0; i < N; i++)
    {
        tomb[i] = rand() % 100;
    }

    long long sum = 0;

    double start = omp_get_wtime();

    #pragma omp parallel for

    for (int i = 0; i < N; i++)
    {
        sum += tomb[i];
    }

    double end = omp_get_wtime();

    printf("az osszeg %lld", sum);


    return 0;
    

}

//primkereses openmpvel

#include <stdio.h>
#include <omp.h>

int isPrime(int x) {

    if (x < 2)
    {
        return 0;
    }

    for (int i = 2; i*i <=x ; i++)
    {
        if (x % 2 ==0)
        {
            return 0;
        }
        
    }
    return 1;
    

}

int main() {
    int count = 0;

    double start = omp_get_wtime();

    #pragma omp parallel for schedule(static)

    for (int i = 2; i < 1000000; i++)
    {
        if (isPrime(i))
        {
            count++;
        }
        
    }

    double end = omp_get_wtime();

    printf("Osszeg: %d\n", count);
    printf("Futasi ido: %lf", end - start);

    return 0;
    

}

//Mátrix összeadás OPENMP

#include <stdio.h>
#include <omp.h>

#define N 1000

int A[N][N];
int B[N][N];
int C[N][N];

int main() {

    #pragma omp parallel for collapse(2)
    for(int i = 0; i < N; i++) {

        for(int j = 0; j < N; j++) {

            C[i][j] = A[i][j] + B[i][j];
        }
    }

    return 0;
}


// MÁTRIX SZORZÁS

#include <stdio.h>
#include <omp.h>

#define N 500

int A[N][N];
int B[N][N];
int C[N][N];

int main() {

    #pragma omp parallel for collapse(2)
    for(int i = 0; i < N; i++) {

        for(int j = 0; j < N; j++) {

            int sum = 0;

            for(int k = 0; k < N; k++) {
                sum += A[i][k] * B[k][j];
            }

            C[i][j] = sum;
        }
    }

    return 0;
}

// minden elem szorzása 2 vel

#include <stdio.h>
#include <omp.h>

#define N 1000000

int arr[N];

int main() {

    #pragma omp parallel for
    for(int i = 0; i < N; i++) {
        arr[i] *= 2;
    }

    return 0;
}
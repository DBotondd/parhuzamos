//A Conway-élőjáték CUDA-s változata: eszközfüggvény a szomszédok számolásához, kernel egy lépés végrehajtásához, valamint a GPU-s futtatás és időmérés logikája.
#include "conway.h"                     // Grid típus és ALIVE/DEAD konstansok
    // CUDA kernel indítási paraméterek
#include <cuda_runtime.h>                // CUDA runtime API
#include <device_launch_parameters.h>
// GPU-eszközön futó szomszédszámláló függvény
// Toroid peremkezeléssel (wrap-around), hasonlóan a CPU-s változathoz
__device__ int count_neighbors_gpu(const int* grid, int x, int y, int width, int height) {
    int count = 0;
    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy) {
            if (dx == 0 && dy == 0) continue;  // saját cella kihagyása
            // perem átlépés modulo szerinti wrap-around
            int nx = (x + dx + width) % width;
            int ny = (y + dy + height) % height;
            count += grid[ny * width + nx];    // ALIVE=1, DEAD=0 összeszámolása
        }
    return count;
}

// CUDA kernel a Conway-szimuláció egy lépéséhez
// Minden thread egy cellát számol ki párhuzamosan
__global__ void step_kernel(const int* current, int* next, int width, int height) {
    // Globális koordináták kiszámítása a blokk- és szálindexekből
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    // Érvénytelen indexek (túl a mátrix határain) kiszűrése
    if (x >= width || y >= height) return;

    int idx = y * width + x;  // lineáris index
    int neighbors = count_neighbors_gpu(current, x, y, width, height);

    // Conway szabályok alkalmazása élő és halott sejtekre
    if (current[idx] == ALIVE)
        next[idx] = (neighbors == 2 || neighbors == 3) ? ALIVE : DEAD;
    else
        next[idx] = (neighbors == 3) ? ALIVE : DEAD;
}

// GPU-szimuláció futtatása és időmérés
// host_grid: a hoszt oldali rács, steps: iterációk száma
// kernel_time: kernel futási idő, memcpy_time: adatátviteli idő (ms)
void run_gpu(Grid& host_grid, int width, int height, int steps, float& kernel_time, float& memcpy_time) {
    size_t size = width * height * sizeof(int);
    int *d_current, *d_next;

    // Eszköz memóriaterület foglalása
    cudaMalloc(&d_current, size);
    cudaMalloc(&d_next, size);

    // Események létrehozása időméréshez
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    // Adat másolása hosztról GPU-ra és idő mérése
    cudaEventRecord(start);
    cudaMemcpy(d_current, host_grid.data(), size, cudaMemcpyHostToDevice);
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    cudaEventElapsedTime(&memcpy_time, start, stop);

    // Indítási konfiguráció: 16×16 szál blokkonként
    dim3 threads(16, 16);
    dim3 blocks((width + threads.x - 1) / threads.x,
                (height + threads.y - 1) / threads.y);

    // Kernel iteratív lefuttatása és idő mérése
    cudaEventRecord(start);
    for (int i = 0; i < steps; ++i) {
        step_kernel<<<blocks, threads>>>(d_current, d_next, width, height);
        std::swap(d_current, d_next);  // pointerek cseréje a következő iterációhoz
    }
    cudaDeviceSynchronize();            // várakozás az összes kernel befejezésére
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    cudaEventElapsedTime(&kernel_time, start, stop);

    // Eredmény visszamásolása GPU-ról hosztra
    cudaMemcpy(host_grid.data(), d_current, size, cudaMemcpyDeviceToHost);

    // Memória és események felszabadítása
    cudaFree(d_current);
    cudaFree(d_next);
    cudaEventDestroy(start);
    cudaEventDestroy(stop);
}

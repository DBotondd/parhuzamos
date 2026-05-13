// A Conway-élőjáték CUDA-s változata: eszközfüggvény, kernel és GPU-időmérési logika.

#include "conway.h"           // Grid típus és ALIVE/DEAD konstansok
#include <cuda_runtime.h>     // CUDA runtime API: memóriakezeléshez és eszközvezérléshez
#include <device_launch_parameters.h> // A blokk- és szálindexek (threadIdx, blockIdx) használatához

//------------------------------------------------------------------------------------------------

// DEVICE FÜGGVÉNY: Csak a GPU-n belül hívható meg

// __device__: Azt jelzi, hogy ez a függvény a videókártyán fut és egy másik GPU-s függvény hívja meg
__device__ int count_neighbors_gpu(const int* grid, int x, int y, int width, int height) {
    int count = 0;
    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy) {
            if (dx == 0 && dy == 0) continue; // Saját cella kihagyása
            
            // Toroid peremkezelés: ha kimegyünk a szélén, a túloldalon jövünk be (%)
            int nx = (x + dx + width) % width;
            int ny = (y + dy + height) % height;
            
            // Az élő sejtek (1-es értékek) összeszámolása
            count += grid[ny * width + nx]; 
        }
    return count;
}

//------------------------------------------------------------------------------------------------

// GPU KERNEL: Ez a függvény fut le párhuzamosan több ezer szálon

// __global__: Olyan függvény, amit a CPU indít el, de a GPU hajt végre
__global__ void step_kernel(const int* current, int* next, int width, int height) {
    
    // MEGHATÁROZZUK, MELYIK SEJTÉRT FELEL EZ A SZÁL:
    // A GPU blokkokra és szálakra osztja a munkát, ebből sakkozzuk ki a 2D koordinátákat
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    // Biztonsági ellenőrzés: ha a rács nem osztható pontosan a blokkmérettel, a felesleges szálak ne csináljanak semmit
    if (x >= width || y >= height) return;

    int idx = y * width + x;  // 2D koordináta átalakítása 1D indexszé a memóriában
    int neighbors = count_neighbors_gpu(current, x, y, width, height);

    // CONWAY SZABÁLYOK:
    if (current[idx] == ALIVE)
        next[idx] = (neighbors == 2 || neighbors == 3) ? ALIVE : DEAD;
    else
        next[idx] = (neighbors == 3) ? ALIVE : DEAD;
}

//------------------------------------------------------------------------------------------------

// GPU FUTTATÓ FÜGGVÉNY: A CPU-n fut, ez vezérli a videókártyát

void run_gpu(Grid& host_grid, int width, int height, int steps, float& kernel_time, float& memcpy_time) {
    size_t size = width * height * sizeof(int); // A mátrix mérete bájtokban
    int *d_current, *d_next; // Mutatók a GPU memóriájára (d_ mint device)

    // 1. MEMÓRIAFOGLALÁS A GPU-N
    cudaMalloc(&d_current, size); // Helyet kérünk a videókártya RAM-jában
    cudaMalloc(&d_next, size);

    // 2. IDŐMÉRŐ ESEMÉNYEK ELŐKÉSZÍTÉSE
    cudaEvent_t start, stop;
    cudaEventCreate(&start);
    cudaEventCreate(&stop);

    // 3. ADATMÁSOLÁS (CPU -> GPU) ÉS MÉRÉSE
    cudaEventRecord(start);
    // Átmásoljuk a kezdő rácsot a gép memóriájából a videókártyára
    cudaMemcpy(d_current, host_grid.data(), size, cudaMemcpyHostToDevice);
    cudaEventRecord(stop);
    cudaEventSynchronize(stop); // Megvárjuk, amíg a másolás tényleg befejeződik
    cudaEventElapsedTime(&memcpy_time, start, stop); // Kiszámoljuk az adatátvitel idejét

    // 4. GRID KONFIGURÁCIÓ: Hogyan osszuk el a munkát?
    dim3 threads(16, 16); // Egy blokkban 16x16 = 256 szál dolgozik
    dim3 blocks((width + threads.x - 1) / threads.x,
                (height + threads.y - 1) / threads.y); // Kiszámoljuk, hány blokk kell a teljes képhez

    // 5. SZÁMÍTÁS FUTTATÁSA A GPU-N
    cudaEventRecord(start);
    for (int i = 0; i < steps; ++i) {
        // Elindítjuk a kernelt: a <<<blocks, threads>>> mondja meg a GPU-nak a párhuzamosság mértékét
        step_kernel<<<blocks, threads>>>(d_current, d_next, width, height);
        
        // POINTER CSERE: A GPU-n is csak a címeket cseréljük fel, nem másolunk adatot
        std::swap(d_current, d_next); 
    }
    cudaDeviceSynchronize(); // Megvárjuk, amíg az összes GPU mag végez az 1000 körrel
    cudaEventRecord(stop);
    cudaEventSynchronize(stop);
    cudaEventElapsedTime(&kernel_time, start, stop); // Kiszámoljuk a tiszta számítási időt

    // 6. EREDMÉNY VISSZAMÁSOLÁSA (GPU -> CPU)
    // A végeredményt visszakérjük a videókártyáról a gép memóriájába
    cudaMemcpy(host_grid.data(), d_current, size, cudaMemcpyDeviceToHost);

    // 7. TAKARÍTÁS
    cudaFree(d_current); // Felszabadítjuk a GPU memóriát
    cudaFree(d_next);
    cudaEventDestroy(start);
    cudaEventDestroy(stop);
}
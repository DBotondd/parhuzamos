// Végigfuttatja és méri a CPU-s és GPU-s implementációt különböző rácsméreteken, majd kiírja az időket és a gyorsulást.
#include "conway.h"      // Grid típus és ALIVE/DEAD konstansok
#include "utils.h"       // Timer osztály (tic/toc funkciók)
#include <iostream>    // std::cout, std::endl
#include <fstream>

// CPU és GPU futtató függvények deklarációja
void step_cpu(const Grid&, Grid&, int, int);
void run_gpu(Grid&, int, int, int, float&, float&);

int main() {
    // Tesztelendő rácsméretek tömbje
    const int sizes[] = {256, 512, 1024};
    const int steps = 1000;  // iterációk száma minden méretnél

    // Végigmegyünk minden méreten
    for (int size : sizes) {
        int width = size, height = size;
        // Rácsok lefoglalása (egydimenziós vektorok)
        Grid current(width * height), next(width * height);

        // Kezdeti random állapot beállítása
        initialize_grid(current, width, height);

        std::cout << "\n=== Grid Size: " << width << "x" << height << " ===\n";

        // CPU-szimuláció futtatása és időmérése
        Timer timer;
        timer.tic();                              // időzítő indítása
        for (int i = 0; i < steps; ++i) {
            step_cpu(current, next, width, height);  // egy lépés CPU-n
            current.swap(next);                     // rácsok felcserélése a következő iterációhoz
        }
        double cpu_time = timer.toc();            // időzítő leállítása és visszaolvasás (ms)
        std::cout << "CPU time: " << cpu_time << " ms\n";

        // GPU-szimuláció futtatása: először újrainicializáljuk a rácsot
        initialize_grid(current, width, height);
        float kernel_time, memcpy_time;
        run_gpu(current, width, height, steps, kernel_time, memcpy_time);
        std::cout << "GPU kernel time: " << kernel_time << " ms\n";
        std::cout << "GPU memcpy time: " << memcpy_time << " ms\n";
        std::cout << "Total GPU time: " << (kernel_time + memcpy_time) << " ms\n";

        // Gyorsulás kiszámítása
        std::cout << "Speedup: "
                  << cpu_time / (kernel_time + memcpy_time)
                  << "x\n";

       std::ofstream out("../results.csv", std::ios::app); // egy szinttel feljebb (projektgyökérbe)
 // app = hozzáfűzés
out << size << "," << cpu_time << "," << kernel_time << "," << memcpy_time << "\n";
    }

    


    return 0;  // program sikeres befejezése
}
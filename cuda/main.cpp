// Végigfuttatja és méri a CPU-s és GPU-s implementációt különböző rácsméreteken, majd kiírja az időket és a gyorsulást.

#include "conway.h"     // " ": saját fejléc fájl; conway.h: Grid típus (std::vector<int>) és ALIVE/DEAD konstansok (0 és 1)
#include "utils.h"      // utils.h: segédfüggvények gyűjteménye; Timer osztály deklarációja a mérésekhez (tic/toc)
#include <iostream>     // <iostream>: Input-Output Stream (standard kiíratáshoz: std::cout, std::endl)
#include <fstream>      // <fstream>: File Stream (fájlműveletekhez, pl. eredmények elmentése CSV fájlba)

//------------------------------------------------------------------------------------------------

// FÜGGVÉNY PROTOTÍPUSOK (DEKLARÁCIÓK)
// Ezek a függvények más forrásfájlokban (.cpp vagy .cu) vannak definiálva.

void step_cpu(const Grid&, Grid&, int, int); // void: nincs visszatérés; const Grid&: bemeneti rács (olvasható); Grid&: kimeneti rács (írható)
void run_gpu(Grid&, int, int, int, float&, float&); // float&: cím szerinti átadás az időmérési eredmények visszaadásához (kernel és memóriaidő)

//------------------------------------------------------------------------------------------------

// MAIN FÜGGVÉNY: A tesztkörnyezet összeállítása és futtatása

int main() {            // int: visszatérési típus; main: a program belépési pontja
    
    // TESZTPARAMÉTEREK BEÁLLÍTÁSA
    const int sizes[] = {256, 512, 1024}; // const: módosíthatatlan; sizes: tömbnév; { }: rácsszélességek/magasságok listája
    const int steps = 1000;               // steps: konstans egész; ennyi generáción keresztül futtatjuk a szimulációt méretenként

    // MÉRÉSI CIKLUS: Végigmegyünk minden méreten (range-based for loop)
    for (int size : sizes) {              // size: az aktuális méret a tömbből
        int width = size, height = size;  // width, height: szélesség és magasság beállítása (négyzetes rács)
        
        // RÁCSOK MEMÓRIAFOGLALÁSA
        // Grid: típus (std::vector); (width * height): kezdeti méretfoglalás a memóriában
        Grid current(width * height), next(width * height); 

        // INICIALIZÁLÁS
        initialize_grid(current, width, height); // Véletlenszerűen feltölti a rácsot cellákkal a kezdéshez

        std::cout << "\n=== Grid Size: " << width << "x" << height << " ===\n"; // Fejléc kiírása a konzolra

        //----------------------------------------------------------------------------------------
        
        // CPU SZIMULÁCIÓ ÉS IDŐMÉRÉS
        Timer timer;        // Timer: objektum létrehozása az időméréshez
        timer.tic();        // timer.tic(): elindítja a stoppert (időpont mentése)
        
        for (int i = 0; i < steps; ++i) { 
            step_cpu(current, next, width, height);  // Conway-szabályok kiszámítása a CPU-n
            current.swap(next);                      // .swap(): hatékony mutatócsere a két állapot között
        }
        
        double cpu_time = timer.toc();               // timer.toc(): leállítja az órát, eredmény ezredmásodpercben (ms)
        std::cout << "CPU time: " << cpu_time << " ms\n";

        //----------------------------------------------------------------------------------------

        // GPU SZIMULÁCIÓ ÉS IDŐMÉRÉS
        initialize_grid(current, width, height);     // Újrainicializálás a fair összehasonlításhoz
        float kernel_time, memcpy_time;              // Lebegőpontos változók a GPU mérésekhez
        
        // run_gpu(): CPU->GPU másolás, 1000 lépés futtatása a GPU-n, majd visszamásolás
        run_gpu(current, width, height, steps, kernel_time, memcpy_time); 
        
        std::cout << "GPU kernel time: " << kernel_time << " ms\n"; // Tiszta számítási idő a videókártyán
        std::cout << "GPU memcpy time: " << memcpy_time << " ms\n"; // Adatmozgatás ideje a PCIE buszon keresztül
        std::cout << "Total GPU time: " << (kernel_time + memcpy_time) << " ms\n"; 

        //----------------------------------------------------------------------------------------

        // GYORSULÁS (SPEEDUP) ÉS STATISZTIKA
        // Hányszor gyorsabb a GPU a CPU-nál (CPU_idő / GPU_összes_idő)
        std::cout << "Speedup: " << cpu_time / (kernel_time + memcpy_time) << "x\n"; 

        // EREDMÉNYEK MENTÉSE CSV FÁJLBA
        std::ofstream out("../results.csv", std::ios::app); // std::ios::app: hozzáfűzés (nem törli a régit)
        
        if (out.is_open()) {
            // Adatok beírása: Méret, CPU idő, GPU kernel idő, Másolási idő
            out << size << "," << cpu_time << "," << kernel_time << "," << memcpy_time << "\n"; 
        }

    } // for ciklus vége: ugrás a következő rácsméretre

    return 0;  // return 0: jelzi az operációs rendszernek, hogy a futás hiba nélkül zárult
} // main függvény vége
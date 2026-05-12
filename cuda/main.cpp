// Végigfuttatja és méri a CPU-s és GPU-s implementációt különböző rácsméreteken, majd kiírja az időket és a gyorsulást.
#include "conway.h"     // " ": saját fejléc fájl; conway.h: Grid típus (std::vector<int>) és ALIVE/DEAD konstansok (0 és 1)
#include "utils.h"      // utils.h: segédfüggvények gyűjteménye; Timer osztály deklarációja a mérésekhez (tic/toc)
#include <iostream>     // <iostream>: Input-Output Stream (standard kiíratáshoz: std::cout, std::endl)
#include <fstream>      // <fstream>: File Stream (fájlműveletekhez, pl. eredmények elmentése CSV fájlba)

// CPU és GPU futtató függvények prototípusai (deklarációja), amelyek más forrásfájlokban vannak megírva
void step_cpu(const Grid&, Grid&, int, int); // void: nincs visszatérés; const Grid&: bemeneti rács (olvasható); Grid&: kimeneti rács (írható)
void run_gpu(Grid&, int, int, int, float&, float&); // float&: cím szerinti átadás az időmérési eredmények visszaadásához (kernel és memóriaidő)

int main() {            // int: visszatérési típus; main: a program belépési pontja
    // Tesztelendő rácsméretek tömbje
    const int sizes[] = {256, 512, 1024}; // const: módosíthatatlan; sizes: tömbnév; { }: rácsszélességek/magasságok listája
    const int steps = 1000;  // steps: konstans egész; 1000: ennyi generáción keresztül futtatjuk a szimulációt méretenként

    // Végigmegyünk minden méreten (range-based for loop)
    for (int size : sizes) { // size: az aktuális méret a tömbből; for: ciklus, amely annyiszor fut, ahány elem van a sizes-ban
        int width = size, height = size; // width, height: szélesség és magasság beállítása az aktuális négyzetes rácshoz
        
        // Rácsok lefoglalása (egydimenziós vektorok a memóriában)
        Grid current(width * height), next(width * height); // Grid: típus (std::vector); (width * height): kezdeti méretfoglalás

        // Kezdeti random állapot beállítása a szimulációhoz
        initialize_grid(current, width, height); // meghívja a segédfüggvényt, ami véletlenszerűen feltölti a rácsot cellákkal

        std::cout << "\n=== Grid Size: " << width << "x" << height << " ===\n"; // std::cout: kiíratás konzolra; \n: újsor

        // CPU-szimuláció futtatása és időmérése
        Timer timer;        // Timer: objektum létrehozása az utils.h-ban definiált osztály alapján
        timer.tic();        // timer.tic(): metódushívás; elindítja a stoppert (aktuális időpont mentése)
        
        for (int i = 0; i < steps; ++i) { // i: ciklusváltozó; i < steps: 1000-szer fut le
            step_cpu(current, next, width, height);  // step_cpu(): a Conway-szabályok kiszámítása a CPU magján
            current.swap(next);                     // .swap(): hatékony vektorcsere (csak a mutatók cserélődnek, nem másol adatot)
        }
        
        double cpu_time = timer.toc();            // timer.toc(): leállítja az órát és visszaadja az eltelt időt ezredmásodpercben (ms)
        std::cout << "CPU time: " << cpu_time << " ms\n"; // az eredmény kiírása a képernyőre

        // GPU-szimuláció futtatása: először újrainicializáljuk a rácsot, hogy tiszta lappal induljon a mérés
        initialize_grid(current, width, height);  // visszaállítjuk az alaphelyzetet a fair összehasonlításhoz
        float kernel_time, memcpy_time;           // float: lebegőpontos változók a GPU specifikus mérésekhez
        
        // run_gpu(): elvégzi a CPU->GPU másolást, a 1000 lépést a videókártyán, majd a visszamásolást
        run_gpu(current, width, height, steps, kernel_time, memcpy_time); 
        
        std::cout << "GPU kernel time: " << kernel_time << " ms\n"; // Csak a számítási idő a GPU-n
        std::cout << "GPU memcpy time: " << memcpy_time << " ms\n"; // Az adatmozgatás ideje (PCIE busz)
        std::cout << "Total GPU time: " << (kernel_time + memcpy_time) << " ms\n"; // Összesített GPU költség

        // Gyorsulás (Speedup) kiszámítása: hányszor gyorsabb a GPU a CPU-nál
        std::cout << "Speedup: " 
                  << cpu_time / (kernel_time + memcpy_time) // osztás: CPU_idő / GPU_idő
                  << "x\n"; // 'x' jelzés a szorzóra

        // Eredmények fájlba írása statisztikai célból
        std::ofstream out("../results.csv", std::ios::app); // std::ofstream: kimeneti fájlfolyam; "../": relatív útvonal; .csv: formátum
        // std::ios::app: append mód; jelentése: nem törli a fájlt, hanem a végéhez fűzi az új sorokat
        
        // out <<: adatok beírása a fájlba vesszővel elválasztva (Excel-ben így oszlopokba kerül)
        out << size << "," << cpu_time << "," << kernel_time << "," << memcpy_time << "\n"; 
    } // for ciklus vége: ugrás a következő rácsméretre

    return 0;  // return 0: a main függvény visszatérése; jelzi az operációs rendszernek, hogy a futás hiba nélkül zárult
} // main függvény vége
//Egyszerű Timer struktúra, amely a <chrono> használatával méri az eltelt időt milliszekundumban.
#ifndef UTILS_H           // Include-őrt (guard) a többszörös behivatkozás elkerülésére
#define UTILS_H

#include <chrono>          // C++11 időméréshez szükséges fejlécek

// Egyszerű időmérő struktúra a kód futási idejének méréséhez
struct Timer {
    std::chrono::high_resolution_clock::time_point start; // a mérések kezdő időpontja

    // A mérés indítása: a start időpont rögzítése
    void tic() { start = std::chrono::high_resolution_clock::now(); }

    // A mérés leállítása és az eltelt idő visszaadása ms-ban
    double toc() {
        auto end = std::chrono::high_resolution_clock::now();  // befejezés időpontja
        // Különbség számítása milliszekundumban
        std::chrono::duration<double, std::milli> diff = end - start;
        return diff.count();  // az eltelt idő double típusban
    }
};

#endif // UTILS_H vége
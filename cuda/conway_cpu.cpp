// A Conway-féle életjáték CPU-s implementációja: rács inicializálása, szomszédszámlálás és egy iteráció végrehajtása.

#include "conway.h"          // Saját fejléc: Grid típus (std::vector) és ALIVE (1) / DEAD (0) konstansok
#include <random>             // Modern C++ véletlenszám-generátor könyvtár

//------------------------------------------------------------------------------------------------

// RÁCS INICIALIZÁLÁSA: A kezdőállapot véletlenszerű generálása

void initialize_grid(Grid& grid, int width, int height) {
    // mt19937: Mersenne Twister generátor; 42: fix kezdőérték (seed), hogy minden futtatáskor ugyanazt a mintát kapjuk
    std::mt19937 rng(42); 
    
    // uniform_int_distribution: Meghatározza a tartományt; 0 vagy 1 értéket fogunk kapni egyenlő eséllyel
    std::uniform_int_distribution<> dist(0, 1); 

    // Végigmegyünk a teljes rácson (szélesség * magasság összesen)
    for (int i = 0; i < width * height; ++i) {
        // Minden cellát véletlenszerűen beállítunk élőre (1) vagy halottra (0)
        grid[i] = dist(rng); 
    }
}

//------------------------------------------------------------------------------------------------

// SZOMSZÉDOK SZÁMOLÁSA: Megnézzük, hány élő sejt van egy adott cella körül

int count_neighbors(const Grid& grid, int x, int y, int width, int height) {
    int count = 0; // Itt gyűjtjük az élő szomszédokat

    // Két egymásba ágyazott ciklus (-1, 0, 1 eltolásokkal) bejárja a 3x3-as környezetet
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            
            // Ha dx és dy is 0, akkor az a középső cella (saját magunk) – ezt nem számoljuk szomszédnak
            if (dx == 0 && dy == 0) continue; 

            // TOROID KEZELÉS (Fánk forma): Ha kilépnénk a szélen, a túlsó oldalon jövünk vissza
            // A modulo (%) művelet biztosítja, hogy az index mindig 0 és (szélesség-1) között maradjon
            int nx = (x + dx + width) % width;
            int ny = (y + dy + height) % height;

            // Mivel ALIVE = 1 és DEAD = 0, az érték hozzáadása pont az élő sejteket számolja meg
            count += grid[ny * width + nx]; 
        }
    }
    return count; // Visszaadjuk az élő szomszédok számát (0 és 8 között)
}

//------------------------------------------------------------------------------------------------

// SZIMULÁCIÓS LÉPÉS (CPU): A szabályok alkalmazása minden egyes cellára

void step_cpu(const Grid& current, Grid& next, int width, int height) {
    
    // Kétdimenziós bejárás: minden soron (y) és minden oszlopon (x) végigmegyünk
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            
            // Kiszámoljuk az aktuális cella szomszédait
            int neighbors = count_neighbors(current, x, y, width, height);
            
            // Kiszámoljuk a cella pontos helyét (indexét) az egydimenziós vektorban
            int idx = y * width + x; 

            // CONWAY SZABÁLYAI:
            if (current[idx] == ALIVE) {
                // TÚLÉLÉS: Ha élő sejtnek 2 vagy 3 szomszédja van, életben marad, különben (magány vagy túlnépesedés) meghal
                next[idx] = (neighbors == 2 || neighbors == 3) ? ALIVE : DEAD;
            } 
            else {
                // SZÜLETÉS: Ha egy halott cellának pontosan 3 élő szomszédja van, "kikel" és élő lesz
                next[idx] = (neighbors == 3) ? ALIVE : DEAD;
            }
        }
    }
}
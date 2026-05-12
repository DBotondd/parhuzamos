//A Conway-féle életjáték CPU-s implementációja: rács inicializálása, szomszédszámlálás és egy iteráció végrehajtása.
#include "conway.h"          // Grid típus és ALIVE/DEAD konstansok definiálva
#include <random>             // véletlenszám-generátor használata

// A rács véletlenszerű inicializálása élő és halott sejtekre
void initialize_grid(Grid& grid, int width, int height) {
    std::mt19937 rng(42);                   // Mersenne Twister RNG rögzített seed-del (reprodukálható eredmény)
    std::uniform_int_distribution<> dist(0, 1);  // Egyenletes eloszlás 0 és 1 között

    // Minden cellára generálunk 0 vagy 1 értéket
    for (int i = 0; i < width * height; ++i)
        grid[i] = dist(rng);               // 0: DEAD, 1: ALIVE
}

// Szomszédok számolása toroid (donut) peremkezeléssel
int count_neighbors(const Grid& grid, int x, int y, int width, int height) {
    int count = 0;
    // dx, dy = -1, 0, 1 kombinációi, kivéve (0,0) – a saját cellát kihagyjuk
    for (int dx = -1; dx <= 1; ++dx)
        for (int dy = -1; dy <= 1; ++dy) {
            if (dx == 0 && dy == 0) continue;  // ne számoljuk magát a cellát
            // peremeken átlépés: modulo művelet segítségével wrap-around
            int nx = (x + dx + width) % width;
            int ny = (y + dy + height) % height;
            count += grid[ny * width + nx];    // ALIVE=1, DEAD=0, így egyszerű összeadás
        }
    return count;  // élő szomszédok száma
}

// Egy iteráció végrehajtása a CPU-n
void step_cpu(const Grid& current, Grid& next, int width, int height) {
    // minden sor és oszlop bejárása
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            int neighbors = count_neighbors(current, x, y, width, height);
            int idx = y * width + x;             // lineáris index a rácsban
            if (current[idx] == ALIVE)
                // ha élő sejt, 2 vagy 3 szomszéd esetén életben marad, különben meghal
                next[idx] = (neighbors == 2 || neighbors == 3) ? ALIVE : DEAD;
            else
                // ha halott és pontosan 3 élő szomszéd, újraéled
                next[idx] = (neighbors == 3) ? ALIVE : DEAD;
        }
}

//Meghatározza a Grid típusát, az ALIVE/DEAD konstansokat és a rácskezelő függvények prototípusait.
#ifndef CONWAY_H       // Ha még nincs definiálva a CONWAY_H, definiáljuk a fejlécet
#define CONWAY_H

#include <vector>    // std::vector használata az rács tárolásához

// Sejtállapot konstansok
constexpr int ALIVE = 1; // élő sejt értéke, érték fix forditás során véglegesedik
constexpr int DEAD = 0;  // halott sejt értéke

// Grid típus alias: egydimenziós vektor int-ekből, sorfolytonos rács reprezentációhoz
using Grid = std::vector<int>;

// Függvényprototípusok
// Véletlenszerűen inicializálja a grid-et a megadott szélesség és magasság alapján
void initialize_grid(Grid& grid, int width, int height);
// Kiírja a grid tartalmát a konzolra mátrix formátumban
void print_grid(const Grid& grid, int width, int height);

#endif 
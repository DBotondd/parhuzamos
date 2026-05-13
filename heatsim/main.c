#include <stdio.h>       // #include: könyvtár beillesztése; <stdio.h>: alapvető I/O (printf, fprintf)
#include <stdlib.h>      // <stdlib.h>: általános függvények (malloc: foglalás, free: felszabadítás, atoi: szöveg->szám)
#include <omp.h>         // <omp.h>: OpenMP API; szálkezelés, ütemezés és nagy pontosságú időmérés (omp_get_wtime)
#include <string.h>      // <string.h>: karakterlánc-műveletek; itt a 'strcmp' (összehasonlítás) miatt kell

// Makró: fordítás előtt behelyettesített szövegrész; N: szélesség; i,j: koordináták
#define INDEX(i, j, N) ((i) * (N) + (j)) // Egy 1D tömböt 2D mátrixként kezel: sorindex * szélesség + oszlopindex

//------------------------------------------------------------------------------------------------

// INICIALIZÁLÁS: A mátrix alaphelyzetbe állítása

// matrix: mutató a memóriaterületre; N: mátrix oldalhossza
void initialize(double* matrix, int N) { // void: nincs visszatérési érték; double*: tizedestört mutató
    for (int i = 0; i < N; ++i)          // külső ciklus: végigmegy az összes soron (0-tól N-1-ig)
        for (int j = 0; j < N; ++j)      // belső ciklus: végigmegy az adott sor összes oszlopán
            matrix[INDEX(i, j, N)] = 0.0; // minden rácspont hőmérsékletét alaphelyzetbe (0.0) állítja

    // N/2: egész osztás, megkeresi a középső indexet; 100.0: egy forró pontot helyez el
    matrix[INDEX(N/2, N/2, N)] = 100.0; // fix hőforrás beállítása a mátrix közepén
}

//------------------------------------------------------------------------------------------------

// JACOBI ITERÁCIÓ: A párhuzamosított számítási mag

// current/next: két puffer az állapotoknak; iterations: ciklusok száma; threads: magok száma
// sched/chunk_size: OpenMP specifikus paraméterek a munka elosztásához
void jacobi_iteration(double* current, double* next, int N,
                      int iterations, int threads,
                      omp_sched_t sched, int chunk_size) {
    omp_set_num_threads(threads);        // omp_set_num_threads: megadja az OpenMP-nek, hány szálat indítson el
    omp_set_schedule(sched, chunk_size); // beállítja az ütemezési stratégiát (hogyan ossza szét a sorokat)

    for (int iter = 0; iter < iterations; ++iter) {  // időbeli iteráció: ennyiszer frissítjük a teljes mátrixot
        #pragma omp parallel for schedule(runtime)   // pragma: fordítónak szóló utasítás; parallel for: ciklus párhuzamosítása
        // schedule(runtime): a futás közben (omp_set_schedule által) megadott módot használja
        for (int i = 1; i < N - 1; ++i) {            // 1-től N-2-ig megy: a széleket (peremfeltétel) nem bántja
            for (int j = 1; j < N - 1; ++j) {        // belső pontok oszlopindexei
                // 0.25 *: átlagolás; a Jacobi-módszer lényege: egy pont új értéke a 4 szomszédja átlaga
                next[INDEX(i, j, N)] = 0.25 * (
                    current[INDEX(i - 1, j, N)] + // felső szomszéd
                    current[INDEX(i + 1, j, N)] + // alsó szomszéd
                    current[INDEX(i, j - 1, N)] + // bal szomszéd
                    current[INDEX(i, j + 1, N)]   // jobb szomszéd
                );
            } // belső ciklus vége
        } // párhuzamosított ciklus vége

        // Pointer csere (Swap): nem másolunk adatot, csak a mutatók címét cseréljük fel
        double* temp = current; // temp: ideiglenes mentés a címnek
        current = next;         // az eddigi "következő" állapot lesz az alapja az újabb körnek
        next = temp;            // a régi állapot helyére írjuk majd a következő eredményt
    }
}

//------------------------------------------------------------------------------------------------

// MAIN FÜGGVÉNY: Paraméterek beolvasása, memóriakezelés és mérés

int main(int argc, char* argv[]) { // argc: paraméterek száma; argv: paraméterek szöveges tömbje
    
    // Argumentum ellenőrzés
    if (argc != 6) {               // ha nem pontosan 5 paramétert (plusz a program neve) kapott
        printf("Használat: %s <matrix_meret> <iteraciok> <szalak> <schedule_tipus> <chunk_meret>\n", argv[0]);
        return 1;                  // hiba jelzése az operációs rendszer felé
    }

    // Paraméterek feldolgozása
    int N = atoi(argv[1]);          // atoi: karakterláncból egész számot csinál (mátrix méret)
    int iterations = atoi(argv[2]); // iterációk száma
    int threads = atoi(argv[3]);    // használandó szálak száma
    char* sched_type_str = argv[4]; // ütemezési mód neve szövegként
    int chunk_size = atoi(argv[5]); // darabolási méret (hány sor jusson egyszerre egy szálnak)

    // Ütemezési mód beállítása
    omp_sched_t schedule;           // OpenMP belső típusa az ütemezéshez
    // strcmp: összehasonlítja a bemenetet a kulcsszavakkal; 0 értéket ad, ha egyeznek
    if (strcmp(sched_type_str, "static") == 0) schedule = omp_sched_static; // fix kiosztás
    else if (strcmp(sched_type_str, "dynamic") == 0) schedule = omp_sched_dynamic; // sorban kérik a szálak a munkát
    else if (strcmp(sched_type_str, "guided") == 0) schedule = omp_sched_guided; // csökkenő méretű csomagok
    else {
        return 1; // ismeretlen paraméter esetén kilépés
    }

    // Memória foglalása
    // malloc: memóriafoglalás a heap-en; sizeof(double): egy szám mérete bájtokban; N*N: összes elem
    double* current = (double*)malloc(N * N * sizeof(double)); // (double*): kényszerített típusátalakítás (cast)
    double* next = (double*)malloc(N * N * sizeof(double));    // puffer a számítások eredményének
    if (!current || !next) {       // hibakezelés: ha elfogyott a RAM, a malloc NULL-t ad vissza
        fprintf(stderr, "Nem sikerült memóriát lefoglalni.\n"); // stderr: hiba-adatfolyam
        return 1;
    }

    // Inicializálás
    initialize(current, N);        // kezdőállapot beállítása
    initialize(next, N);           // célpuffer alaphelyzetbe állítása

    // Időmérés és futtatás
    double start = omp_get_wtime(); // omp_get_wtime: "falóra" idő, másodpercben adja vissza a pontos időt
    jacobi_iteration(current, next, N, iterations, threads, schedule, chunk_size); // Fő számítási blokk
    double end = omp_get_wtime();   // mérés vége
    double elapsed = end - start;   // kivonás: megkapjuk a futási időt
    printf("Time taken with %d threads (%s): %.6f seconds\n", threads, sched_type_str, elapsed);

    // Eredmények fájlba írása
    FILE* f = fopen("results.csv", "a"); // fopen: fájl megnyitása; "a": append (hozzáfűzés a fájl végéhez)
    if (f) {
        // fprintf: formázott írás fájlba; CSV formátum: vesszővel elválasztott értékek
        fprintf(f, "%dx%d,%d,%d,%s,%d,%.6f\n", N, N, iterations, threads,
                sched_type_str, chunk_size, elapsed);
        fclose(f);                  // fclose: fájl lezárása és puffer ürítése a lemezre
    }

    // Felszabadítás
    free(current); // free: a malloc-al foglalt memória felszabadítása (visszaadjuk a rendszernek)
    free(next);    // felszabadítás a memóriaszivárgás megelőzésére
    return 0;      // sikeres befejezés
}
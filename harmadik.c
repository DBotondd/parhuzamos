#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void* gyerek_szal_A(void* arg) {
    printf("[Gyerek] Elindultam, 4 másodpercig dolgozom...\n");
    sleep(4);
    printf("[Gyerek] Befejeztem a munkát.\n");
    return NULL;
}

int main() {
    pthread_t szal;
    pthread_create(&szal, NULL, gyerek_szal_A, NULL);

    printf("[Fő szál] Elindultam, 8 másodpercig dolgozom...\n");
    sleep(8);
    printf("[Fő szál] Befejeztem. Kilépés.\n");

    return 0;
}



//MASODIK



#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define SZALAK_SZAMA 60

void* munka(void* arg) {
    sleep(1); // 1 másodperces munka szimulálása
    return NULL;
}

int main() {
    pthread_t szal_id[SZALAK_SZAMA];
    
    // Időmérés kezdete
    clock_t start = clock(); 
    // Valós (falióra) idő mérése pontosabb, de a sleep() miatt a clock() eltérhet, 
    // ezért time_t-vel is mérhetjük a látványos 1 másodperces eredményhez:
    time_t start_time = time(NULL);

    // 60 szál elindítása
    for (int i = 0; i < SZALAK_SZAMA; i++) {
        pthread_create(&szal_id[i], NULL, munka, NULL);
    }

    // Megvárjuk az összes szálat
    for (int i = 0; i < SZALAK_SZAMA; i++) {
        pthread_join(szal_id[i], NULL);
    }

    time_t end_time = time(NULL);
    printf("A program teljes futási ideje: %ld másodperc.\n", end_time - start_time);

    return 0;
}


// HARMADIK

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>

#define SZALAK_SZAMA 10

// Globális tömb az eredményeknek
int eredmenyek[SZALAK_SZAMA];

// Egyszerű prímteszt
bool is_prime(int n) {
    if (n < 2) return false;
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) return false;
    }
    return true;
}

void* prim_szamlalo(void* arg) {
    int index = *(int*)arg;
    free(arg); // Felszabadítjuk a fő szál által lefoglalt memóriát

    int start = index * 100;
    int end = start + 99;
    int db = 0;

    for (int i = start; i <= end; i++) {
        if (is_prime(i)) {
            db++;
        }
    }

    // Közvetlenül a globális tömb megfelelő indexére írunk
    eredmenyek[index] = db;
    
    return NULL;
}

int main() {
    pthread_t szal_id[SZALAK_SZAMA];

    // Szálak indítása
    for (int i = 0; i < SZALAK_SZAMA; i++) {
        int* arg = malloc(sizeof(*arg));
        *arg = i;
        pthread_create(&szal_id[i], NULL, prim_szamlalo, arg);
    }

    // Szálak bevárása
    for (int i = 0; i < SZALAK_SZAMA; i++) {
        pthread_join(szal_id[i], NULL);
    }

    // Eredmények kiíratása
    int osszesen = 0;
    for (int i = 0; i < SZALAK_SZAMA; i++) {
        printf("[%d, %d] intervallumban a prímek száma: %d\n", i*100, i*100+99, eredmenyek[i]);
        osszesen += eredmenyek[i];
    }
    printf("Összesen 0 és 999 között: %d prím van.\n", osszesen);

    return 0;
}

//NEGYEDIK

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void* hibas_szal(void* arg) {
    sleep(2);
    printf("[Hibás szál] Most elkövetek egy Segmentation Faultot...\n");
    
    int* p = NULL;
    *p = 42; // Nullpointer dereferálás -> Crash!
    
    return NULL;
}

int main() {
    pthread_t szal;
    pthread_create(&szal, NULL, hibas_szal, NULL);

    printf("[Fő szál] Én futnék 5 másodpercig, de a gyerek szál le fog állítani...\n");
    for (int i = 1; i <= 5; i++) {
        sleep(1);
        printf("[Fő szál] %d. másodperc\n", i);
    }

    pthread_join(szal, NULL);
    return 0;
}
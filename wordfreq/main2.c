#include <stdio.h>      // Standard input/output függvények (printf, fopen, stb.)
#include <stdlib.h>     // Memóriakezelés és egyéb segédfüggvények (malloc, free, atoi)
#include <pthread.h>    // POSIX szálkezelés (threadek létrehozása és kezelése)
#include <string.h>     // String műveletek (strcmp, strncpy, memset)
#include <ctype.h>      // Karaktervizsgáló függvények (isalpha, tolower)
#include <sys/stat.h>   // Fájl statisztikák lekérdezése (stat)
#include <time.h>       // Időméréshez szükséges függvények (clock)

#define MAX_WORD_LEN 64     // Egy szó maximális hossza
#define HASH_SIZE 10007     // Hash tábla mérete
#define TOP_N 20            // Kiírandó leggyakoribb szavak száma

//------------------------------------------------------------------------------------------------
// A SZÓ STRUKTÚRÁJA

typedef struct WordNode {               
    char word[MAX_WORD_LEN];            // A tárolt szó
    int count;                          // A szó előfordulásainak száma
    struct WordNode* next;              // Következő elem pointere
} WordNode;                             

// MAGA A HASHTÁBLA 

typedef struct {                        
    WordNode* table[HASH_SIZE];         // Hash tábla pointerekkel
} HashMap;                              

// THREAD PARAMÉTEREK: milyen szövegrészt dolgozzon fel, melyik hashmapbe írjon

typedef struct {                        
    char* buffer;                       // A teljes fájl tartalma
    size_t start;                       // Feldolgozás kezdő indexe
    size_t end;                         // Feldolgozás záró indexe
    HashMap* local_map;                 // A szál saját lokális hash mapje
} ThreadArg;

//----------------------------------------------------------------------------------------------------

pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex: egyszerre csak egy thread írhat a globális mapbe
HashMap global_map;                     // Globális hash map: ide kerül a végső összesített eredmény

// HASH FÜGGVÉNY: szóból számot készít, megmondja melyik bucketbe kerül a szó

unsigned int hash(const char* str) {    // Hash függvény definíció
    unsigned int h = 0;                 // Kezdő hash érték

    while (*str) {                      // Amíg nem érjük el a string végét addig
        h = (h * 31 + *str++) % HASH_SIZE; // Karakterenként hash számítás
    }

    return h;                           // Hash érték visszaadása
}

// INSERT FÜGGVÉNY: szó beszúrása 

void hashmap_insert(HashMap* map, const char* word) { // Szó beszúrása a hash mapbe
    unsigned int h = hash(word);        // Hash index kiszámítása

    WordNode* node = map->table[h];     // Az adott bucket első eleme

    while (node) {                      // Láncolt lista bejárása
        if (strcmp(node->word, word) == 0) { // Ha a szó már létezik
            node->count++;              // Növeljük az előfordulás számát
            return;                     // Kilépünk
        }

        node = node->next;              // Következő listaelemre lépünk
    }

//Idáig akkor jutunk ha a szó még nem létezik

    WordNode* new_node = malloc(sizeof(WordNode)); // Új node foglalása

    strncpy(new_node->word, word, MAX_WORD_LEN); // Szó másolása

    new_node->word[MAX_WORD_LEN-1] = '\0'; // Biztosítjuk a lezárást

    new_node->count = 1;                // Első előfordulás

    new_node->next = map->table[h];     // Beszúrás a lista elejére

    map->table[h] = new_node;           // Új elem lesz az első
}

//HASHMAP EGYESÍTÉS: minden szál külön hashmapben dolgozik végén ezeket egyesíteni kell globálba

void hashmap_merge(HashMap* dest, HashMap* src) { // Két hashmap összefűzése
    for (int i = 0; i < HASH_SIZE; ++i) { // Végigmegyünk az összes bucketen

        WordNode* node = src->table[i]; // Az aktuális bucket első eleme

        while (node) {                  // node bejárása

            pthread_mutex_lock(&global_mutex); // Mutex zárolása: más thread nem írhat

            hashmap_insert(dest, node->word); // Szó beszúrása a cél mapbe

            pthread_mutex_unlock(&global_mutex); // Mutex feloldása

            node = node->next;          // jöhet a következő elem
        }
    }
}

//HASHMAP FREE: felszabadítjuk a memóriát, C-ben nincs külön garbage collector

void hashmap_free(HashMap* map) {       // Hash map memória felszabadítása
    for (int i = 0; i < HASH_SIZE; ++i) { // Végigmegyünk minden bucketen

        WordNode* node = map->table[i]; // Aktuális bucket első eleme

        while (node) {                  // Lista bejárása

            WordNode* tmp = node;       // Ideiglenes pointer eltárolása

            node = node->next;          // Továbblépünk

            free(tmp);                  // Memóriát felszabadítjuk
        }
    }
}

//-----------------------------------------------------------------------------------------


//SZAVAK KINYERÉSE: ha betű építjük tovább a szót, ha nem akkor lezárjuk, eltároljuk 

void extract_words(char* buffer, size_t start, size_t end, HashMap* map) { 
    char word[MAX_WORD_LEN];            // Ideiglenes szó buffer: ide építjük 

    int idx = 0;                        // Aktuális szó karakter indexe

    for (size_t i = start; i < end; ++i) {  // végigmegyünk a szöveg adott részén

        if (isalpha((unsigned char)buffer[i])) { // az aktuális karakter betű?

            if (idx < MAX_WORD_LEN - 1) //  még van hely a szóban?

                word[idx++] = tolower((unsigned char)buffer[i]); // Kisbetűsítés és tárolás

        } else if (idx > 0) {           // Ha nem betű és már gyűjtöttünk szót akkor

            word[idx] = '\0';           // String lezárása

            hashmap_insert(map, word);  // Szó beszúrása

            idx = 0;                    // Új szó kezdése
        }
    }

    if (idx > 0) {                      // Ha a végén maradt befejezetlen szó

        word[idx] = '\0';               // Lezárjuk

        hashmap_insert(map, word);      // Beszúrjuk
    }
}

//THREAD FUNKCIÓ: ez fut le minden threadben, megkapja a részét, feldolgozza, saját hashmapbe ment

void* thread_func(void* arg) {          // A thread által futtatott függvény

    ThreadArg* t = (ThreadArg*)arg;     // Argumentum konvertálása

    extract_words(t->buffer, t->start, t->end, t->local_map); // Szavak feldolgozása

    pthread_exit(NULL);                 // Szál befejezése
}

//COMPARE WORDS: összehasonlítás, qsort használja rendezéshez, nagyobb count előre kerül

int compare_words(const void* a, const void* b) { // qsort összehasonlító függvény

    WordNode* wa = *(WordNode**)a;      // Első elem

    WordNode* wb = *(WordNode**)b;      // Második elem

    return wb->count - wa->count;       // Csökkenő sorrend előfordulás szerint
}

//TOP SZAVAK KIÍRÁSA: összegyűjtés, rendezés, kiírás

void print_top_words(HashMap* map) {    // Leggyakoribb szavak kiírása

    WordNode* list[TOP_N * 10];         // Ideiglenes lista: node pointerek gyűjtése

    int count = 0;                      // Elemek száma

    for (int i = 0; i < HASH_SIZE && count < TOP_N * 10; ++i) { // Bucketek bejárása

        WordNode* node = map->table[i]; // Bucket első eleme

        while (node && count < TOP_N * 10) { // Lista bejárása

            list[count++] = node;       // Elem eltárolása

            node = node->next;          // Következő elem
        }
    }

    qsort(list, count, sizeof(WordNode*), compare_words); // Rendezés gyakoriság szerint

    printf("\nTop %d words:\n", TOP_N); // Fejléc kiírása

    for (int i = 0; i < TOP_N && i < count; ++i) { // Top szavak kiírása

        printf("%s: %d\n", list[i]->word, list[i]->count); // Szó és darabszám
    }
}

//---------------------------------------------------------------------------------------------------------------

//MAIN FÜGGVÉNY: innen indul minden:

int main(int argc, char* argv[]) {      // Program belépési pontja

    int time_only = 0;                  // Csak időmérés flag

//Argumentum ellenőrzés

    if (argc != 3 && argc != 4) {       // Paraméterek ellenőrzése

        fprintf(stderr, "Usage: %s <filename> <num_threads> [--time-only]\n", argv[0]); // Használat kiírása

        return 1;                       // Hibakód visszaadása
    }

//time only ellenőrzés

    if (argc == 4 && strcmp(argv[3], "--time-only") == 0) { // Ha van --time-only kapcsoló

        time_only = 1;                  // Bekapcsoljuk az időmérés módot
    }

//paraméterek feldolgozása

    const char* filename = argv[1];     // Fájlnév eltárolása

    int num_threads = atoi(argv[2]);    // // threadek száma stringből intté alakítva

//fájl megnyitása

    FILE* f = fopen(filename, "rb");    // Fájl megnyitása bináris olvasásra

    if (!f) {                           // Ha nem sikerült megnyitni

        perror("File open failed");     // Hibaüzenet

        return 1;                       // Kilépés hibával
    }

//fájlméret lekérdezés

    struct stat st;                     // stat struktúra deklarálása

    stat(filename, &st);                // Fájl adatainak lekérdezése

    size_t filesize = st.st_size;       // Fájlméret eltárolása

//fájl beolvasása memóriába

    char* buffer = malloc(filesize + 1); // Memória foglalása a fájl tartalmának

    fread(buffer, 1, filesize, f);      // Fájl beolvasása memóriába

    fclose(f);                          // Fájl bezárása

    buffer[filesize] = '\0';            // String lezárása

//thread adatok létrehozása

    pthread_t threads[num_threads];     // Thread objektumok tömbje

    ThreadArg args[num_threads];        // Thread argumentumok tömbje

    HashMap local_maps[num_threads];    // Lokális hashmapek tömbje

//chunk méret számítás

    size_t chunk = filesize / num_threads; // Egy szálra jutó adatmennyiség

//időmérés

    clock_t start_time = clock();       // Start idő mérése

//threadek indítása

    for (int i = 0; i < num_threads; ++i) { // Threadek létrehozása

        args[i].buffer = buffer;        // Buffer pointer beállítása

        args[i].start = i * chunk;      // Kezdő index

        args[i].end = (i == num_threads - 1) ? filesize : (i + 1) * chunk; // Záró index

        args[i].local_map = &local_maps[i]; // Lokális hashmap hozzárendelése

        memset(&local_maps[i], 0, sizeof(HashMap)); // Hashmap nullázása

        pthread_create(&threads[i], NULL, thread_func, &args[i]); // Thread indítása
    }

//threadek megvárása és mergelés

    for (int i = 0; i < num_threads; ++i) { // Threadek befejezésére várunk

        pthread_join(threads[i], NULL); // Thread lezárásának megvárása

        hashmap_merge(&global_map, &local_maps[i]); // Lokális map összevonása

        hashmap_free(&local_maps[i]);   // Lokális map memória felszabadítása
    }

//futási idő számítás

    clock_t end_time = clock();         // End idő mérése

    double time_spent = (double)(end_time - start_time) / CLOCKS_PER_SEC; // Futási idő számítása

//eredmény kiírás

    if (!time_only) {                   // Ha nem csak időmérés mód

        print_top_words(&global_map);   // Leggyakoribb szavak kiírása
    }

    printf("%.3f\n", time_spent);       // Futási idő kiírása

//hashmap free és vége

    hashmap_free(&global_map);          // Globális map memória felszabadítása

    free(buffer);                       // Buffer felszabadítása

    return 0;                           // Sikeres kilépés
}
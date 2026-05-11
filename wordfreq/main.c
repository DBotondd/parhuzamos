#include <stdio.h>      // #: preprocessor jel; include: beillesztés; <stdio.h>: Standard I/O fejléc (standard be/kimenet kezelése, pl. printf)
#include <stdlib.h>     // <stdlib.h>: Standard Library (memóriafoglalás: malloc, konverzió: atoi, programleállítás: exit)
#include <pthread.h>    // <pthread.h>: POSIX Threads könyvtár (többszálú futtatás eszközei: szálak létrehozása, mutexek)
#include <string.h>     // <string.h>: String (karakterlánc) kezelő függvények (másolás: strncpy, összehasonlítás: strcmp)
#include <ctype.h>      // <ctype.h>: Character Type (karaktertípus vizsgáló függvények, pl. betű-e: isalpha, kisbetűvé alakítás: tolower)
#include <sys/stat.h>   // <sys/stat.h>: System Status (rendszerhívások fájlinformációkhoz; stat struktúra a fájlmérethez)
#include <time.h>       // <time.h>: Időkezelés (clock_t típus a processzoridőhöz, CLOCKS_PER_SEC konstans a váltáshoz)

#define MAX_WORD_LEN 64 // #define: szöveges helyettesítés (makró); MAX_WORD_LEN: név; 64: az érték, amire a fordító mindenhol lecseréli
#define HASH_SIZE 10007 // HASH_SIZE: a hash tábla vödreinek száma; a 10007 prímszám, ami segít elkerülni az ütközéseket (collision)
#define TOP_N 20        // TOP_N: konstans; meghatározza, hogy a statisztika végén hány darab leggyakoribb szót listázzunk ki

typedef struct WordNode {             // typedef: új típusnév létrehozása; struct: struktúra (összetett adattípus); WordNode: a struktúra címkéje
    char word[MAX_WORD_LEN];          // char: karakter típus; word: név; [64]: tömbméret; szerepe: a konkrét szó tárolása a memóriában
    int count;                        // int: egész szám típus; count: név; szerepe: számláló, hányszor fordult elő az adott szó
    struct WordNode* next;            // struct WordNode*: típus (önmagára mutató); *: pointer (mutató) jel; next: név; szerepe: láncolt lista címe
} WordNode;                           // WordNode: az új típusnév, amivel innentől hivatkozhatunk erre a szerkezetre

typedef struct {                      // Névtelen struktúra definiálása a hash táblának
    WordNode* table[HASH_SIZE];       // WordNode*: mutató típus; table: név; [10007]: tömbméret; szerepe: a tábla indexeit tároló mutatótömb
} HashMap;                            // HashMap: az új típusnév a teljes táblára

typedef struct {                      // Struktúra a szálak paramétereinek (argumentumainak) becsomagolásához
    char* buffer;                     // char*: karakterre mutató pointer; buffer: név; szerepe: a memóriába beolvasott fájl kezdőcíme
    size_t start;                     // size_t: előjel nélküli egész (méretekhez); start: név; szerepe: a szál feladatának kezdőbájtja
    size_t end;                       // end: név; szerepe: a szál feladatának végpontja (bájtpozíció a pufferben)
    HashMap* local_map;               // HashMap*: táblára mutató pointer; local_map: név; szerepe: a szál saját, privát táblájának címe
} ThreadArg;                          // ThreadArg: a típusnév a szálak "munkacsomagjához"

pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER; // pthread_mutex_t: típus (lakat); global_mutex: név; =: értékadás; INITIALIZER: alaphelyzet (nyitott)
HashMap global_map;                   // HashMap: típus; global_map: név; szerepe: a központi tábla, ahol minden szál eredménye összeadódik

unsigned int hash(const char* str) {  // unsigned int: visszatérési típus (csak pozitív); hash: név; (const char* str): bemeneti paraméter (szöveg)
    unsigned int h = 0;               // h: lokális változó az eredménynek; 0: kezdőérték
    while (*str) {                    // while: ciklus; (*str): dereferálás (nézd meg a karaktert a címen); szerepe: addig fut, amíg nem \0 a karakter
        h = (h * 31 + *str++) % HASH_SIZE; // *: szorzás; +: összeadás; ++: léptetés a következő karakterre; %: modulo (maradékos osztás az indexeléshez)
    }                                 // h * 31: eltolás (hash algoritmus része); % HASH_SIZE: biztosítja, hogy az index a táblán belül maradjon
    return h;                         // return: kulcsszó; h: a kiszámított érték visszaadása a hívónak
}

void hashmap_insert(HashMap* map, const char* word) { // void: nincs visszatérés; map: cél tábla címe; word: beszúrandó szó címe
    unsigned int h = hash(word);      // hívjuk a hash() függvényt; az eredményt az h változóba mentjük
    WordNode* node = map->table[h];   // WordNode*: mutató; node: segédváltozó; ->: indirekt elérés (mutatón keresztül a struktúra elemére)
    while (node) {                    // addig fut, amíg a node nem NULL (végigmegy a láncolt listán az adott indexen)
        if (strcmp(node->word, word) == 0) { // if: feltétel; strcmp(): string összehasonlítás; == 0: ha a két szó karakterre pontosan egyezik
            node->count++;            // ++: inkrementálás (eggyel növelés); szerepe: megtaláltuk a szót, növeljük az előfordulásszámot
            return;                   // return: azonnali kilépés a függvényből (mivel elvégeztük a feladatot)
        }                             // if blokk vége
        node = node->next;            // node: frissítés; =: értékadás; node->next: a következő elem címe; szerepe: továbblépés a listában
    }                                 // while ciklus vége

    WordNode* new_node = malloc(sizeof(WordNode)); // malloc(): memóriafoglalás a heap-en; sizeof(): típus méretének lekérése bájtokban
    strncpy(new_node->word, word, MAX_WORD_LEN); // strncpy(): biztonságos másolás; word -> new_node->word; MAX_WORD_LEN: max hossz
    new_node->word[MAX_WORD_LEN-1] = '\0'; // []: indexelés; =: értékadás; '\0': lezáró nulla; szerepe: garantáljuk a string végét
    new_node->count = 1;              // count: beállítás; 1: kezdőérték (hiszen most láttuk először)
    new_node->next = map->table[h];   // az új elem után kötjük a jelenlegi lista elejét (beszúrás a lista elejére)
    map->table[h] = new_node;         // a tábla adott indexe mostantól az új elemre mutat (fejcsere)
}

void hashmap_merge(HashMap* dest, HashMap* src) { // dest: cél (globális); src: forrás (szál sajátja)
    for (int i = 0; i < HASH_SIZE; ++i) { // for: ciklus; int i = 0: kezdés; i < 10007: határ; ++i: léptetés
        WordNode* node = src->table[i]; // kinyitjuk a forrás tábla i-edik vödrét
        while (node) {                // amíg van elem az adott láncban
            pthread_mutex_lock(&global_mutex); // pthread_mutex_lock(): lakat lezárása; szerepe: csak egy szál írhat a globálisba
            hashmap_insert(dest, node->word); // beszúrjuk a szót a közös táblába
            pthread_mutex_unlock(&global_mutex); // pthread_mutex_unlock(): lakat nyitása; szerepe: szabad az út a többi szálnak
            node = node->next;        // következő elemre lépés a forrás táblában
        }                             // belső while vége
    }                                 // külső for vége
}

void hashmap_free(HashMap* map) {     // felszabadítás; szerepe: memóriaszivárgás megelőzése
    for (int i = 0; i < HASH_SIZE; ++i) { // végig minden vödrön
        WordNode* node = map->table[i]; // aktuális lánc eleje
        while (node) {                // amíg van elem
            WordNode* tmp = node;     // WordNode* tmp: ideiglenes mentés; = node: aktuális cím elraktározása
            node = node->next;        // node: léptetés a következőre (még mielőtt törölnénk az aktuálist)
            free(tmp);                // free(): felszabadítás; szerepe: a memóriát visszaadjuk az operációs rendszernek
        }                             // belső while vége
    }                                 // külső for vége
}

void extract_words(char* buffer, size_t start, size_t end, HashMap* map) { // szövegfeldolgozó magfüggvény
    char word[MAX_WORD_LEN];          // word: helyi tömb (puffer); szerepe: ide gyűjtjük a karaktereket betűnként
    int idx = 0;                      // idx: index a word tömbben; 0: kezdőpozíció

    for (size_t i = start; i < end; ++i) { // i: kurzor a nagy szövegben; start-tól end-ig halad
        if (isalpha((unsigned char)buffer[i])) { // isalpha(): betű vizsgálat; (unsigned char): típuskonverzió a biztonságért
            if (idx < MAX_WORD_LEN - 1) // ellenőrzés: befér-e még a karakter a pufferbe
                word[idx++] = tolower((unsigned char)buffer[i]); // tolower(): kisbetűssé tétel; idx++: mentés és index növelése
        } else if (idx > 0) {         // else if: ha nem betű, de van már összegyűjtött karakterünk (szó vége)
            word[idx] = '\0';         // '\0': string lezárás; szerepe: kész a szó karakterlánca
            hashmap_insert(map, word); // beszúrás a táblába (vagy számláló növelése)
            idx = 0;                  // idx = 0: lenullázzuk a mutatót a következő szóhoz
        }                             // if-else vége
    }                                 // for vége

    if (idx > 0) {                    // ha a ciklus véget ért, de maradt egy szó a pufferben (fájlvégi szó)
        word[idx] = '\0';             // lezárás
        hashmap_insert(map, word);    // utolsó beszúrás
    }
}

void* thread_func(void* arg) {        // void*: általános mutató; thread_func: szál belépési pont
    ThreadArg* t = (ThreadArg*)arg;   // (ThreadArg*): típus kényszerítés (cast); szerepe: visszaadjuk az adat szerkezetét
    extract_words(t->buffer, t->start, t->end, t->local_map); // munka elvégzése a szál saját adataival
    pthread_exit(NULL);               // pthread_exit(): szál leállítása; NULL: nincs visszatérési érték
}

int compare_words(const void* a, const void* b) { // const void*: általános mutatók (qsort követelménye)
    WordNode* wa = *(WordNode**)a;    // *: dereferálás; (WordNode**): mutató mutatója; szerepe: kinyerjük az elem címét a tömbből
    WordNode* wb = *(WordNode**)b;    // wb: a második összehasonlítandó szó
    return wb->count - wa->count;     // -: kivonás; szerepe: ha wb nagyobb, pozitív (csökkenő sorrendhez)
}

void print_top_words(HashMap* map) {  // eredmények megjelenítése
    WordNode* list[TOP_N * 10];       // mutatótömb a rendezéshez; TOP_N * 10: biztonsági tartalék
    int count = 0;                    // gyűjtött szavak száma

    for (int i = 0; i < HASH_SIZE && count < TOP_N * 10; ++i) { // bejárjuk a teljes táblát
        WordNode* node = map->table[i]; // lánc feje
        while (node && count < TOP_N * 10) { // elemek átrakása a tömbbe a rendezéshez
            list[count++] = node;     // mutató mentése; count++: számláló léptetése
            node = node->next;        // láncon belüli léptetés
        }                             // while vége
    }                                 // for vége

    qsort(list, count, sizeof(WordNode*), compare_words); // qsort(): gyorsrendezés; sizeof(WordNode*): mutató mérete
    printf("\nTop %d words:\n", TOP_N); // printf(): kiírás; \n: újsor karakter; %d: egész szám helye
    for (int i = 0; i < TOP_N && i < count; ++i) { // kiírató ciklus a TOP_N elemig
        printf("%s: %d\n", list[i]->word, list[i]->count); // %s: string (szó); %d: egész (darabszám)
    }
}

int main(int argc, char* argv[]) {    // int: visszatérési típus (állapotkód); argc: paraméterek száma; argv: paraméterek listája
    int time_only = 0;                // logikai változó (flag); 0: hamis

    if (argc != 3 && argc != 4) {     // !=: nem egyenlő; &&: ÉS kapcsolat; ha a paraméterek száma hibás
        fprintf(stderr, "Usage: %s <filename> <num_threads> [--time-only]\n", argv[0]); // fprintf(): hiba kimenetre írás (stderr)
        return 1;                     // return 1: kilépés hiba jelzéssel
    }

    if (argc == 4 && strcmp(argv[3], "--time-only") == 0) { // ellenőrizzük a negyedik paramétert
        time_only = 1;                // 1: igaz; bekapcsoljuk az időmérés-fókuszú módot
    }

    const char* filename = argv[1];   // filename: mutató a fájlnévre; argv[1]: az első beírt paraméter
    int num_threads = atoi(argv[2]);  // atoi(): ASCII to Integer; szerepe: a szöveges "4"-ből valódi 4-es számot csinál
    FILE* f = fopen(filename, "rb");  // FILE*: fájlkezelő típus; fopen(): fájlnyitás; "rb": read binary (olvasás)
    if (!f) {                         // if (!f): ha a fájlmutató NULL (sikertelen nyitás)
        perror("File open failed");   // perror(): hibaüzenet kiírása a rendszer hibaüzenetével együtt
        return 1;                     // kilépés
    }

    struct stat st;                   // stat: struktúra deklarálása a fájladatoknak
    stat(filename, &st);              // stat(): függvényhívás; &: címátadás; szerepe: feltölti az 'st' változót adatokkal
    size_t filesize = st.st_size;     // st_size: a fájl mérete bájtokban (ezt a stat hívás mérte le)
    char* buffer = malloc(filesize + 1); // malloc(): memória foglalás a teljes szövegnek; +1: hely a \0-nak
    fread(buffer, 1, filesize, f);    // fread(): beolvasás; 1: bájt egység; filesize: darab; f: forrás fájl
    fclose(f);                        // fclose(): fájl bezárása; f: a lezárandó kezelő
    buffer[filesize] = '\0';          // lezárjuk a nagy puffert, hogy stringként kezelhessük

    pthread_t threads[num_threads];   // pthread_t: típus; threads: tömb azonosítókkal; [num_threads]: szálak száma
    ThreadArg args[num_threads];      // ThreadArg: paramétercsomagok tömbje (minden szálnak egy saját)
    HashMap local_maps[num_threads];  // HashMap: privát táblák tömbje (minden szálnak egy)
    size_t chunk = filesize / num_threads; // /: osztás; chunk: egy szálra eső bájtok száma

    clock_t start_time = clock();     // clock_t: idő típus; clock(): aktuális processzoridő lekérése

    for (int i = 0; i < num_threads; ++i) { // szálindító ciklus
        args[i].buffer = buffer;      // beállítjuk a közös szöveges puffert
        args[i].start = i * chunk;    // *: szorzás; kezdőpont kiszámítása
        args[i].end = (i == num_threads - 1) ? filesize : (i + 1) * chunk; // (condition) ? if_true : if_false (ternary operátor)
        args[i].local_map = &local_maps[i]; // &local_maps[i]: az i-edik privát tábla memóriacíme
        memset(&local_maps[i], 0, sizeof(HashMap)); // memset(): memória nullázása; szerepe: a tábla tiszta legyen indításkor
        pthread_create(&threads[i], NULL, thread_func, &args[i]); // pthread_create(): szál indítás; thread_func: a futtatandó kód
    }

    for (int i = 0; i < num_threads; ++i) { // beváró ciklus
        pthread_join(threads[i], NULL); // pthread_join(): szinkronizáció; megvárjuk, amíg az i-edik szál végez
        hashmap_merge(&global_map, &local_maps[i]); // összeolvasztás: lokális tábla -> globális tábla
        hashmap_free(&local_maps[i]); // felszabadítás: a szál saját táblájára már nincs szükség
    }

    clock_t end_time = clock();       // befejezési időpont lekérése
    double time_spent = (double)(end_time - start_time) / CLOCKS_PER_SEC; // (double): konverzió tizedes törtté; CLOCKS_PER_SEC: váltószám

    if (!time_only) {                 // if (!time_only): ha a flag 0 (vagyis nem csak időt akarunk látni)
        print_top_words(&global_map); // eredmények kiírása
    }
    printf("%.3f\n", time_spent);     // %.3f: tizedes tört kiírása 3 tizedes jegyig; \n: újsor

    hashmap_free(&global_map);        // globális tábla memóriájának takarítása
    free(buffer);                     // a teljes beolvasott fájl memóriájának felszabadítása
    return 0;                         // return 0: a program sikeresen, hiba nélkül fejeződött be
}
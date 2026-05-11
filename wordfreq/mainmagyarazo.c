// ==========================================================
//                 SZÜKSÉGES KÖNYVTÁRAK
// ==========================================================
// Ezek olyan beépített C könyvtárak,
// amik kész funkciókat adnak nekünk.
// Olyanok, mint más nyelvekben az importok.
// ==========================================================

#include <stdio.h>      // printf, fopen, fread -> konzol és fájl kezelés
#include <stdlib.h>     // malloc, free, atoi -> memória és konverziók
#include <pthread.h>    // threadek (szálak) kezelése
#include <string.h>     // strcmp, strncpy, memset -> string műveletek
#include <ctype.h>      // isalpha, tolower -> karakter vizsgálat
#include <sys/stat.h>   // fájlméret lekérdezése
#include <time.h>       // időmérés



// ==========================================================
//                      KONSTANSOK
// ==========================================================
// Ezek fix értékek.
// A program több helyen használja őket.
// ==========================================================

#define MAX_WORD_LEN 64     // egy szó maximum 64 karakter lehet
#define HASH_SIZE 10007     // hashmap mérete (bucketek száma)
#define TOP_N 20            // ennyi leggyakoribb szót írunk ki



// ==========================================================
//                 EGY SZÓ STRUKTÚRÁJA
// ==========================================================
// Ez a struktúra reprezentál egyetlen szót.
//
// Tároljuk benne:
// - a szót
// - előfordulások számát
// - a következő elemet a linked list-ben
// ==========================================================

typedef struct WordNode {

    char word[MAX_WORD_LEN];
    // maga a szó pl: "alma"

    int count;
    // hányszor szerepelt

    struct WordNode* next;
    // pointer a következő elemre
    // collision esetén linked list-et építünk

} WordNode;



// ==========================================================
//                     HASHMAP
// ==========================================================
// Ez maga a hash tábla.
//
// table[] = bucketek tömbje
//
// Minden bucket:
/// - vagy NULL
// - vagy linked list eleje
// ==========================================================

typedef struct {

    WordNode* table[HASH_SIZE];

} HashMap;



// ==========================================================
//                 THREAD PARAMÉTEREK
// ==========================================================
// Minden thread ezt kapja meg.
//
// Ez mondja meg:
// - milyen szövegrészt dolgozzon fel
// - melyik hashmapbe írjon
// ==========================================================

typedef struct {

    char* buffer;
    // teljes fájl memóriában

    size_t start;
    // kezdő pozíció

    size_t end;
    // vég pozíció

    HashMap* local_map;
    // a thread saját hashmapje

} ThreadArg;



// ==========================================================
//                   GLOBÁLIS ADATOK
// ==========================================================

pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;
// mutex = zár
// egyszerre csak egy thread írhat a globális mapbe

HashMap global_map;
// ide kerül a végső összesített eredmény



// ==========================================================
//                     HASH FÜGGVÉNY
// ==========================================================
// Egy szóból készít egy számot.
//
// Példa:
// "alma" -> 5231
//
// Ez mondja meg,
// melyik bucketbe kerüljön a szó.
// ==========================================================

unsigned int hash(const char* str) {

    unsigned int h = 0;
    // kezdő hash érték

    while (*str) {
        // amíg nem érjük el a string végét

        h = (h * 31 + *str++) % HASH_SIZE;

        // h * 31
        // az előző hash érték szorzása

        // + *str
        // aktuális karakter hozzáadása

        // str++
        // továbblép a következő karakterre

        // % HASH_SIZE
        // biztosan a bucket méreten belül maradunk
    }

    return h;
    // visszaadjuk a bucket indexet
}



// ==========================================================
//                 SZÓ BESZÚRÁSA
// ==========================================================
// Ez a program egyik legfontosabb része.
//
// Ha a szó már létezik:
// -> count növelése
//
// Ha még nem létezik:
// -> új node létrehozása
// ==========================================================

void hashmap_insert(HashMap* map, const char* word) {

    unsigned int h = hash(word);
    // kiszámoljuk a bucket indexet

    WordNode* node = map->table[h];
    // az adott bucket első eleme

    while (node) {
        // végigmegyünk a linked list-en

        if (strcmp(node->word, word) == 0) {
            // strcmp == 0
            // a két string teljesen megegyezik

            node->count++;
            // növeljük az előfordulási számot

            return;
            // kész vagyunk
        }

        node = node->next;
        // továbblépés a következő elemre
    }



    // ======================================================
    // Ha idáig jutottunk:
    // a szó még nem létezik
    // ======================================================

    WordNode* new_node = malloc(sizeof(WordNode));
    // memóriafoglalás új node számára

    strncpy(new_node->word, word, MAX_WORD_LEN);
    // szó bemásolása

    new_node->word[MAX_WORD_LEN - 1] = '\0';
    // biztonsági lezárás

    new_node->count = 1;
    // első előfordulás

    new_node->next = map->table[h];
    // linked list elejére szúrjuk

    map->table[h] = new_node;
    // az új elem lesz a bucket eleje
}



// ==========================================================
//                 HASHMAPEK EGYESÍTÉSE
// ==========================================================
// Minden thread külön hashmapben dolgozik.
//
// A végén ezeket össze kell olvasztani
// a globális hashmapbe.
// ==========================================================

void hashmap_merge(HashMap* dest, HashMap* src) {

    for (int i = 0; i < HASH_SIZE; ++i) {
        // végigmegyünk minden bucketen

        WordNode* node = src->table[i];
        // aktuális bucket első eleme

        while (node) {
            // végigmegyünk a linked list-en

            pthread_mutex_lock(&global_mutex);
            // lock:
            // más thread most nem írhat

            hashmap_insert(dest, node->word);
            // szó hozzáadása a globális maphez

            pthread_mutex_unlock(&global_mutex);
            // unlock:
            // más thread újra írhat

            node = node->next;
            // következő listaelem
        }
    }
}



// ==========================================================
//                 MEMÓRIA FELSZABADÍTÁS
// ==========================================================
// Minden malloc után kell free.
//
// C-ben nincs automatikus garbage collector.
// Nekünk kell felszabadítani a memóriát.
// ==========================================================

void hashmap_free(HashMap* map) {

    for (int i = 0; i < HASH_SIZE; ++i) {
        // végigmegyünk az összes bucketen

        WordNode* node = map->table[i];
        // bucket első eleme

        while (node) {
            // amíg van elem

            WordNode* tmp = node;
            // eltároljuk az aktuális elemet

            node = node->next;
            // továbblépünk

            free(tmp);
            // memória felszabadítása
        }
    }
}



// ==========================================================
//                 SZAVAK KINYERÉSE
// ==========================================================
// Karakterenként végigmegyünk a szövegen.
//
// Ha betű:
// -> építjük a szót
//
// Ha nem betű:
// -> lezárjuk és eltároljuk a szót
// ==========================================================

void extract_words(char* buffer,
                   size_t start,
                   size_t end,
                   HashMap* map) {

    char word[MAX_WORD_LEN];
    // ide építjük az aktuális szót

    int idx = 0;
    // aktuális karakter index a szóban

    for (size_t i = start; i < end; ++i) {
        // végigmegyünk a szöveg adott részén

        if (isalpha((unsigned char)buffer[i])) {
            // ha az aktuális karakter betű

            if (idx < MAX_WORD_LEN - 1)
                // nehogy túlcsorduljon a szó

                word[idx++] =
                    tolower((unsigned char)buffer[i]);
                // kisbetűsítjük és eltároljuk
        }

        else if (idx > 0) {
            // ha nem betű
            // és van készülő szó

            word[idx] = '\0';
            // string lezárása

            hashmap_insert(map, word);
            // szó eltárolása hashmapben

            idx = 0;
            // új szó kezdése
        }
    }



    // ======================================================
    // Ha a szöveg végén maradt szó
    // ======================================================

    if (idx > 0) {

        word[idx] = '\0';
        // string lezárása

        hashmap_insert(map, word);
        // utolsó szó eltárolása
    }
}



// ==========================================================
//                    THREAD FUNKCIÓ
// ==========================================================
// Ez fut le minden threadben.
//
// A thread:
// - megkapja a saját szövegrészét
// - feldolgozza
// - saját hashmapbe ment
// ==========================================================

void* thread_func(void* arg) {

    ThreadArg* t = (ThreadArg*)arg;
    // void* -> ThreadArg* konvertálás

    extract_words(
        t->buffer,
        t->start,
        t->end,
        t->local_map
    );
    // szavak feldolgozása

    pthread_exit(NULL);
    // thread befejezése
}



// ==========================================================
//                ÖSSZEHASONLÍTÓ FÜGGVÉNY
// ==========================================================
// A qsort ezt használja rendezéshez.
//
// Nagyobb count előre kerül.
// ==========================================================

int compare_words(const void* a, const void* b) {

    WordNode* wa = *(WordNode**)a;
    // első elem

    WordNode* wb = *(WordNode**)b;
    // második elem

    return wb->count - wa->count;
    // csökkenő sorrend
}



// ==========================================================
//                 TOP SZAVAK KIÍRÁSA
// ==========================================================
// Összegyűjtjük a szavakat,
// rendezzük őket,
// majd kiírjuk a leggyakoribbakat.
// ==========================================================

void print_top_words(HashMap* map) {

    WordNode* list[TOP_N * 10];
    // ide gyűjtjük a node pointereket

    int count = 0;
    // eddigi elemszám

    for (int i = 0;
         i < HASH_SIZE && count < TOP_N * 10;
         ++i) {
        // végigmegyünk a bucketeken

        WordNode* node = map->table[i];
        // bucket első eleme

        while (node && count < TOP_N * 10) {
            // linked list bejárása

            list[count++] = node;
            // node eltárolása a tömbben

            node = node->next;
            // következő elem
        }
    }



    qsort(
        list,
        count,
        sizeof(WordNode*),
        compare_words
    );
    // rendezés count alapján



    printf("\nTop %d words:\n", TOP_N);
    // fejléc kiírása

    for (int i = 0;
         i < TOP_N && i < count;
         ++i) {
        // top szavak kiírása

        printf(
            "%s: %d\n",
            list[i]->word,
            list[i]->count
        );
    }
}



// ==========================================================
//                         MAIN
// ==========================================================
// A program belépési pontja.
//
// Innen indul minden.
//
// Itt történik:
// - argumentumok kezelése
// - fájl beolvasása
// - threadek indítása
// - merge
// - eredmény kiírás
// ==========================================================

int main(int argc, char* argv[]) {

    int time_only = 0;
    // flag:
    // csak időmérést akarunk-e



    // ======================================================
    // Argumentum ellenőrzés
    // ======================================================

    if (argc != 3 && argc != 4) {
        // ha nem megfelelő számú argumentum érkezett

        fprintf(
            stderr,
            "Usage: %s <filename> <num_threads> [--time-only]\n",
            argv[0]
        );
        // hibás használat kiírása

        return 1;
        // hibával kilépünk
    }



    // ======================================================
    // --time-only kapcsoló ellenőrzése
    // ======================================================

    if (
        argc == 4 &&
        strcmp(argv[3], "--time-only") == 0
    ) {

        time_only = 1;
        // csak időmérés mód bekapcsolása
    }



    // ======================================================
    // Paraméterek feldolgozása
    // ======================================================

    const char* filename = argv[1];
    // fájlnév eltárolása

    int num_threads = atoi(argv[2]);
    // threadek száma stringből intté alakítva



    // ======================================================
    // Fájl megnyitása
    // ======================================================

    FILE* f = fopen(filename, "rb");
    // fájl megnyitása bináris olvasási módban

    if (!f) {
        // ha nem sikerült megnyitni

        perror("File open failed");
        // rendszer hibaüzenet

        return 1;
        // hibával kilépés
    }



    // ======================================================
    // Fájlméret lekérdezése
    // ======================================================

    struct stat st;
    // stat struktúra fájl információkhoz

    stat(filename, &st);
    // fájl adatainak lekérése

    size_t filesize = st.st_size;
    // fájlméret byte-ban



    // ======================================================
    // Fájl teljes beolvasása memóriába
    // ======================================================

    char* buffer = malloc(filesize + 1);
    // memóriafoglalás a teljes fájlnak

    fread(buffer, 1, filesize, f);
    // teljes fájl beolvasása

    fclose(f);
    // fájl bezárása

    buffer[filesize] = '\0';
    // string lezárása



    // ======================================================
    // Thread adatok létrehozása
    // ======================================================

    pthread_t threads[num_threads];
    // thread objektumok

    ThreadArg args[num_threads];
    // thread argumentumok

    HashMap local_maps[num_threads];
    // minden thread saját hashmapje



    // ======================================================
    // Chunk méret számítása
    // ======================================================

    size_t chunk = filesize / num_threads;
    // egy thread mekkora részt kapjon



    // ======================================================
    // Időmérés kezdete
    // ======================================================

    clock_t start_time = clock();
    // kezdő idő eltárolása



    // ======================================================
    // Threadek indítása
    // ======================================================

    for (int i = 0; i < num_threads; ++i) {

        args[i].buffer = buffer;
        // teljes buffer átadása

        args[i].start = i * chunk;
        // kezdő pozíció

        args[i].end =
            (i == num_threads - 1)
            ? filesize
            : (i + 1) * chunk;
        // utolsó thread megkapja a maradékot is

        args[i].local_map = &local_maps[i];
        // saját hashmap átadása

        memset(&local_maps[i], 0, sizeof(HashMap));
        // hashmap nullázása

        pthread_create(
            &threads[i],
            NULL,
            thread_func,
            &args[i]
        );
        // thread indítása
    }



    // ======================================================
    // Threadek megvárása + merge
    // ======================================================

    for (int i = 0; i < num_threads; ++i) {

        pthread_join(threads[i], NULL);
        // megvárjuk a thread végét

        hashmap_merge(
            &global_map,
            &local_maps[i]
        );
        // local hashmap beolvasztása a global mapbe

        hashmap_free(&local_maps[i]);
        // local hashmap memória felszabadítása
    }



    // ======================================================
    // Futási idő számítása
    // ======================================================

    clock_t end_time = clock();
    // végidő eltárolása

    double time_spent =
        (double)(end_time - start_time)
        / CLOCKS_PER_SEC;
    // eltelt idő másodpercben



    // ======================================================
    // Eredmények kiírása
    // ======================================================

    if (!time_only) {
        // ha nem csak időmérés mód

        print_top_words(&global_map);
        // top szavak kiírása
    }

    printf("%.3f\n", time_spent);
    // futási idő kiírása



    // ======================================================
    // Memória felszabadítás
    // ======================================================

    hashmap_free(&global_map);
    // globális hashmap felszabadítása

    free(buffer);
    // fájl buffer felszabadítása



    // ======================================================
    // Program sikeresen lefutott
    // ======================================================

    return 0;
    // 0 = sikeres programfutás
}
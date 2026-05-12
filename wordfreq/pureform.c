#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <time.h>

#define MAX_WORD_LEN 64
#define HASH_SIZE 10007
#define TOP_N 20

typedef struct WordNode {
    char word[MAX_WORD_LEN];
    int count;
    struct WordNode* next;
} WordNode;

typedef struct {
    WordNode* table[HASH_SIZE];
} HashMap;

typedef struct {
    char* buffer;
    size_t start;
    size_t end;
    HashMap* local_map;
} ThreadArg;

pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER;
HashMap global_map;

unsigned int hash(const char* str) {
    unsigned int h = 0;
    while (*str) {
        h = (h * 31 + *str++) % HASH_SIZE;
    }
    return h;
}

void hashmap_insert(HashMap* map, const char* word) {
    unsigned int h = hash(word);
    WordNode* node = map->table[h];
    while (node) {
        if (strcmp(node->word, word) == 0) {
            node->count++;
            return;
        }
        node = node->next;
    }

    WordNode* new_node = malloc(sizeof(WordNode));
    strncpy(new_node->word, word, MAX_WORD_LEN);
    new_node->word[MAX_WORD_LEN-1] = '\0';
    new_node->count = 1;
    new_node->next = map->table[h];
    map->table[h] = new_node;
}

void hashmap_merge(HashMap* dest, HashMap* src) {
    for (int i = 0; i < HASH_SIZE; ++i) {
        WordNode* node = src->table[i];
        while (node) {
            pthread_mutex_lock(&global_mutex);
            hashmap_insert(dest, node->word);
            pthread_mutex_unlock(&global_mutex);
            node = node->next;
        }
    }
}

void hashmap_free(HashMap* map) {
    for (int i = 0; i < HASH_SIZE; ++i) {
        WordNode* node = map->table[i];
        while (node) {
            WordNode* tmp = node;
            node = node->next;
            free(tmp);
        }
    }
}

void extract_words(char* buffer, size_t start, size_t end, HashMap* map) { 
    char word[MAX_WORD_LEN];
    int idx = 0;

    for (size_t i = start; i < end; ++i) {
        if (isalpha((unsigned char)buffer[i])) {
            if (idx < MAX_WORD_LEN - 1)
                word[idx++] = tolower((unsigned char)buffer[i]);
        } else if (idx > 0) {
            word[idx] = '\0';
            hashmap_insert(map, word);
            idx = 0;
        }
    }

    if (idx > 0) {
        word[idx] = '\0';
        hashmap_insert(map, word);
    }
}

void* thread_func(void* arg) {
    ThreadArg* t = (ThreadArg*)arg;
    extract_words(t->buffer, t->start, t->end, t->local_map);
    pthread_exit(NULL);
}

int compare_words(const void* a, const void* b) {
    WordNode* wa = *(WordNode**)a;
    WordNode* wb = *(WordNode**)b;
    return wb->count - wa->count;
}

void print_top_words(HashMap* map) {
    WordNode* list[TOP_N * 10];
    int count = 0;

    for (int i = 0; i < HASH_SIZE && count < TOP_N * 10; ++i) {
        WordNode* node = map->table[i];
        while (node && count < TOP_N * 10) {
            list[count++] = node;
            node = node->next;
        }
    }

    qsort(list, count, sizeof(WordNode*), compare_words);
    printf("\nTop %d words:\n", TOP_N);
    for (int i = 0; i < TOP_N && i < count; ++i) {
        printf("%s: %d\n", list[i]->word, list[i]->count);
    }
}

int main(int argc, char* argv[]) {
    int time_only = 0;

    if (argc != 3 && argc != 4) {
        fprintf(stderr, "Usage: %s <filename> <num_threads> [--time-only]\n", argv[0]);
        return 1;
    }

    if (argc == 4 && strcmp(argv[3], "--time-only") == 0) {
        time_only = 1;
    }

    const char* filename = argv[1];
    int num_threads = atoi(argv[2]);
    FILE* f = fopen(filename, "rb");
    if (!f) {
        perror("File open failed");
        return 1;
    }

    struct stat st;
    stat(filename, &st);
    size_t filesize = st.st_size;
    char* buffer = malloc(filesize + 1);
    fread(buffer, 1, filesize, f);
    fclose(f);
    buffer[filesize] = '\0';

    pthread_t threads[num_threads];
    ThreadArg args[num_threads];
    HashMap local_maps[num_threads];
    size_t chunk = filesize / num_threads;

    clock_t start_time = clock();

    for (int i = 0; i < num_threads; ++i) {
        args[i].buffer = buffer;
        args[i].start = i * chunk;
        args[i].end = (i == num_threads - 1) ? filesize : (i + 1) * chunk;
        args[i].local_map = &local_maps[i];
        memset(&local_maps[i], 0, sizeof(HashMap));
        pthread_create(&threads[i], NULL, thread_func, &args[i]);
    }

    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], NULL);
        hashmap_merge(&global_map, &local_maps[i]);
        hashmap_free(&local_maps[i]);
    }

    clock_t end_time = clock();
    double time_spent = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    if (!time_only) {
        print_top_words(&global_map);
    }
    printf("%.3f\n", time_spent);

    hashmap_free(&global_map);
    free(buffer);
    return 0;
}
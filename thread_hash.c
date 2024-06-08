#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <crypt.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include "thread_hash.h"

#define MAX_THREADS 24
#define BUFFER_SIZE 256

typedef struct {
    int thread_id;
    int num_threads;
    char **passwords;
    int num_passwords;
    char **dictionary;
    int num_words;
} thread_data_t;

pthread_mutex_t lock;

void *crack_passwords(void *arg);

// Function to identify the hash algorithm used
hash_algorithm_t get_hash_algorithm(const char *hash) {
    if (hash[0] != '$') {
        return DES;
    }
    switch (hash[1]) {
        case '1':
            return MD5;
        case '5':
            return SHA256;
        case '6':
            return SHA512;
        case 'y':
            return YESCRYPT;
        case 'g':
            return GOST_YESCRYPT;
        case 'b':
            return BCRYPT;
        case '3':
            return NT;
        default:
            return ALGORITHM_MAX;
    }
}

void *crack_passwords(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    int i, j;
    struct crypt_data crypt_data;
    struct timespec start, end;
    double elapsed;
    char *hash;
    int algo_count[ALGORITHM_MAX] = {0};  // Count of each algorithm processed

    crypt_data.initialized = 0;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (i = data->thread_id; i < data->num_words; i += data->num_threads) {
        for (j = 0; j < data->num_passwords; j++) {
            hash = crypt_r(data->dictionary[i], data->passwords[j], &crypt_data);
            if (hash && strcmp(hash, data->passwords[j]) == 0) {
                hash_algorithm_t algo = get_hash_algorithm(data->passwords[j]);
                pthread_mutex_lock(&lock);
                printf("cracked %s %s\n", data->dictionary[i], data->passwords[j]);
                algo_count[algo]++;
                pthread_mutex_unlock(&lock);
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1E9;

    fprintf(stderr, "Thread %d: Time = %f seconds\n", data->thread_id, elapsed);
    fprintf(stderr, "Thread %d: Processed %d passwords\n", data->thread_id, data->num_passwords);
    for (i = 0; i < ALGORITHM_MAX; i++) {
        fprintf(stderr, "Thread %d: %s count = %d\n", data->thread_id, algorithm_string[i], algo_count[i]);
    }

    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    int opt;
    char *input_file = NULL, *dict_file = NULL;
    int num_threads = 1;
    FILE *input_fp = NULL, *dict_fp = NULL;
    char **passwords;
    char **dictionary;
    int num_passwords = 0, num_words = 0;
    char line[BUFFER_SIZE];
    pthread_t *threads;
    thread_data_t *thread_data;
    int i;

    while ((opt = getopt(argc, argv, "i:d:t:vh")) != -1) {
        switch (opt) {
            case 'i':
                input_file = optarg;
                break;
            case 'd':
                dict_file = optarg;
                break;
            case 't':
                num_threads = atoi(optarg);
                if (num_threads > MAX_THREADS) num_threads = MAX_THREADS;
                break;
            case 'v':
                fprintf(stderr, "Verbose mode enabled\n");
                break;
            case 'h':
                fprintf(stderr, "Usage: %s -i input_file -d dict_file [-t num_threads] [-v] [-h]\n", argv[0]);
                exit(EXIT_FAILURE);
            default:
                fprintf(stderr, "Unknown option: %c\n", opt);
                exit(EXIT_FAILURE);
        }
    }

    if (!input_file || !dict_file) {
        fprintf(stderr, "Input and dictionary files are required.\n");
        exit(EXIT_FAILURE);
    }

    if ((input_fp = fopen(input_file, "r")) == NULL) {
        perror("Error opening input file");
        exit(EXIT_FAILURE);
    }

    if ((dict_fp = fopen(dict_file, "r")) == NULL) {
        perror("Error opening dictionary file");
        exit(EXIT_FAILURE);
    }

    passwords = malloc(sizeof(char *) * BUFFER_SIZE);
    dictionary = malloc(sizeof(char *) * BUFFER_SIZE);

    while (fgets(line, sizeof(line), input_fp)) {
        line[strcspn(line, "\n")] = '\0';  // Remove newline character
        passwords[num_passwords] = strdup(line);
        num_passwords++;
    }

    while (fgets(line, sizeof(line), dict_fp)) {
        line[strcspn(line, "\n")] = '\0';  // Remove newline character
        dictionary[num_words] = strdup(line);
        num_words++;
    }

    fclose(input_fp);
    fclose(dict_fp);

    fprintf(stderr, "Read %d passwords:\n", num_passwords);
    for (i = 0; i < num_passwords; i++) {
        fprintf(stderr, "%s\n", passwords[i]);
    }

    fprintf(stderr, "Read %d dictionary words:\n", num_words);
    for (i = 0; i < num_words; i++) {
        fprintf(stderr, "%s\n", dictionary[i]);
    }

    threads = malloc(num_threads * sizeof(pthread_t));
    thread_data = malloc(num_threads * sizeof(thread_data_t));

    pthread_mutex_init(&lock, NULL);

    for (i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].num_threads = num_threads;
        thread_data[i].passwords = passwords;
        thread_data[i].num_passwords = num_passwords;
        thread_data[i].dictionary = dictionary;
        thread_data[i].num_words = num_words;
        pthread_create(&threads[i], NULL, crack_passwords, &thread_data[i]);
    }

    for (i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&lock);

    for (i = 0; i < num_passwords; i++) {
        free(passwords[i]);
    }
    for (i = 0; i < num_words; i++) {
        free(dictionary[i]);
    }

    free(passwords);
    free(dictionary);
    free(threads);
    free(thread_data);

    return EXIT_SUCCESS;
}

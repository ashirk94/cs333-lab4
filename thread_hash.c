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
#define BUFFER_SIZE 2500

typedef struct {
    int thread_id;
    int num_threads;
    char **passwords;
    int num_passwords;
    char **dictionary;
    int num_words;
    FILE *output_file;
    int verbose;
} thread_data_t;

pthread_mutex_t lock;
int total_algo_count[ALGORITHM_MAX] = {0};  // Global count of each algorithm processed
double total_time = 0.0;

void *crack_passwords(void *arg);
hash_algorithm_t get_hash_algorithm(const char *hash);
void print_help(void);

int main(int argc, char **argv) {
    int opt;
    char *input_file = NULL, *dict_file = NULL, *output_file = NULL;
    int num_threads = 1;
    int apply_nice = 0;
    int verbose = 0;
    FILE *input_fp = NULL, *dict_fp = NULL, *out_fp = NULL;
    char **passwords = NULL;
    char **dictionary = NULL;
    int num_passwords = 0, num_words = 0;
    char line[BUFFER_SIZE];
    pthread_t *threads = NULL;
    thread_data_t *thread_data = NULL;
    int passwords_capacity = 1000, dictionary_capacity = 1000;
    int i;

    // Command line processing
    while ((opt = getopt(argc, argv, "i:d:o:t:nvh")) != -1) {
        switch (opt) {
            case 'i':
                input_file = optarg;
                break;
            case 'd':
                dict_file = optarg;
                break;
            case 'o':
                output_file = optarg;
                break;
            case 't':
                num_threads = atoi(optarg);
                if (num_threads > MAX_THREADS) num_threads = MAX_THREADS;
                break;
            case 'n':
                apply_nice = 1;
                break;
            case 'v':
                verbose = 1;
                fprintf(stderr, "Verbose mode enabled\n");
                break;
            case 'h':
                print_help();
                exit(EXIT_SUCCESS);
            default:
                fprintf(stderr, "Unknown option: %c\n", opt);
                exit(EXIT_FAILURE);
        }
    }

    if (!input_file || !dict_file) {
        fprintf(stderr, "Input and dictionary files are required.\n");
        exit(EXIT_FAILURE);
    }

    if (apply_nice) {
        if (nice(10) == -1) {
            perror("Error applying nice");
            exit(EXIT_FAILURE);
        }
    }

    input_fp = fopen(input_file, "r");
    if (!input_fp) {
        perror("Error opening input file");
        exit(EXIT_FAILURE);
    }

    dict_fp = fopen(dict_file, "r");
    if (!dict_fp) {
        perror("Error opening dictionary file");
        fclose(input_fp);
        exit(EXIT_FAILURE);
    }

    if (output_file) {
        out_fp = fopen(output_file, "w");
        if (!out_fp) {
            perror("Error opening output file");
            fclose(input_fp);
            fclose(dict_fp);
            exit(EXIT_FAILURE);
        }
    } else {
        out_fp = stdout;
    }

    // Dynamic allocation for passwords and dictionary based on the number of lines

    passwords = malloc(passwords_capacity * sizeof(char *));
    dictionary = malloc(dictionary_capacity * sizeof(char *));
    if (!passwords || !dictionary) {
        perror("Error allocating memory for passwords or dictionary");
        fclose(input_fp);
        fclose(dict_fp);
        fclose(out_fp);
        free(passwords);
        free(dictionary);
        exit(EXIT_FAILURE);
    }

    while (fgets(line, sizeof(line), input_fp)) {
        line[strcspn(line, "\n")] = '\0';
        if (num_passwords >= passwords_capacity) {
            passwords_capacity *= 2;
            passwords = realloc(passwords, passwords_capacity * sizeof(char *));
            if (!passwords) {
                perror("Error reallocating memory for passwords");
                fclose(input_fp);
                fclose(dict_fp);
                fclose(out_fp);
                free(dictionary);
                exit(EXIT_FAILURE);
            }
        }
        passwords[num_passwords] = strdup(line);
        if (!passwords[num_passwords]) {
            perror("Error duplicating password line");
            break;
        }
        num_passwords++;
    }

    while (fgets(line, sizeof(line), dict_fp)) {
        line[strcspn(line, "\n")] = '\0';
        if (num_words >= dictionary_capacity) {
            dictionary_capacity *= 2;
            dictionary = realloc(dictionary, dictionary_capacity * sizeof(char *));
            if (!dictionary) {
                perror("Error reallocating memory for dictionary");
                fclose(input_fp);
                fclose(dict_fp);
                fclose(out_fp);
                free(passwords);
                exit(EXIT_FAILURE);
            }
        }
        dictionary[num_words] = strdup(line);
        if (!dictionary[num_words]) {
            perror("Error duplicating dictionary line");
            break;
        }
        num_words++;
    }

    fclose(input_fp);
    fclose(dict_fp);

    threads = malloc(num_threads * sizeof(pthread_t));
    thread_data = malloc(num_threads * sizeof(thread_data_t));
    if (!threads || !thread_data) {
        perror("Error allocating memory for threads or thread_data");
        free(passwords);
        free(dictionary);
        fclose(out_fp);
        exit(EXIT_FAILURE);
    }

    pthread_mutex_init(&lock, NULL);

    for (i = 0; i < num_threads; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].num_threads = num_threads;
        thread_data[i].passwords = passwords;
        thread_data[i].num_passwords = num_passwords;
        thread_data[i].dictionary = dictionary;
        thread_data[i].num_words = num_words;
        thread_data[i].output_file = out_fp;
        thread_data[i].verbose = verbose;
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

    if (output_file) {
        fclose(out_fp);
    }

    // Print total statistics
    printf("total:   %d\t%.2f sec\t", num_threads, total_time);
    for (i = 0; i < ALGORITHM_MAX; i++) {
        printf("%s: %5d\t", algorithm_string[i], total_algo_count[i]);
    }
    printf("total: %8d\n", num_passwords);

    return EXIT_SUCCESS;
}

// Prints help messages
void print_help(void) {
    printf("help text\n");
    printf("\t./thread_hash ...\n");
    printf("\tOptions: i:o:d:hvt:n\n");
    printf("\t\t-i file\t\tinput file name (required)\n");
    printf("\t\t-o file\t\toutput file name (default stdout)\n");
    printf("\t\t-d file\t\tdictionary file name (default stdout)\n");
    printf("\t\t-t #\t\tnumber of threads to create (default 1)\n");
    printf("\t\t-v\t\tenable verbose mode\n");
    printf("\t\t-h\t\thelpful text\n");
}

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

// Function to crack the passwords
void *crack_passwords(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    int i, j;
    struct crypt_data crypt_data;
    struct timespec start, end;
    double elapsed;
    char *hash;
    int algo_count[ALGORITHM_MAX] = {0};  // Count of each algorithm processed

    memset(&crypt_data, 0, sizeof(crypt_data));
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (i = data->thread_id; i < data->num_words; i += data->num_threads) {
        for (j = 0; j < data->num_passwords; j++) {
            hash = crypt_r(data->dictionary[i], data->passwords[j], &crypt_data);
            if (hash && strcmp(hash, data->passwords[j]) == 0) {
                hash_algorithm_t algo = get_hash_algorithm(data->passwords[j]);
                pthread_mutex_lock(&lock);
                fprintf(data->output_file, "cracked  %s  %s\n", data->dictionary[i], data->passwords[j]);
                algo_count[algo]++;
                total_algo_count[algo]++;
                pthread_mutex_unlock(&lock);
            }
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1E9;

    pthread_mutex_lock(&lock);
    total_time += elapsed;
    pthread_mutex_unlock(&lock);

    if (data->verbose) {
        fprintf(stderr, "thread:  %d\t%.2f sec\t", data->thread_id, elapsed);
        for (i = 0; i < ALGORITHM_MAX; i++) {
            fprintf(stderr, "%s: %5d\t", algorithm_string[i], algo_count[i]);
        }
        fprintf(stderr, "total: %8d\n", data->num_passwords);
    }

    pthread_exit(NULL);
}

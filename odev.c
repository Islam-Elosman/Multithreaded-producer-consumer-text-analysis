#include <stdio.h>
#include <omp.h>
#include <string.h>

#define MAX_SENTENCES 1024
#define MAX_SENTENCE_LENGTH 1024
#define NUM_PRODUCERS 8
#define NUM_CONSUMERS 4

// Shared sentence queue
char queue[MAX_SENTENCES][MAX_SENTENCE_LENGTH];
int queue_size = 0;
int queue_index = 0;

// Shared total word and character counts
int total_words = 0;
int total_chars = 0;

// Locks for shared resources
omp_lock_t queue_lock;
omp_lock_t count_lock;

// Producer function
void producer(const char* file_name) {
    FILE* fp = fopen(file_name, "r");
    if (fp == NULL) {
        printf("Failed to open file\n");
        return;
    }

    char line[MAX_SENTENCE_LENGTH];
    while (fgets(line, MAX_SENTENCE_LENGTH, fp)) {
        line[strcspn(line, "\n")] = '\0'; // Remove newline character

        omp_set_lock(&queue_lock);
        if (queue_size < MAX_SENTENCES && line[0] != '\0') {
            strcpy(queue[queue_index], line);
            queue_index = (queue_index + 1) % MAX_SENTENCES;
            queue_size++;
        }
        omp_unset_lock(&queue_lock);
    }

    // Signal to consumers that this producer is done
    omp_set_lock(&queue_lock);
    if (queue_size < MAX_SENTENCES) {
        strcpy(queue[queue_index], "");
        queue_index = (queue_index + 1) % MAX_SENTENCES;
        queue_size++;
    }
    omp_unset_lock(&queue_lock);

    fclose(fp);
}

// Consumer function
void consumer() {
    int local_words = 0, local_chars = 0;
    int thread_id = omp_get_thread_num();

    while (1) {
        char sentence[MAX_SENTENCE_LENGTH] = "";
        omp_set_lock(&queue_lock);
        if (queue_size > 0) {
            strcpy(sentence, queue[(queue_index - queue_size + MAX_SENTENCES) % MAX_SENTENCES]);
            queue_size--;
        }
        omp_unset_lock(&queue_lock);

        // If the consumed sentence is a null string, break the loop
        if (strcmp(sentence, "") == 0) {
            break;
        }

        int word_count = 0, char_count = 0;
        for (int i = 0; sentence[i] != '\0'; i++) {
            char_count++;
            if (sentence[i] == ' ' && sentence[i + 1] != ' ') {
                word_count++;
            }
        }

        // Update local counts
        local_words += word_count;
        local_chars += char_count;

        printf("Thread %d: Sentence: %s\n", thread_id, sentence);
        printf("Thread %d: Words: %d, Characters: %d\n", thread_id, word_count, char_count);
    }

    // Update total counts
    omp_set_lock(&count_lock);
    total_words += local_words;
    total_chars += local_chars;
    omp_unset_lock(&count_lock);

    printf("Thread %d done. Local Words: %d, Local Characters: %d\n", thread_id, local_words, local_chars);
}

int main() {
    omp_init_lock(&queue_lock);
    omp_init_lock(&count_lock);
    char* file_name = "C:\\Users\\ESMA\\CLionProjects\\untitled\\text_file.txt"; // The text file to read sentences from

#pragma omp parallel num_threads(NUM_PRODUCERS + NUM_CONSUMERS)
    {
        int thread_id = omp_get_thread_num();

        if (thread_id < NUM_PRODUCERS) {
#pragma omp single
            {
                producer(file_name);
            }
        }
        else {
            consumer();
        }
    }

    printf("Total Words: %d, Total Characters: %d\n", total_words, total_chars);

    omp_destroy_lock(&queue_lock);
    omp_destroy_lock(&count_lock);

    return 0;
}

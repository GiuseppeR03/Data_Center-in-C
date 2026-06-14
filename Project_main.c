#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Constants and Error Codes
#define FILE_NAME "jobs.txt"
#define HASH_SIZE 101

#define OK (0)
#define FILE_ERROR (-1)
#define MEMORY_ERROR (-2)
#define INPUT_ERROR (-4)
#define CLUSTER_NOT_FOUND (-8)
#define QUEUE_EMPTY (-10)

// Data Structures

// Job details
typedef struct {
    char code[20];
    char submission_date[20];
    float execution_time;
    char cluster[20];
} Job;

// Linked list node for Jobs
typedef struct job_node {
    Job info;
    struct job_node *next;
} JobNode;

// Queue structure for a server cluster
typedef struct {
    char cluster[20];
    JobNode *head;
    JobNode *tail;
    unsigned int num_elements;
} ClusterQueue;

// Hash Table structure
typedef struct {
    JobNode *array[HASH_SIZE];
} HashTable;

// Helper Functions

// Initialize an empty queue
void initializeQueue(ClusterQueue *queue, const char cluster_name[]) {
    queue->head = NULL;
    queue->tail = NULL;
    strcpy(queue->cluster, cluster_name);
    queue->num_elements = 0;
}

// Find a cluster by name
ClusterQueue* findCluster(const ClusterQueue cluster_array[], unsigned num_clusters, const char cluster_name[]) {
    if (num_clusters == 0 || cluster_array == NULL) return NULL;
    for (unsigned i = 0; i < num_clusters; i++) {
        if (strcmp(cluster_array[i].cluster, cluster_name) == 0) {
            return (ClusterQueue*)&cluster_array[i];
        }
    }
    return NULL;
}

// Append a job to the queue
int addJobToQueue(ClusterQueue *queue, Job j) {
    JobNode *new_node = malloc(sizeof(JobNode));
    if (new_node == NULL) return MEMORY_ERROR;

    new_node->info = j;
    new_node->next = NULL;

    if (queue->tail == NULL) {
        queue->head = new_node;
    } else {
        queue->tail->next = new_node;
    }
    queue->tail = new_node;
    queue->num_elements++;
    return OK;
}

// Hash function for strings
unsigned int hashFunction(const char *str) {
    int h = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        h += str[i];
    }
    return h % HASH_SIZE;
}

// Initialize Hash Table
void initializeHashTable(HashTable *ht) {
    for (int i = 0; i < HASH_SIZE; i++) {
        ht->array[i] = NULL;
    }
}

// Print Job details
void printJob(const Job *j) {
    if (j != NULL) {
        printf("Code: %s | Date: %s | Time: %.1fs | Cluster: %s\n",
               j->code, j->submission_date, j->execution_time, j->cluster);
    }
}

// Print entire Queue
void printQueue(const ClusterQueue *queue) {
    if (queue == NULL || queue->head == NULL) {
        printf("Empty queue.\n");
        return;
    }
    JobNode *curr = queue->head;
    while (curr != NULL) {
        printJob(&curr->info);
        curr = curr->next;
    }
}

// Core Logic Functions

// Load clusters and jobs from file
ClusterQueue* loadClustersAndJobs(const char file_name[], unsigned *num_clusters) {
    ClusterQueue *cluster_array = NULL;
    *num_clusters = 0;

    FILE *fp = fopen(file_name, "r");
    if (!fp) return NULL;

    if (fscanf(fp, "%d", num_clusters) != 1 || *num_clusters == 0) {
        fclose(fp);
        return NULL;
    }

    cluster_array = malloc((*num_clusters) * sizeof(ClusterQueue));
    if (cluster_array == NULL) {
        fclose(fp);
        return NULL;
    }

    for (unsigned i = 0; i < *num_clusters; i++) {
        char cluster[20];
        fscanf(fp, "%s", cluster);
        initializeQueue(&cluster_array[i], cluster);
    }

    Job j;
    while (fscanf(fp, "%s %s %f %s", j.code, j.submission_date, &j.execution_time, j.cluster) == 4) {
        ClusterQueue *curr_queue = findCluster(cluster_array, *num_clusters, j.cluster);
        if (curr_queue != NULL) {
            addJobToQueue(curr_queue, j);
        }
    }

    fclose(fp);
    return cluster_array;
}

// Calculate total jobs and execution time
int quantifyJobs(const ClusterQueue cluster_array[], const char cluster_name[],
                  unsigned num_clusters, unsigned *num_elements, float *total_time) {
    if (cluster_array == NULL || num_clusters == 0) return INPUT_ERROR;

    *num_elements = 0;
    *total_time = 0.0f;

    ClusterQueue *curr_queue = findCluster(cluster_array, num_clusters, cluster_name);
    if (curr_queue == NULL || curr_queue->head == NULL) return QUEUE_EMPTY;

    JobNode *curr_node = curr_queue->head;
    while (curr_node != NULL) {
        (*total_time) += curr_node->info.execution_time;
        (*num_elements)++;
        curr_node = curr_node->next;
    }

    return OK;
}

// Extract the first N jobs from a cluster
Job* extractFirstNJobs(ClusterQueue cluster_array[], const char cluster_name[],
                       unsigned num_clusters, unsigned N, unsigned *num_extracted) {
    *num_extracted = 0;

    if (cluster_array == NULL || cluster_name == NULL || N == 0) return NULL;

    ClusterQueue *curr_queue = findCluster(cluster_array, num_clusters, cluster_name);
    if (curr_queue == NULL) return NULL;

    if (curr_queue->num_elements < N) {
        N = curr_queue->num_elements;
    }

    Job *extracted_jobs = malloc(N * sizeof(Job));
    if (extracted_jobs == NULL) return NULL;

    *num_extracted = N;

    for (unsigned i = 0; i < N; i++) {
        JobNode *node_to_delete = curr_queue->head;
        extracted_jobs[i] = node_to_delete->info;

        curr_queue->head = node_to_delete->next;
        free(node_to_delete);
        curr_queue->num_elements--;
    }

    if (curr_queue->head == NULL) {
        curr_queue->tail = NULL;
    }

    return extracted_jobs;
}

// Populate the Hash Table with all active jobs
int populateHashTable(HashTable *ht, const ClusterQueue cluster_array[], unsigned num_clusters) {
    if (ht == NULL || cluster_array == NULL || num_clusters == 0) return INPUT_ERROR;

    initializeHashTable(ht);

    for (unsigned i = 0; i < num_clusters; i++) {
        JobNode *curr_node = cluster_array[i].head;
        while (curr_node != NULL) {
            int idx = hashFunction(curr_node->info.code);
            JobNode *new_node = malloc(sizeof(JobNode));
            if (new_node == NULL) return MEMORY_ERROR;

            new_node->info = curr_node->info;
            new_node->next = ht->array[idx];
            ht->array[idx] = new_node;

            curr_node = curr_node->next;
        }
    }

    return OK;
}

// Search for a Job in the Hash Table by code
Job* searchJobInHash(const HashTable *ht, const char job_code[]) {
    if (ht == NULL || job_code == NULL) return NULL;

    int idx = hashFunction(job_code);
    JobNode *curr_node = ht->array[idx];

    while (curr_node != NULL) {
        if (strcmp(curr_node->info.code, job_code) == 0) {
            return &curr_node->info;
        }
        curr_node = curr_node->next;
    }

    return NULL;
}

// Remove a completed Job from the Hash Table
int completeAndDeleteJobHash(HashTable *ht, const char job_code[]) {
    if (ht == NULL || job_code == NULL) return 0;

    int idx = hashFunction(job_code);
    JobNode *curr = ht->array[idx];
    JobNode *prev = NULL;

    while (curr != NULL) {
        if (strcmp(curr->info.code, job_code) == 0) {
            JobNode *node_to_delete = curr;

            if (prev == NULL) {
                ht->array[idx] = curr->next;
            } else {
                prev->next = curr->next;
            }

            free(node_to_delete);
            return 1;
        }
        prev = curr;
        curr = curr->next;
    }

    return 0;
}

// Entry Point
int main() {
    ClusterQueue *clusters = NULL;
    unsigned num_clusters = 0;
    HashTable ht;

    initializeHashTable(&ht);

    // Initialization and Data Loading
    clusters = loadClustersAndJobs(FILE_NAME, &num_clusters);
    if (clusters == NULL) {
        printf("Error: Could not load data from %s\n", FILE_NAME);
        return FILE_ERROR;
    }

    // Setup Hash Table
    populateHashTable(&ht, clusters, num_clusters);

    return 0;
}
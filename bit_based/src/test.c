#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h> 
#include <string.h>
#include <float.h>
#include <sys/resource.h>
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <signal.h>

#include "genetic_alg.h"
#include "stdgraph.h"


struct test_return {
    block_t *solution;
    int color_count;
    int fitness;
    int uncolored;
    float best_time;
    char summary[512];
};

struct test_param {
    int size;
    int target_color;
    int thread_count;
    int iteration_count;
    int test_count;
    int population_size;
    char graph_filename[128];
    char weight_filename[128];
    char result_filename[128];
    struct test_return result;
};


atomic_int test_thread_count;
atomic_bool terminated;


void* test_graph(void *param) {
    int size = ((struct test_param*)param)->size;
    int thread_count = ((struct test_param*)param)->thread_count;
    int iteration_count = ((struct test_param*)param)->iteration_count;
    int test_count = ((struct test_param*)param)->test_count;
    int target_color = ((struct test_param*)param)->target_color;
    int population_size = ((struct test_param*)param)->population_size;
    char *graph_filename = ((struct test_param*)param)->graph_filename;
    char *weight_filename = ((struct test_param*)param)->weight_filename;
    genetic_criteria_t criteria = MIN_POOL;

    block_t edges[size][TOTAL_BLOCK_NUM(size)];
    if(!read_graph(graph_filename, size, edges)) {
        printf("Could not initialize graph from %s, exitting ...\n", graph_filename);
        return NULL;
    }

    int weights[size];
    if(strncmp(weight_filename, "null", 4) != 0) {
        if(!read_weights(weight_filename, size, weights)) {
            printf("Could not initialize graph weights from %s, exitting ...\n", weight_filename);
            return NULL;
        }

        criteria = MIN_COST;

    } else {
        for(int i = 0; i < size; i++) 
            weights[i] = 1;
        
        criteria = MIN_POOL;
    }

    srand(time(NULL));
    srandom(time(NULL));

    // printf("Testing the dataset %s:\n", graph_filename);

    int edge_count[size];
    count_edges(size, edges, edge_count);
    
    int max_edge_count = 0;
    for(int i = 0; i < size; i++) 
        if(max_edge_count < edge_count[i])
            max_edge_count = edge_count[i];

    float best_time = FLT_MAX, temp_time, total_time = 0;
    int best_fitness = __INT_MAX__, temp_fitness, total_fitness = 0;
    int best_color_count = __INT_MAX__, temp_color_count, total_color_count = 0;
    int best_uncolored = __INT_MAX__, temp_uncolored, total_uncolored = 0;

    block_t best_colors[max_edge_count][TOTAL_BLOCK_NUM(size)], temp_colors[max_edge_count][TOTAL_BLOCK_NUM(size)];
    memset(temp_colors, 0, max_edge_count*TOTAL_BLOCK_NUM(size)*sizeof(block_t));
    memset(best_colors, 0, max_edge_count*TOTAL_BLOCK_NUM(size)*sizeof(block_t));

    if(target_color < 1)
        target_color = graph_color_greedy(size, edges, temp_colors, max_edge_count);

    is_valid(size, edges, target_color, temp_colors);

    struct timeval t1, t2;
    float total_excecution_time = 0;
    for(int k = 0; k < test_count; k++) {
        memset(temp_colors, 0, max_edge_count*TOTAL_BLOCK_NUM(size)*sizeof(block_t));

        gettimeofday(&t1, NULL);
        temp_color_count = graph_color_genetic(
            size,
            edges,
            weights,
            population_size,
            target_color,
            iteration_count,
            temp_colors,
            &temp_fitness,
            &temp_time,
            &temp_uncolored,
            thread_count,
            criteria
        );
        gettimeofday(&t2, NULL);
        total_excecution_time += (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;

        total_color_count += temp_color_count;
        total_fitness += temp_fitness;
        total_time += temp_time;
        total_uncolored += temp_uncolored;

        if (temp_fitness < best_fitness ||
            (temp_fitness == best_fitness && temp_color_count < best_color_count) ||
            (temp_fitness == best_fitness && temp_color_count == best_color_count && temp_time < best_time)) 
        {
            best_color_count = temp_color_count;
            best_fitness = temp_fitness;
            best_time = temp_time;
            best_uncolored = temp_uncolored;
            memcpy(best_colors, temp_colors, max_edge_count*TOTAL_BLOCK_NUM(size)*sizeof(block_t));
        }
    }

    is_valid(size, edges, target_color, best_colors);

    ((struct test_param*)param)->result.color_count = best_color_count;
    ((struct test_param*)param)->result.fitness = best_fitness;
    ((struct test_param*)param)->result.uncolored = best_uncolored;
    ((struct test_param*)param)->result.best_time = best_time;
    ((struct test_param*)param)->result.solution = calloc(best_color_count*TOTAL_BLOCK_NUM(size), sizeof(block_t));
    memmove(((struct test_param*)param)->result.solution, best_colors, best_color_count*TOTAL_BLOCK_NUM(size)*sizeof(block_t));
    sprintf(((struct test_param*)param)->result.summary,
        "|%s|%d|%lf|%d|%d|%d|%lf|%f|%f|%f|%f|\n",
        graph_filename, 
        target_color,
        best_time, 
        best_color_count,
        best_fitness,
        best_uncolored,
        total_time/test_count, 
        total_color_count/((float)test_count),
        total_fitness/((float)test_count),
        total_uncolored/((float)test_count),
        total_excecution_time/((float)test_count)
    );

    return NULL;
}

void __attribute__((optimize("O0"))) test_rand_incea(int size, int crossover_count, int thread_count, float color_density, char *graph_name, char *weight_filename, char *result_filename) {
    block_t edges[size][TOTAL_BLOCK_NUM(size)]; 

    int weights[size];
    if(!read_weights(weight_filename, size, weights)) {
        printf("Could not initialize graph weights from %s, exitting ...\n", weight_filename);
        return;
    }

    srand(time(NULL));
    srandom(time(NULL));

    int temp_uncolored, total_uncolored = 0;
    float temp_time, total_time = 0;
    int temp_fitness, total_fitness = 0;
    int temp_color_count, total_color_count = 0;

    block_t best_colors[(int)(size*color_density)][TOTAL_BLOCK_NUM(size)], temp_colors[(int)(size*color_density)][TOTAL_BLOCK_NUM(size)];
    memset(best_colors, 0, (int)(size*color_density)*TOTAL_BLOCK_NUM(size)*sizeof(block_t));

    char graph_files[6][5][36];
    char *edge_density[] = {
        "10",
        "25",
        "45",
        "60",
        "75",
        "90"
    };
    int i, j, k;
    for(i = 0; i < 6; i++)
        for(j = 1; j < 6; j++)
            sprintf(graph_files[i][j-1], "%s.%s.%d.col", graph_name, edge_density[i], j);

    int iteration_count = 1;
    clock_t temp_clock;
    float total_execution = 0;
    for(i = 0; i < 6; i++) {
        for(j = 0; j < 5; j++) {
            read_graph(graph_files[i][j], size, edges);

            for(k = 0; k < iteration_count; k++) {
                // printf("%s\n", graph_files[i][j]);
                memset(temp_colors, 0, (int)(size*color_density)*TOTAL_BLOCK_NUM(size)*sizeof(block_t));

                temp_clock = clock();
                temp_color_count = graph_color_genetic(
                    size,
                    edges,
                    weights,
                    100,
                    size*color_density,
                    crossover_count,
                    temp_colors,
                    &temp_fitness,
                    &temp_time,
                    &temp_uncolored,
                    thread_count,
                    MIN_COST
                );
                total_execution += (((double)(clock() - temp_clock))/CLOCKS_PER_SEC)/thread_count;
                
                total_color_count += temp_color_count;
                total_fitness += temp_fitness;
                total_time += temp_time;
                total_uncolored += temp_uncolored;
            }
        }
    }

    char buffer[512];
    sprintf(buffer,
        "| %10.6lf | %10.6lf | %10.6lf | %10.6lf |",
        ((double)total_fitness)/(((double)iteration_count) * 5 * 6),
        ((double)total_uncolored)/(((double)iteration_count) * 5 * 6),
        ((double)total_time)/((double)iteration_count * 5 * 6), 
        ((double)total_execution)/((double)iteration_count * 5 * 6)
    );

    printf("%s\n", buffer);
}

void terminate_jobs(int sig) {
    terminated = true;
}

int main(int argc, char *argv[]) {
    if(argc < 3) {
        printf("Too few arguments.\n");
        return 0;
    }

    // const rlim_t kStackSize = 8192L * 1024L * 1024L;   // min stack size = 64 Mb
    // struct rlimit rl;
    // int result;

    // result = getrlimit(RLIMIT_STACK, &rl);
    // if (result == 0)
    // {
    //     if (rl.rlim_cur < kStackSize)
    //     {
    //         rl.rlim_cur = kStackSize;
    //         result = setrlimit(RLIMIT_STACK, &rl);
    //         if (result != 0)
    //         {
    //             fprintf(stderr, "setrlimit returned result = %d\n", result);
    //         }
    //     }
    // }

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = terminate_jobs;
    sigaction(SIGINT, &action, NULL);

    terminated = false;
    test_thread_count = 0;

    FILE *test_list_file = fopen(argv[1], "r");
    FILE *result_file = fopen(argv[2], "w");

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    int num_of_threads = atoi(argv[3]);
    pthread_t thread_id_list[num_of_threads];
    struct test_param *param_list[num_of_threads];
    struct timespec thread_wait_time;
    thread_wait_time.tv_nsec = 100000000;
    thread_wait_time.tv_sec = 0;

    printf("|  graph name   | target color | best k time | best k | best cost | best uncolored | avg k time | avg k | avg cost | avg uncolored | avg total time |\n");
    fprintf(result_file, "|  graph name   | target color | best k time | best k | best cost | best uncolored | avg k time | avg k | avg cost | avg uncolored | avg total time |\n");

    char buffer[256];
    int i, j;
    while(!feof(test_list_file) && !terminated) {
        param_list[test_thread_count] = malloc(sizeof(struct test_param));

        fgets(buffer, 256, test_list_file);
        buffer[strcspn(buffer, "\n")] = 0;

        param_list[test_thread_count]->size = atoi(strtok(buffer, " "));
        param_list[test_thread_count]->target_color = atoi(strtok(NULL, " "));
        param_list[test_thread_count]->thread_count = atoi(strtok(NULL, " "));
        param_list[test_thread_count]->iteration_count = atoi(strtok(NULL, " "));
        param_list[test_thread_count]->test_count = atoi(strtok(NULL, " "));
        param_list[test_thread_count]->population_size = atoi(strtok(NULL, " "));
        strcpy(param_list[test_thread_count]->graph_filename, strtok(NULL, " "));
        strcpy(param_list[test_thread_count]->weight_filename, strtok(NULL, " "));
        strcpy(param_list[test_thread_count]->result_filename, strtok(NULL, " "));

        pthread_create(&thread_id_list[test_thread_count], &attr, test_graph, param_list[test_thread_count]);
        test_thread_count++;

        while(test_thread_count == num_of_threads && !terminated) {
            for(i = 0; i < test_thread_count; i++) {
                if(pthread_timedjoin_np(thread_id_list[i], NULL, &thread_wait_time) == 0) {
                    fprintf(result_file, "%s", param_list[i]->result.summary);
                    printf("%s", param_list[i]->result.summary);
                	free(param_list[i]->result.solution);
                    free(param_list[i]);

                    for(j = i+1; j < num_of_threads; j++) {
                        param_list[j - 1] = param_list[j];
                        thread_id_list[j - 1] = thread_id_list[j];
                    }
                    param_list[num_of_threads-1] = NULL;

                    test_thread_count--;
                    break;
                }
            }
        }
    }

    while(test_thread_count > 0 && !terminated) {
        for(i = 0; i < test_thread_count; i++) {
            if(pthread_timedjoin_np(thread_id_list[i], NULL, &thread_wait_time) == 0) {
                fprintf(result_file, "%s", param_list[i]->result.summary);
                printf("%s", param_list[i]->result.summary);
                free(param_list[i]->result.solution);
                free(param_list[i]);

                for(j = i+1; j < num_of_threads; j++) {
                    param_list[j - 1] = param_list[j];
                    thread_id_list[j - 1] = thread_id_list[j];
                }
                param_list[num_of_threads-1] = NULL;

                test_thread_count--;
                break;
            }
        }
    }

    if(terminated) {
        for(i = 0; i < test_thread_count; i++) {
            pthread_cancel(thread_id_list[i]);
            if(param_list[i] != NULL)
                free(param_list[i]);
        }

        test_thread_count = 0;
    }

    fclose(test_list_file);
    fclose(result_file);

    // printf("|  fitness   | uncolored  |    time    | execution time |\n");
    // char buffer[128];
    // int thread_count = 5;
    // int crossover_count = 10000;
    // strcpy(buffer, "../graph_datasets/INCEA100");
    // test_rand_incea(100, crossover_count, thread_count, 0.04, buffer, "../graph_datasets/INCEA100.10.1.colw", "results/INCEA100.10.1.txt");
    // test_rand_incea(100, crossover_count, thread_count, 0.08, buffer, "../graph_datasets/INCEA100.10.1.colw", "results/INCEA100.10.1.txt");
    // test_rand_incea(100, crossover_count, thread_count, 0.1, buffer, "../graph_datasets/INCEA100.10.1.colw", "results/INCEA100.10.1.txt");
    // test_rand_incea(100, crossover_count, thread_count, 0.15, buffer, "../graph_datasets/INCEA100.10.1.colw", "results/INCEA100.10.1.txt");
    // test_rand_incea(100, crossover_count, thread_count, 0.2, buffer, "../graph_datasets/INCEA100.10.1.colw", "results/INCEA100.10.1.txt");

    // strcpy(buffer, "../graph_datasets/INCEA200");
    // test_rand_incea(200, crossover_count, thread_count, 0.04, buffer, "../graph_datasets/INCEA200.10.1.colw", "results/INCEA200.10.1.txt");
    // test_rand_incea(200, crossover_count, thread_count, 0.08, buffer, "../graph_datasets/INCEA200.10.1.colw", "results/INCEA200.10.1.txt");
    // test_rand_incea(200, crossover_count, thread_count, 0.1, buffer, "../graph_datasets/INCEA200.10.1.colw", "results/INCEA200.10.1.txt");
    // test_rand_incea(200, crossover_count, thread_count, 0.15, buffer, "../graph_datasets/INCEA200.10.1.colw", "results/INCEA200.10.1.txt");
    // test_rand_incea(200, crossover_count, thread_count, 0.2, buffer, "../graph_datasets/INCEA200.10.1.colw", "results/INCEA200.10.1.txt");
}

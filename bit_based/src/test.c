#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h> 
#include <string.h>
#include <float.h>
#include <sys/resource.h>
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>

#include "genetic_alg.h"
#include "stdgraph.h"


struct test_param {
    int size;
    int target_color;
    int multiverse_size;
    int crossover_count;
    char graph_filename[128];
    char weight_filename[128];
    char result_filename[128];
};


atomic_int thread_count;
atomic_int reading;

void* test_graph(void *param) {
    int size = ((struct test_param*)param)->size;
    int multiverse_size = ((struct test_param*)param)->multiverse_size;
    int crossover_count = ((struct test_param*)param)->crossover_count;
    char *graph_filename = ((struct test_param*)param)->graph_filename;
    char *weight_filename = ((struct test_param*)param)->weight_filename;
    char *result_filename = ((struct test_param*)param)->result_filename;

    uint32_t edges[BLOCK_INDEX(size*(size-1)/2-1)+1];
    if(!read_graph(graph_filename, size, edges)) {
        printf("Could not initialize graph from %s, exitting ...\n", graph_filename);
        free(param);
        thread_count--;
        return NULL;
    }

    int weights[size];
    if(strncmp(weight_filename, "unweighted", 10) != 0) {
        if(!read_weights(weight_filename, size, weights)) {
            printf("Could not initialize graph weights from %s, exitting ...\n", weight_filename);
            free(param);
            thread_count--;
            return NULL;
        }
    } else {
        for(int i = 0; i < size; i++) 
            weights[i] = 1;
    }

    srand(time(NULL));
    srandom(time(NULL));

    printf("Testing the dataset %s:\n", graph_filename);

    int edge_count[size];
    count_edges(size, edges, edge_count);
    
    int max_edge_count = 0;
    for(int i = 0; i < size; i++) 
        if(max_edge_count < edge_count[i])
            max_edge_count = edge_count[i];

    int temp_uncolored;
    float best_time = FLT_MAX, temp_time, total_time = 0;
    int best_fitness = __INT_MAX__, temp_fitness, total_fitness = 0;
    int best_color_count = __INT_MAX__, temp_color_count, total_color_count = 0;
    uint32_t best_colors[max_edge_count][BLOCK_INDEX(size-1)+1], temp_colors[max_edge_count][BLOCK_INDEX(size-1)+1];
    memset(temp_colors, 0, max_edge_count*(BLOCK_INDEX(size-1)+1)*sizeof(uint32_t));
    memset(best_colors, 0, max_edge_count*(BLOCK_INDEX(size-1)+1)*sizeof(uint32_t));

    int greedy_color_count = graph_color_greedy(size, edges, temp_colors, max_edge_count);
    is_valid(size, edges, greedy_color_count, temp_colors);
    int iteration_count = 10;
    for(int k = 0; k < iteration_count; k++) {
        // graph_color_genetic_time = 0;
        // get_rand_color_time = 0;
        // graph_color_random_time = 0;
        // merge_colors_time = 0;
        // rm_vertex_time = 0;
        // search_back_time = 0;
        // local_search_time = 0;
        // crossover_time = 0;
        // crossover_thread_time = 0;
        // is_valid_time = 0;
        // count_edges_time = 0;
        // graph_color_greedy_time = 0;
        // count_conflicts_time = 0;

        memset(temp_colors, 0, max_edge_count*(BLOCK_INDEX(size-1)+1)*sizeof(uint32_t));

        temp_color_count = graph_color_genetic(
            size,
            edges,
            weights,
            greedy_color_count,
            crossover_count,
            temp_colors,
            &temp_fitness,
            &temp_time,
            &temp_uncolored,
            multiverse_size,
            MIN_COLOR_COUNT
        );

        total_color_count += temp_color_count;
        total_fitness += temp_fitness;
        total_time += temp_time;

        if (temp_fitness < best_fitness ||
            (temp_fitness == best_fitness && temp_color_count < best_color_count) ||
            (temp_fitness == best_fitness && temp_color_count == best_color_count && temp_time < best_time)) 
        {
            best_color_count = temp_color_count;
            best_fitness = temp_fitness;
            best_time = temp_time;
            memcpy(best_colors, temp_colors, max_edge_count*(BLOCK_INDEX(size-1)+1)*sizeof(uint32_t));
        }
    }

    char buffer[512];
    is_valid(size, edges, greedy_color_count, best_colors);
    sprintf(buffer,
        "\ngraph %s:\n"\
        "    base color: %d\n"\
        "    best solution:\n"\
        "        time    = %lf\n"\
        "        colors  = %d\n"\
        "    avg values:\n"\
        "        time    = %lf\n"\
        "        colors  = %f\n",
        graph_filename, 
        greedy_color_count,
        best_time, 
        best_color_count,
        total_time/iteration_count, 
        total_color_count/((float)iteration_count)
    );

    printf("%s\n", buffer);
    print_colors(result_filename, buffer, greedy_color_count, size, best_colors);

    free(param);
    thread_count--;
    return NULL;
}

void test_weighted(int size, int crossover_count, float color_density, char *graph_filename, char *weight_filename, char *result_filename) {
    uint32_t edges[BLOCK_INDEX(size*(size-1)/2)];
    if(!read_graph(graph_filename, size, edges)) {
        printf("Could not initialize graph from %s, exitting ...\n", graph_filename);
        return;
    }

    int weights[size];
    if(strncmp(weight_filename, "unweighted", 10) != 0) {
        if(!read_weights(weight_filename, size, weights)) {
            printf("Could not initialize graph weights from %s, exitting ...\n", weight_filename);
            return;
        }
    } else {
        for(int i = 0; i < size; i++) 
            weights[i] = 1;
    }

    srand(time(NULL));
    srandom(time(NULL));

    int edge_count[size];
    count_edges(size, edges, edge_count);
    
    int max_edge_count = 0;
    for(int i = 0; i < size; i++) 
        if(max_edge_count < edge_count[i])
            max_edge_count = edge_count[i];

    int temp_uncolored, total_uncolored = 0;
    float temp_time, total_time = 0;
    int temp_fitness, total_fitness = 0;
    int temp_color_count, total_color_count = 0;
    uint32_t best_colors[max_edge_count][BLOCK_INDEX(size-1)+1], temp_colors[max_edge_count][BLOCK_INDEX(size-1)+1];
    memset(best_colors, 0, max_edge_count*(BLOCK_INDEX(size-1)+1)*sizeof(uint32_t));

    int iteration_count = 10;
    char edge_density[6][3];
    strcpy(edge_density[0], "10");
    strcpy(edge_density[1], "25");
    strcpy(edge_density[2], "45");
    strcpy(edge_density[3], "60");
    strcpy(edge_density[4], "75");
    strcpy(edge_density[5], "90");
    for(int i = 1; i < 6; i++) {
        graph_filename[strlen(graph_filename) - 5] = i + '0';
        for(int j = 0; j < 6; j++) {
            strncpy(&graph_filename[strlen(graph_filename) - 8], edge_density[j], 2);
            read_graph(graph_filename, size, edges);
            for(int k = 0; k < iteration_count; k++) {
                // graph_color_genetic_time = 0;
                // get_rand_color_time = 0;
                // graph_color_random_time = 0;
                // merge_colors_time = 0;
                // rm_vertex_time = 0;
                // search_back_time = 0;
                // local_search_time = 0;
                // crossover_time = 0;
                // crossover_thread_time = 0;
                // is_valid_time = 0;
                // count_edges_time = 0;
                // graph_color_greedy_time = 0;
                // count_conflicts_time = 0;

                memset(temp_colors, 0, max_edge_count*size);

                temp_color_count = graph_color_genetic(
                    size,
                    edges,
                    weights,
                    size*color_density,
                    crossover_count,
                    temp_colors,
                    &temp_fitness,
                    &temp_time,
                    &temp_uncolored,
                    4,
                    MIN_COST
                );

                total_color_count += temp_color_count;
                total_fitness += temp_fitness;
                total_time += temp_time;
                total_uncolored += temp_uncolored;
            }
        }
    }

    char buffer[512];
    sprintf(buffer,
        "| %f | %f | %f |",
        total_time/iteration_count, 
        total_fitness/(((float)iteration_count) * 5 * 6),
        total_uncolored/(((float)iteration_count) * 5 * 6)
    );

    printf("%s\n", buffer);
}

int main(int argc, char *argv[]) {
    if(argc < 5) {
        printf("Too few arguments.\n");
        return 0;
    }

    const rlim_t kStackSize = 8192L * 1024L * 1024L;   // min stack size = 64 Mb
    struct rlimit rl;
    int result;

    result = getrlimit(RLIMIT_STACK, &rl);
    if (result == 0)
    {
        if (rl.rlim_cur < kStackSize)
        {
            rl.rlim_cur = kStackSize;
            result = setrlimit(RLIMIT_STACK, &rl);
            if (result != 0)
            {
                fprintf(stderr, "setrlimit returned result = %d\n", result);
            }
        }
    }

    thread_count = 0;
    reading = 0;

    FILE *test_list_file = fopen(argv[1], "r");
    
    pthread_attr_t attr;
    pthread_t temp;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 2048L*1024L*1024L);

    int num_of_threads = atoi(argv[2]);
    int multiverse_size = atoi(argv[3]);
    int crossover_count = atoi(argv[4]);

    char buffer[256];
    struct test_param *temp_param;
    while(!feof(test_list_file)) {
        temp_param = malloc(sizeof(struct test_param));

        thread_count++;
        fgets(buffer, 256, test_list_file);
        buffer[strcspn(buffer, "\n")] = 0;

        temp_param->size = atoi(strtok(buffer, " "));
        temp_param->multiverse_size = multiverse_size;
        temp_param->crossover_count = crossover_count;
        strcpy(temp_param->graph_filename, strtok(NULL, " "));
        strcpy(temp_param->weight_filename, strtok(NULL, " "));
        strcpy(temp_param->result_filename, strtok(NULL, " "));

        pthread_create(&temp, &attr, test_graph, temp_param);
        // test_graph(temp_param);
        // break;

        while (thread_count == num_of_threads);
    }

    while (thread_count > 0);

    fclose(test_list_file);

    // printf("|   time    |  fitness   | uncolored |\n");
    // char buffer[128];
    // strcpy(buffer, "../graph_datasets/INCEA100.10.1.col");
    // test_weighted(100, 1000, 0.04, buffer, "../graph_datasets/INCEA100.10.1.colw", "results/INCEA100.10.1.txt");
    // test_weighted(100, 1000, 0.08, buffer, "../graph_datasets/INCEA100.10.1.colw", "results/INCEA100.10.1.txt");
    // test_weighted(100, 1000, 0.1, buffer, "../graph_datasets/INCEA100.10.1.colw", "results/INCEA100.10.1.txt");
    // test_weighted(100, 1000, 0.15, buffer, "../graph_datasets/INCEA100.10.1.colw", "results/INCEA100.10.1.txt");
    // test_weighted(100, 1000, 0.2, buffer, "../graph_datasets/INCEA100.10.1.colw", "results/INCEA100.10.1.txt");

    // strcpy(buffer, "../graph_datasets/INCEA200.10.1.col");
    // test_weighted(200, 1000, 0.04, buffer, "../graph_datasets/INCEA200.10.1.colw", "results/INCEA200.10.1.txt");
    // test_weighted(200, 1000, 0.08, buffer, "../graph_datasets/INCEA200.10.1.colw", "results/INCEA200.10.1.txt");
    // test_weighted(200, 1000, 0.1, buffer, "../graph_datasets/INCEA200.10.1.colw", "results/INCEA200.10.1.txt");
    // test_weighted(200, 1000, 0.15, buffer, "../graph_datasets/INCEA200.10.1.colw", "results/INCEA200.10.1.txt");
    // test_weighted(200, 1000, 0.2, buffer, "../graph_datasets/INCEA200.10.1.colw", "results/INCEA200.10.1.txt");
}

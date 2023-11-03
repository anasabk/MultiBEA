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
    char graph_filename[128];
    char weight_filename[128];
    char result_filename[128];
};


atomic_int thread_count;
atomic_int reading;

void* test_graph(void *param) {
    int size = ((struct test_param*)param)->size;
    char *graph_filename = ((struct test_param*)param)->graph_filename;
    char *weight_filename = ((struct test_param*)param)->weight_filename;
    char *result_filename = ((struct test_param*)param)->result_filename;

    char edges[size][size];
    if(!read_graph(graph_filename, size, edges)) {
        printf("Could not initialize graph from %s, exitting ...\n", graph_filename);
        free(param);
        thread_count--;
        return NULL;
    }

    int weights[size];
    if(!read_weights(weight_filename, size, weights)) {
        printf("Could not initialize graph weights from %s, exitting ...\n", weight_filename);
        free(param);
        thread_count--;
        return NULL;
    }

    srand(time(NULL));

    printf("Testing the dataset %s:\n", graph_filename);

    int edge_count[size];
    const int max_edge_count = count_edges(size, edges, edge_count);

    float best_time = FLT_MAX, temp_time, total_time = 0;
    int best_fitness = __INT_MAX__, temp_fitness, total_fitness = 0;
    int best_color_count = __INT_MAX__, temp_color_count, total_color_count = 0;
    char best_colors[max_edge_count][size], temp_colors[max_edge_count][size];
    memset(best_colors, 0, max_edge_count*size);

    int greedy_color_count = graph_color_greedy(size, edges, temp_colors, max_edge_count);
    int iteration_count = 1;
    for(int k = 0; k < iteration_count; k++) {
        memset(temp_colors, 0, max_edge_count*size);

        temp_color_count = graph_color_genetic(
            size,
            edges,
            weights,
            greedy_color_count,
            20,
            temp_colors,
            &temp_fitness,
            &temp_time
        );

        total_color_count += temp_color_count;
        total_fitness += temp_fitness;
        total_time += temp_time;

        if (temp_fitness < best_fitness || 
            (temp_fitness == best_fitness && temp_color_count < best_color_count) ||
            (temp_fitness == best_fitness && temp_color_count <= best_color_count && temp_time < best_time)) 
        {
            best_color_count = temp_color_count;
            best_fitness = temp_fitness;
            best_time = temp_time;
            memcpy(best_colors, temp_colors, max_edge_count*size);
        }
    }

    char buffer[512];
    if(is_valid(size, edges, greedy_color_count, best_colors)) {
        double total_exec = crossover_time + merge_colors_time + rm_vertex_time + local_search_time;
        sprintf(buffer,
            "\ngraph %s:\n"\
            "    base color: %d\n"\
            "    best solution:\n"\
            "        time    = %lf\n"\
            "        colors  = %d\n"\
            "    avg values:\n"\
            "        time    = %lf\n"\
            "        colors  = %f\n"\
            "    Time used: \n"\
            "        Crossover:      %f%%\n"\
            "        Color Merge:    %f%%\n"\
            "        Vertex Removal: %f%%\n"\
            "        Local Search:   %f%%\n"\
            "        count conflicts:%f%%\n",
            graph_filename, 
            greedy_color_count,
            best_time, 
            best_color_count,
            total_time/iteration_count, 
            total_color_count/((float)iteration_count),
            crossover_time,
            merge_colors_time,
            rm_vertex_time,
            local_search_time,
            count_conflicts_time
        );

    } else {
        sprintf(buffer, 
            "\ngraph %s:\n"\
            "Could not find a solution. Base color: %d / Best fitness: %d.\n", 
            graph_filename, greedy_color_count, best_fitness
        );
    }

    printf("%s\n", buffer);
    print_colors(result_filename, buffer, greedy_color_count, size, best_colors);

    free(param);
    thread_count--;
    return NULL;
}


int main(int argc, char *argv[]) {
    if(argc < 2) {
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

    char buffer[256];
    struct test_param *temp_param;
    while(!feof(test_list_file)) {
        temp_param = malloc(sizeof(struct test_param));

        thread_count++;
        fgets(buffer, 256, test_list_file);
        buffer[strcspn(buffer, "\n")] = 0;

        temp_param->size = atoi(strtok(buffer, " "));
        strcpy(temp_param->graph_filename, strtok(NULL, " "));
        strcpy(temp_param->weight_filename, strtok(NULL, " "));
        strcpy(temp_param->result_filename, strtok(NULL, " "));

        // pthread_create(&temp, &attr, test_graph, temp_param);
        test_graph(temp_param);
        break;

        // while (thread_count == 10);
    }

    while (thread_count > 0);

    fclose(test_list_file);
}

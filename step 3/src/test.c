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


struct test_param {
    int size;
    char graph_filename[128];
    char result_filename[128];
};


atomic_int thread_count;

void* test_graph(void *param) {
    int size = ((struct test_param*)param)->size;
    char *graph_filename = ((struct test_param*)param)->graph_filename;
    char *result_filename = ((struct test_param*)param)->result_filename;

    char edges[size][size];
    if(!init_graph(graph_filename, size, edges)) {
        printf("Could not initialize graph %s, exitting ...\n", graph_filename);
        return NULL;
    }

    printf("Testing the dataset %s:\n", graph_filename);

    srand(time(NULL));

    int edge_count[size];
    const int max_edge_count = count_edges(size, edges, edge_count);

    char temp_colors[max_edge_count][size], best_colors[max_edge_count][size];
    memset(best_colors, 0, max_edge_count*size);

    clock_t t;
    float progress = 0, total_time = 0;
    int best_color_num = __INT_MAX__, temp_color_num;
    // print_progress(progress, 20);
    for(int k = 0; k < 20; k++) {
        memset(temp_colors, 0, max_edge_count*size);

        t = clock();
        temp_color_num = genetic_color(
            size,
            edges,
            edge_count,
            max_edge_count,
            temp_colors
        );
        t = clock() - t;
        total_time += ((double)t)/CLOCKS_PER_SEC;

        if(temp_color_num < best_color_num) {
            best_color_num = temp_color_num;
            memcpy(best_colors, temp_colors, max_edge_count*size);
        }

        // progress++;
        // print_progress(progress, 20);
    }

    if(is_valid(size, edges, best_color_num, best_colors)) {
        char buffer[256];
        sprintf(buffer, 
            "graph %s:\n"\
            "    best solution = %d\n\n"\
            "    time: avg  = %lf\n"
            , graph_filename, best_color_num, total_time/20);

        print_colors(result_filename, buffer, best_color_num, size, best_colors);
    }

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

    FILE *test_list = fopen(argv[1], "r");
    
    pthread_attr_t attr;
    pthread_t temp;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 1024L*1024L*1024L);

    char buffer[256];
    struct test_param *temp_param;
    while(!feof(test_list)) {
        temp_param = malloc(sizeof(struct test_param));

        thread_count++;
        fgets(buffer, 256, test_list);
        buffer[strcspn(buffer, "\n")] = 0;

        temp_param->size = atoi(strtok(buffer, " "));
        strcpy(temp_param->graph_filename, strtok(NULL, " "));
        strcpy(temp_param->result_filename, strtok(NULL, " "));

        pthread_create(&temp, &attr, test_graph, temp_param);
        // test_graph(temp_param);

        while (thread_count == 7);
    }


    // test_graph(138, "graph_datasets/anna.col");
    // test_graph(87, "graph_datasets/david.col");
    // // test_graph(561, "graph_datasets/homer.col");
    // test_graph(74, "resources/graph_datasets/huck.col", "resources/results/huck_colored.txt");
    // test_graph(80, "graph_datasets/jean.col");

    // test_graph(496, "graph_datasets/fpsol2.i.1.col");
    // test_graph(451, "graph_datasets/fpsol2.i.2.col");
    // test_graph(425, "graph_datasets/fpsol2.i.3.col");

    // test_graph(120, "resources/graph_datasets/games120.col", "resources/results/games120_colored.txt");

    // test_graph(864, "graph_datasets/inithx.i.1.col");
    // test_graph(645, "graph_datasets/inithx.i.2.col");
    // test_graph(621, "graph_datasets/inithx.i.3.col");

    // test_graph(250, "resources/graph_datasets/dsjc250.5.col", "resources/results/dsjc250_5_colored.txt");
    // test_graph(500, "resources/graph_datasets/dsjc500.1.col", "resources/results/dsjc500_1_colored.txt");
    // test_graph(500, "graph_datasets/dsjc500.5.col");
    // test_graph(500, "graph_datasets/dsjc500.9.col");
    // test_graph(1000, "graph_datasets/dsjc1000.1.col");
    // test_graph(1000, "graph_datasets/dsjc1000.5.col");
    // test_graph(1000, "graph_datasets/dsjc1000.9.col");

    // test_graph(500, "graph_datasets/dsjr500.1c.col");
    // test_graph(500, "graph_datasets/dsjr500.5.col");

    // test_graph(300, "graph_datasets/flat300_28_0.col");
    // test_graph(1000, "graph_datasets/flat1000_50_0.col");
    // test_graph(1000, "graph_datasets/flat1000_60_0.col");
    // test_graph(1000, "graph_datasets/flat1000_76_0.col");

    // test_graph(900, "graph_datasets/latin_square.col");

    // test_graph(450, "graph_datasets/le450_5a.col");
    // test_graph(450, "graph_datasets/le450_5b.col");
    // test_graph(450, "graph_datasets/le450_5c.col");
    // test_graph(450, "graph_datasets/le450_5d.col");
    // test_graph(450, "graph_datasets/le450_15a.col");
    // test_graph(450, "graph_datasets/le450_15b.col");
    // test_graph(450, "graph_datasets/le450_15c.col");
    // test_graph(450, "graph_datasets/le450_15d.col");
    // test_graph(450, "graph_datasets/le450_25a.col");
    // test_graph(450, "graph_datasets/le450_25b.col");
    // test_graph(450, "graph_datasets/le450_25c.col");
    // test_graph(450, "graph_datasets/le450_25d.col");

    // test_graph(128, "graph_datasets/miles250.col");
    // test_graph(128, "graph_datasets/miles500.col");
    // test_graph(128, "graph_datasets/miles750.col");
    // test_graph(128, "graph_datasets/miles1000.col");
    // test_graph(128, "graph_datasets/miles1500.col");

    // test_graph(197, "graph_datasets/mulsol.i.1.col");
    // test_graph(188, "graph_datasets/mulsol.i.2.col");
    // test_graph(184, "graph_datasets/mulsol.i.3.col");
    // test_graph(185, "graph_datasets/mulsol.i.4.col");
    // test_graph(186, "graph_datasets/mulsol.i.5.col");

    // // test_graph(5, "graph_datasets/myciel2.col");
    // test_graph(11, "graph_datasets/myciel3.col");
    // test_graph(23, "graph_datasets/myciel4.col");
    // test_graph(47, "graph_datasets/myciel5.col");
    // test_graph(95, "graph_datasets/myciel6.col");
    // test_graph(191, "graph_datasets/myciel7.col");

    // test_graph(25, "graph_datasets/queen5_5.col");
    // test_graph(36, "graph_datasets/queen6_6.col");
    // test_graph(49, "graph_datasets/queen7_7.col");
    // test_graph(96, "graph_datasets/queen8_8.col");
    // test_graph(64, "graph_datasets/queen8_12.col");
    // test_graph(81, "graph_datasets/queen9_9.col");
    // test_graph(100, "graph_datasets/queen10_10.col");
    // test_graph(121, "graph_datasets/queen11_11.col");
    // test_graph(144, "graph_datasets/queen12_12.col");
    // test_graph(169, "graph_datasets/queen13_13.col");
    // test_graph(196, "graph_datasets/queen14_14.col");
    // test_graph(225, "graph_datasets/queen15_15.col");
    // test_graph(256, "graph_datasets/queen16_16.col");

    // test_graph(250, "graph_datasets/r250.5.col");
    // test_graph(1000, "graph_datasets/r1000.1c.col");
    // test_graph(1000, "graph_datasets/r1000.5.col");

    // test_graph(352, "graph_datasets/school1_nsh.col");
    // test_graph(385, "graph_datasets/school1.col");

    // test_graph(211, "graph_datasets/zeroin.i.1.col");
    // test_graph(211, "graph_datasets/zeroin.i.2.col");
    // test_graph(206, "graph_datasets/zeroin.i.3.col");

    // test_graph(2000, "graph_datasets/C2000.5.col");
    // test_graph(4000, "graph_datasets/C4000.5.col");
}

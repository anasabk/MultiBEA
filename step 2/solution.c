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

#define VERTEX_NUM 4000

void print_progress(size_t count, size_t max) {
    const int bar_width = 50;

    float progress = (float) count / max;
    int bar_length = progress * bar_width;

    printf("\rProgress: [");
    for (int i = 0; i < bar_length; ++i) {
        printf("#");
    }
    for (int i = bar_length; i < bar_width; ++i) {
        printf(" ");
    }
    printf("] %.2f%%", progress * 100);

    fflush(stdout);
}

bool exists(int* arr, int len, int target) {
    for(int i = 0; i < len; i++)
        if(target == arr[i]) return true;

    return false;
}

bool init_graph(char* filename, int size, char edges[][size]) {
    FILE *fp = fopen(filename, "r");
    
    if(fp == NULL)
        return false;

    memset(edges, 0, size*size);

    char buffer[64];
    char* token1, *token2;
    int row, column;
    int i;
    while(!feof(fp)) {
        fgets(buffer, 64, fp);
        buffer[strlen(buffer) - 1] = 0;
        token1 = strtok (buffer, " ");
        row = atoi(token1) - 1;
        token2 = strtok (NULL, " ");
        column = atoi(token2) -1;
        edges[row][column] = 1;
        edges[column][row] = 1;
    }
    
    fclose(fp);
    return true;
}

bool is_valid(int size, char edges[][size], char colors[][size]) {
    // Scan for vertices with multiple colors.
    int is_colored;
    // Iterate through vertices.
    for(int i = 0; i < size; i++) {
        is_colored = 0;
        // Iterate through colors and look for the vertex.
        for(int j = 0; j < size; j++)
            is_colored += colors[j][i];

        // Check if the vertex had more then one color.
        if(is_colored > 1) {
            printf("The vertex %d has more than one color, exitting\n", i);
            return false;
        }
    }


    // Scan for adjacent vertices with the same color. 
    int color1, color2;
    // Iterate through each vertex.
    for(int i = 0; i < size; i++) {
        // Iterate through each edge of the vertex.
        for(int j = 0; j < size; j++) {
            if(edges[i][j]) {
                // Edge exists.
                color1 = 0;
                color2 = 0;

                // Check for the other vertex's color.
                for(int k = 0; k < size; k++) {
                    if(colors[k][j] & colors[k][i]){
                        // The two vertices have the same color.
                        printf("The verteces %d and %d are connected and have the same color %d, exitting\n", i, j, k);
                        return 0;
                    }
                }
            }
        }
    }

    return true;
}

int count_edges(int size, char edges[][size], int count[]) {
    for(int i = 0; i < size; i++)
        count[i] = 0;

    for(int i = 0; i < size; i++) {
        for(int j = 0; j <= i; j++) {
            count[i] += edges[i][j];
            count[j] += edges[i][j];
        }
    }

    int max_count = 0;
    for(int i = 0; i < size; i++)
        if(max_count < count[i])
            max_count = count[i];

    return max_count;
}

int color_graph(int size, char edges[][size], char colors[][size]) {
    // Create a random queue of vertices to propagate.
    srand(time(0));
    int prob_queue[size];
    int i = 0;
    while (i < size) {
        prob_queue[i] = rand()%size;
        if(!exists(prob_queue, i, prob_queue[i]))
            i++;
    }
    
    // Go through the queue and color each vertex.
    unsigned char adjacent_colors[size];
    unsigned int max_color = 0;
    int current_vert;
    for(int i = 0; i < size; i++) {
        // Initialize the temporary data.
        current_vert = prob_queue[i];
        for(int j = 0; j < size; j++)
            adjacent_colors[j] = 0;

        // Go through each edge of the vertex and save the used colors in adjacent_colors array.
        for(int j = 0; j < size; j++) {
            // This is a neighbor
            if(edges[current_vert][j] == 1) {
                // Check its color.
                for(int k = 0; k < size; k++) {
                    if(colors[k][j]) 
                        adjacent_colors[k] = 1;
                }
            }
        }

        // Find the first unused color (starting from 0) and assign this vertex to it.
        for(int j = 0; j < size; j++) {
            if(!adjacent_colors[j]) {
                colors[j][current_vert] = 1;
                if(max_color < j) max_color = j;
                break;
            }
        }
    }

    return max_color;
}

void test_graph(int size, char* filename) {
    unsigned int total_color, min_color, temp_color;
    double total_time, temp_time, min_time, avg_time;
    clock_t t;

    char edges[size][size];
    if(!init_graph(filename, size, edges)) {
        printf("Could not initialize graph %s, exitting ...\n", filename);
        return;
    }

    printf("Calculating 100 samples of %s:\n", filename);

    int edge_count[size];
    int max_edge_count = count_edges(size, edges, edge_count);
    char colors[100][max_edge_count][size];
    memset(colors, 0, size*max_edge_count*100);

    min_color = -1;
    min_time = DBL_MAX;
    total_time = 0;
    total_color = 0;
    FILE* fresults;
    char result_filename[128];
    for (int i = 0; i < 100; i++) {
        t = clock();
        temp_color = color_graph(size, edges, colors[i]);
        t = clock() - t;

        temp_time = ((double)t)/CLOCKS_PER_SEC;

        if(temp_color < min_color)
            min_color = temp_color;

        if(temp_time < min_time)
            min_time = temp_time;

        total_time += temp_time;
        total_color += temp_color;

        sprintf(result_filename, "results/%d_%s", i, &filename[15]);
        fresults = fopen(result_filename, "w");
        for(int j = 0; j < max_edge_count; j++) {
            for(int k = 0; k < size; k++) {
                if(colors[i][j][k]) 
                    fprintf(fresults, "%d %d\n", j, k);
            }
        }
        fclose(fresults);

        print_progress(i + 1, 100);
    }

    printf("\n\ngraph %s:\n"\
           "    coloring: avg  = %d\n"\
           "              best = %d\n\n"\
           "    time: avg  = %lf\n"\
           "          best = %lf\n\n"
           , filename, total_color/100, min_color, total_time/100, min_time);
}


int main() {
    const rlim_t kStackSize = 1024L * 1024L * 1024L;   // min stack size = 64 Mb
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

    test_graph(250, "graph_datasets/dsjc250.5.col");
    test_graph(500, "graph_datasets/dsjc500.1.col");
    test_graph(500, "graph_datasets/dsjc500.5.col");
    test_graph(500, "graph_datasets/dsjc500.9.col");
    test_graph(1000, "graph_datasets/dsjc1000.1.col");
    test_graph(1000, "graph_datasets/dsjc1000.5.col");
    test_graph(1000, "graph_datasets/dsjc1000.9.col");
    test_graph(500, "graph_datasets/dsjr500.1c.col");
    test_graph(500, "graph_datasets/dsjr500.5.col");
    test_graph(300, "graph_datasets/flat300_28_0.col");
    test_graph(1000, "graph_datasets/flat1000_50_0.col");
    test_graph(1000, "graph_datasets/flat1000_60_0.col");
    test_graph(1000, "graph_datasets/flat1000_76_0.col");
    test_graph(900, "graph_datasets/latin_square.col");
    test_graph(450, "graph_datasets/le450_25c.col");
    test_graph(450, "graph_datasets/le450_25d.col");
    test_graph(250, "graph_datasets/r250.5.col");
    test_graph(1000, "graph_datasets/r1000.1c.col");
    test_graph(1000, "graph_datasets/r1000.5.col");
    test_graph(2000, "graph_datasets/C2000.5.col");
    test_graph(4000, "graph_datasets/C4000.5.col");
}

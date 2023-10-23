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
        buffer[strcspn(buffer, "\n")] = 0;
        
        token1 = strtok (buffer, " ");
        row = atoi(token1) - 1;
        
        token2 = strtok (NULL, " ");
        column = atoi(token2) - 1;
        
        edges[row][column] = 1;
        edges[column][row] = 1;
    }
    
    fclose(fp);
    return true;
}

bool is_valid(int size, const char edges[size][size], int color_num, const char colors[color_num][size]) {
    int i, j, k;
    // Scan for vertices with multiple colors.
    int is_colored;
    // Iterate through vertices.
    for(i = 0; i < size; i++) {
        is_colored = 0;
        // Iterate through colors and look for the vertex.
        for(j = 0; j < color_num; j++)
            is_colored += colors[j][i];

        // Check if the vertex had more then one color.
        if(is_colored > 1) {
            printf("The vertex %d has more than one color, exitting\n", i+1);
            return false;
        }
    }

    for(i = 0; i < color_num; i++) { // Through every color.
        for(j = 0; j < size; j++) { // Through every vertex in color i.
            if(colors[i][j]) {
                for(k = j; k < size; k++) { // Through every vertex after j in color i.
                    if(colors[i][k] && edges[j][k]) {
                        // The two vertices have the same color.
                        printf("The verteces %d and %d are connected and have the same color %d, exitting\n", i+1, j+1, k);
                        return 0;
                    }
                }
            }
        }
    }

    return true;
}

int count_edges(int size, const char edges[][size], int count[]) {
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

int color_graph(int size, const char edges[][size], char colors[][size]) {
    // Create a random queue of vertices to propagate.
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

    return max_color + 1;
}

int get_rand_color(int size, int colors_used, char used_list[]) {
    if(colors_used >= size - 2) {
        for(int i = 0; i < size; i++) {
            if(!used_list[i]) {
                used_list[i] = 1;
                return i;
            }
        }

        return -1;
    }

    int temp;
    while(1) {
        temp = rand()%size;
        if(!used_list[temp]) {
            used_list[temp] = 1;
            return temp;
        }
    }
}

int crossover(
    int size, 
    int color_num1, 
    int color_num2, 
    const char parent1[][size], 
    const char parent2[][size], 
    char child[][size], 
    const char edges[][size], 
    const int num_of_edges[]) 
{
    int max_color_num = color_num1 > color_num2 ? color_num1 : color_num2;

    char used_color_list[2][max_color_num];
    memset(used_color_list, 0, 2*max_color_num);

    char used_vertex_list[size];
    memset(used_vertex_list, 0, size);

    char pool[size];
    memset(pool, 0, size);

    int color1, color2, last_color = 0;
    int i, j, k, h;
    for(i = 0; i < max_color_num; i++) { // i = number of colors used, the index of the current color in the child.
        // Pick 2 random colors.
        color1 = get_rand_color(color_num1, i, used_color_list[0]);
        color2 = get_rand_color(color_num2, i, used_color_list[1]);

        if(color1 == -1) {
            // Fill a color in the child.
            for(j = 0; j < size; j++) {  // j = index of a vertex.
                if(!used_vertex_list[j] && parent2[color2][j]) {
                    pool[j] = 1;
                    used_vertex_list[j] = 1;
                }
            }

        } else if (color2 == -1) {
            // Fill a color in the child.
            for(j = 0; j < size; j++) {  // j = index of a vertex.
                if(!used_vertex_list[j] && parent1[color1][j]) {
                    pool[j] = 1;
                    used_vertex_list[j] = 1;
                }
            }
        }

        // Fill a color in the child.
        for(j = 0; j < size; j++) {  // j = index of a vertex.
            if(!used_vertex_list[j]) {
                child[i][j] = parent1[color1][j] | parent2[color2][j] | pool[j];
                used_vertex_list[j] = child[i][j];
            }
        }

        if(last_color < i) last_color = i;

        // Check for any conflicts.
        for(j = 0; j < size; j++) {   // j = index of the vertex to search for conflicts.
            if(child[i][j]) { // Check if the vertex j has this color i.
                for(k = j + 1; k < size; k++) {  // k = index of a vertex to check if it has an edge with j.
                    if(child[i][k] && edges[j][k]) { // Check if the vertex k has the color i and has an edge with the vertex j.
                        if(num_of_edges[j] > num_of_edges[k]) {
                            child[i][k] = 0;
                            pool[k] = 1;

                        } else {
                            child[i][j] = 0;
                            pool[j] = 1;
                            break;
                        }
                    }
                }
            }
        }
    }

    for(k = 0; k < size; k++) { // k = vertex in pool
        if(pool[k]) {
            for(j = 0; j < size; j++) { // j = current color in child
                for(h = 0; h < size; h++) { // h = current vertex in color j
                    if(child[j][h] && edges[k][h]) { // Check if the vertex h has the color j and has an edge with the vertex k.
                        break;
                    }
                }

                if(h == size) {
                    if(last_color < j) last_color = j;
                    child[j][k] = 1;
                    pool[k] = 0;
                    break;
                }
            }
        }
    }

    return last_color + 1;
}

void print_colors(char *graph_name, int max_color_num, int size, char colors[][size]) {
    FILE* fresults;
    char filename[128];
    sprintf(filename, "%s.txt", graph_name);

    fresults = fopen(filename, "w");
    for(int j = 0; j < max_color_num; j++)
        for(int k = 0; k < size; k++)
            if(colors[j][k]) 
                fprintf(fresults, "%d %d\n", j, k + 1);

    fclose(fresults);
}

void test_graph(int size, char* filename) {
    char edges[size][size];
    if(!init_graph(filename, size, edges)) {
        printf("Could not initialize graph %s, exitting ...\n", filename);
        return;
    }

    printf("Testing the dataset %s:\n", filename);

    int s, numb;
    FILE *f;

    clock_t t;
    int edge_count[size];
    const int max_edge_count = count_edges(size, edges, edge_count);
    int color_count[100];
    char colors[100][max_edge_count][size];
    char child[max_edge_count][size];
    FILE* fresults;
    char result_filename[128];
    int child_colors, parent1, parent2, temp_color, best = 0, best_child = 0, best_base = 0;
    float progress = 1, total_time = 0;
    print_progress(progress, 22000);
    for(int k = 0; k < 20; k++) {
        f = fopen("/dev/urandom", "rb");
        fread(&s, sizeof(int), 1, f);
        fclose(f);
        srand(s);
        memset(colors, 0, size*max_edge_count*100);

        t = clock();

        best = 0;
        for (int i = 0; i < 100; i++) {
            color_count[i] = color_graph(size, edges, colors[i]);

            if(color_count[best] > color_count[i])
                best = i;

            if(color_count[best_base] > color_count[i])
                best_base = i;

            progress++;
            print_progress(progress, 22000);
        }

        best_child = 0;
        for(int i = 0; i < 1000; i++) {
            memset(child, 0, size*max_edge_count);
            parent1 = rand()%100;
            parent2 = rand()%100;

            child_colors = crossover(
                size, 
                color_count[parent1], 
                color_count[parent2], 
                colors[parent1], 
                colors[parent2], 
                child, 
                edges, 
                edge_count
            );

            if(child_colors < color_count[parent1]) {
                color_count[parent1] = child_colors;
                for(int j = 0; j < max_edge_count; j++) 
                    for(int k = 0; k < size; k++) 
                        colors[parent1][j][k] = child[j][k];

                if(color_count[best] > child_colors)
                    best = parent2;

                if(color_count[best_child] > child_colors)
                    best_child = parent2;

            } else if(child_colors < color_count[parent2]) {
                color_count[parent2] = child_colors;
                for(int j = 0; j < max_edge_count; j++) 
                    for(int k = 0; k < size; k++) 
                        colors[parent2][j][k] = child[j][k];
                
                if(color_count[best] > child_colors)
                    best = parent2;

                if(color_count[best_child] > child_colors)
                    best_child = parent2;
            }

            progress++;
            print_progress(progress, 22000);
        }


        t = clock() - t;
        total_time += ((double)t)/CLOCKS_PER_SEC;
    }


    if(is_valid(size, edges, color_count[best], colors[best])) {
        printf("\n\ngraph %s:\n"\
            "    coloring: best overall solution = %d\n"\
            "              best base solution = %d\n"\
            "              best child solution = %d\n\n"\
            "    time: avg  = %lf\n\n"
            , filename, color_count[best], color_count[best_base], color_count[best_child], total_time/20);
    }
}


int main() {
    const rlim_t kStackSize = 2048L * 1024L * 1024L;   // min stack size = 64 Mb
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


    test_graph(138, "graph_datasets/anna.col");
    test_graph(87, "graph_datasets/david.col");
    // test_graph(561, "graph_datasets/homer.col");
    test_graph(74, "graph_datasets/huck.col");
    test_graph(80, "graph_datasets/jean.col");

    test_graph(496, "graph_datasets/fpsol2.i.1.col");
    test_graph(451, "graph_datasets/fpsol2.i.2.col");
    test_graph(425, "graph_datasets/fpsol2.i.3.col");

    test_graph(120, "graph_datasets/games120.col");

    test_graph(864, "graph_datasets/inithx.i.1.col");
    test_graph(645, "graph_datasets/inithx.i.2.col");
    test_graph(621, "graph_datasets/inithx.i.3.col");

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

    test_graph(450, "graph_datasets/le450_5a.col");
    test_graph(450, "graph_datasets/le450_5b.col");
    test_graph(450, "graph_datasets/le450_5c.col");
    test_graph(450, "graph_datasets/le450_5d.col");
    test_graph(450, "graph_datasets/le450_15a.col");
    test_graph(450, "graph_datasets/le450_15b.col");
    test_graph(450, "graph_datasets/le450_15c.col");
    test_graph(450, "graph_datasets/le450_15d.col");
    test_graph(450, "graph_datasets/le450_25a.col");
    test_graph(450, "graph_datasets/le450_25b.col");
    test_graph(450, "graph_datasets/le450_25c.col");
    test_graph(450, "graph_datasets/le450_25d.col");

    test_graph(128, "graph_datasets/miles250.col");
    test_graph(128, "graph_datasets/miles500.col");
    test_graph(128, "graph_datasets/miles750.col");
    test_graph(128, "graph_datasets/miles1000.col");
    test_graph(128, "graph_datasets/miles1500.col");

    test_graph(197, "graph_datasets/mulsol.i.1.col");
    test_graph(188, "graph_datasets/mulsol.i.2.col");
    test_graph(184, "graph_datasets/mulsol.i.3.col");
    test_graph(185, "graph_datasets/mulsol.i.4.col");
    test_graph(186, "graph_datasets/mulsol.i.5.col");

    // test_graph(5, "graph_datasets/myciel2.col");
    test_graph(11, "graph_datasets/myciel3.col");
    test_graph(23, "graph_datasets/myciel4.col");
    test_graph(47, "graph_datasets/myciel5.col");
    test_graph(95, "graph_datasets/myciel6.col");
    test_graph(191, "graph_datasets/myciel7.col");

    test_graph(25, "graph_datasets/queen5_5.col");
    test_graph(36, "graph_datasets/queen6_6.col");
    test_graph(49, "graph_datasets/queen7_7.col");
    test_graph(96, "graph_datasets/queen8_8.col");
    test_graph(64, "graph_datasets/queen8_12.col");
    test_graph(81, "graph_datasets/queen9_9.col");
    test_graph(100, "graph_datasets/queen10_10.col");
    test_graph(121, "graph_datasets/queen11_11.col");
    test_graph(144, "graph_datasets/queen12_12.col");
    test_graph(169, "graph_datasets/queen13_13.col");
    test_graph(196, "graph_datasets/queen14_14.col");
    test_graph(225, "graph_datasets/queen15_15.col");
    test_graph(256, "graph_datasets/queen16_16.col");

    test_graph(250, "graph_datasets/r250.5.col");
    test_graph(1000, "graph_datasets/r1000.1c.col");
    test_graph(1000, "graph_datasets/r1000.5.col");

    test_graph(352, "graph_datasets/school1_nsh.col");
    test_graph(385, "graph_datasets/school1.col");

    test_graph(211, "graph_datasets/zeroin.i.1.col");
    test_graph(211, "graph_datasets/zeroin.i.2.col");
    test_graph(206, "graph_datasets/zeroin.i.3.col");

    test_graph(2000, "graph_datasets/C2000.5.col");
    test_graph(4000, "graph_datasets/C4000.5.col");
}

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h> 
#include <string.h>

#define VERTEX_NUM 4000


bool exists(int* arr, int len, int target) {
    for(int i = 0; i < len; i++)
        if(target == arr[i]) return true;

    return false;
}

bool init_graph(char* filename, int size, char edges[][size]) {
    FILE *fp = fopen(filename, "r");
    
    if(fp == NULL)
        return false;

    for(int i = 0; i < size; i++)
        for(int j = 0; j < size; j++)
            edges[i][j] = 0;

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
    
    return true;
}

bool is_valid(int size, char edges[][size], char colors[][size]) {
    int is_colored;
    for(int i = 0; i < size; i++) {
        is_colored = 0;
        for(int j = 0; j < size; j++) {
            is_colored += colors[j][i];
        }

        if(is_colored > 1) {
            printf("The vertex %d has more than one color, exitting\n", i);
            return false;
        }
    }


    int color1, color2;
    for(int i = 0; i < size; i++) {
        for(int j = 0; j < size; j++) {
            if(edges[i][j]) {
                color1 = 0;
                color2 = 0;

                for(int k = 0; k < size; k++) {
                    if(colors[k][j] & colors[k][i]){
                        printf("The verteces %d and %d are connected and have the same color %d, exitting\n", i, j, k);
                        return 0;
                    }
                }
            }
        }
    }

    return true;
}

int color_graph(char* dataset_name, int size, double* time_taken) {
    /**
     * This 2D array stores the edges of every vertex in the graph.
     * The numbers of rows and columns present the numbers of vertices,
     * and the place where they cross is an edges, if it is assigned to
     * 1, then the edge exists, otherwise, it doesn't.
     */
    char edges[size][size];
    if(!init_graph(dataset_name, size, edges)) {
        printf("Could not initialize graph, exitting ...\n");
        return 0;
    }


    /**
     * This 2D array stores the color of every vertex.
     * The row numbers present the colors, and the column numbers 
     * present the vertex numbers. When a cell is assigned to 1,
     * it means that the the vertex has the color of that row.
     */
    char colors[size][size];
    for(int i = 0; i < size; i++)
        for(int j = 0; j < size; j++)
            colors[i][j] = 0;


    // Create a random queue of vertices to propagate.
    srand(time(0));
    int prob_queue[size];
    int current;
    int i = 0;
    while (i < size) {
        current = rand()%size;
        if(!exists(prob_queue, i, current)) {
            prob_queue[i] = current;
            i++;
        }
    }


    clock_t t;
    t = clock(); 
    
    // Go through the queue and color each vertex.
    unsigned char adjacent_colors[size];
    int current_vert;
    int max_color = 0;
    for(int i = 0; i < size; i++) {
        // Initialize the temporary data.
        for(int j = 0; j < size; j++)
            adjacent_colors[j] = 0;

        current_vert = prob_queue[i];


        // Go through each edge of the vertex and list the used colors.
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

        // Find the least unused color and assign this vertex to it.
        for(int j = 0; j < size; j++) {
            if(!adjacent_colors[j]) {
                colors[j][current_vert] = 1;
                if(max_color < j) max_color = j;
                break;
            }
        }
    }

    t = clock() - t; 
    *time_taken = ((double)t)/CLOCKS_PER_SEC; // in seconds 
    

    if(!is_valid(size, edges, colors)) {
        // printf("graph %s could not be colored.\n");
        return 0;
    }
 
    // printf("%s took %f seconds to finish, with %d colors.\n", dataset_name, time_taken, max_color); 

    // for(int i = 0; i < max_color; i++) {
    //     printf("color %d: ", i);
    //     for(int j = 0; j < size; j++) 
    //         if(colors[i][j]) printf("%d, ", j);
    //     printf("\n");
    // }

    return max_color;
}


int main() {
    unsigned int min_color, temp_color;
    double temp_time, min_time; 

    min_color = -1;
    for (size_t i = 0; i < 10; i++) {
        temp_color = color_graph("graph_datasets/C2000.5.col", 2000, &temp_time);
        if(temp_color < min_color) {
            min_time = temp_time;
            min_color = temp_color;
        }
        printf("done,\n");
    }
    printf("graph C2000.5 colored in %f seconds and with %d colors.\n", min_time, min_color);

    // min_color = -1;
    // for (size_t i = 0; i < 100; i++) {
    //     temp_color = color_graph("graph_datasets/C4000.5.col", 4000, &temp_time);
    //     if(temp_color < min_color) {
    //         min_time = temp_time;
    //         min_color = temp_color;
    //     }
    // }
    // printf("graph C4000.5 colored in %f seconds and with %d colors.\n", min_time, min_color);

    // min_color = 250;
    // for (size_t i = 0; i < 100; i++) {
    //     temp_color = color_graph("graph_datasets/dsjc250.5.col", 250, &temp_time);
    //     if(temp_color < min_color) {
    //         min_time = temp_time;
    //         min_color = temp_color;
    //     }
    // }
    // printf("graph dsjc250.5 colored in %f seconds and with %d colors.\n", (double)min_time, min_color);

    // min_color = 500;
    // for (size_t i = 0; i < 100; i++) {
    //     temp_color = color_graph("graph_datasets/dsjc500.1.col", 500, &temp_time);
    //     if(temp_color < min_color) {
    //         min_time = temp_time;
    //         min_color = temp_color;
    //     }
    // }
    // printf("graph dsjc500.1 colored in %f seconds and with %d colors.\n", (double)min_time, min_color);

    // min_color = 500;
    // for (size_t i = 0; i < 100; i++) {
    //     temp_color = color_graph("graph_datasets/dsjc500.5.col", 500, &temp_time);
    //     if(temp_color < min_color) {
    //         min_time = temp_time;
    //         min_color = temp_color;
    //     }
    // }
    // printf("graph dsjc500.5 colored in %f seconds and with %d colors.\n", (double)min_time, min_color);

    // min_color = 500;
    // for (size_t i = 0; i < 100; i++) {
    //     temp_color = color_graph("graph_datasets/dsjc500.9.col", 500, &temp_time);
    //     if(temp_color < min_color) {
    //         min_time = temp_time;
    //         min_color = temp_color;
    //     }
    // }
    // printf("graph dsjc500.9 colored in %f seconds and with %d colors.\n", (double)min_time, min_color);

    // min_color = 500;
    // for (size_t i = 0; i < 100; i++) {
    //     temp_color = color_graph("graph_datasets/dsjc1000.1.col", 1000, &temp_time);
    //     if(temp_color < min_color) {
    //         min_time = temp_time;
    //         min_color = temp_color;
    //     }
    // }
    // printf("graph dsjc1000.1 colored in %f seconds and with %d colors.\n", (double)min_time, min_color);

    // min_color = 500;
    // for (size_t i = 0; i < 100; i++) {
    //     temp_color = color_graph("graph_datasets/dsjc1000.5.col", 1000, &temp_time);
    //     if(temp_color < min_color) {
    //         min_time = temp_time;
    //         min_color = temp_color;
    //     }
    // }
    // printf("graph dsjc1000.5 colored in %f seconds and with %d colors.\n", (double)min_time, min_color);

    // min_color = 1000;
    // for (size_t i = 0; i < 100; i++) {
    //     temp_color = color_graph("graph_datasets/dsjc1000.9.col", 1000, &temp_time);
    //     if(temp_color < min_color) {
    //         min_time = temp_time;
    //         min_color = temp_color;
    //     }
    // }
    // printf("graph dsjc1000.9 colored in %f seconds and with %d colors.\n", (double)min_time, min_color);

    // min_color = 500;
    // for (size_t i = 0; i < 100; i++) {
    //     temp_color = color_graph("graph_datasets/dsjr500.1c.col", 500, &temp_time);
    //     if(temp_color < min_color) {
    //         min_time = temp_time;
    //         min_color = temp_color;
    //     }
    // }
    // printf("graph dsjr500.1c colored in %f seconds and with %d colors.\n", (double)min_time, min_color);

    // min_color = 500;
    // for (size_t i = 0; i < 100; i++) {
    //     temp_color = color_graph("graph_datasets/dsjr500.5.col", 500, &temp_time);
    //     if(temp_color < min_color) {
    //         min_time = temp_time;
    //         min_color = temp_color;
    //     }
    // }
    // printf("graph dsjr500.5 colored in %f seconds and with %d colors.\n", (double)min_time, min_color);

    // min_color = 300;
    // for (size_t i = 0; i < 100; i++) {
    //     temp_color = color_graph("graph_datasets/flat300_28_0.col", 300, &temp_time);
    //     if(temp_color < min_color) {
    //         min_time = temp_time;
    //         min_color = temp_color;
    //     }
    // }
    // printf("graph flat300_28_0 colored in %f seconds and with %d colors.\n", (double)min_time, min_color);

    // min_color = 1000;
    // for (size_t i = 0; i < 100; i++) {
    //     temp_color = color_graph("graph_datasets/flat1000_50_0.col", 1000, &temp_time);
    //     if(temp_color < min_color) {
    //         min_time = temp_time;
    //         min_color = temp_color;
    //     }
    // }
    // printf("graph flat1000_50_0 colored in %f seconds and with %d colors.\n", (double)min_time, min_color);

    // min_color = 1000;
    // for (size_t i = 0; i < 100; i++) {
    //     temp_color = color_graph("graph_datasets/flat1000_60_0.col", 1000, &temp_time);
    //     if(temp_color < min_color) {
    //         min_time = temp_time;
    //         min_color = temp_color;
    //     }
    // }
    // printf("graph flat1000_60_0 colored in %f seconds and with %d colors.\n", (double)min_time, min_color);

    // min_color = 1000;
    // for (size_t i = 0; i < 100; i++) {
    //     temp_color = color_graph("graph_datasets/flat1000_76_0.col", 1000, &temp_time);
    //     if(temp_color < min_color) {
    //         min_time = temp_time;
    //         min_color = temp_color;
    //     }
    // }
    // printf("graph flat1000_76_0 colored in %f seconds and with %d colors.\n", (double)min_time, min_color);

    // min_color = 900;
    // for (size_t i = 0; i < 100; i++) {
    //     temp_color = color_graph("graph_datasets/latin_square.col", 900, &temp_time);
    //     if(temp_color < min_color) {
    //         min_time = temp_time;
    //         min_color = temp_color;
    //     }
    // }
    // printf("graph latin_square colored in %f seconds and with %d colors.\n", (double)min_time, min_color);

    // min_color = 450;
    // for (size_t i = 0; i < 100; i++) {
    //     temp_color = color_graph("graph_datasets/le450_25c.col", 450, &temp_time);
    //     if(temp_color < min_color) {
    //         min_time = temp_time;
    //         min_color = temp_color;
    //     }
    // }
    // printf("graph le450_25c colored in %f seconds and with %d colors.\n", (double)min_time, min_color);

    // min_color = 450;
    // for (size_t i = 0; i < 100; i++) {
    //     temp_color = color_graph("graph_datasets/le450_25d.col", 450, &temp_time);
    //     if(temp_color < min_color) {
    //         min_time = temp_time;
    //         min_color = temp_color;
    //     }
    // }
    // printf("graph le450_25d colored in %f seconds and with %d colors.\n", (double)min_time, min_color);

    // min_color = 250;
    // for (size_t i = 0; i < 100; i++) {
    //     temp_color = color_graph("graph_datasets/r250.5.col", 250, &temp_time);
    //     if(temp_color < min_color) {
    //         min_time = temp_time;
    //         min_color = temp_color;
    //     }
    // }
    // printf("graph r250.5 colored in %f seconds and with %d colors.\n", (double)min_time, min_color);

    // min_color = 1000;
    // for (size_t i = 0; i < 100; i++) {
    //     temp_color = color_graph("graph_datasets/r1000.1c.col", 1000, &temp_time);
    //     if(temp_color < min_color) {
    //         min_time = temp_time;
    //         min_color = temp_color;
    //     }
    // }
    // printf("graph r1000.1c colored in %f seconds and with %d colors.\n", (double)min_time, min_color);

    // min_color = 1000;
    // for (size_t i = 0; i < 100; i++) {
    //     temp_color = color_graph("graph_datasets/r1000.5.col", 1000, &temp_time);
    //     if(temp_color < min_color) {
    //         min_time = temp_time;
    //         min_color = temp_color;
    //     }
    // }
    // printf("graph r1000.5 colored in %f seconds and with %d colors.\n", (double)min_time, min_color);
}

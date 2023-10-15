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

bool init_graph(char* filename, char edges[][VERTEX_NUM]) {
    FILE *fp = fopen(filename, "r");
    
    if(fp == NULL)
        return false;

    for(int i = 0; i < VERTEX_NUM; i++)
        for(int j = 0; j < VERTEX_NUM; j++)
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
    for(int i = 0; i < VERTEX_NUM; i++) {
        is_colored = 0;
        for(int j = 0; j < VERTEX_NUM; j++) {
            is_colored += colors[j][i];
        }

        if(is_colored > 1) {
            printf("The vertex %d has more than one color, exitting\n", i);
            return false;
        }
    }


    int color1, color2;
    for(int i = 0; i < VERTEX_NUM; i++) {
        for(int j = 0; j < VERTEX_NUM; j++) {
            if(edges[i][j]) {
                color1 = 0;
                color2 = 0;

                for(int k = 0; k < VERTEX_NUM; k++) {
                    if(colors[k][j] & colors[k][i]){
                        printf("The verteces %d and %d are connected and have the same color %d, exitting\n", i, j, k);
                        return 0;
                    }
                }
            }
        }
    }
}

int main() {
    /**
     * This 2D array stores the edges of every vertex in the graph.
     * The numbers of rows and columns present the numbers of vertices,
     * and the place where they cross is an edges, if it is assigned to
     * 1, then the edge exists, otherwise, it doesn't.
     */
    static char edges[VERTEX_NUM][VERTEX_NUM];

    if(!init_graph("graph_datasets/C4000.5.col", edges)) {
        printf("Could not initialize graph, exitting ...\n");
        return 0;
    }


    /**
     * This 2D array stores the color of every vertex.
     * The row numbers present the colors, and the column numbers 
     * present the vertex numbers. When a cell is assigned to 1,
     * it means that the the vertex has the color of that row.
     */
    static char colors[VERTEX_NUM][VERTEX_NUM];

    for(int i = 0; i < VERTEX_NUM; i++)
        for(int j = 0; j < VERTEX_NUM; j++)
            colors[i][j] = 0;


    // Create a random queue of vertices to propagate.
    srand(time(0));
    int prob_queue[VERTEX_NUM];
    int current;
    int i = 0;
    while (i < VERTEX_NUM) {
        current = rand()%VERTEX_NUM;
        if(!exists(prob_queue, i, current)) {
            prob_queue[i] = current;
            i++;
        }
        // prob_queue[i] = i;
        // i++;
    }


    clock_t t; 
    t = clock(); 
    
    // Go through the queue and color each vertex.
    unsigned char adjacent_colors[VERTEX_NUM];
    int current_vert;
    int max_color = 0;
    for(int i = 0; i < VERTEX_NUM; i++) {
        // Initialize the temporary data.
        for(int j = 0; j < VERTEX_NUM; j++)
            adjacent_colors[j] = 0;

        current_vert = prob_queue[i];


        // Go through each edge of the vertex and list the used colors.
        for(int j = 0; j < VERTEX_NUM; j++) {
            // This is a neighbor
            if(edges[current_vert][j] == 1) {
                // Check its color.
                for(int k = 0; k < VERTEX_NUM; k++) {
                    if(colors[k][j]) 
                        adjacent_colors[k] = 1;
                }
            }
        }

        // Find the least unused color and assign this vertex to it.
        for(int j = 0; j < VERTEX_NUM; j++) {
            if(!adjacent_colors[j]) {
                colors[j][current_vert] = 1;
                if(max_color < j) max_color = j;
                break;
            }
        }
    }

    t = clock() - t; 
    double time_taken = ((double)t)/CLOCKS_PER_SEC; // in seconds 
 
    printf("dsatur took %f seconds to execute \n", time_taken); 

    for(int i = 0; i < max_color; i++) {
        printf("color %d: ", i);
        for(int j = 0; j < VERTEX_NUM; j++) 
            if(colors[i][j]) printf("%d, ", j);
        printf("\n");
    }

    is_valid(VERTEX_NUM, edges, colors);

    return 0;
}

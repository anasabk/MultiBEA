#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>


bool exists(int* arr, int len, int target) {
    for(int i = 0; i < len; i++)
        if(target == arr[i]) return true;

    return false;
}

int main() {
    /**
     * This 2D array stores the edges of every vertex in the graph.
     * The numbers of rows and columns present the numbers of vertices,
     * and the place where they cross is an edges, if it is assigned to
     * 1, then the edge exists, otherwise, it doesn't.
     */
    int edges[10][10] = {
        {0, 1, 0, 1, 0, 1, 0, 0, 0, 0},
        {1, 0, 0, 1, 0, 0, 0, 0, 1, 0},
        {0, 0, 0, 0, 0, 1, 0, 1, 1, 0},
        {1, 1, 0, 0, 1, 0, 1, 0, 0, 0},
        {0, 0, 0, 1, 0, 0, 1, 0, 0, 1},
        {1, 0, 1, 0, 0, 0, 1, 0, 0, 0},
        {0, 0, 0, 1, 1, 1, 0, 0, 0, 0},
        {0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
        {0, 1, 1, 0, 0, 0, 0, 0, 0, 1},
        {0, 0, 0, 0, 1, 0, 0, 0, 1, 0}
    };

    /**
     * This 2D array stores the color of every vertex.
     * The row numbers present the colors, and the column numbers 
     * present the vertex numbers. When a cell is assigned to 1,
     * it means that the the vertex has the color of that row.
     */
    int colors[10][10] = {
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
    };


    // Create a random queue of vertices to propagate.
    srand(time(0));
    int prob_queue[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int current;
    int i = 0;
    while (i < 10) {
        current = rand()%10;
        if(!exists(prob_queue, i, current)) {
            prob_queue[i] = current;
            i++;
        }
    }

    // Go through the queue and color each vertex.
    int adjacent_colors[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    bool color_assigned;
    int current_node;
    for(int i = 0; i < 10; i++) {
        // Initialize the temporary data.
        color_assigned = false;
        for(int i = 0; i < 10; i++)
            adjacent_colors[i] = 0;

        current_node = prob_queue[i];

        // Go through each edge of the vertex and list the uased colors.
        for(int j = 0; j < 10; j++) {
            // This is a neighbor
            if(edges[current_node][j]) {
                // Check its color.
                for(int k = 0; k < 10; k++) {
                    if(colors[k][j]) adjacent_colors[k] = 1;
                }
            }
        }

        // Find the least unused color and assign this vertex to it.
        for(int j = 0; j < 10; j++) {
            if(!adjacent_colors[j]) {
                colors[j][current_node] = 1;
                break;
            }
        }
    }

    for(int i = 0; i < 10; i++) {
        printf("color %d: ", i);
        for(int j = 0; j < 10; j++) 
            if(colors[i][j]) printf("%d, ", j);
        printf("\n");
    }

    return 0;
}

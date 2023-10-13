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
    // 10 nodes with 10 possible edges.
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

    // 10 colors with 10 possible nodes with each.
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


    srand(time(0));

    // Create a random queue of nodes to propagate.
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

    int adjacent_colors[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    bool color_assigned;
    int current_node;
    for(int i = 0; i < 10; i++) {
        color_assigned = false;
        for(int i = 0; i < 10; i++)
            adjacent_colors[i] = 0;

        current_node = prob_queue[i];

        /**
         * Find the unusable colors
         */
        for(int j = 0; j < 10; j++) {
            // This is a neighbor
            if(edges[current_node][j]) {
                for(int k = 0; k < 10; k++) {
                    if(colors[k][j]) adjacent_colors[k] = 1;
                }
            }
        }

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

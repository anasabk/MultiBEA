#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>

#include "stdgraph.h"
#include "common.h"


void graph_color_random (
    int graph_size, 
    const block_t edges[][TOTAL_BLOCK_NUM(graph_size)],  
    block_t colors[][TOTAL_BLOCK_NUM(graph_size)], 
    int max_color
) {
    int index;
    for(int i = 0; i < graph_size; i++) {
        index = rand()%max_color;
        SET_COLOR(colors[index], i);
    }
}


bool read_graph (
    const char* filename, 
    int graph_size, 
    block_t edges[][TOTAL_BLOCK_NUM(graph_size)], 
    int offset_i
) {
    FILE *fp = fopen(filename, "r");
    
    if(fp == NULL)
        return false;

    memset(edges, 0, graph_size*TOTAL_BLOCK_NUM(graph_size)*sizeof(block_t));

    char buffer[64];
    char *token, *saveptr;
    int row, column;
    while(fgets(buffer, 64, fp) != NULL) {
        buffer[strcspn(buffer, "\n")] = 0;

        token = strtok_r (buffer, " ", &saveptr);
        if(saveptr[0] == 0) 
            break;
        row = atoi(token) + offset_i;
        token = strtok_r (NULL, " ", &saveptr);
        column = atoi(token) + offset_i;

        SET_EDGE(row, column, edges);
    }
    
    fclose(fp);
    return true;
}


bool read_weights(const char* filename, int graph_size, int weights[]) {
    FILE *fp = fopen(filename, "r");
    
    if(fp == NULL)
        return false;

    memset(weights, 0, graph_size * sizeof(int));

    char buffer[64];
    int vertex = 0;
    while(fgets(buffer, 64, fp) != NULL && vertex < graph_size) {
        buffer[strcspn(buffer, "\n")] = 0;
        weights[vertex] = atoi(buffer);
        vertex++;
    }
    
    fclose(fp);
    return true;
}


bool is_valid(
    int graph_size, 
    const block_t edges[][TOTAL_BLOCK_NUM(graph_size)], 
    int color_num, 
    const block_t colors[][TOTAL_BLOCK_NUM(graph_size)]
) {
    // Iterate through vertices.
    int i, j, k, i_block;
    block_t i_mask;
    bool vertex_is_colored, error_flag = false, over_colored;
    for(i = 0; i < graph_size; i++) {
        vertex_is_colored = false;
        over_colored = false;
        i_block = BLOCK_INDEX(i);
        i_mask = MASK(i);

        // Iterate through colors and look for the vertex.
        for(j = 0; j < color_num; j++){
            if((colors[j][i_block] & i_mask)) {
                if(!vertex_is_colored) {
                    vertex_is_colored = true;

                } else {
                    over_colored = true;
                }

                for(k = i + 1; k < graph_size; k++) { // Through every vertex after i in color j.
                    if(CHECK_COLOR(colors[j], k) && (edges[k][i_block] & i_mask)) {
                        // The two vertices have the same color.
                        printf("The vertices %d and %d are connected and have the same color %d.\n", i, k, j);
                        error_flag = true;
                    }
                }
            }
        }

        // Check if the vertex had more then one color.
        if(!vertex_is_colored) {
            printf("The vertex %d has no color.\n", i);
            error_flag = true;
        }

        // Check if the vertex had more then one color.
        if(over_colored) {
            printf("The vertex %d has more than one color.\n", i);
            error_flag = true;
        }
    }

    return !error_flag;
}


int count_edges(int graph_size, const block_t edges[][TOTAL_BLOCK_NUM(graph_size)], int degrees[]) {
    memset(degrees, 0, graph_size*sizeof(int));

    int i, j, total = 0;
    for(i = 0; i < graph_size; i++) {
        for(j = 0; j < TOTAL_BLOCK_NUM(graph_size); j++)
            degrees[i] += __builtin_popcountl(edges[i][j]);
        total += degrees[i];
    }

    return total;
}


void print_colors(
    const char *filename, 
    const char *header, 
    int color_num, 
    int graph_size, 
    const block_t colors[][TOTAL_BLOCK_NUM(graph_size)]
) {
    FILE* fresults;
    fresults = fopen(filename, "w");

    if(!fresults) {
        printf("%s\ncould not print results, aborting ...\n", strerror(errno));
        return;
    }

    fprintf(fresults, "%s\n\n", header);

    for(int i = 0; i < color_num; i++)
        for(int j = 0; j < graph_size; j++)
            if(CHECK_COLOR(colors[i], j)) 
                fprintf(fresults, "%d %d\n", i, j);

    fclose(fresults);
}


bool exists(int* arr, int len, int target) {
    for(int i = 0; i < len; i++)
        if(target == arr[i]) return true;

    return false;
}


int graph_color_greedy(
    int graph_size, 
    const block_t edges[][TOTAL_BLOCK_NUM(graph_size)], 
    block_t colors[][TOTAL_BLOCK_NUM(graph_size)], 
    int max_color_possible
) {
    // Go through the queue and color each vertex.
    int prob_queue[graph_size];
    block_t adjacent_colors[TOTAL_BLOCK_NUM(max_color_possible)];
    int max_color = 0, current_vert;
    int i, j, k;
    for(i = 0; i < graph_size; i++) {
        // Get a new random vertex.
        do { prob_queue[i] = rand()%graph_size; } while(exists(prob_queue, i, prob_queue[i]));
        current_vert = prob_queue[i];

        // Initialize the temporary data.
        memset(adjacent_colors, 0, (TOTAL_BLOCK_NUM(max_color_possible))*sizeof(block_t));
        for(j = 0; j < TOTAL_BLOCK_NUM(graph_size); j++)
            for(k = 0; k < max_color_possible; k++)
                if((edges[current_vert][j] & colors[k][j]))
                    SET_COLOR(adjacent_colors, k);

        // Find the first unused color (starting from 0) and assign this vertex to it.
        for(j = 0; j < max_color_possible; j++) {
            if(!CHECK_COLOR(adjacent_colors, j)) {
                SET_COLOR(colors[j], current_vert);
                if(max_color < j) 
                    max_color = j;
                break;
            }
        }
    }

    return max_color + 1;
}


int count_conflicts(
    int graph_size, 
    const block_t color[], 
    const block_t edges[][TOTAL_BLOCK_NUM(graph_size)], 
    int conflict_count[]
) {
    int i, j, total_conflicts = 0;
    for(i = 0; i < graph_size; i++) {
        if(CHECK_COLOR(color, i)) {
            conflict_count[i] = 0;
            for(j = 0; j < TOTAL_BLOCK_NUM(graph_size); j++)
                conflict_count[i] += __builtin_popcountl(color[j] & edges[i][j]);
            total_conflicts += conflict_count[i];
        }
    }

    return total_conflicts/2;
}

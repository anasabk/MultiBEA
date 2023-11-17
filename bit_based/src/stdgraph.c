#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <inttypes.h>

#include "stdgraph.h"
#include "common.h"


bool read_graph(const char* filename, int graph_size, uint32_t edges[]) {
    FILE *fp = fopen(filename, "r");
    
    if(fp == NULL)
        return false;

    memset(edges, 0, (BLOCK_INDEX(graph_size*(graph_size-1)/2))*sizeof(uint32_t));

    char buffer[64];
    char *token, *saveptr;
    int row, column, edge_index = 0;
    while(!feof(fp)) {
        fgets(buffer, 64, fp);
        buffer[strcspn(buffer, "\n")] = 0;

        token = strtok_r (buffer, " ", &saveptr);
        if(saveptr[0] == 0) 
            break;
        row = atoi(token);
        token = strtok_r (NULL, " ", &saveptr);
        column = atoi(token);

        if(row < column)
            edge_index = EDGE_BIT_INDEX(row, column);
        else
            edge_index = EDGE_BIT_INDEX(column, row);

        SET_EDGE(edge_index, edges);
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
    while(!feof(fp) && vertex < graph_size) {
        fgets(buffer, 64, fp);
        buffer[strcspn(buffer, "\n")] = 0;
        weights[vertex] = atoi(buffer);
        vertex++;
    }
    
    fclose(fp);
    return true;
}

bool is_valid(
    int graph_size, 
    const uint32_t edges[], 
    int color_num, 
    const uint32_t colors[][BLOCK_INDEX(graph_size-1)+1]
) {
    // Iterate through vertices.
    int i, j, k;
    bool vertex_is_colored, error_flag = false;
    int vertex_block, vertex_mask;
    for(i = 0; i < graph_size; i++) {
        vertex_is_colored = false;
        vertex_block = BLOCK_INDEX(i);
        vertex_mask = MASK(i);

        // Iterate through colors and look for the vertex.
        for(j = 0; j < color_num; j++){
            if((colors[j][vertex_block] & vertex_mask)) {
                if(!vertex_is_colored) {
                    vertex_is_colored = true;

                } else {
                    error_flag = true;
                    break;
                }

                for(k = i + 1; k < graph_size; k++) { // Through every vertex after k in color j.
                    if((colors[j][BLOCK_INDEX(k)] & MASK(k)) && CHECK_EDGE(EDGE_BIT_INDEX(i, k), edges)) {
                        // The two vertices have the same color.
                        printf("The verteces %d and %d are connected and have the same color %d.\n", i, k, j);
                        return 0;
                    }
                }
            }
        }

        // Check if the vertex had more then one color.
        if(error_flag) {
            printf("The vertex %d has more than one color.\n", i);
            return false;
        }
    }

    return true;
}

void count_edges(int graph_size, const uint32_t edges[], int edge_count[]) {
    memset(edge_count, 0, graph_size*sizeof(int));

    int i, j, edge_list_start;
    for(i = 0; i < graph_size; i++) {
        edge_list_start = EDGE_BIT_INDEX(0, i);
        for(j = edge_list_start; j < (edge_list_start + i); j++) {
            if(CHECK_EDGE(j, edges)) {
                edge_count[i]++;
                edge_count[j - edge_list_start]++;
            }
        }
    }
}

void print_colors(
    const char *filename, 
    const char *header, 
    int color_num, 
    int graph_size, 
    const uint32_t colors[][BLOCK_INDEX(graph_size-1)+1]
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

int graph_color_greedy(
    int graph_size, 
    const uint32_t edges[], 
    uint32_t colors[][BLOCK_INDEX(graph_size-1)+1], 
    int max_color_possible
) {
    // clock_t start = clock();
    // Go through the queue and color each vertex.
    int prob_queue[graph_size];
    uint32_t adjacent_colors[BLOCK_INDEX(max_color_possible-1)+1];
    int max_color = 0, current_vert, temp_block, temp_mask;
    int i, j, k;
    for(i = 0; i < graph_size; i++) {
        // Get a new random vertex.
        do { prob_queue[i] = rand()%graph_size; } while(exists(prob_queue, i, prob_queue[i]));
        current_vert = prob_queue[i];

        // Initialize the temporary data.
        memset(adjacent_colors, 0, (BLOCK_INDEX(max_color_possible-1)+1)*sizeof(uint32_t));

        // Go through each edge of the vertex and save the used colors in adjacent_colors array.
        for(j = 0; j < graph_size; j++) {
            // This is a neighbor
            if(current_vert<j ? CHECK_EDGE(EDGE_BIT_INDEX(current_vert, j), edges) : CHECK_EDGE(EDGE_BIT_INDEX(j, current_vert), edges)) {
                temp_block = BLOCK_INDEX(j);
                temp_mask  = MASK(j);

                // Check its color.
                for(k = 0; k < max_color_possible; k++) {
                    if((colors[k][temp_block] & temp_mask)) {
                        SET_COLOR(adjacent_colors, k);
                        break;
                    }
                }
            }
        }

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

    // graph_color_greedy_time += ((double)(clock() - start))/CLOCKS_PER_SEC;
    return max_color + 1;
}

int count_conflicts(
    int graph_size, 
    const uint32_t color[], 
    const uint32_t edges[], 
    int conflict_count[]
) {
    // clock_t start = clock();
    int i, j, total_conflicts = 0;
    for(i = 0; i < graph_size; i++) {   // i = index of the vertex to search for conflicts.
        if(CHECK_COLOR(color, i)) { // Check if the vertex i has this color i.
            for(j = i + 1; j < graph_size; j++) {  // j = index of a vertex to check if it has an edge with i.
                // Check if the vertex k has the color i and has an edge with the vertex i.
                if (CHECK_COLOR(color, j) && CHECK_EDGE(EDGE_BIT_INDEX(i, j), edges)) { 
                    conflict_count[i]++;
                    conflict_count[j]++;
                    total_conflicts++;
                }
            }
        }
    }

    // count_conflicts_time += ((double)(clock() - start))/CLOCKS_PER_SEC;
    return total_conflicts;
}

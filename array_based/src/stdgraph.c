#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include "stdgraph.h"
#include "common.h"


double is_valid_time = 0;
double count_edges_time = 0;
double graph_color_greedy_time = 0;
double count_conflicts_time = 0;


bool read_graph(const char* filename, int size, char edges[][size]) {
    FILE *fp = fopen(filename, "r");
    
    if(fp == NULL)
        return false;

    memset(edges, 0, size*size);

    char buffer[64];
    char *token, *saveptr;
    int row, column;
    while(!feof(fp)) {
        fgets(buffer, 64, fp);
        buffer[strcspn(buffer, "\n")] = 0;
        token = strtok_r (buffer, " ", &saveptr);
        
        if(saveptr[0] == 0)
            break;

        row = atoi(token);
        token = strtok_r (NULL, " ", &saveptr);
        column = atoi(token);

        edges[row][column] = 1;
        edges[column][row] = 1;
    }
    
    fclose(fp);
    return true;
}

bool read_weights(const char* filename, int size, int weights[]) {
    FILE *fp = fopen(filename, "r");
    
    if(fp == NULL)
        return false;

    memset(weights, 0, size);

    char buffer[64];
    int vertex = 0;
    while(!feof(fp) && vertex < size) {
        fgets(buffer, 64, fp);
        buffer[strcspn(buffer, "\n")] = 0;
        weights[vertex] = atoi(buffer);
        vertex++;
    }
    
    fclose(fp);
    return true;
}

bool is_valid(int size, const char edges[][size], int color_num, const char colors[][size]) {
    // clock_t start = clock();
    // Iterate through vertices.
    int i, j, k;
    bool vertex_is_colored, error_flag = false;
    for(i = 0; i < size; i++) {
        vertex_is_colored = false;
        // Iterate through colors and look for the vertex.
        for(j = 0; j < color_num; j++){
            if(colors[j][i]) {
                if(!vertex_is_colored) {
                    vertex_is_colored = true;

                } else {
                    error_flag = true;
                    break;
                }

                for(k = i + 1; k < size; k++) { // Through every vertex after j in color i.
                    if(colors[j][k] && edges[i][k]) {
                        // The two vertices have the same color.
                        printf("The verteces %d and %d are connected and have the same color %d.\n", i, j, k);
                        return 0;
                    }
                }
            }
        }

        // Check if the vertex had more then one color.
        if(!vertex_is_colored) {
            printf("The vertex %d has no color.\n", i);
            return false;
        }

        // Check if the vertex had more then one color.
        if(error_flag) {
            printf("The vertex %d has more than one color.\n", i);
            return false;
        }
    }

    // is_valid_time += ((double)(clock() - start))/CLOCKS_PER_SEC;
    return true;
}

int count_edges(int size, const char edges[][size], int count[]) {
    // clock_t start = clock();
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

    // count_edges_time += ((double)(clock() - start))/CLOCKS_PER_SEC;
    return max_count;
}

void print_colors(const char *filename, const char *header, int max_color_num, int size, const char colors[][size]) {
    FILE* fresults;
    fresults = fopen(filename, "w");

    if(!fresults) {
        printf("%s\ncould not print results, aborting ...\n", strerror(errno));
        return;
    }

    fprintf(fresults, "%s\n\n", header);

    for(int j = 0; j < max_color_num; j++)
        for(int k = 0; k < size; k++)
            if(colors[j][k]) 
                fprintf(fresults, "%d %d\n", j, k);

    fclose(fresults);
}

int graph_color_greedy(int size, const char edges[][size], char colors[][size], int max_color_possible) {
    // clock_t start = clock();
    // Go through the queue and color each vertex.
    int prob_queue[size];
    char adjacent_colors[max_color_possible];
    int max_color = 0;
    int current_vert;
    int i, j, k;
    for(i = 0; i < size; i++) {
        // Get a new random vertex.
        do { prob_queue[i] = rand()%size; } while(exists(prob_queue, i, prob_queue[i]));
        current_vert = prob_queue[i];
    
        // Initialize the temporary data.
        memset(adjacent_colors, 0, max_color_possible);

        // Go through each edge of the vertex and save the used colors in adjacent_colors array.
        for(j = 0; j < size; j++) {
            // This is a neighbor
            if(edges[current_vert][j]) {
                // Check its color.
                for(k = 0; k < max_color_possible; k++) {
                    if(colors[k][j]) {
                        adjacent_colors[k] = 1;
                        break;
                    }
                }
            }
        }

        // Find the first unused color (starting from 0) and assign this vertex to it.
        for(j = 0; j < max_color_possible; j++) {
            if(!adjacent_colors[j]) {
                colors[j][current_vert] = 1;
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
    int size, 
    const char color[], 
    const char edges[][size], 
    int conflict_count[]
) {
    // clock_t start = clock();
    int i, j, total_conflicts = 0;
    for(i = 0; i < size; i++) {   // i = index of the vertex to search for conflicts.
        if(color[i]) { // Check if the vertex i has this color i.
            for(j = i + 1; j < size; j++) {  // k = index of a vertex to check if it has an edge with i.
                if(color[j] && edges[i][j]) { // Check if the vertex k has the color i and has an edge with the vertex i.
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

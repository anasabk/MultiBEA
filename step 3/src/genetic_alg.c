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
#include "genetic_alg.h"
#include "crossover.h"


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
    char* token1, *token2, *saveptr;
    int row, column;
    while(!feof(fp)) {
        fgets(buffer, 64, fp);
        buffer[strcspn(buffer, "\n")] = 0;
        
        token1 = strtok_r (buffer, " ", &saveptr);
        row = atoi(token1) - 1;
        
        token2 = strtok_r (NULL, " ", &saveptr);
        column = atoi(token2) - 1;
        
        edges[row][column] = 1;
        edges[column][row] = 1;
    }
    
    fclose(fp);
    return true;
}

bool is_valid(int size, const char edges[size][size], int color_num, const char colors[color_num][size]) {
    // Iterate through vertices.
    int i, j, k, is_colored;
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

void print_colors(char *filename, char *header, int max_color_num, int size, char colors[][size]) {
    FILE* fresults;
    fresults = fopen(filename, "w");

    fprintf(fresults, "%s\n\n", header);

    for(int j = 0; j < max_color_num; j++)
        for(int k = 0; k < size; k++)
            if(colors[j][k]) 
                fprintf(fresults, "%d %d\n", j, k + 1);

    fclose(fresults);
}

int greedy_color(int size, const char edges[][size], char colors[][size], int max_color_possible) {
    // Create a random queue of vertices to propagate.
    int prob_queue[size];
    int i = 0;
    while (i < size) {
        prob_queue[i] = rand()%size;
        if(!exists(prob_queue, i, prob_queue[i]))
            i++;
    }
    
    // Go through the queue and color each vertex.
    char adjacent_colors[max_color_possible];
    int max_color = 0;
    int current_vert;
    int j, k;
    for(i = 0; i < size; i++) {
        // Initialize the temporary data.
        current_vert = prob_queue[i];
        memset(adjacent_colors, 0, max_color_possible);

        // Go through each edge of the vertex and save the used colors in adjacent_colors array.
        for(j = 0; j < size; j++) {
            // This is a neighbor
            if(edges[current_vert][j]) {
                // Check its color.
                for(k = 0; k < max_color_possible; k++) {
                    adjacent_colors[k] |= colors[k][j];
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

    return max_color + 1;
}

void random_color(int size, const char edges[][size], char colors[][size], int color_target) {
    for(int i = 0; i < size; i++)
        colors[rand()%color_target][i] = 1;
}

int genetic_color(int size, char edges[][size], int edge_count[size], int max_edge_count, char result_colors[][size], int color_target) {
    int best = 0, best_child = 0, best_base = 0;
    // for (i = 0; i < 10; i++) {
    //     color_count[i] = greedy_color(size, edges, colors[i], max_edge_count);

    //     if(color_count[best] > color_count[i])
    //         best = i;

    //     if(color_count[best_base] > color_count[i])
    //         best_base = i;

    //     // is_valid(size, edges, color_count[i], colors[i]);
    // }

    int fitness[100];
    int color_count[100];
    char colors[100][max_edge_count][size];
    // int color_target = greedy_color(size, edges, colors[0], max_edge_count);

    memset(colors, 0, 100*max_edge_count*size);
    memset(fitness, 0, 100*sizeof(int));

    int i, j, k, h;
    for (i = 0; i < 100; i++) {
        color_count[i] = color_target;
        random_color(size, edges, colors[i], color_target);

        for(j = 0; j < color_target; j++)
            for(k = 0; k < size; k++) 
                if(colors[i][j][k])
                    for(h = k + 1; h < size; h++)
                        if(colors[i][j][h] && edges[h][k])
                            fitness[i] += 1;

        // is_valid(size, edges, color_count[i], colors[i]);
    }

    char child[max_edge_count][size];
    int parent1, parent2, child_colors, temp_fitness;
    for(i = 0; i < 1000; i++) {
        memset(child, 0, size*max_edge_count);

        parent1 = rand()%100;
        do { parent2 = rand()%100; } while (parent1 == parent2);

        temp_fitness = crossover(
            size, 
            edges, 
            edge_count,
            color_count[parent1], 
            color_count[parent2], 
            colors[parent1], 
            colors[parent2], 
            child, 
            color_target,
            &child_colors
        );

        if(temp_fitness == 0) {
            memcpy(result_colors, child, size*max_edge_count);
            return child_colors;

        } else if (child_colors <= color_count[parent1] && temp_fitness <= fitness[parent1]) {
            memcpy(colors[parent1], child, size*max_edge_count);
            color_count[parent1] = child_colors;
            fitness[parent1] = temp_fitness;

            if(color_count[best] > child_colors)
                best = parent1;

            if(color_count[best_child] > child_colors)
                best_child = parent1;

        } else if (child_colors == color_count[parent2] && temp_fitness <= fitness[parent2]) {
            memcpy(colors[parent2], child, size*max_edge_count);
            color_count[parent2] = child_colors;
            fitness[parent2] = temp_fitness;

            if(color_count[best] > child_colors)
                best = parent2;

            if(color_count[best_child] > child_colors)
                best_child = parent2;
        }
    }

    memcpy(result_colors, colors[best], size*max_edge_count);

    is_valid(size, edges, color_count[best], colors[best]);
    return color_count[best];
}

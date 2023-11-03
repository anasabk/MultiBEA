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
#include "stdgraph.h"
#include "common.h"


void graph_color_random(int size, const char edges[][size], char colors[][size], int max_color) {
    for(int i = 0; i < size; i++)
        colors[rand()%max_color][i] = 1;
}

int graph_color_genetic(
    int size, 
    char edges[][size], 
    int weights[],
    int base_color_count,
    int max_gen_num,
    char best_solution[][size],
    int *best_fitness,
    float *best_solution_time
) {
    merge_colors_time = 0;
    rm_vertex_time = 0;
    local_search_time = 0;
    crossover_time = 0;

    clock_t start_time = clock();
    float last_solution_time = 0;

    // Count the degrees of each vertex
    int edge_count_list[size];
    count_edges(size, edges, edge_count_list);

    char colors[100][base_color_count][size];
    int color_count[100];
    int fitness[100];

    // Initialize The arrays.
    memset(colors, 0, 100*base_color_count*size);
    memset(fitness, 0, 100*sizeof(int));

    /**
     * Generate a random population, where each individual has
     * the base number of colors.
     */
    int i;
    // int temp_conflict_count[size];
    for (i = 0; i < 100; i++) {
        color_count[i] = base_color_count;
        graph_color_random(size, edges, colors[i], base_color_count);   
        fitness[i] = __INT_MAX__;
    }

    // Keep generating solutions for max_gen_num generations.
    char child[base_color_count][size];
    int target_color_count = base_color_count;
    int parent1, parent2, dead_parent, child_colors, temp_fitness, best = 0;
    for(i = 0; i < max_gen_num; i++) {
        LOGD("\nDoing Crossover %d: target color count %d\n", i, target_color_count);

        // Initialize the child
        memset(child, 0, size*base_color_count);

        // Pick 2 random parents
        dead_parent = -1;
        parent1 = rand()%100;
        do { parent2 = rand()%100; } while (parent1 == parent2);

        LOGD("    Chosen Parents:\n        individual %2d fitness %2d:", parent1, fitness[parent1]);
        for(int j = 0; j < color_count[parent1]; j++) {
            LOGDSTR(" | ");
            for(int k = 0; k < size; k++)
                if(colors[parent1][j][k])
                    LOGD("%2d ", k);
        }

        LOGD("\n        individual %2d fitness %2d:", parent2, fitness[parent2]);
        for(int j = 0; j < color_count[parent2]; j++) {
            LOGDSTR(" | ");
            for(int k = 0; k < size; k++)
                if(colors[parent2][j][k])
                    LOGD("%2d ", k);
        }

        // Do a crossover
        temp_fitness = crossover(
            size, 
            edges, 
            weights,
            edge_count_list,
            color_count[parent1], 
            color_count[parent2], 
            colors[parent1], 
            colors[parent2], 
            target_color_count,
            child, 
            &child_colors
        );

        LOGD("\n        child with fitness %2d:   ", temp_fitness);
        for(int j = 0; j < target_color_count; j++) {
            LOGDSTR(" | ");
            for(int k = 0; k < size; k++)
                if(child[j][k])
                    LOGD("%2d ", k);
        }
        LOGDSTR("\n");

        // Check if the child is better than any of the parents.
        if (temp_fitness <= fitness[parent1] && child_colors <= color_count[parent1])
            dead_parent = parent1;
        else if (temp_fitness <= fitness[parent2] && child_colors <= color_count[parent2])
            dead_parent = parent2;
        else if (rand()%100 < 15)
            dead_parent = parent1 == best ? parent2 : parent1;

        // Replace a dead parent.
        if(dead_parent > -1) {
            LOGD("Child replaced the individual %d.\n", dead_parent);
            memmove(colors[dead_parent], child, size*base_color_count);
            color_count[dead_parent] = child_colors;
            fitness[dead_parent] = temp_fitness;

            if(fitness[best] >= temp_fitness) {
                best = dead_parent;
                last_solution_time = ((double)(clock() - start_time))/CLOCKS_PER_SEC;
            }
        }

        // Make the target harder if it was found.
        if(temp_fitness == 0) {
            // is_valid(size, edges, child_colors, child);
            target_color_count = child_colors - 1;

            if(target_color_count == 0)
                break;
        }
    }

    if(last_solution_time == -1)
        last_solution_time = ((double)(clock() - start_time))/CLOCKS_PER_SEC;

    *best_fitness = fitness[best];
    *best_solution_time = last_solution_time;
    memcpy(best_solution, colors[best], size*base_color_count);
    return color_count[best];
}

int get_rand_color(int size, int colors_used, char used_color_list[]) {
    if(colors_used >= size) {
        return -1;

    } else if(colors_used > size - 2) {
        for(int i = 0; i < size; i++) {
            if(!used_color_list[i]) {
                used_color_list[i] = 1;
                return i;
            }
        }
    }

    int temp;
    while(1) {
        temp = rand()%size;
        if(!used_color_list[temp]) {
            used_color_list[temp] = 1;
            return temp;
        }
    }
}

int merge_colors(
    int size,
    const char *parent_color[2],
    char child_color[],
    char pool[],
    int *pool_total,
    char used_vertex_list[]
) {
    clock_t start = clock();

    int total_used = 0;
    for(int i = 0; i < size; i++) {  // for every vertex
        if(!used_vertex_list[i]) { // if vertex is unused
            if(parent_color[0] != NULL)
                child_color[i] |= parent_color[0][i];

            if(parent_color[1] != NULL)
                child_color[i] |= parent_color[1][i];

            used_vertex_list[i] |= child_color[i];
            total_used += child_color[i];

        } else if(pool[i]) {   // if vertex is in the pool
            child_color[i] |= pool[i];
            *pool_total -= pool[i];
            pool[i] = 0;
        }
    }

    merge_colors_time += ((double)(clock() - start))/CLOCKS_PER_SEC;
    return total_used;
}

void rm_vertex(
    int vertex,
    int size, 
    const char edges[][size], 
    const int weights[],
    char color[], 
    char pool[], 
    int competition[],
    int conflict_count[],
    int *total_conflicts,
    int *pool_total
) {
    clock_t start = clock();
    for(int i = 0; i < size; i++) {
        if(color[i] && edges[vertex][i]) {
            conflict_count[i]--;
            competition[i] -= weights[vertex];
        }
    }

    color[vertex] = 0;
    pool[vertex] = 1;
    (*pool_total)++;

    competition[vertex] = 0;
    (*total_conflicts) -= conflict_count[vertex];
    conflict_count[vertex] = 0;
    rm_vertex_time += ((double)(clock() - start))/CLOCKS_PER_SEC;
}

void local_search(
    int size,
    const char edges[][size],
    const int weights[],
    const int num_of_edges[],
    char color[],
    char pool[],
    int *pool_total
) {
    clock_t start = clock();

    int conflict_count[size];
    memset(conflict_count, 0, size*sizeof(int));
    
    int competition[size];
    memset(competition, 0, size*sizeof(int));

    // Count the conflicts..
    int total_conflicts = count_conflicts(
        size,
        color,
        edges,
        weights,
        competition,
        conflict_count
    );
    LOGD("\n        Local Search to fix conflicts: %d conflicts are present", total_conflicts);
    LOGDSTR("\n            conflicts of each vertex:   |");
    for(int j = 0; j < size; j++) {
        LOGD(" %2d |", conflict_count[j]);
    }
    LOGDSTR("\n            competition of each vertex: |");
    for(int j = 0; j < size; j++) {
        LOGD(" %2d |", competition[j]);
    }
    LOGDSTR("\n            weight of each vertex:      |");
    for(int j = 0; j < size; j++) {
        LOGD(" %2d |", weights[j]);
    }

    // Keep removing problematic vertices until all conflicts are gone.
    int i, worst_vert = 0;
    while(total_conflicts > 0) {
        LOGDSTR("\n");
        // Find the vertex with the most conflicts.
        for(i = 0; i < size; i++) {
            if (conflict_count[worst_vert] < conflict_count[i] ||
                (conflict_count[worst_vert] == conflict_count[i] &&
                competition[worst_vert] <= competition[i]))
                worst_vert = i;
        }
        LOGD("\n            vertex with the most conflicts and competition: %d", worst_vert);

        if(weights[worst_vert] <= competition[worst_vert]) { 
            LOGD("\n            removing the vertex %d", worst_vert);
            rm_vertex(
                worst_vert,
                size,
                edges,
                weights,
                color,
                pool,
                competition,
                conflict_count,
                &total_conflicts,
                pool_total
            );

        } else {
            // Update the conflicts of the vertices connected to the removed vertex.
            LOGD("\n            removing all conflicts of vertex %d", worst_vert);
            for(i = 0; i < size; i++){
                if(edges[worst_vert][i] && color[i]) {
                    rm_vertex(
                        i,
                        size,
                        edges,
                        weights,
                        color,
                        pool,
                        competition,
                        conflict_count,
                        &total_conflicts,
                        pool_total
                    );
                }
            }
        }
    }
    local_search_time += ((double)(clock() - start))/CLOCKS_PER_SEC;
    LOGDSTR("\n");
}

int crossover(
    int size, 
    const char edges[][size], 
    const int weights[],
    const int num_of_edges[],
    int color_num1, 
    int color_num2, 
    const char parent1[][size], 
    const char parent2[][size], 
    int target_color_count,
    char child[][size],
    int *child_color_count
) {
    clock_t start = clock();

    // max number of colors of the two parents.
    int max_color_num = color_num1 > color_num2 ? color_num1 : color_num2;

    // The max number of iterations over colors in the main loop.
    int max_iter_num = max_color_num > target_color_count ? max_color_num : target_color_count;

    // list of used colors in the parents.
    char used_color_list[2][max_color_num];
    memset(used_color_list, 0, 2*max_color_num);

    // info of vertices taken out of the parents.
    int used_vertex_count = 0;
    char used_vertex_list[size];
    memset(used_vertex_list, 0, size);

    // info of pool.
    int pool_count = 0;
    int pool_age[size];
    char pool[size];
    for(int i = 0; i < size; i++)
        pool_age[i] = 0;

    memset(pool, 0, size);

    // Main loop that iterates over all of the colors of the parents.
    char const *parent_color_p[2];
    int color1, color2, last_color = 0;
    int i, j, k, child_color = 0;
    for(i = 0; i < max_iter_num && used_vertex_count < size; i++) {
        LOGD("\n    Creating color %2d:\n", i);

        // Pick 2 random colors.
        color1 = get_rand_color(color_num1, i, used_color_list[0]);
        color2 = get_rand_color(color_num2, i, used_color_list[1]);

        if(color1 == -1)
            parent_color_p[0] = NULL;
        else
            parent_color_p[0] = parent1[color1];

        if(color2 == -1)
            parent_color_p[1] = NULL;
        else
            parent_color_p[1] = parent2[color2];

        LOGD("        Chosen colors:\n            color %2d:", color1);
        for(j = 0; j < size; j++) {
            if(parent_color_p[0][j])
                LOGD("%2d ", j);
        }

        LOGD("\n            color %2d:", color2);
        for(j = 0; j < size; j++) {
            if(parent_color_p[1][j])
                LOGD("%2d ", j);
        }

        // The child still has colors that weren't populated.
        if(i < target_color_count) {
            child_color = i;

            LOGDSTR("\n        Merging colors:");
            used_vertex_count += merge_colors(
                size,
                parent_color_p,
                child[child_color],
                pool,
                &pool_count,
                used_vertex_list
            );
            LOGDSTR("\n            new color after merge:");
            for(j = 0; j < size; j++) {
                if(child[i][j])
                    LOGD("%2d ", j);
            }
            LOGDSTR("\n            pool state: ");
            for(j = 0; j < size; j++) {
                if(pool[j])
                    LOGD("%2d ", j);
            }

            local_search(
                size,
                edges,
                weights,
                num_of_edges,
                child[child_color],
                pool,
                &pool_count
            );
            LOGDSTR("\n            new color after local search:");
            for(j = 0; j < size; j++) {
                if(child[i][j])
                    LOGD("%2d ", j);
            }
            LOGDSTR("\n            new pool state: ");
            for(j = 0; j < size; j++) {
                if(pool[j])
                    LOGD("%2d ", j);
            }

        /**
         * All of the child's colors were visited, merge the current colors
         * of the parents with the pool.
         */
        } else if(used_vertex_count < size) {
            LOGDSTR("\n        All target colors were visited, put all unused vertices in the pool");
            for(j = 0; j < size; j++) {
                if(!used_vertex_list[j]) {
                    pool[j] = 1;
                    pool_count++;
                    used_vertex_list[j] = 1;
                    used_vertex_count++;
                }
            }
        }

        LOGDSTR("\n        Search Back:");
        // Search back to try to place vertices in the pool in previous colors.
        for(j = 0; j < size && pool_count > 0; j++) {
            for(k = pool_age[j]; k < child_color && pool[j]; k++) {
                LOGD("\n            Try placing vertex %d in color %d.", j, k);
                child[k][j] = 1;
                pool[j] = 0;
                pool_count--;

                local_search(
                    size,
                    edges,
                    weights,
                    num_of_edges,
                    child[k],
                    pool,
                    &pool_count
                );
            }
        }

        // Update the age records of vertices in the pool.
        for(j = 0; j < size; j++) {
            if(pool[j])
                pool_age[j]++;
            else
                pool_age[j] = 0;
        }

        LOGDSTR("\n        Child state: ");
        for(int j = 0; j < target_color_count; j++) {
            LOGDSTR(" | ");
            for(int k = 0; k < size; k++)
                if(child[j][k])
                    LOGD("%2d ", k);
        }
        LOGDSTR("\n        Pool state: ");
        for(int j = 0; j < size; j++) {
            if(pool[j])
                LOGD("%2d ", j);
        }
        LOGDSTR("\n");
    }

    LOGDSTR("\n        Another last Search Back:");
    // Search back to try to place vertices in the pool in previous colors.
    for(j = 0; j < size && pool_count > 0; j++) {
        for(k = pool_age[j]; k < child_color && pool[j]; k++) {
            LOGD("\n            Try placing vertex %d in color %d.", j, k);
            child[k][j] = 1;
            pool[j] = 0;
            pool_count--;

            local_search(
                size,
                edges,
                weights,
                num_of_edges,
                child[k],
                pool,
                &pool_count
            );
        }
    }

    // Record the last color of the child.
    last_color = child_color + 1;

    int fitness = 0;
    int competition[size];
    memset(competition, 0, size*sizeof(int));
    
    LOGDSTR("        Randomly allocate the pool in the colors.");
    // If the pool is not empty, randomly allocate the remaining vertices in the colors.
    if(pool_count > 0) {
        int color_num;
        int temp_count[size];
        for(i = 0; i < size; i++) {
            if(pool[i]) {
                color_num = rand()%target_color_count;
                child[color_num][i] = 1;

                count_conflicts(
                    size,
                    child[color_num],
                    edges,
                    weights,
                    competition,
                    temp_count
                );

                if(color_num + 1 > last_color)
                    last_color = color_num + 1;

                fitness += weights[i];
            }
        }

    // All of the vertices were allocated and no conflicts were detected.
    } else {
        fitness = 0;
    }

    *child_color_count = last_color;
    crossover_time += ((double)(clock() - start))/CLOCKS_PER_SEC;
    return fitness;
}

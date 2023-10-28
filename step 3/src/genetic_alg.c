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


void graph_color_random(int size, const char edges[][size], char colors[][size], int max_color) {
    for(int i = 0; i < size; i++)
        colors[rand()%max_color][i] = 1;
}

void graph_color_genetic(
    int size, 
    char edges[][size], 
    int base_color_count,
    int max_gen_num,
    char best_solution[][size], 
    int *best_color_count,
    int *best_fitness,
    float *best_solution_time
) {
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
    int i, j;
    int temp_conflict_count[size];
    for (i = 0; i < 100; i++) {
        color_count[i] = base_color_count;
        graph_color_random(size, edges, colors[i], base_color_count);

        for(j = 0; j < base_color_count; j++)
            fitness[i] += count_conflicts(size, colors[i][j], edges, temp_conflict_count);
    }

    // Keep generating solutions for max_gen_num generations.
    char child[base_color_count][size];
    int target_color_count = base_color_count;
    int parent1, parent2, dead_parent, child_colors, temp_fitness, best = 0;
    for(i = 0; i < max_gen_num; i++) {
        // Initialize the child
        memset(child, 0, size*base_color_count);

        // Pick 2 random parents
        dead_parent = -1;
        parent1 = rand()%100;
        do { parent2 = rand()%100; } while (parent1 == parent2);

        // Do a crossover
        temp_fitness = crossover(
            size, 
            edges, 
            edge_count_list,
            color_count[parent1], 
            color_count[parent2], 
            colors[parent1], 
            colors[parent2], 
            target_color_count,
            child, 
            &child_colors
        );

        // Check if the child is better than any of the parents.
        if (child_colors <= color_count[parent1] && temp_fitness <= fitness[parent1])
            dead_parent = parent1;
        else if (child_colors == color_count[parent2] && temp_fitness <= fitness[parent2])
            dead_parent = parent2;

        // Replace a dead parent.
        if(dead_parent > -1) {
            memmove(colors[dead_parent], child, size*base_color_count);
            color_count[dead_parent] = child_colors;
            fitness[dead_parent] = temp_fitness;

            if(color_count[best] >= child_colors && fitness[best] >= temp_fitness) {
                best = dead_parent;
                last_solution_time = ((double)(clock() - start_time))/CLOCKS_PER_SEC;
            }
        }

        // Make the target harder if it was found.
        if(temp_fitness == 0) {
            target_color_count--;

            if(target_color_count == 0)
                break;
        }
    }

    if(last_solution_time == -1)
        last_solution_time = ((double)(clock() - start_time))/CLOCKS_PER_SEC;

    *best_fitness = fitness[best];
    *best_solution_time = last_solution_time;
    *best_color_count = color_count[best];
    memcpy(best_solution, colors[best], size*base_color_count);
}

int get_rand_color(int size, int colors_used, char used_color_list[]) {
    if(colors_used >= size - 2) {
        for(int i = 0; i < size; i++) {
            if(!used_color_list[i]) {
                used_color_list[i] = 1;
                return i;
            }
        }

        return -1;
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

    return total_used;
}

void fix_conflicts(
    int size,
    const char edges[][size],
    const int num_of_edges[],
    char color[],
    char pool[],
    int *pool_total
) {
    int conflict_count[size];
    memset(conflict_count, 0, size*sizeof(int));

    // Count the conflicts..
    int total_conflicts = count_conflicts(
        size,
        color,
        edges,
        conflict_count
    );

    // Keep removing problematic vertices until all conflicts are gone.
    int i, max_conflict_vert;
    while(total_conflicts > 0) {
        // Find the most problematic vertex.
        max_conflict_vert = 0;
        for(i = 0; i < size; i++){
            if(conflict_count[max_conflict_vert] < conflict_count[i])
                max_conflict_vert = i;

            else if(
                conflict_count[max_conflict_vert] == conflict_count[i] &&
                num_of_edges[max_conflict_vert] <= num_of_edges[i])
                max_conflict_vert = i;
        }

        // Throw the most problematic vertex to the pool.
        color[max_conflict_vert] = 0;
        total_conflicts -= conflict_count[max_conflict_vert];
        conflict_count[max_conflict_vert] = 0;
        pool[max_conflict_vert] = 1;
        (*pool_total)++;

        // Update the conflicts of the vertices connected to the removed vertex.
        for(i = 0; i < size; i++){
            if(edges[max_conflict_vert][i] && color[i]) {
                conflict_count[i] -= 1;
                total_conflicts--;
            }
        }
    }
}

int crossover(
    int size, 
    const char edges[][size], 
    const int num_of_edges[],
    int color_num1, 
    int color_num2, 
    const char parent1[][size], 
    const char parent2[][size], 
    int target_color_count,
    char child[][size],
    int *child_color_count
) {
    // max number of colors of the two parents.
    int max_color_num = color_num1 > color_num2 ? color_num1 : color_num2;

    // The max number of iterations over colors in the main loop.
    int max_iter_num = max_color_num > target_color_count ? max_color_num : target_color_count;

    // list of used colors in the parents.
    char used_color_list[2][max_color_num];
    memset(used_color_list, 0, 2*max_color_num);

    // info of vertices taken out of the parents.
    int used_vertex_count = 0, temp_used_count;
    char used_vertex_list[size];
    memset(used_vertex_list, 0, size);

    // info of pool.
    int pool_count = 0, dummy_pool_count;
    int pool_age[size];
    char pool[size], dummy_pool[size];
    for(int i = 0; i < size; i++)
        pool_age[i] = 0;

    memset(pool, 0, size);
    memset(dummy_pool, 0, size);

    // Main loop that iterates over all of the colors of the parents.
    char const *parent_color_p[2];
    int color1, color2, last_color = 0;
    int i, j, k, child_color;
    for(i = 0; i < max_iter_num && (pool_count > 0 || used_vertex_count < size); i++) {
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

        // The child still has colors that weren't populated.
        if(i < target_color_count) {
            child_color = i;

            used_vertex_count += merge_colors(
                size,
                parent_color_p,
                child[child_color],
                pool,
                &pool_count,
                used_vertex_list
            );

            fix_conflicts(
                size,
                edges,
                num_of_edges,
                child[child_color],
                pool,
                &pool_count
            );

        /**
         * All of the child's colors were visited, merge the current colors
         * of the parents with the pool.
         */
        } else if(used_vertex_count < size) {
            // if(parent_color_p[0] != NULL && parent_color_p[1] != NULL) {
            //     for(j = 0; j < size; j++) {
            //         if(!used_vertex_list[j] && (parent_color_p[0][j] | parent_color_p[1][j])) {
            //             pool[j] = 1;
            //             pool_total += 1;
            //             used_vertex_list[j] = 1;
            //             used_vertex_count++;
            //         }
            //     }
            
            // } else if((parent_color_p[0] == NULL) != (parent_color_p[1] == NULL)) {
            //     const char* parent = parent_color_p[0] > parent_color_p[1] ? parent_color_p[0] : parent_color_p[1];
            //     for(j = 0; j < size; j++) {
            //         if(!used_vertex_list[j] && parent[j]) {
            //             pool[j] = 1;
            //             pool_total += 1;
            //             used_vertex_list[j] = 1;
            //             used_vertex_count++;
            //         }
            //     }
            // }

            temp_used_count = merge_colors(
                size,
                parent_color_p,
                pool,
                dummy_pool,
                &dummy_pool_count,
                used_vertex_list
            );

            pool_count += temp_used_count;
            used_vertex_count += temp_used_count;
        }

        // Search back to try to place vertices in the pool in previous colors.
        for(j = 0; j < size && pool_count > 0; j++) {
            for(k = pool_age[j]; k < child_color && pool[j]; k++) {
                child[k][j] = 1;
                pool[j] = 0;
                pool_count--;

                fix_conflicts(
                    size,
                    edges,
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
    }

    // Record the last color of the child.
    last_color = child_color + 1;

    int fitness = 0;
    
    // If the pool is not empty, randomly allocate the remaining vertices in the colors.
    if(pool_count > 0) {
        int color_num;
        int temp_count[size];
        for(i = 0; i < size; i++) {
            if(pool[i]) {
                color_num = rand()%target_color_count;
                child[color_num][i] = 1;

                fitness += count_conflicts(
                    size,
                    child[color_num],
                    edges,
                    temp_count
                );

                if(color_num + 1 > last_color)
                    last_color = color_num + 1;
            }
        }

    // All of the vertices were allocated and no conflicts were detected.
    } else {
        fitness = 0;
    }

    *child_color_count = last_color;
    return fitness;
}

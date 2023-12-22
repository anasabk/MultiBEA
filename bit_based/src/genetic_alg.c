#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h> 
#include <string.h>
#include <float.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <errno.h>
#include <stdatomic.h>
#include <pthread.h>
#include <math.h>

#include "genetic_alg.h"
#include "stdgraph.h"
#include "common.h"


void graph_color_random(
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

int graph_color_genetic(
    int graph_size, 
    const block_t edges[][TOTAL_BLOCK_NUM(graph_size)], 
    int weights[], 
    int population_size,
    int base_color_count, 
    int max_gen_num, 
    block_t best_solution[][TOTAL_BLOCK_NUM(graph_size)], 
    int *best_fitness, 
    float *best_solution_time,
    int *uncolored_num,
    int thread_num,
    genetic_criteria_t criteria
) {
    // Count the degrees of each vertex
    int edge_count_list[graph_size];
    count_edges(graph_size, edges, edge_count_list);

    block_t population[population_size][base_color_count][TOTAL_BLOCK_NUM(graph_size)];
    int color_count[population_size];
    int uncolored[population_size];
    int fitness[population_size];

    // Initialize The arrays.
    memset(population, 0, population_size * base_color_count * (TOTAL_BLOCK_NUM(graph_size)) * sizeof(block_t));
    memset(uncolored, 0, population_size*sizeof(int));
    memset(fitness, 0, population_size*sizeof(int));

    /**
     * Generate a random population, where each individual has
     * the base number of colors.
     */
    int i;
    for (i = 0; i < population_size; i++) {
        graph_color_random(graph_size, edges, population[i], base_color_count);   
        color_count[i] = base_color_count;
        fitness[i] = __INT_MAX__;
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    atomic_long used_parents[(TOTAL_BLOCK_NUM(population_size))];
    memset(used_parents, 0, (TOTAL_BLOCK_NUM(population_size))*sizeof(atomic_long));

    atomic_int target_color = base_color_count;
    atomic_int best_i = 0;
    struct crossover_param_s temp_param = {
        base_color_count,
        &target_color,
        &best_i,
        graph_size,
        max_gen_num,
        (block_t*)edges,
        criteria == MIN_COST ? weights : edge_count_list,
        color_count,
        fitness,
        uncolored,
        population_size,
        (block_t*)population,
        used_parents
    };

    pthread_t thread_id[thread_num];
    for(i = 0; i < thread_num; i++)
        pthread_create(&thread_id[i], &attr, crossover_thread, &temp_param);

    struct crossover_result_s *results[thread_num];
    // for(i = 0; i < thread_num; i++)
    //     pthread_join(thread_id[i], (void**)&results[i]);

    i = 0;
    struct timespec wait_time;
    while(i < thread_num) {
        clock_gettime(CLOCK_REALTIME, &wait_time);
        wait_time.tv_sec += 1;
        if(pthread_timedjoin_np(thread_id[i], (void**)&results[i], &wait_time) == 0)
            i++;

        printf("%10.6lf\n", ((double)fitness[best_i]));
    }

    double best_time = 0;
    for(i = 0; i < thread_num; i++) {
        if(best_i == results[i]->best_i)
            best_time = results[i]->best_time;
        free(results[i]);
    }

    *best_fitness = fitness[best_i];
    *uncolored_num = uncolored[best_i];
    *best_solution_time = best_time;
    memcpy(best_solution, population[best_i], base_color_count * (TOTAL_BLOCK_NUM(graph_size)) * sizeof(block_t));

    return color_count[best_i];
}

int get_rand_color(int max_color_num, int colors_used, block_t used_color_list[]) {
    if(colors_used >= max_color_num) {
        return -1;

    } else if(colors_used > max_color_num - 2) {
        for(int i = 0; i < max_color_num; i++) {
            if(!(used_color_list[BLOCK_INDEX(i)] & MASK(i))) {
                used_color_list[BLOCK_INDEX(i)] |= MASK(i);
                return i;
            }
        }
    }

    int temp;
    while(1) {
        temp = rand()%max_color_num;
        if(!(used_color_list[BLOCK_INDEX(temp)] & MASK(temp))) {
            used_color_list[BLOCK_INDEX(temp)] |= MASK(temp);
            return temp;
        }
    }
}

int merge_colors(
    int graph_size,
    const block_t *parent_color[2],
    block_t child_color[],
    block_t pool[],
    int *pool_total,
    block_t used_vertex_list[]
) {
    int total_used = 0;
    if(parent_color[0] == NULL || parent_color[1] == NULL) {
        const block_t *non_null = parent_color[0] == NULL ? parent_color[1] : parent_color[0];
        
        if(non_null != NULL) {
            for(int i = 0; i < (TOTAL_BLOCK_NUM(graph_size)); i++) {
                child_color[i] |= (non_null[i] & ~(used_vertex_list[i]));
                used_vertex_list[i] |= child_color[i];
                total_used += __builtin_popcountl(child_color[i]);
            }
        }

    } else {
        for(int i = 0; i < (TOTAL_BLOCK_NUM(graph_size)); i++) {
            child_color[i] |= ((parent_color[0][i] | parent_color[1][i]) & ~(used_vertex_list[i]));
            used_vertex_list[i] |= child_color[i];
            total_used += __builtin_popcountl(child_color[i]);
        }
    }

    for(int i = 0; i < (TOTAL_BLOCK_NUM(graph_size)); i++) {
        if(pool[i]) {
            child_color[i] |= pool[i];
            *pool_total -= __builtin_popcountl(pool[i]);
            pool[i] = 0;
        }
    }

    return total_used;
}

void fix_conflicts(
    int graph_size,
    const block_t edges[][TOTAL_BLOCK_NUM(graph_size)], 
    const int weights[],
    int conflict_count[],
    int *total_conflicts,
    block_t color[],
    block_t pool[],
    int *pool_total
) {
    // Keep removing problematic vertices until all conflicts are gone.
    int i, worst_vert = 0, vert_block;
    block_t vert_mask;
    while(*total_conflicts > 0) {
        // Find the vertex with the most conflicts.
        for(i = 0; i < graph_size; i++) {
            if (CHECK_COLOR(color, i) &&
                (conflict_count[worst_vert] < conflict_count[i] ||
                 (conflict_count[worst_vert] == conflict_count[i] && weights[worst_vert] >= weights[i]))
            ) {
                worst_vert = i;
            }
        }

        vert_mask = MASK(worst_vert);
        vert_block = BLOCK_INDEX(worst_vert);
        for(i = 0; i < graph_size; i++)
            if(CHECK_COLOR(color, i) && (edges[i][vert_block] & vert_mask))
                conflict_count[i]--;

        color[vert_block] &= ~vert_mask;
        pool[vert_block] |= vert_mask;
        (*pool_total)++;

        (*total_conflicts) -= conflict_count[worst_vert];
        conflict_count[worst_vert] = 0;
    }
}

int search_back(
    int graph_size,
    const block_t edges[][TOTAL_BLOCK_NUM(graph_size)], 
    const int weights[],
    block_t child[][TOTAL_BLOCK_NUM(graph_size)], 
    int current_color,
    block_t pool[],
    int *pool_count
) {
    // Search back to try to place vertices in the pool in previous colors.
    int conflict_count, last_conflict, temp_block, last_conflict_block;
    block_t i_mask, temp_mask;
    int i, j, k, i_block;
    int switch_count = 0;
    for(i = 0; i < graph_size && (*pool_count) > 0; i++) {
        i_block = BLOCK_INDEX(i);
        i_mask = MASK(i);

        if(pool[i_block] & i_mask) {
            for(j = 0; j < current_color; j++) {
                conflict_count = 0;
                for(k = 0; k < TOTAL_BLOCK_NUM(graph_size) && conflict_count < 2; k++) {
                    if(child[j][k] & edges[i][k]) {
                        conflict_count += __builtin_popcountl(child[j][k] & edges[i][k]);
                        last_conflict_block = k;
                    }
                }

                if(conflict_count == 0) {
                    child[j][i_block] |= i_mask;
                    pool[i_block] &= ~i_mask;
                    (*pool_count)--;
                    break;

                } else if (conflict_count == 1) {
                    for(k = 0; k < sizeof(block_t)*8; k++) 
                        if((((child[j][last_conflict_block] & edges[i][last_conflict_block]) >> k) & 1))
                            break;
                    last_conflict = last_conflict_block*sizeof(block_t)*8 + k;

                    if(weights[last_conflict] < weights[i]) {
                        child[j][i_block] |= i_mask;
                        pool[i_block] &= ~i_mask;

                        temp_block = BLOCK_INDEX(last_conflict);
                        temp_mask = MASK(last_conflict);
                        child[j][temp_block] &= ~temp_mask;
                        pool[temp_block] |= temp_mask;

                        switch_count++;
                        break;
                    }
                }
            }
        }
    }

    return switch_count;
}

int local_search(
    int graph_size,
    const block_t edges[][TOTAL_BLOCK_NUM(graph_size)], 
    const int weights[],
    block_t child[][TOTAL_BLOCK_NUM(graph_size)], 
    int color_count,
    block_t pool[],
    int *pool_count
) {
    int i, j, k, i_block, switch_count = 0;
    block_t i_mask;
    int competition;
    int conflict_count;
    block_t conflict_array[TOTAL_BLOCK_NUM(graph_size)];

    // Search back to try to place vertices in the pool in previous colors.
    for(i = 0; i < graph_size && (*pool_count) > 0; i++) { // Through every vertex until the pool is empty.
        i_block = BLOCK_INDEX(i);
        i_mask = MASK(i);

        if(pool[i_block] & i_mask) {
            for(j = 0; j < color_count; j++) {  // Throught every color until the vertex is not in the pool.
                conflict_count = 0;
                competition = 0;

                for(k = 0; k < TOTAL_BLOCK_NUM(graph_size); k++) {
                    conflict_array[k] = edges[i][k] & child[j][k];
                    conflict_count += __builtin_popcountl(conflict_array[k]);
                }

                for(k = 0; k < graph_size; k++)
                    if(CHECK_COLOR(conflict_array, k))
                        competition += weights[k];

                // Swap if no conflicts were found, or competition was smaller than the weight.
                if(competition < weights[i]) {
                    switch_count++;

                    for(k = 0; k < TOTAL_BLOCK_NUM(graph_size); k++) {
                        child[j][k] &= ~conflict_array[k];
                        pool[k] |= conflict_array[k];
                    }

                    child[j][i_block] |= i_mask;
                    pool[i_block] &= ~i_mask;

                    (*pool_count) += conflict_count - 1;
                    break;
                }
            }
        }
    }

    return switch_count;
}

int crossover(
    int graph_size, 
    const block_t edges[][TOTAL_BLOCK_NUM(graph_size)], 
    const int weights[],
    int color_num1, 
    int color_num2, 
    const block_t parent1[][TOTAL_BLOCK_NUM(graph_size)], 
    const block_t parent2[][TOTAL_BLOCK_NUM(graph_size)], 
    int target_color_count,
    block_t child[][TOTAL_BLOCK_NUM(graph_size)],
    int *child_color_count,
    int *uncolored
) {
    // max number of colors of the two parents.
    int max_color_num = color_num1 > color_num2 ? color_num1 : color_num2;

    // list of used colors in the parents.
    block_t used_color_list[2][TOTAL_BLOCK_NUM(max_color_num)];
    memset(used_color_list, 0, 2*TOTAL_BLOCK_NUM(max_color_num)*sizeof(block_t));

    // info of vertices taken out of the parents.
    int used_vertex_count = 0;
    block_t used_vertex_list[TOTAL_BLOCK_NUM(graph_size)];
    memset(used_vertex_list, 0, (TOTAL_BLOCK_NUM(graph_size))*sizeof(block_t));

    // info of pool.
    int pool_count = 0;
    block_t pool[TOTAL_BLOCK_NUM(graph_size)];
    memset(pool, 0, (TOTAL_BLOCK_NUM(graph_size))*sizeof(block_t));

    int conflict_count[graph_size];
    memset(conflict_count, 0, graph_size*sizeof(int));

    // Main loop that iterates over all of the colors of the parents.
    int color1, color2, last_color = 0;
    int i, j, total_conflicts = 0;
    int temp_v_count;
    for(i = 0; i < target_color_count; i++) {
        // The child still has colors that weren't populated.
        if(used_vertex_count < graph_size) {
            // Pick 2 random colors.
            color1 = get_rand_color(color_num1, i, used_color_list[0]);
            color2 = get_rand_color(color_num2, i, used_color_list[1]);

            temp_v_count = 0;  
            if(color1 != -1 && color2 != -1)
                for(j = 0; j < (TOTAL_BLOCK_NUM(graph_size)); j++) {
                    child[i][j] = ((parent1[color1][j] | parent2[color2][j]) & ~(used_vertex_list[j]));
                    temp_v_count += __builtin_popcountl(child[i][j]);
                }

            else if(color1 != -1)
                for(j = 0; j < (TOTAL_BLOCK_NUM(graph_size)); j++) {
                    child[i][j] = (parent1[color1][j] & ~(used_vertex_list[j]));
                    temp_v_count += __builtin_popcountl(child[i][j]);
                }

            else if(color2 != -1)
                for(j = 0; j < (TOTAL_BLOCK_NUM(graph_size)); j++) {
                    child[i][j] = (parent2[color2][j] & ~(used_vertex_list[j]));
                    temp_v_count += __builtin_popcountl(child[i][j]);
                }

            used_vertex_count += temp_v_count;

            for(j = 0; j < (TOTAL_BLOCK_NUM(graph_size)); j++) {
                child[i][j] |= pool[j];
                used_vertex_list[j] |= child[i][j];
            }

            memset(pool, 0, (TOTAL_BLOCK_NUM(graph_size))*sizeof(block_t));
            pool_count = 0;

            total_conflicts = count_conflicts(
                graph_size,
                child[i],
                edges,
                conflict_count
            );

            fix_conflicts(
                graph_size,
                edges,
                weights,
                conflict_count,
                &total_conflicts,
                child[i],
                pool,
                &pool_count
            );

        /**
         * All of the child's colors were visited, merge the current colors
         * of the parents with the pool.
         */
        } else if(pool_count == 0) {
            break;
        }

        search_back(
            graph_size,
            edges,
            weights,
            child, 
            i,
            pool,
            &pool_count
        );
    }

    last_color = i;

    if(used_vertex_count < graph_size) {
        for(j = 0; j < (TOTAL_BLOCK_NUM(graph_size)); j++)
            pool[j] |= ~used_vertex_list[j];
        pool[TOTAL_BLOCK_NUM(graph_size)] &= ((0xFFFFFFFFFFFFFFFF) >> (TOTAL_BLOCK_NUM(graph_size)*sizeof(block_t)*8 - graph_size));

        pool_count += (graph_size - used_vertex_count);
        used_vertex_count = graph_size;
        memset(used_vertex_list, 0xFF, (TOTAL_BLOCK_NUM(graph_size))*sizeof(block_t));
    } 

    // Another last local search.
    local_search(
        graph_size,
        edges,
        weights,
        child,
        target_color_count,
        pool,
        &pool_count
    );

    search_back(
        graph_size,
        edges,
        weights,
        child, 
        target_color_count,
        pool,
        &pool_count
    );

    // If the pool is not empty, randomly allocate the remaining vertices in the colors.
    int fitness = 0, temp_block;
    block_t temp_mask;
    if(pool_count > 0) {
        int color_num;
        for(i = 0; i < graph_size; i++) {
            temp_block = BLOCK_INDEX(i);
            temp_mask = MASK(i);
            if(pool[temp_block] & temp_mask) {
                color_num = rand()%target_color_count;
                child[color_num][temp_block] |= temp_mask;

                if(color_num + 1 > last_color)
                    last_color = color_num + 1;

                fitness += weights[i];
            }
        }

    // All of the vertices were allocated and no conflicts were detected.
    } else {
        fitness = 0;
    }

    *uncolored = pool_count;
    *child_color_count = last_color;
    return fitness;
}

void* crossover_thread(void *param) {
    struct timeval t1, t2;
    float last_solution_time = 0;
    gettimeofday(&t1, NULL);

    int base_color_count = ((struct crossover_param_s*)param)->base_color_count;
    atomic_int *target_color_count = ((struct crossover_param_s*)param)->target_color_count;
    atomic_int *best_i = ((struct crossover_param_s*)param)->best_i;
    int graph_size = ((struct crossover_param_s*)param)->size;
    int max_gen_num = ((struct crossover_param_s*)param)->max_gen_num;
    const block_t (*edges)[][TOTAL_BLOCK_NUM(graph_size)] = (block_t (*)[][TOTAL_BLOCK_NUM(graph_size)])((struct crossover_param_s*)param)->edges;
    int *weights = ((struct crossover_param_s*)param)->weights;
    int *color_count = ((struct crossover_param_s*)param)->color_count;
    int *fitness = ((struct crossover_param_s*)param)->fitness;
    int *uncolored = ((struct crossover_param_s*)param)->uncolored;
    int population_size = ((struct crossover_param_s*)param)->population_size;
    block_t (*population)[][base_color_count][TOTAL_BLOCK_NUM(graph_size)] = (block_t(*)[][base_color_count][TOTAL_BLOCK_NUM(graph_size)])((struct crossover_param_s*)param)->population;
    atomic_long *used_parents = ((struct crossover_param_s*)param)->used_parents;

    // Keep generating solutions for max_gen_num generations.
    block_t child[base_color_count][TOTAL_BLOCK_NUM(graph_size)];
    int temp_target_color = *target_color_count;
    int temp_uncolored;
    int parent1, parent2, child_colors, temp_fitness, best = 0;
    int bad_parent;
    for(int i = 0; i < max_gen_num; i++) {
        temp_target_color = *target_color_count;

        // Initialize the child
        memset(child, 0, (TOTAL_BLOCK_NUM(graph_size))*base_color_count*sizeof(block_t));

        // Pick 2 random parents
        do { parent1 = rand()%population_size; } while (CHECK_COLOR(used_parents, parent1));
        SET_COLOR(used_parents, parent1);
        do { parent2 = rand()%population_size; } while (CHECK_COLOR(used_parents, parent2));
        SET_COLOR(used_parents, parent2);

        // Do a crossover
        temp_fitness = crossover(
            graph_size, 
            *edges, 
            weights,
            color_count[parent1], 
            color_count[parent2], 
            (*population)[parent1], 
            (*population)[parent2], 
            temp_target_color,
            child, 
            &child_colors,
            &temp_uncolored
        );


        if(fitness[parent1] <= fitness[parent2] && color_count[parent1] <= color_count[parent2])
            bad_parent = parent2;
        else
            bad_parent = parent1;

        // Replace a dead parent.
        if(temp_fitness <= fitness[bad_parent] && child_colors <= color_count[bad_parent]) {
            memmove((*population)[bad_parent], child, (TOTAL_BLOCK_NUM(graph_size))*base_color_count*sizeof(block_t));
            color_count[bad_parent] = child_colors;
            fitness[bad_parent] = temp_fitness;
            uncolored[bad_parent] = temp_uncolored;

            if (temp_fitness < fitness[*best_i] ||
                (temp_fitness == fitness[*best_i] && child_colors < color_count[*best_i])
            ) {
                *best_i = bad_parent;
                best = bad_parent;
                gettimeofday(&t2, NULL);
                last_solution_time = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;   // us to ms

                // printf("%10.6lf | %10.6lf\n", ((double)fitness[*best_i]), last_solution_time);
            }
        }

        // Make the target harder if it was found.
        if(temp_fitness == 0 && child_colors <= *target_color_count) {
            *target_color_count = child_colors - 1;
            if(*target_color_count == 0)
                break;
        }

        RESET_COLOR(used_parents, parent1);
        RESET_COLOR(used_parents, parent2);
    }

    struct crossover_result_s *result = malloc(sizeof(struct crossover_result_s));
    result->best_i = best;
    result->best_time = last_solution_time;

    return (void*)result;
}

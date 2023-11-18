#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h> 
#include <string.h>
#include <float.h>
#include <sys/resource.h>
#include <errno.h>
#include <stdatomic.h>
#include <pthread.h>
#include <math.h>

#include "genetic_alg.h"
#include "stdgraph.h"
#include "common.h"


double graph_color_genetic_time = 0;
double get_rand_color_time = 0;
double graph_color_random_time = 0;
double merge_colors_time = 0;
double rm_vertex_time = 0;
double search_back_time = 0;
double local_search_time = 0;
double crossover_time = 0;
double crossover_thread_time = 0;


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
    float *best_solution_time,
    int *uncolored_num,
    int thread_num,
    genetic_criteria_t criteria
) {
    // Count the degrees of each vertex
    int edge_count_list[size];
    count_edges(size, edges, edge_count_list);

    char population[100][base_color_count][size];
    int color_count[100];
    int uncolored[100];
    int fitness[100];

    // Initialize The arrays.
    memset(population, 0, 100*base_color_count*size);
    memset(uncolored, 0, 100*sizeof(int));
    memset(fitness, 0, 100*sizeof(int));

    /**
     * Generate a random population, where each individual has
     * the base number of colors.
     */
    int i;
    for (i = 0; i < 100; i++) {
        graph_color_random(size, edges, population[i], base_color_count);   
        color_count[i] = base_color_count;
        fitness[i] = __INT_MAX__;
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    atomic_bool used_parents[size];
    memset(used_parents, 0, size*sizeof(atomic_bool));

    atomic_int target_color = base_color_count;
    atomic_int best_i = 0;
    struct crossover_param_s temp_param = {
        base_color_count,
        &target_color,
        &best_i,
        size,
        max_gen_num,
        (char*)edges,
        weights,
        edge_count_list,
        color_count,
        fitness,
        uncolored,
        (char*)population,
        used_parents,
        criteria
    };

    pthread_t thread_id[thread_num];
    for(int i = 0; i < thread_num; i++)
        pthread_create(&thread_id[i], &attr, crossover_thread, &temp_param);

    struct crossover_result_s *results[thread_num];
    for(int i = 0; i < thread_num; i++)
        pthread_join(thread_id[i], (void**)&results[i]);

    double best_time = 0;
    for(int i = 0; i < thread_num; i++) {
        if(best_i == results[i]->best_i)
            best_time = results[i]->best_time;
        free(results[i]);
    }

    *best_fitness = fitness[best_i];
    *uncolored_num = uncolored[best_i];
    *best_solution_time = best_time;
    memcpy(best_solution, population[best_i], size*base_color_count);

    return color_count[best_i];
}

int get_rand_color(int max_color_num, int colors_used, char used_color_list[]) {
    if(colors_used >= max_color_num) {
        return -1;

    } else if(colors_used > max_color_num - 2) {
        for(int i = 0; i < max_color_num; i++) {
            if(!used_color_list[i]) {
                used_color_list[i] = 1;
                return i;
            }
        }
    }

    int temp;
    while(1) {
        temp = rand()%max_color_num;
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

void rm_vertex(
    int vertex,
    int size, 
    const char edges[][size], 
    char color[], 
    char pool[], 
    int conflict_count[],
    int *total_conflicts,
    int *pool_total
) {
    for(int i = 0; i < size; i++)
        if(color[i] && edges[vertex][i])
            conflict_count[i]--;

    color[vertex] = 0;
    pool[vertex] = 1;
    (*pool_total)++;

    (*total_conflicts) -= conflict_count[vertex];
    conflict_count[vertex] = 0;
}

void fix_conflicts(
    int size,
    const char edges[][size],
    const int weights[],
    const int degrees[],
    int conflict_count[],
    int *total_conflicts,
    char color[],
    char pool[],
    int *pool_total,
    genetic_criteria_t criteria
) {
    // Keep removing problematic vertices until all conflicts are gone.
    int i, worst_vert = 0;
    while(*total_conflicts > 0) {
        // Find the vertex with the most conflicts.
        for(i = 0; i < size; i++) {
            if (color[i] &&
                (conflict_count[worst_vert] < conflict_count[i] ||
                (conflict_count[worst_vert] == conflict_count[i] &&
                ((criteria == MIN_COST && weights[worst_vert] >= weights[i]) ||
                 (criteria == MIN_POOL && degrees[worst_vert] >= degrees[i]))))
            ) {
                worst_vert = i;
            }
        }

        rm_vertex(
            worst_vert,
            size,
            edges,
            color,
            pool,
            conflict_count,
            total_conflicts,
            pool_total
        );
    }
}

void search_back(
    int size,
    const char edges[][size],
    const int weights[],
    const int degrees[],
    int conflict_counts[],
    char child[][size], 
    int current_color,
    char pool[],
    int pool_age[],
    int *pool_count,
    int *total_conflicts,
    genetic_criteria_t criteria
) {
    int conflict_count, last_conflict;
    // Search back to try to place vertices in the pool in previous colors.
    int i, j, k;
    for(i = 0; i < size && (*pool_count) > 0; i++) {
        for(j = pool_age[i]; j < current_color && pool[i]; j++) {
            conflict_count = 0;
            for(k = 0; k < size; k++) {
                if(edges[i][k] && child[j][k]) {
                    conflict_count++;
                    last_conflict = k;
                    if(conflict_count > 1)
                        break;
                }
            }

            if(conflict_count == 0) {
                child[j][i] = 1;
                pool[i] = 0;
                (*pool_count)--;

            } else if(conflict_count == 1 && 
                ((criteria == MIN_COST && weights[last_conflict] <= weights[i]) ||
                 (criteria == MIN_POOL && degrees[last_conflict] <= degrees[i]))) 
            {
                child[j][i] = 1;
                pool[i] = 0;

                child[j][last_conflict] = 0;
                pool[last_conflict] = 1;
            }
        }
    }
}

void local_search(
    int size,
    const char edges[][size],
    const int weights[],
    const int degrees[],
    char child[][size], 
    int color_count,
    char pool[],
    int *pool_count,
    genetic_criteria_t criteria
) {
    int i, j, k;
    int competition;
    int conflict_count;

    // Search back to try to place vertices in the pool in previous colors.
    for(i = 0; i < size && (*pool_count) > 0; i++) { // Through every vertex until the pool is empty.
        for(j = 0; j < color_count && pool[i]; j++) {  // Throught every color until the vertex is not in the pool.
            conflict_count = 0;
            competition = 0;

            for(k = 0; k < size; k++) {
                if(edges[i][k] && child[j][k]) {
                    conflict_count++;
                    if(criteria == MIN_COST)
                        competition += weights[k];
                    else if(criteria == MIN_POOL)
                        competition += degrees[k];
                }
            }

            // Swap if no conflicts were found, or competition was smaller than the weight.
            if(conflict_count == 0) {
                child[j][i] = 1;
                pool[i] = 0;
                (*pool_count)--;

            } else if((criteria == MIN_COST && competition < weights[i]) ||
                (criteria == MIN_POOL && competition < degrees[i])) 
            {
                child[j][i] = 1;
                pool[i] = 0;
                (*pool_count)--;

                for(k = 0; k < size; k++) {
                    if(edges[i][k] && child[j][k]) {
                        child[j][k] = 0;
                        pool[k] = 1;
                        (*pool_count)++;
                    }
                }
            }
        }
    }
}

int crossover(
    int size, 
    const char edges[][size], 
    const int weights[],
    const int degrees[],
    int color_num1, 
    int color_num2, 
    const char parent1[][size], 
    const char parent2[][size], 
    int target_color_count,
    genetic_criteria_t criteria,
    char child[][size],
    int *child_color_count,
    int *uncolored
) {
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
    memset(pool, 0, size);
    memset(pool_age, 0, size*sizeof(int));

    int conflict_count[size];
    memset(conflict_count, 0, size*sizeof(int));

    // Main loop that iterates over all of the colors of the parents.
    char const *parent_color_p[2];
    int color1, color2, last_color = 0;
    int i, j, child_color = 0, total_conflicts = 0;
    for(i = 0; i < max_iter_num && used_vertex_count < size; i++) {
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

            total_conflicts = count_conflicts(
                size,
                child[child_color],
                edges,
                conflict_count
            );

            fix_conflicts(
                size,
                edges,
                weights,
                degrees,
                conflict_count,
                &total_conflicts,
                child[child_color],
                pool,
                &pool_count,
                criteria
            );

        /**
         * All of the child's colors were visited, merge the current colors
         * of the parents with the pool.
         */
        } else if(used_vertex_count < size) {
            for(j = 0; j < size; j++)
                pool[j] |= !used_vertex_list[j];

            pool_count += (size - used_vertex_count);
            used_vertex_count = size;
            memset(used_vertex_list, 1, size);
        }

        search_back(
            size,
            edges,
            weights,
            degrees,
            conflict_count,
            child, 
            child_color,
            pool,
            pool_age,
            &pool_count,
            &total_conflicts,
            criteria
        );

        // Update the age records of vertices in the pool.
        for(j = 0; j < size; j++) {
            if(pool[j])
                pool_age[j]++;
            else
                pool_age[j] = 0;
        }
    }

    if(used_vertex_count < size) {
        for(j = 0; j < size; j++)
            pool[j] |= !used_vertex_list[j];

        pool_count += (size - used_vertex_count);
        used_vertex_count = size;
        memset(used_vertex_list, 1, size);
    }

    // Record the last color of the child.
    last_color = child_color + 1;

    // Another last local search.
    local_search (
        size,
        edges,
        weights,
        degrees,
        child,
        child_color,
        pool,
        &pool_count,
        criteria
    );

    // If the pool is not empty, randomly allocate the remaining vertices in the colors.
    int fitness = 0;
    if(pool_count > 0) {
        int color_num;
        for(i = 0; i < size; i++) {
            if(pool[i]) {
                color_num = rand()%target_color_count;
                child[color_num][i] = 1;

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
    int size = ((struct crossover_param_s*)param)->size;
    int max_gen_num = ((struct crossover_param_s*)param)->max_gen_num;
    char (*edges)[][size] = (char(*)[][size])((struct crossover_param_s*)param)->edges;
    int *weights = ((struct crossover_param_s*)param)->weights;
    int *edge_count_list = ((struct crossover_param_s*)param)->edge_count_list;
    int *color_count = ((struct crossover_param_s*)param)->color_count;
    int *fitness = ((struct crossover_param_s*)param)->fitness;
    int *uncolored = ((struct crossover_param_s*)param)->uncolored;
    char (*population)[100][base_color_count][size] = (char(*)[][base_color_count][size])((struct crossover_param_s*)param)->population;
    atomic_bool *used_parents = ((struct crossover_param_s*)param)->used_parents;
    genetic_criteria_t criteria = ((struct crossover_param_s*)param)->criteria;

    // Keep generating solutions for max_gen_num generations.
    char child[base_color_count][size];
    int temp_target_color = *target_color_count;
    int temp_uncolored;
    int parent1, parent2, dead_parent, child_colors, temp_fitness, best = 0;
    for(int i = 0; i < max_gen_num; i++) {
        temp_target_color = *target_color_count;

        // Initialize the child
        memset(child, 0, size*base_color_count);

        // Pick 2 random parents
        dead_parent = -1;
        do { parent1 = rand()%100; } while (used_parents[parent1]);
        used_parents[parent1] = 1;
        do { parent2 = rand()%100; } while (used_parents[parent2]);
        used_parents[parent2] = 1;

        // Do a crossover
        temp_fitness = crossover(
            size, 
            *edges, 
            weights,
            edge_count_list,
            color_count[parent1], 
            color_count[parent2], 
            (*population)[parent1], 
            (*population)[parent2], 
            temp_target_color,
            criteria,
            child, 
            &child_colors,
            &temp_uncolored
        );

        // Check if the child is better than any of the parents.
        if (temp_fitness <= fitness[parent1] && child_colors <= color_count[parent1])
            dead_parent = parent1;

        else if (temp_fitness <= fitness[parent2] && child_colors <= color_count[parent2])
            dead_parent = parent2;

        else if (random()%10000 < 5)
            dead_parent = (parent1 == *best_i) ? parent2 : parent1;

        // Replace a dead parent.
        if(dead_parent > -1) {
            memmove((*population)[dead_parent], child, size*base_color_count);
            color_count[dead_parent] = child_colors;
            fitness[dead_parent] = temp_fitness;
            uncolored[dead_parent] = temp_uncolored;

            if (temp_fitness < fitness[*best_i] ||
                (temp_fitness == fitness[*best_i] && child_colors < color_count[*best_i])
            ) {
                *best_i = dead_parent;
                best = dead_parent;
                gettimeofday(&t2, NULL);
                last_solution_time = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;   // us to ms
            }
        }

        // Make the target harder if it was found.
        if(temp_fitness == 0 && child_colors <= *target_color_count) {
            *target_color_count = child_colors - 1;
            if(*target_color_count == 0)
                break;
        }

        used_parents[parent1] = 0;
        used_parents[parent2] = 0;
    }

    struct crossover_result_s *result = malloc(sizeof(struct crossover_result_s));
    result->best_i = best;
    result->best_time = last_solution_time;

    // crossover_thread_time += ((double)(clock() - start_time))/CLOCKS_PER_SEC;
    return (void*)result;
}

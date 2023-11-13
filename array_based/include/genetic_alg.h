#ifndef GENETEIC_ALG_H
#define GENETEIC_ALG_H


#include <stdatomic.h>


typedef enum {
    MIN_COST,
    MIN_COLOR_COUNT
} genetic_criteria_t;

struct crossover_param_s {
    int base_color_count;
    atomic_int *target_color_count;
    atomic_int *best_i;
    int size;
    int max_gen_num;
    char *edges;
    int *weights;
    int *edge_count_list;
    int *color_count;
    int *fitness;
    int *uncolored;
    char *colors;
    atomic_bool *used_parents;
    genetic_criteria_t criteria;
};

struct crossover_result_s {
    int best_i;
    double best_time;
};


extern double graph_color_genetic_time;
extern double get_rand_color_time;
extern double graph_color_random_time;
extern double merge_colors_time;
extern double rm_vertex_time;
extern double search_back_time;
extern double local_search_time;
extern double crossover_time;
extern double crossover_thread_time;


/**
 * @brief randomly color the graph with max_color being the
 * upper bound of colors used.
 * 
 * @param size Size of the graph.
 * @param edges The edge matrix of the graph.
 * @param colors The result color matrix of the graph.
 * @param max_color The upper bound of colors to be used.
 */
void graph_color_random(int size, const char edges[][size], char colors[][size], int max_color);


/**
 * @brief Color the graph using a genetic algorithm.
 * 
 * @param size Size of the graph.
 * @param edges The edge matric of the graph.
 * @param weights The weights of vertices in the graph.
 * @param base_color_count The base number of colors to start from.
 * @param max_gen_num The max number of generations to finish the search after reaching.
 * @param best_solution Output pointer to the result color matrix.
 * @param best_fitness Output pointer to the result solution fitness.
 * @param best_solution_time Output pointer to the time it took to find the last solution.
 * @param thread_num Number of concurrent crossover threads.
 * @returns Number of colors of the solution.
 */
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
);

/**
 * @brief Get a random color not used previously in the used_color_list.
 * When a color is returned, it is added to the used_color_list.
 * 
 * @param size Max number of colors.
 * @param colors_used Number of colors used.
 * @param used_color_list List of used colors.
 * @return If an unused color is found, return it. If all colors are 
 * used, return -1.
 */
int get_rand_color(int size, int colors_used, char used_color_list[]);


/**
 * @brief Merge two parent colors and the pool into the child color. Vertices are
 * checked if used previously through used_vertex_list before being added
 * to the child color.
 * 
 * @param size Size of the graph.
 * @param parent_color Array of pointers to two parents.
 * @param child_color Pointer to the child color
 * @param pool Pool to be dumped into the child color.
 * @param pool_total Total number of vertices in the pool.
 * @param used_vertex_list List of used vertices.
 * @return Return the total number of newly used vertices.
 */
int merge_colors(
    int size,
    const char *parent_color[2],
    char child_color[],
    char pool[],
    int *pool_total,
    char used_vertex_list[]
);


/**
 * @brief Fix the conflicts in the color by throwing problematic
 * vertices to the pool.
 * 
 * @param size Size of the graph.
 * @param edges The edge matric of the graph.
 * @param num_of_edges List of the degrees of every vertex.
 * @param color Color to be fixed.
 * @param pool Pool to store vertices removed from the color.
 * @param pool_total Total number of vertices in the pool.
 */
void local_search(
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
);


/**
 * @brief Performs crossover between two parents to produce
 * a new individual.
 * 
 * @param size Size of the graph.
 * @param edges The edge matric of the graph.
 * @param num_of_edges List of the degrees of every vertex.
 * @param color_num1 Number of colors of the first parent.
 * @param color_num2 Number of colors of the second parent.
 * @param parent1 The first parent.
 * @param parent2 The second parent.
 * @param target_color_count Target color count of the new individual.
 * @param child Output pointer to the result individual.
 * @param child_color_count Output pointer to the number of colors of
 * the new individual.
 * @return Return the fitness of the new individual.
 */
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
);


void* crossover_thread(void *param);


#endif

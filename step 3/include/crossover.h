int get_rand_color(int size, int colors_used, char used_list[]);

void merge_colors(
    int size,
    const char *parent_color[2],
    char child_color[],
    char pool[],
    int *pool_total,
    char used_vertex_list[],
    int *used_vertex_count
);

void fix_conflicts(
    int size,
    const char edges[][size],
    const int num_of_edges[],
    char child_color[],
    char pool[],
    int *pool_total
);

int crossover(
    int size, 
    const char edges[][size], 
    const int num_of_edges[],
    int color_num1, 
    int color_num2, 
    const char parent1[][size], 
    const char parent2[][size], 
    char child[][size],
    int target_color_count,
    int *child_color_count
);

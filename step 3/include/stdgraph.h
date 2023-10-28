#include <stdlib.h>
#include <stdbool.h>


bool init_graph(const char* filename, int size, char edges[][size]);

bool is_valid(int size, const char edges[size][size], int color_num, const char colors[color_num][size]);

int count_edges(int size, const char edges[][size], int count[]);

void print_colors(const char *filename, const char *header, int max_color_num, int size, const char colors[][size]);

int graph_color_greedy(int size, const char edges[][size], char colors[][size], int max_color_possible);

int count_conflicts(int size, const char color[], const char edges[][size], int conflict_count[]);

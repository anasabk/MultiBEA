void print_progress(size_t count, size_t max);

bool exists(int* arr, int len, int target);

bool init_graph(char* filename, int size, char edges[][size]);

bool is_valid(int size, const char edges[size][size], int color_num, const char colors[color_num][size]);

int count_edges(int size, const char edges[][size], int count[]);

int greedy_color(int size, const char edges[][size], char colors[][size], int max_color_possible);

void print_colors(char *filename, char *header, int max_color_num, int size, char colors[][size]);

int genetic_color(int size, char edges[][size], int edge_count[size], int max_edge_count, char result_colors[][size]);

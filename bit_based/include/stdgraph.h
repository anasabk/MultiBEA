#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>


#define EDGE_BIT_INDEX(small, large)    (((large) * (large - 1)/2) + small)
#define BLOCK_INDEX(bit_index)          ((bit_index)/32)
#define MASK_INDEX(bit_index)           ((bit_index)%32)
#define MASK(bit_index)                 (1 << MASK_INDEX(bit_index))

#define CHECK_EDGE(bit_index, edges)    (edges[BLOCK_INDEX(bit_index)] & MASK(bit_index))
#define SET_EDGE(bit_index, edges)      edges[BLOCK_INDEX(bit_index)] |=  MASK(bit_index)
#define RESET_EDGE(bit_index, edges)    edges[BLOCK_INDEX(bit_index)] &= ~MASK(bit_index)

#define CHECK_COLOR(color, vertex)      (color[BLOCK_INDEX(vertex)] & MASK(vertex))
#define SET_COLOR(color, vertex)        color[BLOCK_INDEX(vertex)] |=  MASK(vertex)
#define RESET_COLOR(color, vertex)      color[BLOCK_INDEX(vertex)] &= ~MASK(vertex)


bool read_graph(const char* filename, int sizgraph_sizee, uint32_t edges[]);

bool read_weights(const char* filename, int size, int weights[]);

bool is_valid(int graph_size, const uint32_t edges[], int color_num, const uint32_t colors[][BLOCK_INDEX(graph_size-1)+1]);

void count_edges(int size, const uint32_t edges[], int count[]);

void print_colors(
    const char *filename, 
    const char *header, 
    int color_num, 
    int graph_size, 
    const uint32_t colors[][BLOCK_INDEX(graph_size-1)+1]
);

int graph_color_greedy(
    int graph_size, 
    const uint32_t edges[], 
    uint32_t colors[][BLOCK_INDEX(graph_size-1)+1], 
    int max_color_possible
);

int count_conflicts(
    int graph_size, 
    const uint32_t color[], 
    const uint32_t edges[], 
    int conflict_count[]
);

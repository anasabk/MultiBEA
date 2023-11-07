#ifndef STDGRAPH_H
#define STDGRAPH_H


#include <stdlib.h>
#include <stdbool.h>


bool read_graph(const char* filename, int size, char edges[][size]);

bool read_weights(const char* filename, int size, int weights[]);

bool is_valid(int size, const char edges[][size], int color_num, const char colors[][size]);

int count_edges(int size, const char edges[][size], int count[]);

void print_colors(const char *filename, const char *header, int max_color_num, int size, const char colors[][size]);

int graph_color_greedy(int size, const char edges[][size], char colors[][size], int max_color_possible);

int count_conflicts(
    int size, 
    const char color[], 
    const char edges[][size], 
    // const int weights[], 
    // int competition[], 
    int conflict_count[]
);


#endif

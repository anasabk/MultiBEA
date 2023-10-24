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

#include "crossover.h"


int get_rand_color(int size, int colors_used, char used_list[]) {
    if(colors_used >= size - 2) {
        for(int i = 0; i < size; i++) {
            if(!used_list[i]) {
                used_list[i] = 1;
                return i;
            }
        }

        return -1;
    }

    int temp;
    while(1) {
        temp = rand()%size;
        if(!used_list[temp]) {
            used_list[temp] = 1;
            return temp;
        }
    }
}

void merge_colors(
    int size,
    const char *parent_color[2],
    char child_color[],
    char pool[],
    int *pool_total,
    char used_vertex_list[]
) {
    // Fill a color in the child.
    for(int i = 0; i < size; i++) {  // i = index of a vertex.
        if(!used_vertex_list[i]) {
            if(parent_color[0] != NULL)
                child_color[i] |= parent_color[0][i];

            if(parent_color[1] != NULL)
                child_color[i] |= parent_color[1][i];

            used_vertex_list[i] |= child_color[i];
        }

        if(pool[i]) {
            child_color[i] |= pool[i];
            *pool_total -= pool[i];
            pool[i] = 0;
        }
    }
}

void fix_conflicts(
    int size,
    const char edges[][size],
    const int num_of_edges[],
    char child_color[],
    char pool[],
    int *pool_total
) {
    int total_conflicts = 0;
    int conflict_count[size];
    memset(conflict_count, 0, size);

    // Check for any conflicts.
    int i, j;
    for(i = 0; i < size; i++) {   // i = index of the vertex to search for conflicts.
        if(child_color[i]) { // Check if the vertex i has this color i.
            for(j = i + 1; j < size; j++) {  // k = index of a vertex to check if it has an edge with i.
                if(child_color[j] && edges[i][j]) { // Check if the vertex k has the color i and has an edge with the vertex i.
                    conflict_count[i] += 1;
                    conflict_count[j] += 1;
                    total_conflicts += 2;
                }
            }
        }
    }

    int max_conflict_vert;
    while(total_conflicts > 0) {
        max_conflict_vert = 0;
        for(i = 0; i < size; i++){
            if(conflict_count[max_conflict_vert] < conflict_count[i])
                max_conflict_vert = i;

            else if(
                conflict_count[max_conflict_vert] == conflict_count[i] &&
                num_of_edges[max_conflict_vert] <= num_of_edges[i])
                max_conflict_vert = i;
        }

        child_color[max_conflict_vert] = 0;
        conflict_count[max_conflict_vert] = 0;
        pool[max_conflict_vert] = 1;
        (*pool_total)++;

        for(i = 0; i < size; i++){
            if(edges[max_conflict_vert][i] && child_color[i]) {
                conflict_count[i] -= 1;
                total_conflicts -= 2;
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
    char child[][size], 
    int max_color_possible) 
{
    int max_color_num = color_num1 > color_num2 ? color_num1 : color_num2;

    char used_color_list[2][max_color_num];
    memset(used_color_list, 0, 2*max_color_num);

    char used_vertex_list[size];
    memset(used_vertex_list, 0, size);

    int pool_total = 0;
    char pool[size];
    memset(pool, 0, size);

    char const *parent_color_p[2];
    int color1, color2, last_color = 0;
    int i, child_color;
    for(child_color = 0; child_color < max_color_num || (child_color < max_color_possible && pool_total > 0); child_color++) { // child_color = number of colors used, the index of the current color in the child.
        if(child_color < max_color_num) {
/*---------------------------------------  Merge the two colors and the pool in he new color  ---------------------------------------*/

            // Pick 2 random colors.
            color1 = get_rand_color(color_num1, child_color, used_color_list[0]);
            color2 = get_rand_color(color_num2, child_color, used_color_list[1]);

            if(color1 == -1)
                parent_color_p[0] = NULL;
            else
                parent_color_p[0] = parent1[color1];

            if(color2 == -1)
                parent_color_p[1] = NULL;
            else
                parent_color_p[1] = parent2[color2];

            merge_colors(
                size,
                parent_color_p,
                child[child_color],
                pool,
                &pool_total,
                used_vertex_list
            );


/*--------------------------------------  Resolve the conflicts and fill the pool accordingly  --------------------------------------*/

            fix_conflicts(
                size,
                edges,
                num_of_edges,
                child[child_color],
                pool,
                &pool_total
            );
        }

/*-------------------------------------------  Merge the pool with every previous color  --------------------------------------------*/

        if(pool_total > 0) {
            parent_color_p[0] = NULL;
            parent_color_p[1] = NULL;
            for(i = 0; i < child_color; i++) {
                merge_colors(
                    size,
                    parent_color_p,
                    child[i],
                    pool,
                    &pool_total,
                    used_vertex_list
                );

                fix_conflicts(
                    size,
                    edges,
                    num_of_edges,
                    child[i],
                    pool,
                    &pool_total
                );
            }
        }

        last_color = child_color;
    }

    return last_color + 1;
}

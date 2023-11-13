#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>

#include "common.h"


void print_progress(size_t count, size_t max) {
    const int bar_width = 50;

    float progress = (float) count / max;
    int bar_length = progress * bar_width;

    printf("\rProgress: [");
    for (int i = 0; i < bar_length; ++i) {
        printf("#");
    }
    for (int i = bar_length; i < bar_width; ++i) {
        printf(" ");
    }
    printf("] %.2f%%", progress * 100);

    fflush(stdout);
}

bool exists(int* arr, int len, int target) {
    for(int i = 0; i < len; i++)
        if(target == arr[i]) return true;

    return false;
}

int count_bits(uint32_t num) {
     num = num - ((num >> 1) & 0x55555555);        // add pairs of bits
     num = (num & 0x33333333) + ((num >> 2) & 0x33333333);  // quads
     num = (num + (num >> 4)) & 0x0F0F0F0F;        // groups of 8
     num *= 0x01010101;                        // horizontal sum of bytes
     return  num >> 24;               // return just that top byte (after truncating to 32-bit even when int is wider than uint32_t)
}

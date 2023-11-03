#include <stdlib.h>

#ifndef WITH_DEBUG
#define WITH_DEBUG 0
#endif

#define LOGD(fmt, ...) \
    do { if (WITH_DEBUG) fprintf(stderr, fmt, __VA_ARGS__); } while (0)

#define LOGDSTR(fmt) \
    do { if (WITH_DEBUG) fprintf(stderr, fmt); } while (0)


void print_progress(size_t count, size_t max);

bool exists(int* arr, int len, int target);

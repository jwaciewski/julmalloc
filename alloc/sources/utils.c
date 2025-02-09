#include <stdio.h>
#include <stdlib.h>

void a_abort(char *file, int line, char *expr) {
    fprintf(stderr, "Assertion %s in file %s:%d failed", expr, file, line);
    abort();
}

size_t round_up(size_t num_to_round, size_t multiple) {
    if (multiple == 0)
        return num_to_round;

    size_t remainder = num_to_round % multiple;
    if (remainder == 0)
        return num_to_round;

    return num_to_round + multiple - remainder;
}

size_t round_down(size_t num_to_round, size_t multiple) {
    if (multiple == 0)
        return num_to_round;

    size_t remainder = num_to_round % multiple;
    if (remainder == 0)
        return num_to_round;

    return num_to_round - remainder;
}
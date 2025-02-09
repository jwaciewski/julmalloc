#include "unittests/defines.h"
#include <alloc/defines.h>

#include <stdlib.h>

int main() {
    uint8_t *array[STORAGE_SIZE_TESTING];
    uint8_t *anchor = malloc(1);
    free(anchor);

    for (int i = 1; i <= STORAGE_SIZE_TESTING; i++) {
        for (int j = 0; j < STORAGE_SIZE_TESTING / i; i++) {
            array[i] = malloc(i);
        }
        size_t step_size = array[0] - array[1];
        for (int j = 0; j < STORAGE_SIZE_TESTING / i; i++) {
            free(anchor + j * step_size);
        }
    }

    return EXIT_SUCCESS;
}
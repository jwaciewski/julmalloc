#include "unittests/defines.h"
#include <alloc/defines.h>

#include <stdlib.h>

bool is_aligned(void *ptr) { return (uintptr_t)ptr % ALIGNMENT == 0; }

static bool is_empty(uint8_t *addr, size_t size) {
    pr_info("Checking is storage is empty");
    for (size_t i = 0; i < size; i++) {
        if (*(addr + i) != 0) {
            pr_error("Not zero!!! Urgently check your implementation");
            return false;
        }
        pr_info("Checked byte %zu", i);
    }
    return true;
}

int main() {
    uint8_t *array[STORAGE_SIZE_TESTING];
    uint8_t *anchor = malloc(1);
    free(anchor);

    for (int i = 1; i <= STORAGE_SIZE_TESTING; i++) {
        for (int j = 0; j < STORAGE_SIZE_TESTING / i; i++) {
            pr_info("Allocating elements of size i");
            array[i] = calloc(1, i);
            if (!is_empty(array[i], i)) {
                return EXIT_FAILURE;
            }
            if (!is_aligned(array[i])) {
                return EXIT_FAILURE;
            }
        }

        size_t step_size = array[0] - array[1];
        for (int j = 0; j < STORAGE_SIZE_TESTING / i; i++) {
            free(anchor + j * step_size);
        }
    }

    return EXIT_SUCCESS;
}
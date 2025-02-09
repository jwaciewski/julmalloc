#include "alloc/memory_mgmt.h"
#include "unittests/defines.h"
#include <alloc/defines.h>

#include <stdlib.h>

int remove_not_first_segment_end() {
    uint8_t *addr1 = malloc(1);
    uint8_t *addr2 = malloc(1);

    free(addr2);
    return EXIT_SUCCESS;
}

int remove_not_first_segment_middle() {
    uint8_t *addr1 = malloc(1);
    uint8_t *addr2 = malloc(1);
    uint8_t *addr3 = malloc(1);

    free(addr2);
    return EXIT_SUCCESS;
}

int remove_first_segment_not_the_only_segment() {
    uint8_t *addr1 = malloc(1);
    uint8_t *addr2 = malloc(1);

    free(addr1);
    return EXIT_SUCCESS;
}

int remove_first_segment_the_only_segment() {
    uint8_t *addr1 = malloc(1);

    free(addr1);
    return EXIT_SUCCESS;
}

// Whitebox-testing of remove-entry function
int main() {
    set_alloc_function(FIRST_FIT);

    if (remove_not_first_segment_end()) {
        return EXIT_FAILURE;
    }

    clear_alloc_storage();

    if (remove_not_first_segment_middle()) {
        return EXIT_FAILURE;
    }

    clear_alloc_storage();

    if (remove_first_segment_not_the_only_segment()) {
        return EXIT_FAILURE;
    }

    clear_alloc_storage();

    if (remove_first_segment_the_only_segment()) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
#include "alloc/memory_mgmt.h"
#include "unittests/defines.h"
#include <alloc/defines.h>

#include <stdlib.h>

int alloc_empty_list() {

    // See what happens when allocating on an empty list
    for (int i = 1; i < STORAGE_SIZE_TESTING; i++) {
        uint8_t *addr = malloc(1);
        if (!addr) {
            pr_error("Invalid alloc");
            return EXIT_FAILURE;
        }

        free(addr);
    }
    return EXIT_SUCCESS;
}

int alloc_before_previous_segment() {

    uint8_t *addr1 = malloc(1);
    uint8_t *addr2 = malloc(1);
    free(addr1);
    if (malloc(1) != addr1) {
        pr_error("Invalid alloc");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int alloc_after_previous_segment() {

    uint8_t *addr1 = malloc(1);
    uint8_t *addr2 = malloc(1);
    free(addr2);

    if (malloc(1) != addr2) {
        pr_error("Invalid alloc");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int alloc_between_two_segments() {

    uint8_t *addr1 = malloc(1);
    uint8_t *addr2 = malloc(1);
    uint8_t *addr3 = malloc(1);

    free(addr2);

    if (malloc(1) != addr2) {
        pr_error("Invalid alloc");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int alloc_in_the_middle_of_empty_table() {

    uint8_t *addr1 = malloc(3);

    free(addr1);

    if (!add_entry(addr1 - sizeof(struct seg_head_s) + 1, 1)) {
        pr_error("Invalid alloc");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int alloc_in_the_middle_between_2_segments() {

    uint8_t *addr1 = malloc(1);
    uint8_t *addr2 = malloc(2);
    uint8_t *addr3 = malloc(1);

    free(addr2);

    if (!add_entry(addr2 - sizeof(struct seg_head_s) + 1, 1)) {
        pr_error("Invalid alloc");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// Whitebox-testing of add-entry function
int main() {
    set_alloc_function(FIRST_FIT);

    if (alloc_empty_list()) {
        return EXIT_FAILURE;
    }

    clear_alloc_storage();

    if (alloc_before_previous_segment()) {
        return EXIT_FAILURE;
    }

    clear_alloc_storage();

    if (alloc_after_previous_segment()) {
        return EXIT_FAILURE;
    }

    clear_alloc_storage();

    if (alloc_between_two_segments()) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
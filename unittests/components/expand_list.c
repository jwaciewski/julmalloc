#include "alloc/memory_mgmt.h"
#include "unittests/defines.h"
#include <alloc/defines.h>

#include <stdlib.h>

int expand_empty_list() {
    pr_info("Testing expanding of empty list");
    // Let's expand an empty list a few times and see what happens
    for (int i = 1; i < STORAGE_SIZE_TESTING; i++) {
        uint8_t *addr = malloc(i);
        if (!addr) {
            pr_error("Invalid alloc");
            return EXIT_FAILURE;
        }
        free(addr);
        clear_alloc_storage();
    }
    return EXIT_SUCCESS;
}

int expand_full_list() {
    pr_info("Testing expanding of full list");

    // Continuously malloc elements so that the full list needs to be expanded
    // every time
    for (int i = 1; i < STORAGE_SIZE_TESTING; i++) {
        uint8_t *addr = malloc(i);
        if (!addr) {
            pr_error("Invalid alloc");
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

int expand_not_full_list() {
    pr_info("Testing expanding of not full list");

    // Malloc elements, then free at the end so that there is not a large enough
    // gap
    uint8_t *addr = malloc(2);
    if (!addr) {
        pr_error("Invalid alloc");
        return EXIT_FAILURE;
    }
    free(addr);

    // We add an anchor element
    addr = malloc(1);
    if (!addr) {
        pr_error("Invalid alloc");
        return EXIT_FAILURE;
    }

    // Now there is a gap of size 1 after the anchor element

    // In each iteration, we allocate an element which is just too large by 1
    // byte, so the list needs to be expanded every time
    for (int i = 2; i < STORAGE_SIZE_TESTING; i++) {
        uint8_t *addr2 = malloc(i);
        if (!addr) {
            pr_error("Invalid alloc");
            return EXIT_FAILURE;
        }
        free(addr2);
    }
    return EXIT_SUCCESS;
}

int expand_free_existing_list() {
    pr_info("Testing expanding of free existing list");

    // Check what happens if we try to allocate on an empty list which is 1 too
    // small for the desired size

    // In each iteration, we allocate an element which is just too large by 1
    // byte, so the list needs to be expanded every time to fit
    for (int i = 1; i < STORAGE_SIZE_TESTING; i++) {
        uint8_t *addr = malloc(i);
        if (!addr) {
            pr_error("Invalid alloc");
            return EXIT_FAILURE;
        }
        free(addr);
    }
    return EXIT_SUCCESS;
}

// Whitebox-testing of expand-list function
int main() {
    set_alloc_function(FIRST_FIT);

    if (expand_empty_list()) {
        return EXIT_FAILURE;
    }

    clear_alloc_storage();

    if (expand_full_list()) {
        return EXIT_FAILURE;
    }

    clear_alloc_storage();

    if (expand_not_full_list()) {
        return EXIT_FAILURE;
    }

    clear_alloc_storage();

    if (expand_free_existing_list()) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
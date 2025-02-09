#include "alloc/defines.h"
#include "alloc/memory_mgmt.h"
#include "alloc/strats.h"

#include "unittests/defines.h"
#include <stdlib.h>

bool is_aligned(void *ptr) { return (uintptr_t)ptr % ALIGNMENT == 0; }

size_t sum_aligned(size_t num_gaps) {
    size_t sum = 0;

    for (size_t i = 1; i <= num_gaps; i++) {
        sum += (size_t)ceil((double)i / ALIGNMENT) * ALIGNMENT;
    }

    ASSERT(sum % ALIGNMENT == 0);

    return sum;
}

// Create a grid of gap sizes ascending by 1, separated by allocated memory of
// size 1
// Like this:
// 1 * 2 * 3 * ...
// where the number denotes the gap size and * denotes allocated storage of size
// 1
// If next_fit works correctly, the proper gap should be allocated
int grid_test_simple() {
    set_alloc_function(FIRST_FIT);
    size_t num_gaps = 0;
    uint8_t *anchor = malloc(1);
    ASSERT(is_aligned(anchor));
    free(anchor);
    anchor -= sizeof(struct seg_head_s);

    // Measure maximum number of gaps. The last gap has to be strictly larger
    // than the previous ones
    while (sum_aligned(num_gaps + 1) + num_gaps * ALIGNMENT +
               (2 * num_gaps + 1) *
                   (sizeof(struct seg_head_s) + sizeof(struct seg_tail_s)) <=
           STORAGE_SIZE_TESTING) {
        num_gaps++;
    }

    uint8_t *segments[num_gaps];

    // Add "barriers between gaps"
    for (size_t i = 0; i < num_gaps - 1; i++) {
        segments[i] = malloc(i + 1);
        uint8_t *barrier = malloc(1);
        ASSERT(is_aligned(segments[i]) && is_aligned(barrier));

        pr_info("%d", barrier - segments[i]);
        ASSERT(barrier == segments[i] + round_up(i + 1, ALIGNMENT) +
                              sizeof(struct seg_head_s) +
                              sizeof(struct seg_tail_s))
        ASSERT(segments[i] ==
               (uint8_t *)anchor + (sum_aligned(i) + i * ALIGNMENT +
                                    (2 * i) * (sizeof(struct seg_head_s) +
                                               sizeof(struct seg_tail_s)) +
                                    sizeof(struct seg_head_s)));
        ASSERT(barrier ==
               (uint8_t *)anchor + (sum_aligned(i + 1) + i * ALIGNMENT +
                                    (2 * i + 1) * (sizeof(struct seg_head_s) +
                                                   sizeof(struct seg_tail_s)) +
                                    sizeof(struct seg_head_s)));
    }

    segments[num_gaps - 1] = malloc(num_gaps);
    ASSERT(segments[num_gaps - 1] ==
           (uint8_t *)anchor +
               (sum_aligned(num_gaps - 1) + (num_gaps - 1) * ALIGNMENT +
                (2 * (num_gaps - 1)) *
                    (sizeof(struct seg_head_s) + sizeof(struct seg_tail_s)) +
                sizeof(struct seg_head_s)));
    ASSERT(is_aligned(segments[num_gaps - 1]));

    for (size_t i = 0; i < num_gaps; i++) {
        free(segments[i]);
    }

    set_alloc_function(NEXT_FIT);
    set_last_addr(nullptr);

    // Allocate num_gaps many elements of ascending size and check if they
    // land in the right grid
    for (size_t i = 0; i < num_gaps; i++) {
        uint8_t *addr = malloc(i + 1);
        ASSERT(addr ==
               (uint8_t *)anchor + (sum_aligned(i) + i * ALIGNMENT +
                                    (2 * i) * (sizeof(struct seg_head_s) +
                                               sizeof(struct seg_tail_s)) +
                                    sizeof(struct seg_head_s)));
        ASSERT(is_aligned(addr));
        // free(addr);
    }

    return EXIT_SUCCESS;
}

// Similarly to grid_test_simple, but here is the constraint: Only allocate
// elements of size 1 inbetween gaps, they should be put in the proper gap (and
// *not* the first gap!)
int grid_test_complex() {
    set_alloc_function(NEXT_FIT);
    size_t num_gaps = 0;
    uint8_t *anchor = malloc(1);
    ASSERT(is_aligned(anchor));
    free(malloc(STORAGE_SIZE_TESTING));

    anchor += ALIGNMENT + sizeof(struct seg_tail_s);

    // Measure maximum number of gaps. The last gap has to be strictly larger
    // than the previous ones
    while (sum_aligned(num_gaps + 1) + num_gaps * ALIGNMENT +
               (2 * num_gaps + 1) *
                   (sizeof(struct seg_head_s) + sizeof(struct seg_tail_s)) <=
           STORAGE_SIZE_TESTING) {
        num_gaps++;
    }

    // Add "barriers between gaps"
    for (size_t i = 0; i < num_gaps - 1; i++) {
        uint8_t *addr = malloc(i + 1);
        uint8_t *barrier = malloc(1);
        ASSERT(is_aligned(addr) && is_aligned(barrier));

        // free(addr);
        uint8_t *test = malloc(1);

        ASSERT(addr ==
               (uint8_t *)anchor + (sum_aligned(i) + i * ALIGNMENT +
                                    (2 * i) * (sizeof(struct seg_head_s) +
                                               sizeof(struct seg_tail_s)) +
                                    sizeof(struct seg_head_s)));
        ASSERT(barrier ==
               (uint8_t *)anchor + (sum_aligned(i + 1) + i * ALIGNMENT +
                                    (2 * i + 1) * (sizeof(struct seg_head_s) +
                                                   sizeof(struct seg_tail_s)) +
                                    sizeof(struct seg_head_s)));
        ASSERT(is_aligned(test));

        pr_info("Address of barrier: %p", barrier);
        pr_info("Address of p: %p", test);

        ASSERT(test == (uint8_t *)anchor +
                           (sum_aligned(i + 1) + (i + 1) * ALIGNMENT +
                            (2 * (i + 1)) * (sizeof(struct seg_head_s) +
                                             sizeof(struct seg_tail_s)) +
                            sizeof(struct seg_head_s)));
        free(test);
    }

    return EXIT_SUCCESS;
}

// Check if next_addr gets updated correctly on empty list
int next_fit_two_segments() {

    set_alloc_function(NEXT_FIT);
    set_last_addr(nullptr);
    uint8_t *addr = malloc(1);
    uint8_t *addr2 = malloc(1);
    ASSERT(is_aligned(addr) && is_aligned(addr2));
    if (addr2 != addr + ALIGNMENT + sizeof(struct seg_tail_s) +
                     sizeof(struct seg_head_s)) {
        pr_error("last_addr pointer did not got updated correctly");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// Check if next_addr searches for gap after last allocated storage
int next_fit_after_last() {

    // Create two large gaps, then allocate one element in the middle of the
    // gap. Check if malloc allocates after that element and not before
    set_alloc_function(NEXT_FIT);
    set_last_addr(nullptr);

    free(malloc(200));

    uint8_t *addr = malloc(100);
    uint8_t *addr2 = malloc(1);
    ASSERT(is_aligned(addr) && is_aligned(addr2));
    free(addr);

    if (malloc(1) != addr2 + ALIGNMENT + sizeof(struct seg_tail_s) +
                         sizeof(struct seg_head_s)) {
        pr_error("last_addr started searching at the beginning of table, not "
                 "at the last allocated segment");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// Check if next_addr works like first_fit when reaching end of table
int next_fit_after_last_first() {

    // Create two large gaps, then allocate one element in the middle of the
    // gap. Check if malloc allocates after that element and not before
    set_alloc_function(NEXT_FIT);
    set_last_addr(nullptr);

    free(malloc(PAGE_SIZE -
                (sizeof(struct seg_head_s) + sizeof(struct seg_tail_s))));

    uint8_t *addr = malloc(PAGE_SIZE / 2);
    uint8_t *addr2 = malloc(1);
    ASSERT(is_aligned(addr) && is_aligned(addr2));
    free(addr);

    if (malloc(PAGE_SIZE / 2) != addr) {
        pr_error("malloc did something strange, like expanding the table "
                 "instead of searching for a gap in the beginning. Check "
                 "next_fit implementation");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int main() {

    if (grid_test_simple()) {
        return EXIT_FAILURE;
    }

    clear_alloc_storage();

    if (next_fit_two_segments()) {
        return EXIT_FAILURE;
    }

    clear_alloc_storage();

    if (next_fit_after_last()) {
        return EXIT_FAILURE;
    }

    clear_alloc_storage();

    if (next_fit_after_last_first()) {
        return EXIT_FAILURE;
    }

    clear_alloc_storage();

    if (grid_test_complex()) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
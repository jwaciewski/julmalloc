#include "unittests/defines.h"
#include <alloc/defines.h>

#include <stdlib.h>

bool is_aligned(void *ptr) { return (uintptr_t)ptr % ALIGNMENT == 0; }

int main() {
    uint8_t *addr = malloc(STORAGE_SIZE_TESTING);
    uint8_t *addr2 = malloc(STORAGE_SIZE_TESTING);

    if (!addr) {
        return EXIT_FAILURE;
    }
    for (size_t i = 1; i < STORAGE_SIZE_TESTING; i++) {
        if (realloc(addr, STORAGE_SIZE_TESTING - i) != addr) {
            pr_error("Invalid realloc");
            return EXIT_FAILURE;
        }
        if (realloc(addr2, STORAGE_SIZE_TESTING - i) != addr2) {
            pr_error("Invalid realloc");
            return EXIT_FAILURE;
        }
        if (!is_aligned(addr) || !is_aligned(addr2)) {
            return EXIT_FAILURE;
        }
    }

    for (size_t i = 2; i <= STORAGE_SIZE_TESTING; i++) {
        if (realloc(addr, i) != addr) {
            pr_error("Invalid realloc");
            return EXIT_FAILURE;
        }
        if (realloc(addr2, i) != addr2) {
            pr_error("Invalid realloc");
            return EXIT_FAILURE;
        }
        if (!is_aligned(addr) || !is_aligned(addr2)) {
            return EXIT_FAILURE;
        }
    }

    free(addr);
    free(addr2);

    for (size_t i = 1; i < STORAGE_SIZE_TESTING / 2 - ALIGNMENT; i++) {
        pr_info("Allocating segment of size %zu", i);
        uint8_t *temp = malloc(round_up(i, ALIGNMENT));
        if (!is_aligned(temp)) {
            return EXIT_FAILURE;
        }
        for (size_t k = 0; k < round_up(i, ALIGNMENT); k++) {
            *(temp + k) = k % 256;
        }
        uint8_t *barrier = malloc(1);
        if (!is_aligned(barrier)) {
            return EXIT_FAILURE;
        }
        pr_info("Reallocating segment to size %zu", i + 1);
        uint8_t *new_addr = realloc(temp, round_up(i, ALIGNMENT) + 1);
        if (!new_addr || new_addr == temp) {
            pr_error("Invalid realloc");
            return EXIT_FAILURE;
        }
        if (!is_aligned(new_addr)) {
            return EXIT_FAILURE;
        }
        for (size_t k = 0; k < round_up(i, ALIGNMENT); k++) {
            if (*(new_addr + k) != k % 256) {
                pr_error("Copy error");
                return EXIT_FAILURE;
            }
        }
        free(new_addr);
        free(barrier);
    }

    addr = malloc(1);

    for (size_t i = 2; i <= STORAGE_SIZE_TESTING * 10; i++) {
        if (!(addr = realloc(addr, i))) {
            pr_error("Invalid realloc");
            return EXIT_FAILURE;
        }
    }
}
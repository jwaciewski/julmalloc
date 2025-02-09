#include "alloc/defines.h"
#include "alloc/methods.h"

#include "math.h"
#include <stdlib.h>

// This unit test will pass if EXIT_FAILURE is called.
int main(int argc, char *argv[]) {

    // Test freeing of nullptr. Should continue without modifying pointer.
    void *ptr = nullptr;

    free(ptr);

    if (ptr) {

        pr_error("Pointer has been modified");

        return EXIT_SUCCESS;
    }

    free((void *)1);

    return EXIT_SUCCESS;
}
#include "alloc/defines.h"
#include "alloc/methods.h"

#include "math.h"
#include <stdlib.h>

// This unit test will pass if EXIT_FAILURE is called.
int main(int argc, char *argv[]) {

    realloc((void *)1, 5);

    return EXIT_SUCCESS;
}
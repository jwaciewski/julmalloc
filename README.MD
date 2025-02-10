  # Introduction

A malloc() implementation, written from scratch in C.

This library implements the malloc(), calloc(), free() and realloc() functions according to the C23 standard. No prior code is used.

The library is thread-safe, which is ensured through the usage of a mutex.

Storage is allocated in a linked list of chunks. Each chunk consists of a header, the payload (that is, usable space for the caller of malloc()), and a tail. The header contains information about the payload size, the tail contains information about the number of free bytes until the next chunk.
The header contains pointers to the next following tail, and the previous tail. The tail contains information to the next following header, and the previous header.
Storage is allocated on one arena (that is, one area of storage) which consists of one heap, the main heap. The arena consists of an arena header, which contains basic information about the arena, such as a pointer to the first chunk, and a pointer to the end of the arena.
The arena is expanded, and contracted, in pages of size 4096 bytes. The rationale for expanding in pages, compared to, say, per-chunk sizes, is to avoid frequent, expensive system calls to sbrk().

The library supports allocation with four different allocation strategies, first-fit, next-fit, best-fit and worst-fit. Benchmarks have shown that next-fit is by far the fastest implementation. On default, first-fit is set as an allocation strategy.

# Build instructions

The project can be compiled with e.g. gcc, especially using CMAKE.

To build with cmake, run the following command:
```
cmake -B build
```
This will set up the cmake configuration files in the folder `build`, assuming the folder exists. To then compile the project, enter the build folder and run `make`

# Usage

Cmake will create a shared library in the folder `build/alloc`. There will be according version symbolic links pointing to the actual library.

It is not sufficient to statically compile link the library, as the malloc()-... functions would be overriden if <stdlib.h> is included. To load the library properly, you need to set the the LD_PRELOAD environment variable as follows:
```
export LD_PRELOAD=/path/to/shared/library

./[your_program]
```
If you first export the environment variable, your program ./[your_program] will use the malloc()-implementations from the shared library, even if <stdlib.h> is loaded. If you don't want to set the environment variable with export, you can run:
```
LD_PRELOAD=/path/to/shared/library ./[your_program]
```
which will set the environment variable only for this execution.

If you want to test the library in a real environment, it is useful to add a
```
#DEFINE NDEBUG
```
define in alloc/defines.h so that warnings and assertions won` get executed.

# Testing

This library includes a predefined set of tests which will be build with CMAKE. To execute all tests, from the build directory, run
```
ctest
```
This command will execute all tests sequentially.

You can also run all tests with
```
make test
```
# Documentation

To generate a documentation, run
```
doxygen
```
The corresponding doxygen files will be generated in the `doxygen` folder and can be accessed by opening `index.html` in a web browser

# COPYRIGHT

Copyright information are provided in the LICENSE file




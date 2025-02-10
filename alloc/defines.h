/**
 * @file
 * @brief Defines for memory allocation
 */
#ifndef ALLOC_DEFINES_H
#define ALLOC_DEFINES_H

#include "alloc/types.h"
#include "alloc/utils.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>

//! The storage will only be expanded in segments of PAGE_SIZE to avoid frequent
//! brk/sbrk calls.
#define PAGE_SIZE 4096

// #define NDEBUG

// The storage addresses need to be aligned. This macro contains the largest
// align supported by the OS for some data type. According to the malloc()
// definition in C23, addresses returned by malloc() need to be "suitably
// aligned", which we interpret as they need to be aligned to a multiple equal
// or larger than _Alignof(max_align_t). All returned addresses by malloc will
// be aligned to ALIGNMENT, that is (intptr_t)malloc(...) % ALIGNMENT == 0

// Note this value is only used for checking on 64 bit. The alignments are
// hardcoded for 64 bit (16 byte) for simplicity. Since the alignments for
// 64-bit are larger than for 32-bit anyways due to larger primitive data types,
// this is not problematic.
#define ALIGNMENT _Alignof(max_align_t)

//! Macro for LD_PRELOAD asserts because normal assert somehow don't work
#ifndef NDEBUG
#define ASSERT(expr)                                                           \
    if (!(expr)) {                                                             \
        a_abort(__FILE__, __LINE__, #expr);                                    \
    }
#else
#define ASSERT(expr)                                                           \
    do {                                                                       \
    } while (0);
#endif

// ERROR define for returns
#define ERROR -1

// SUCCESS define for returns
#define SUCCESS 0

//! ANSI control sequence to reset foreground color.
#define ANSI_RESET "\x1b[0m"
//! ANSI color code for red.
#define ANSI_RED "\x1b[0;31m"
//! ANSI color code for yellow.
#define ANSI_YELLOW "\x1b[0;33m"
//! ANSI color code for green.
#define ANSI_GREEN "\x1b[0;32m"
//! ANSI color code for blue.
#define ANSI_BLUE "\x1b[0;34m"

//! Various output macros
#define pr_error(format, ...)                                                  \
    (fprintf(stderr, ANSI_RED "[ERROR] (%s:%d) %s:" format ANSI_RESET "\n",    \
             __FILE__, __LINE__, __func__ __VA_OPT__(, ) __VA_ARGS__))
#ifdef NDEBUG
#define pr_warning(format, ...) ((void)0)
#define pr_info(format, ...) ((void)0)
#else
#define pr_warning(format, ...)                                                \
    (fprintf(stderr, ANSI_YELLOW "[WARN] (%s:%d) %s:" format ANSI_RESET "\n",  \
             __FILE__, __LINE__, __func__ __VA_OPT__(, ) __VA_ARGS__))
#define pr_info(format, ...)                                                   \
    (fprintf(stderr, ANSI_GREEN "[INFO] (%s:%d) %s:" format ANSI_RESET "\n",   \
             __FILE__, __LINE__, __func__ __VA_OPT__(, ) __VA_ARGS__))
#endif

#endif
/**
 * @file
 * @brief Allocation related typedefs
 */
#ifndef ALLOC_TYPES_H
#define ALLOC_TYPES_H

#include "alloc/defines.h"
#include "alloc/types.h"
#include <stddef.h>
#include <stdint.h>

typedef struct seg_list_head_s {
    struct seg_head_s
        *first_seg;    /**< Pointer to first allocated segment header. */
    uint8_t *end_addr; /**< Pointer to end of storage table (not segment, just
                          an address). Any address up to this address is valid
                          to read*/
} seg_list_head_s;

typedef struct seg_tail_s {
    struct seg_head_s *next_seg_head; /**< Pointer to head of next segment */
    struct seg_head_s *prev_seg_head; /**< Pointer to head of current segment */
    size_t free_following; /**< Number of free bytes till next segment header or
                              end of storage table (end_addr) */
    char pad[8];
} seg_tail_s;

typedef struct seg_head_s {
    struct seg_tail_s *next_seg_tail; /**< Pointer to tail of current segment */
    struct seg_tail_s
        *prev_seg_tail; /**< Pointer to tail of previous segment */
    size_t seg_size;    /**< Number of actually usable bytes between header and
                           tail for user */
    char pad[8];
} seg_head_s;

typedef enum sched_strat_e {
    FIRST_FIT, /**< First-Fit strategs */
    NEXT_FIT,  /**< Next-Fit strategy */
    BEST_FIT,  /**< Best-Fit strategy */
    WORST_FIT  /**< Worst-Fit strategx */
} sched_strat_e;

//! Function pointer to allocator function being used
typedef uint8_t *(*alloc_function)(seg_list_head_s *, size_t);

#endif
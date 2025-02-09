/**
 * @file
 * @brief Functions operating on linked list
 */
#ifndef ALLOC_MEMORY_MGMT_H
#define ALLOC_MEMORY_MGMT_H

#include "alloc/types.h"
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Allocates storage in the storage table
 *
 * This function searches for gaps with one of the memory allocation algorithms
 *(First, Next, Best,
 * Worst etc.). If no gap is found, the table is expanded with sbrk() and a
 *suitable beginning of free segment is returned. It sets up the initial
 *structures (Table header) on beginning
 *
 * @note This function does not actually allocate anything, for this, call
 *add_entry()
 *
 * @attention This function returns the beginning of a possible *segment header*
 *and ***NOT!!!*** the beginning of possible usable storage for the caller
 *
 * @param[in] size Size of new segment
 *
 * @return Address of gap where there is at least size many space free (not
 * allocated yet).
 *
 */
uint8_t *find_free_seg(size_t size);

/**
 * @brief Remove segment
 *
 * This function removes a segment by changing all corresponding parameters of
 * the previous and subsequent blocks. It also handles edge cases like: Storage
 * Block is first block, last block, the only block left (in this case the table
 * header "segment header" entry will be set to nullptr).
 *
 * @warning If addr does not point to a valid segment, expect undefined
 * behaviour
 *
 * @param[in] addr A suitable parameter pointing to the beginning of some
 * segment.
 *
 */
void remove_segment(uint8_t *addr);

/**
 * @brief Shrinks a segment
 *
 * This function shrinks a given segment by moving the old segment tail closer
 * to the header and reducing the number of segment bytes in the segment header.
 *
 * @note Useful for realloc
 *
 * @warning If addr does not point to a valid address, expect undefined
 * behaviour
 *
 * @param[in] addr Address of valid segment
 * @param[in] size The size to shrink *by*, that is afterwards the segment will
 * be size bytes smaller
 *
 * @return SUCCESS on success, ERROR otherwise
 */
int shrink_segment(uint8_t *addr, size_t size);

/**
 * @brief Expands a segment by some bytes
 *
 * This function expands an already existing segment by @p size bytes by
 * shifting the tail further away from the segment header.a_abort
 *
 * @note Useful for realloc
 * @warning This function only works if there is actually enough space past the
 * segment tail
 *
 * @param[in] addr Address of valid segment
 * @param[in] size Size to expand by, that is afterwards the segment will have
 * size more bytes
 *
 * @return SUCCESS on success, ERROR otherwise
 *
 */
int expand_segment(uint8_t *addr, size_t size);

/**
 * @brief Allocates a segment
 *
 * This function does the actually allocation by adding an entry to the segment
 * list. It does so by adding a header and tail infront and after @p addr with
 * necessary information, such as: Size of segment, size of subsequent free
 * space, adjacent segments etc.a_abort
 *
 * @warning If addr is not a valid address, expect undefined behaviour
 *
 * @param[in] addr Valid address with sufficient space where segment should be
 * allocated
 * @param[in] size Size of new segment (that is, without header and tail size)
 *
 * @return Address of new segment. This is ***NOT*** addr, because addr points
 * to the *header* of the new segment. This address is of no use for the user,
 * though, as such the return value is shifted by sizeof(struct seg_head_s).
 *
 */
uint8_t *add_entry(uint8_t *addr, size_t size);

/**
 * @brief Change alloc function
 *
 * This function changes the alloc function. Currently, NEXT_FIT, BEST_FIT,
 * WORST_FIT and FIRST_FIT are supported.
 *
 * @note Only useful for testing purposes
 *
 * @param[in] strat Enum constant of the corresponding memory strategy
 *
 */
void set_alloc_function(sched_strat_e strat);

/**
 * @brief Destroy allocated storage
 *
 * This function unallocates all storage at once by erasing the corresponding
 * table. In the future, sbrk will also be called
 *
 */
void clear_alloc_storage();

#endif
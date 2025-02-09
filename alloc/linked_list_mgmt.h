/**
 * @file
 * @brief Functions operating on linked list
 */
#ifndef ALLOC_TABLEOPTS_H
#define ALLOC_TABLEOPTS_H

#include "alloc/types.h"
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Get size of segment
 *
 * This function gets the size of an allocated segment by reading out the header
 * information
 *
 * @note If addr is an invalid address, expect undefined behaviour
 *
 * @param[in] addr Address of segment
 *
 * @return Size of segment
 */
size_t get_segment_size(uint8_t *addr);

/**
 * @brief Get subsequent gap size of segment
 *
 * This function reads out the free_following parameter from the segment tail
 * until the storage end.
 *
 * @note If addr is an invalid address, expect undefined behaviour
 *
 * @param[in] addr Address of segment
 *
 * @return Size of subsequent gap
 */
size_t get_following_gap_size(uint8_t *addr);

/**
 * @brief Get previous tail of segment
 *
 * This function fetches the tail of the last allocated storage in the segment
 *
 * @param[in] list A pointer to a storage list
 *
 * @return Tail of last allocated segment
 */
seg_tail_s *find_last_tail(seg_list_head_s *list);

/**
 * @brief Initialize a new storage linked list
 *
 * This function sets up the seg_list_head_s structure in the beginning of the
 * table for management, as well as setting the pointers in it.aligned_alloc
 *
 * @return Pointer to seg_list_head_s, nullptr on sbreak()-error
 */
seg_list_head_s *create_list();

/**
 * @brief Fetch previous segment header or table header of any given address
 *
 * This function first tires to fetch the header of the segment before the
 * current addr. If no segment is before addr, it means the only segment before
 * addr is pointing to is the header
 *
 * @warning The user needs to check if the returned value is a segment header,
 * or the table header. This can be done by comparing the addresses with the
 * table header addresses.
 *
 * @param[in] list A pointer to a storage list
 * @param[in] addr Some address (does not have to point to a segment beginning)
 *
 * @return Address of table header or header of previous segment
 *
 */
uint8_t *get_prev_reference(seg_list_head_s *list, uint8_t *addr);

/**
 * @brief Reset the list
 *
 * This function clears the first segment entry in @p list and sets the tail to
 * the address directly after @p list, that is after the storage table header.
 *
 * @param[in] list A list to be resetted
 * @return SUCCESS on success, ERROR on error.
 */
int reset_list(seg_list_head_s *list);

#endif

/**
 * @file
 * @brief Module to change the currently used memory algorithm
 */
#ifndef ALLOC_STRATS_H
#define ALLOC_STRATS_H

#include "alloc/types.h"

/**
 * @brief A best-fit implementation
 *
 * Performs a best-fit search. For more details see the comments in the
 * function.
 *
 * @param[in] list A pointer to a storage list header
 * @param[in] size A gap size to search for (size means user space size
 * excluding chunk header and chunk tail size. This will be considered in the
 * function)
 *
 * @return Address where a valid chunk of size @p size can be placed (not the
 * address where user storage begins), nullptr if no gap has been found or some
 * other error occured.
 */
uint8_t *best_fit(seg_list_head_s *list, size_t size);

/**
 * @brief A worst-fit implementation
 *
 * Performs a worst-fit search. For more details see the comments in the
 * function.
 *
 * @param[in] list A pointer to a storage list header
 * @param[in] size A gap size to search for (size means user space size
 * excluding chunk header and chunk tail size. This will be considered in the
 * function)
 *
 * @return Address where a valid chunk of size @p size can be placed (not the
 * address where user storage begins), nullptr if no gap has been found or some
 * other error occured.
 */
uint8_t *worst_fit(seg_list_head_s *list, size_t size);

/**
 * @brief A first-fit implementation
 *
 * Performs a first-fit search. For more details see the comments in the
 * function.
 *
 * @param[in] list A pointer to a storage list header
 * @param[in] size A gap size to search for (size means user space size
 * excluding chunk header and chunk tail size. This will be considered in the
 * function)
 *
 * @return Address where a valid chunk of size @p size can be placed (not the
 * address where user storage begins), nullptr if no gap has been found or some
 * other error occured.
 */
uint8_t *first_fit(seg_list_head_s *list, size_t size);

/**
 * @brief A next-fit implementation
 *
 * Performs a best-fit search. For more details see the comments in the
 * function.
 *
 * @param[in] list A pointer to a storage list header
 * @param[in] size A gap size to search for (size means user space size
 * excluding chunk header and chunk tail size. This will be considered in the
 * function)
 *
 * @return Address where a valid chunk of size @p size can be placed (not the
 * address where user storage begins), nullptr if no gap has been found or some
 * other error occured.
 */
uint8_t *next_fit(seg_list_head_s *list, size_t size);

/**
 * @brief Set the address of last addr pointer
 *
 * This function updates the last_addr pointer in case of new allocation or
 * freeing of blocks. Will be called by said functions
 *
 * @param[in] addr Some tail address, or nullptr if no segment is left anymore
 */
void set_last_addr(seg_tail_s *addr);

/**
 * @brief Returns the address of last addr pointer
 *
 * This function returns the last_addr pointer in case of freeing of blocks.
 * Will be called by said functions.
 *
 */
seg_tail_s *get_last_addr();

#endif

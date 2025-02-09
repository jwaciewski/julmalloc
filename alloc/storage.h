/**
 * @file
 * @brief Module for storage drivers
 */
#ifndef ALLOC_STORAGE_H
#define ALLOC_STORAGE_H

#include "alloc/types.h"

/** @brief Reads byte where address points to
 *
 * Given an address @p addr, this function reads the byte value where addr
 * points to
 *
 * @param[in] addr An address
 * @return Value of byte where address points to
 *
 */
uint8_t read_byte(const uint8_t *addr);

/** @brief Sets byte where address points to
 *
 * Given an address @p addr, this function sets the byte where @p addr points to
 * to @p v
 *
 * @param[in] addr An address
 * @param[in] value A byte value
 *
 */
void set_byte(uint8_t *addr, uint8_t v);

/**
 * @brief Sets a storage segment to zero
 *
 * This function sets @p size many bytes on a given address to
 * zero.aligned_alloc
 *
 * @warning This function does not check if the given address is valid
 *
 * @param[in] addr An address of a storage segment beginning to be set to zero
 * @param[in] size The number of bytes to be set.
 *
 * @return SUCCESS on success, ERROR on error.
 */
int set_mem_zero(uint8_t *addr, size_t size);

/**
 * @brief Copies bytes from one address to another
 *
 * Given two addresses, an old one and a new one, and a size, this function
 * copies @p size many bytes from @p old_addr to @new_addr
 *
 * @warning No boundary conditions are checked, make sure addresses are valid
 *
 * @param[in] old_addr Address of storage segment to be copied from
 * @param[in] new_addr Address of storage segment to be copied to
 * @param[in] size Number of bytes to be copied
 *
 * @return  SUCCESS on success, ERROR otherwise
 */
int copy_mem(uint8_t *old_addr, uint8_t *new_addr, size_t size);

#endif

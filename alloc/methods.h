/**
 * @file
 * @brief Module for allocation function clones
 */
#ifndef ALLOC_METHODS_H
#define ALLOC_METHODS_H

#include <stddef.h>

/** @brief A malloc clone
 *
 * This function acts like malloc. It allocates spaces of size @p size if
 * size>0, otherwise it returns nullptr. For more documentation see malloc.
 *
 * @param[in] size  Size of space to be allocated
 * @return If malloc managed to allocate space, return pointer to beginning of
 * such space. Otherwise return nullptr.
 *
 */
void *malloc(size_t size);

/** @brief A free clone
 *
 * This function acts like free(). It frees valid spaces and causes a
 * termination of the program on invalid access. For more documentation see
 * free().
 *
 * @param[in] ptr  Pointer pointing to start of segment to be deallocated
 *
 */
void free(void *ptr);

/** @brief A malloc clone
 *
 * This function acts like calloc(). It allocates spaces of size @p size if
 * size>0 with malloc, otherwise it returns nullptr. Then it sets all memory
 * entries to zero. For more documentation see malloc.
 *
 * @param[in] size  Size of space to be allocated
 * @return If malloc managed to allocate space, return pointer to beginning of
 * such space. Otherwise return nullptr.
 *
 */
void *calloc(size_t n_memb, size_t size);

/** @brief A realloc clone
 *
 * This function acts like realloc(). It changes the space of allocated space to
 * new size and returns a pointer to the new segment. If ptr is nullptr, a
 * simple malloc() is performed. If size is null, nullptr is returned without
 * doing anything.
 *
 * @param[in] ptr Pointer to beginning of segment
 * @param[in] size  Size of new space containing data of old segment
 * @return If realloc managed to allocate space, return pointer to beginning of
 * such space. Otherwise return nullptr, if ptr is invalid or size is 0.
 *
 */
void *realloc(void *ptr, size_t size);

#endif

/**
 * @brief Implementation of allocation functions
 */

#include "alloc/defines.h"
#include "alloc/linked_list_mgmt.h"
#include "alloc/memory_mgmt.h"
#include "alloc/storage.h"
#include "alloc/strats.h"
#include "alloc/types.h"

#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//! Lock for heap access so that threads don't interfere with each other
pthread_mutex_t storage_lock = PTHREAD_MUTEX_INITIALIZER;

// A malloc implementation according to the C23 standard

// "Allocates size bytes of uninitialized storage.
// If allocation succeeds, returns a pointer that is suitably aligned for any
// object type with *fundamental alignment*.

// If size is zero, the behavior of malloc is implementation-defined. For
// example, a null pointer may be returned. Alternatively, a non-null pointer
// may be returned; but such a pointer should not be dereferenced, and should be
// passed to free to avoid memory leaks. We choose to return nullptr for
// simplicity.
//
// malloc() is thread - safe:
// it behaves as though only accessing the memory locations visible through its
// argument, and not any static storage.
void *malloc(size_t size) {

    // If size is zero, return a nullptr
    if (!size) {
        pr_warning("Size zero");
        return nullptr;
    }
    pr_info("Allocating with size %zu", size);

    // Lock mutex
    pthread_mutex_lock(&storage_lock);

    // First, we search for a new gap. Either a gap is found or the table is
    // expanded.
    uint8_t *new_a = find_free_seg(size);

    pr_info("Found a gap at address %p\n", new_a);

    // If no gap has been found, this likely means heap storage is exhausted,
    // that is sbrk failed. Return a nullptr in this case and unlock mutex
    if (!new_a) {
        pthread_mutex_unlock(&storage_lock);
        pr_error("Did not find gap and neither could expand");
        return nullptr;
    }

    // If allocation succeeded, we add an entry which should always work if we
    // found a gap above
    uint8_t *user_a = add_entry(new_a, size);

    // This should always work since we found a gap of sufficient size above,
    // but in the unlikely case adding an entry failed, we return a nullptr
    if (!user_a) {
        pthread_mutex_unlock(&storage_lock);
        pr_error("Could not add map entry");
        return nullptr;
    }

    // Unlock mutex
    pthread_mutex_unlock(&storage_lock);
    pr_info("Successfully allocated segment of size %zu", size);

    // Return new address which is *fundamentally* aligned to any data type
    return user_a;
}

// A free()-implementation according to the C23 standard.
// Deallocates the space previously allocated by malloc(), calloc(),
// aligned_alloc(),(since C11) or realloc().

// If ptr is a null pointer, the function does nothing.

// The behavior is undefined if the value of ptr does not equal a value returned
// earlier by malloc(), calloc(), realloc(), or aligned_alloc()(since C11). The
// most likely behaviour is a segfault

// The behavior is undefined if the memory area referred to by ptr has already
// been deallocated, that is, free(), free_sized(), free_aligned_sized()(since
// C23), or realloc() has already been called with ptr as the argument and no
// calls to malloc(), calloc(), realloc(), or aligned_alloc()(since C11)
// resulted in a pointer equal to ptr afterwards. The most likely behaviour is
// segfault.

// The behavior is undefined if after free() returns, an access is made through
// the pointer ptr (unless another allocation function happened to result in a
// pointer value equal to ptr. Expect heap corruption if trying to write on
// freed storage area

// free is thread - safe : it behaves as though only accessing the memory
// locations
// visible through its argument, and not any static storage.

// The function accepts (and does nothing with) the null pointer to reduce the
// amount of special-casing. Whether allocation succeeds or not, the pointer
// returned by an allocation function can be passed to free().
void free(void *ptr) {

    // Return nullptr on nullptr
    if (!ptr) {
        pr_warning("Null pointer %ptr", ptr);
        return;
    }

    pr_info("Freeing");

    // We simply remove a segment by removing all references to it in the
    // linked list and/or the storage table header. For that, we lock the mutex
    // and unlock it afterwards.
    pthread_mutex_lock(&storage_lock);
    remove_segment((uint8_t *)ptr);
    pthread_mutex_unlock(&storage_lock);
}

void *calloc(size_t n_memb, size_t size) {

    if (!n_memb || !size) {
        pr_warning("Product of input is zero. No alloc");
        return nullptr;
    }

    // First of all, allocate new storage
    uint8_t *new_a = (uint8_t *)malloc(n_memb * size);

    if (!new_a) {
        pr_error("Malloc error %s", strerror(errno));
        return nullptr;
    }

    pr_info("Valid pointer");
    // Set the entire storage to zeroes
    set_mem_zero(new_a, n_memb * size);

    pr_info("Set memory to zero");
    return new_a;
}

// A realloc implementation according to the C23 standard.

// Reallocates the given area of memory. If ptr is not NULL, it must be
// previously allocated by malloc, calloc or realloc and not yet freed with a
// call to free or realloc. Otherwise, the results are undefined. Expect
// segfault.

// The reallocation is done by either:
// a) expanding or contracting the existing area pointed to by ptr, if possible.
// The contents of the area remain unchanged up to the lesser of the new and old
// sizes. If the area is expanded, the contents of the new part of the array are
// undefined.
// b) allocating a new memory block of size new_size bytes (with malloc()),
// copying memory area with size equal the lesser of the new and the old sizes,
// and freeing the old block.

// If there is not enough memory,
// the old memory block is not freed and null pointer is returned. That is, the
// old pointer is still valid.

// If ptr is NULL, the behavior is the same as calling malloc(new_size).

// if new_size is zero, the behavior is undefined.

// realloc is thread-safe: it behaves as though only accessing the memory
// locations visible through its argument, and not any static storage.
void *realloc(void *ptr, size_t size) {

    // (C23: Undefined behaviour. We explicitely decided not to free, and simply
    // return a nullptr since we believe this is the intention of the C23
    // standard, and in general the least complicated interpretation of a
    // standard is the most useful)

    // In previous standard versions, the behaviour was stricter and explicitely
    // implementation defined, and one option was to free on size of 0, return a
    // nullptr or do nothing, an return a nullptr.
    if (size == 0) {
        return nullptr;
    }

    // If ptr is nullptr, just pretend as if malloc has been called
    if (!ptr) {
        pr_warning("ptr is NULL. realloc acts like malloc");
        void *new_a = malloc(size);
        return new_a;
    }

    // Lock the mutex
    pthread_mutex_lock(&storage_lock);

    // Get the size of allocated storage the segment where ptr points to (if
    // possible. If ptr is an invalid pointer, expect undefined behaviour). This
    // is necessary for expanding to check if the following free size is
    // sufficiently large
    size_t old_size = get_segment_size((uint8_t *)ptr);
    pr_info("Old size: %zu New size: %zu", old_size, size);

    // This should not really happen, but in this case nothing can be
    // expanded because guaranteed, ptr does not point to any segment
    if (old_size == 0) {
        pr_warning("Invalid pointer");

        // Unlock mutex
        pthread_mutex_unlock(&storage_lock);

        return nullptr;
    }

    // If the new size is the same as the old segment size, nothing needs to
    // be done
    if (old_size == size) {

        pr_warning("Same size, do nothing");

        // Unlock mutex
        pthread_mutex_unlock(&storage_lock);

        // Return the same pointer being passed to the realloc and do nothing
        return ptr;
    }

    // Try to shrink the existing segment first by a difference of old size
    // minus new size, but only if the new size is smaller than the old segment
    // size
    if (size < old_size) {

        // If shrink_segment fails, return nullptr, the old pointer will not be
        // modified. shrink_segment can only fail if the parameters are invalid,
        // which really should not happen.
        if (shrink_segment((uint8_t *)ptr, old_size - size)) {

            // Unlock mutex
            pthread_mutex_unlock(&storage_lock);

            // Return nullptr
            return nullptr;
        } else {

            // Unlock mutex
            pthread_mutex_unlock(&storage_lock);

            // Return same pointer as being passed to realloc
            return ptr;
        }
    }

    pr_info("Could not shrink, trying to expand");

    // Try to expand the segment by the new size minus the existing size, if
    // the following gap size is larger than the to be expanded size.
    // Note since structs are fundamentally aligned and padded, especially a
    // chunk header and tail, the tail is only moved in steps of ALIGNMENT. For
    // this reason, the old size and new size need to be rounded up to the next
    // multiple of ALIGNMENT for comparison
    if (round_up(old_size, ALIGNMENT) +
            round_up(get_following_gap_size((uint8_t *)ptr), ALIGNMENT) >=
        round_up(size, ALIGNMENT)) {

        // However, we still pass the actual size to expand_segment and not the
        // rounded up values above, simply because the actual usable size for
        // the user defined in the header is dependent on the actual size.

        // If expand_segment fails, return nullptr, the old pointer will not be
        // modified. expand_segment can only fail if the parameters are invalid,
        // which really should not happen.
        if (expand_segment((uint8_t *)ptr, size - old_size)) {

            // Unlock mutex
            pthread_mutex_unlock(&storage_lock);

            return nullptr;
        } else {

            // Unlock mutex.
            pthread_mutex_unlock(&storage_lock);

            pr_info("Successfully expanded");

            // Return the same pointer as passed to realloc
            return ptr;
        }
    }

    pr_info("Could neither shrink nor expand. Trying to aquire new storage "
            "segment");

    // Some remarks on how to reallocate: First, we allocate new space.
    // Then, we copy all data.
    // The big advantage of this approach is that if
    // malloc fails, the old pointer is left unmodified. This is really
    // important.
    // The second advantage is this approach is really
    // straightforward.
    // The disadvantage is that this approach doesn't
    // consider that the old segment is technically "available for
    // allocation", so the allocation functions might miss a gap because
    // they rightfully assume the old segment still exists and might
    // unnecessarily expand the table. But, considering suchs cases
    // overcomplicate everything and possibly breaks the old pointer. So,
    // let's not do that.

    // Unlock mutex
    pthread_mutex_unlock(&storage_lock);

    // This behaviour is also called "malloc-copy-free"
    uint8_t *new_a = malloc(size);

    // Could not realloc. Note how the old pointer is left untouched because
    if (!new_a) {

        pr_error("No new storage found");

        // Return the nullptr
        return nullptr;
    }

    pr_info("Found new segment");

    // Copy memory from old space to new space.
    // copy_mem might fail for some reason. In this case, note how the old
    // memory where ptr points to is left untouched due to the assertion above
    // that the new segment is at a different location than the old segment
    int status = copy_mem((uint8_t *)ptr, new_a, old_size);
    if (status == ERROR) {

        pr_error("Could not move memory");

        // Return the nullptr
        return nullptr;
    }

    // Finally, after successfull copying, we can actually, and safely, free
    // the old segment and return the new address properly casted
    free(ptr);

    // Return new address
    return (void *)new_a;
}
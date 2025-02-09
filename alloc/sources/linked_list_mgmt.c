#include "alloc/linked_list_mgmt.h"
#include "alloc/defines.h"
#include "alloc/storage.h"
#include "alloc/types.h"
#include "alloc/utils.h"

#include <errno.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Initialization of table_head structure located at the beginning of storage
// table
// Because initially, there is no segment allocated, set the initial segment
// header pointer to nullptr
static void init_first(seg_list_head_s *list) { list->first_seg = nullptr; }

// This function sets up the tail pointer
//  Initially, there is no segment allocated, as such the tail pointer points to
//  the end of the table_header struct
static void init_tail(seg_list_head_s *list) {

    // The uint8t_t* typecast is important, otherwise the bitshift will produce
    // nonsensical results
    list->end_addr = (uint8_t *)list + sizeof(struct seg_list_head_s);
    pr_info("%ld %ld", list->end_addr - (uint8_t *)list,
            sizeof(struct seg_list_head_s));

    // The table pointer *needs* to point to the end of the table_header struct!
    ASSERT(list->end_addr - (uint8_t *)list == sizeof(struct seg_list_head_s));
}

// Declares and initializes a storage list. In addiction this function calls the
// init_firster and init_tail functions
seg_list_head_s *create_list() {

    // Create a new address which is not yet aligned yet
    void *addr = (seg_list_head_s *)sbrk((int)sizeof(struct seg_list_head_s) +
                                         ALIGNMENT);

    if (addr == (void *)-1) {
        // sbrk failed for some reason, aborting

        pr_error("sbrk error: %s", strerror(errno));

        return nullptr;
    }

    seg_list_head_s *list =
        (seg_list_head_s *)round_up((uintptr_t)addr, ALIGNMENT);

    // Initialize header ...
    init_first(list);

    // and tail of the new list
    init_tail(list);

    return list;
}

// This function searches for the tail of the segment the furthest away from the
// seg_list_head_s struct (so the "last" segment, so to speak).
// It only makes sense to call this function if the list->first_seger field is
// initialized
seg_tail_s *find_last_tail(seg_list_head_s *list) {

    // The list is not initialized, which should never happen at this point
    if (!list) {
        pr_error("List unitialized");
        return nullptr;
    }

    // There is no tail if there is no storage segment at all
    if (!list->first_seg) {
        pr_error(
            "Inappropriate call of function, list header is not initialized");
        return nullptr;
    }

    // Trick: Find the last segment header by iterating backwards over the list.
    // First look up the initial segment header, then from there go to the
    // previous segment tail
    seg_tail_s *temp = list->first_seg->prev_seg_tail;

    // It is necessary for the last segment tail address plus segment tail size
    // to be smaller than the end of the storage list
    ASSERT((uint8_t *)temp + sizeof(struct seg_tail_s) <= list->end_addr)

    // Integrity check: The sum of last segment tail address, segment tail size,
    // and free following space needs to add up to the storage tail address.
    // Assume this is less than the storage tail address. This would mean some
    // other segment existed before the end of the storage list, which is a
    // contradiction. Thus this condiction needs to hold
    ASSERT((uint8_t *)temp + sizeof(struct seg_tail_s) + temp->free_following ==
           list->end_addr)

    return temp;
}

// This function, considering addr points to the header of a valid segment,
// returns the size of subsequent free space.
size_t get_following_gap_size(uint8_t *addr) {

    // We assume addr points points to a memory segment and not a storage
    // segment because this function is called from realloc. As such, to get a
    // pointer to the header, we need to shift the pointer by the size of the
    // segment header struct to the left
    seg_head_s *header = (seg_head_s *)(addr - sizeof(struct seg_head_s));

    pr_info("Following gap size %zu", header->next_seg_tail->free_following);

    // The number of free bytes after the tail is stored in the tail, this is
    // exactly the value we need
    return header->next_seg_tail->free_following;
}

// This function returns the segment size, assuming addr points to a valid
// segment
size_t get_segment_size(uint8_t *addr) {

    // We assume addr points points to a memory segment and not a storage
    // segment because this function is called from realloc. As such, to get a
    // pointer to the header, we need to shift the pointer by the size of the
    // segment header struct to the left.
    seg_head_s *header = (seg_head_s *)(addr - sizeof(struct seg_head_s));

    pr_info("Segment size is %zu", header->seg_size);

    // The size of the segment (without header and tail size, that is storage
    // size only) is stored in the header.
    return header->seg_size;
}

// This function tries to find the header of a storage segment located before
// addr, otherwise it returns the storage table header
uint8_t *get_prev_reference(seg_list_head_s *list, uint8_t *addr) {

    // The address needs to be smaller than the tail address of the storage
    ASSERT((uint8_t *)addr < (uint8_t *)list->end_addr);

    // The address needs to be larger than the address of the storage header,
    // plus the storage header size
    ASSERT((uint8_t *)addr >= (uint8_t *)list + sizeof(struct seg_list_head_s));

    // If the list has no entries, only the header can be before the address.
    if (!list->first_seg) {

        // The value of the list header address needs to be smaller than the
        // given address
        ASSERT((uint8_t *)list < addr);
        return (uint8_t *)list;
    }

    // If the value of addr is smaller than the address of the first allocated
    // segment, this means only the storage table header is before addr
    if ((uint8_t *)list->first_seg > addr) {

        // The value of the list header address needs to be smaller than the
        // given address
        ASSERT((uint8_t *)list < addr);
        return (uint8_t *)list;
    }

    // Otherwise, we know there is a segment directly before addr and we return
    // addr minus tail size, then go one pointer back to the head to obtain the
    // previous head address

    return (uint8_t *)((seg_tail_s *)(addr - sizeof(struct seg_tail_s)))
        ->prev_seg_head;

    /*
    // Iterate over all segment heads, beginning from the first segment
    seg_head_s *iterator = list->first_seg;

    // We want to iterate over all segments, this means as long as the address
    // of the next segment is larger than the address of the current segment
    while ((uint8_t *)iterator <
           (uint8_t *)iterator->next_seg_tail->next_seg_head) {

        // Update the iterator variable
        iterator = iterator->next_seg_tail->next_seg_head;

        // If the address of the current segment head is larger than addr, this
        // means addr is after the *previous* segment header
        // Note that it is not possible to accidentally return the last segment
        // header because that case is covered above where addr is head than
        // list->first_seg
        if ((uint8_t *)iterator > addr) {
            return (uint8_t *)iterator->prev_seg_tail->prev_seg_head;
        }
    }

    // If we left the loop, this means the last segment at all is the last
    // segment before addr

    */
}

// This function resets the list
int reset_list(seg_list_head_s *list) {

    // Reset head
    init_first(list);

    // Reset tail
    init_tail(list);

    // Reset program break all the way back to the end of the storage table
    // header
    if (brk((void *)((uint8_t *)list + sizeof(*list)))) {

        pr_error("Failed to reset program break: %s", strerror(errno));

        return ERROR;
    }

    return SUCCESS;
}

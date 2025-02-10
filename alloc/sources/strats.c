#include "alloc/defines.h"
#include "alloc/linked_list_mgmt.h"
#include "alloc/types.h"

#include <stddef.h>

// State variable for next_fit. Points to the last allocated chunk, or some
// chunk before if the last allocated chunk is freed. If no chunk is allocated,
// points to nullptr
seg_tail_s *last_addr = nullptr;

// An implementation according to the best-fit algorithm. Measures the gaps
// between each allocated chunk, and tries to find the smallest fitting one. If
// no chunk is found, simply allocate at the beginning of storage table if
// storage table is large enough. Otherwise return nullptr if no gap has been
// found or storage table is too small.

// !!IMPORTANT!!: Returns beginning of free space, which is !!NOT!! the
// beginning of usable space which will be returned later. The returned address
// merely indicated the beginning of a possible block.
uint8_t *best_fit(seg_list_head_s *list, size_t size) {

    size_t effective_size = round_up(size, ALIGNMENT);

    size_t total_size =
        sizeof(struct seg_head_s) + effective_size + sizeof(struct seg_tail_s);

    // pr_info("Allocating size %zu", total_size);
    if (!list->end_addr) {
        pr_error("Sorry, list not initialized");
        return nullptr;
    }
    if (!list->first_seg) {
        // pr_info("List is empty, maybe there is storage left though");
        int free_size = (uint8_t *)list->end_addr -
                        ((uint8_t *)list + sizeof(struct seg_list_head_s));
        if (free_size >= (int)total_size) {
            return (uint8_t *)list + sizeof(struct seg_list_head_s);
        }
        // pr_info("Storage not large enough, consider expanding");
        return nullptr;
    }
    int startgapsize = (uint8_t *)list->first_seg -
                       ((uint8_t *)list + sizeof(struct seg_list_head_s));
    uint8_t *best_gap_addr = nullptr;
    size_t best_gap_size = 0;

    if (startgapsize >= (int)total_size) {
        // pr_info("Start segment is large enough with size %d", startgapsize);
        best_gap_size = startgapsize;
        best_gap_addr = ((uint8_t *)list + sizeof(struct seg_list_head_s));
    } else {
        // pr_info("Start segment not large enough with size %d", startgapsize);
    }
    seg_tail_s *iterator = list->first_seg->next_seg_tail;

    do {
        if (iterator->free_following >= total_size &&
            (!best_gap_addr || iterator->free_following < best_gap_size)) {
            // pr_info("Found smaller gap of size %zu",
            // iterator->free_following);
            best_gap_size = iterator->free_following;
            best_gap_addr = (uint8_t *)iterator + sizeof(struct seg_tail_s);
        }
        iterator = iterator->next_seg_head->next_seg_tail;

    } while ((uint8_t *)iterator->prev_seg_head->prev_seg_tail <
             (uint8_t *)iterator);
    return (uint8_t *)best_gap_addr;
}

// An implementation according to the worst-fit algorithm. Measures the gaps
// between each allocated chunk, and tries to find the largest fitting one. If
// no chunk is found, simply allocate at the beginning of storage table if
// storage table is large enough. Otherwise return nullptr if no gap has been
// found or storage table is too small.

// !!IMPORTANT!!: Returns beginning of free space, which is !!NOT!! the
// beginning of usable space which will be returned later. The returned address
// merely indicated the beginning of a possible block.
uint8_t *worst_fit(seg_list_head_s *list, size_t size) {

    size_t effective_size = round_up(size, ALIGNMENT);

    size_t total_size =
        sizeof(struct seg_head_s) + effective_size + sizeof(struct seg_tail_s);

    if (!list->end_addr) {
        pr_error("Sorry, list not initialized");
        return nullptr;
    }
    if (!list->first_seg) {
        // pr_info("List is empty, maybe there is storage left though");
        int free_size = (uint8_t *)list->end_addr -
                        ((uint8_t *)list + sizeof(struct seg_list_head_s));
        if (free_size >= (int)total_size) {
            return (uint8_t *)list + sizeof(struct seg_list_head_s);
        }

        return nullptr;
    }

    int startgapsize = (uint8_t *)list->first_seg -
                       ((uint8_t *)list + sizeof(struct seg_list_head_s));
    uint8_t *largest_gap_addr = nullptr;
    size_t largest_gap_size = 0;

    if (startgapsize >= (int)total_size) {
        // pr_info("Start segment is large enough with size %d", startgapsize);
        largest_gap_size = startgapsize;
        largest_gap_addr = ((uint8_t *)list + sizeof(struct seg_list_head_s));
    }
    int i = 1;
    seg_tail_s *temp = nullptr;

    temp = list->first_seg->next_seg_tail;

    do {
        if (temp->free_following >= total_size &&
            (!largest_gap_addr || temp->free_following > largest_gap_size)) {
            // pr_info("Segment is large enough with size %zu at gap num %d",
            //        largest_gap_size, i);
            largest_gap_size = temp->free_following;
            largest_gap_addr = (uint8_t *)temp + sizeof(struct seg_tail_s);
            i++;
        } else {
            // pr_info("Gap %zu too small or not larger", temp->free_following);
        }
        temp = temp->next_seg_head->next_seg_tail;

    } while ((uint8_t *)temp->prev_seg_head->prev_seg_tail < (uint8_t *)temp);
    // pr_info("Reached end of list");
    return (uint8_t *)largest_gap_addr;
}

// An implementation according to the first-fit algorithm. Measures the gaps
// between each allocated chunk, and tries to find the first fitting one. If
// no chunk is found, simply allocate at the beginning of storage table if
// storage table is large enough. Otherwise return nullptr if no gap has been
// found or storage table is too small.

// !!IMPORTANT!!: Returns beginning of free space, which is !!NOT!! the
// beginning of usable space which will be returned later. The returned address
// merely indicated the beginning of a possible block.
uint8_t *first_fit(seg_list_head_s *list, size_t size) {

    size_t effective_size = round_up(size, ALIGNMENT);

    size_t total_size =
        sizeof(struct seg_head_s) + effective_size + sizeof(struct seg_tail_s);

    if (!list->end_addr) {
        pr_error("Sorry, list not initialized");
        return nullptr;
    }
    if (!list->first_seg) {
        // pr_info("List is empty, maybe there is storage left though");
        int free_size =
            (uint8_t *)list->end_addr - ((uint8_t *)list + sizeof(*list));
        if (free_size >= (int)total_size) {
            // pr_info("Found a gap of size %d at %zu", free_size,
            //            (size_t)((uint8_t *)list + sizeof(*list)));
            return (uint8_t *)list + sizeof(*list);
        }

        return nullptr;
    }
    int startgapsize = (uint8_t *)list->first_seg -
                       ((uint8_t *)list + sizeof(struct seg_list_head_s));

    if (startgapsize >= (int)total_size) {
        // pr_info("Start segment is large enough with size %d", startgapsize);
        return (uint8_t *)list + sizeof(struct seg_list_head_s);
    }
    // pr_info("Start segment not large enough with size %d", startgapsize);
    seg_tail_s *temp = list->first_seg->next_seg_tail;

    do {

        if (temp->free_following >= total_size) {
            // pr_info("Found a gap of size %zu", temp->free_following);
            return (uint8_t *)temp + sizeof(*temp);
        }
        temp = temp->next_seg_head->next_seg_tail;
    } while ((uint8_t *)temp->prev_seg_head->prev_seg_tail < (uint8_t *)temp);
    // pr_info("No gap found");
    return nullptr;
}

// An implementation according to the next-fit algorithm. Measures the gaps
// between each allocated chunk, and tries to find the first fitting one.
// Unlike first-fit, the search is begun from the last allocated chunk. If
// last_addr, the next-fit pointer, is nullptr, a simple first-fit
// algorithm is performed.

// For simplicity, we move the last_addr pointer backwards to the previous chunk
// if the last allocated chunk is freed. We think this is in the spirit of
// next-fit, because otherwise one would have to consider cases like: Allocation
// in the middle of a free segment, which only increases fragmentation.
// Benchmarks also have shown that this implementation is quite efficient.

// If no chunk is found, simply allocate at the beginning of storage table if
// storage table is large enough. Otherwise return nullptr if no gap has been
// found or storage table is too small.
uint8_t *next_fit(seg_list_head_s *list, size_t size) {

    size_t effective_size = round_up(size, ALIGNMENT);

    size_t total_size =
        sizeof(struct seg_head_s) + effective_size + sizeof(struct seg_tail_s);

    if (!list->end_addr) {
        pr_error("Sorry, list not initialized");
        return nullptr;
    }

    if (!last_addr) {

        // pr_info("Last_addr is empty");

        // Do normal first fit
        return first_fit(list, size);
    }

    // The last_addr pointer needs to point somewhere *before* the end of the
    // list, otherwise this would mean last_addr points to some segment outside
    // of the current storage range, which is undesirable (more like,
    // nonsensical) behaviour
    ASSERT((uint8_t *)last_addr < list->end_addr);

    // pr_info("List not empty");

    // Initialize an iterator variable which will be used throughout searching
    // for a gap
    seg_tail_s *iter = last_addr;

    // We iterate the list in a do-while fashing simply because maybe, iter
    // points to the very last segment, and we still want to check if the size
    // afterwards is large enough
    do {

        // Check is space after tail is large enough
        if (iter->free_following >= total_size) {
            // pr_info("Found a gap of size %zu", iter->free_following);

            // Iter points to the tail, we need to return the *beginning* of the
            // free segment though
            return (uint8_t *)iter + sizeof(*iter);
        }

        // Otherwise, go to the next segment
        iter = iter->next_seg_head->next_seg_tail;

        // This condition states one single thing: After moving iter to the next
        // tail, the address of said tail is larger. If this condition is false,
        // it means we reached the beginning of the table, in which case we need
        // to abort. Another possibility is that only one segment exists at all.
    } while ((uint8_t *)iter->prev_seg_head->prev_seg_tail < (uint8_t *)iter);

    // pr_info("No gap found from where we left off. Now searching from the "
    //        "beginning");

    // Check the size between first segment and table header addr
    int header_offset =
        (uint8_t *)list->first_seg - ((uint8_t *)list + sizeof(*list));
    if (header_offset >= (int)total_size) {
        // pr_info("It seems like a gap at the beginning of the table
        // has been "
        //         "found. Congratulations");
        return (uint8_t *)list + sizeof(*list);
    }

    // pr_info("The gap at the beginning of the table is not large
    // enough. "
    //        "Continue searching until last_addr has been reached");

    // Reset iterator variable to the first segment tail. We already
    // checked the initial gap size, as such nothing else is left to do
    iter = list->first_seg->next_seg_tail;

    // Remember that assertion above? We can be sure that
    // last-addr<list->end_addr, as such we don't need to separately
    // check if the iterator is less than the last addr. We can also be
    // sure that because last_addr points to some segment tail, this
    // loop *will* terminate with utmost certainty. For example, if
    // there is only one segment in the middle of the storage table,
    // then this loop will terminate instantly as next_addr is assumed
    // to point at some tail, and since there is only one tail,
    // next_addr will point to the only tail possible This loop stops
    // exactly at the segment where we started beginning at

    while (iter < last_addr) {

        // If the size of the following gap is large enough, we found a
        // gap of suitable size!
        if (iter->free_following >= total_size) {
            // pr_info("Found a gap");
            return (uint8_t *)iter + sizeof(*iter);
        }

        // Move iter to the subsequent tail
        iter = iter->next_seg_head->next_seg_tail;
    }

    // pr_info("Did not find a gap");

    // If no gap has been found, we return nullptr
    return nullptr;
}

// This function sets the last_addr pointer used for next-fit to some chunk
// tail, or to nullptr, depending on the input.
void set_last_addr(seg_tail_s *addr) { last_addr = addr; }

// This function simply retrieves the value of last_addr, used by next-fit, to
// avoid having to define a global variable.
seg_tail_s *get_last_addr() { return last_addr; }
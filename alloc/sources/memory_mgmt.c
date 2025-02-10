#include "alloc/memory_mgmt.h"
#include "alloc/defines.h"
#include "alloc/linked_list_mgmt.h"
#include "alloc/storage.h"
#include "alloc/strats.h"
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

// Declaration of storage list
seg_list_head_s *start = nullptr;

// Declaration and initialization of allocation function
alloc_function g_alloc_function = &first_fit;

// This function actually allocated spaces for a given address by adding a
// segment head, and segment tail with minimum distance size
uint8_t *add_entry(uint8_t *addr, size_t size) {

    // If neither the list nor the end pointer is initialized, it is not
    // possible to add an entry
    if (!start || !start->end_addr) {
        pr_error("List is not initialized");
        return nullptr;
    }

    size_t effective_size = round_up(size, ALIGNMENT);

    // Note that size only described the user space size. But if we want to
    // actually allocate a new segment, we need to consider the segment header
    // and segment tail size!
    // Note this means that this shrinks gaps more than necessary, however this
    // is something unavoidable. You can choose: LUT (Look up table) of constant
    // size, but consistent gap sizes, or linked segment list of dynamic size,
    // but of inconsistent (shrinking) gap sizes
    int new_size =
        (int)(effective_size + sizeof(seg_head_s) + sizeof(seg_tail_s));

    // addr needs to point to a gap sufficiently far away from the end of the
    // storage table. This is an invariant that should always be the case,
    // otherwise the allocator functions messed up somewhere in the process,
    // which is bad
    ASSERT((uint8_t *)addr <= (uint8_t *)start->end_addr - new_size);

    // At the same time, addr needs to point to an address which is larger than
    // the table header address plus table header size.
    ASSERT((uint8_t *)addr >= (uint8_t *)start + sizeof(*start));

    // Let us store the returned address value globally for easier return and
    // initialize it with nullptr...
    seg_head_s *new_seg = nullptr;

    // If the storage list already has an entry, there are some special cases to
    // consider. Otherwise, check if the existing table is large enough
    if (start->first_seg) {
        pr_info("Head not empty");
        // This means a segment already exists in the table
        // We need to check if addr points to a segment *before* or *after* the
        // first segment

        // We need to find the
        // segment located "before" the current segment and repoint its
        // pointers

        // To repoint, fetch the "previous" segment located before the
        // current address
        uint8_t *previous = get_prev_reference(start, addr);

        seg_tail_s *temp;

        if (previous == (uint8_t *)start) {

            pr_info("Previous pointer is start");

            // If the returned address matches the storage segment header,
            // this means that addr points to some area before the first
            // segment, which means the "previous" tail is the last tail of the
            // entire list!
            temp = start->first_seg->prev_seg_tail;

            // This case is slightly easier than the following case, because
            // we don't need to recalculate the previous (last) tails number
            // of following free bytes

            // Store the distance of the new segment location compared to
            // the table start
            int offset = addr - ((uint8_t *)start + sizeof(*start));

            // The new address needs to point after the storage table start
            // Extremely important check. If this assertion is violated, this
            // means addr points *into* the storage table header and tries to
            // allocate space where it is absolutely not possible. In this case,
            // the allocator function *really* messed up somewhere, which is
            // really bad
            // This assertion should not trigger as the validity of the address
            // is checked above already. Regardless, this assertion should stay
            // in cases the code is changed
            ASSERT(offset >= 0);

            // Store the gap size until the first segment occurs
            int start_gap = (uint8_t *)start->first_seg -
                            ((uint8_t *)start + sizeof(*start));

            // The old gap size with offset needs to be larger than the desired
            // new size (with overhead), otherwise the allocator messed up
            // somewhere
            ASSERT(start_gap - offset >= new_size);

            // The new segment head simply begins at the given address
            new_seg = (seg_head_s *)addr;

            // The predecessor tail of the new segment header is exactly the
            // previous tail
            new_seg->prev_seg_tail = temp;

            // The tail of the new segment is located at an offset of the
            // size of the segment header, plus the user size
            new_seg->next_seg_tail =
                (seg_tail_s *)((uint8_t *)new_seg + sizeof(*new_seg) +
                               effective_size);

            // The new segment user space size is the desired size
            new_seg->seg_size = size;

            // The predecessor of the new segment tail is the new segment
            // header, quite obviously
            new_seg->next_seg_tail->prev_seg_head = new_seg;

            // The successor of the new segment tail is the old successor of
            // the previous tail, which is exactly the list head
            new_seg->next_seg_tail->next_seg_head = temp->next_seg_head;

            // The number of free following bytes is the old start gap size,
            // minus the new necessary size (with overhead) together with
            // the new segment offset
            new_seg->next_seg_tail->free_following =
                start_gap - (offset + new_size);

            // The successor of the predecessor of the new head needs to
            // point to the new head
            new_seg->prev_seg_tail->next_seg_head = new_seg;

            // The predecessor, of the successor, of the new segment tail,
            // points to the new segment tail
            new_seg->next_seg_tail->next_seg_head->prev_seg_tail =
                new_seg->next_seg_tail;

            // The address of the new segment, minus the address of the
            // start, needs to be exactly the offset plus the start table
            // header size
            ASSERT((uint8_t *)new_seg - (uint8_t *)start ==
                   (int)(sizeof(*start) + offset));

            // Integrity check that the number of free bytes is consistent:
            // The new offset, plus the segment size (with overhead) plus
            // the subsequent number of free bytes needs to
            // be exactly the old number of free bytes when the new segment
            // did not yet exist at the current gap
            ASSERT(offset + new_size +
                       (int)new_seg->next_seg_tail->free_following ==
                   start_gap);

            // Also, the new segment address, plus the entire segment size
            // (with overhead), plus the subsequent number of free bytes
            // needs to be smaller than the end of the storage table,
            // otherwise something went wrong and we can expect nice
            // SEGFAULTS we all like.
            ASSERT((uint8_t *)new_seg + new_size +
                       new_seg->next_seg_tail->free_following <=
                   start->end_addr);

            // Finally, update the first segment pointer
            start->first_seg = new_seg;

        } else {

            pr_info("Previous pointer is not start");

            // Otherwise, the previous tail is simply the returned value
            // casted to an appropriate pointer and we can assume that addr
            // points *after* some existing segment, an important distinction

            temp = (seg_tail_s *)((seg_head_s *)previous)->next_seg_tail;

            // If this assertion doesn't hold, that would mean addr points
            // directly into the previous tail, which is quite bad
            ASSERT((uint8_t *)temp + sizeof(*temp) <= addr);

            pr_info("Segment size of previous segment: %zu",
                    temp->prev_seg_head->seg_size);

            // Store the old consequtive number of free bytes
            size_t old_free_size = temp->free_following;

            // Store the distance of the new segment location compared to
            // the previous segment location
            int offset = addr - ((uint8_t *)temp + sizeof(*temp));

            // Extremely important check. If this assertion is violated, this
            // means addr points *into* the previous segment and tries to
            // allocate space where it is absolutely not possible. In this case,
            // the allocator function *really* messed up somewhere, which is
            // really bad.
            // Should not trigger because validity of addr is already checked
            // above. Regardless, keep this sanity check in case the code is
            // changed!
            ASSERT(offset >= 0);

            // The old gap size minus offset needs to be larger than the desired
            // new size (with overhead), otherwise the allocator messed up
            // somewhere badly
            ASSERT((int)old_free_size - offset >= new_size);

            // The new segment head simply begins at the given address
            new_seg = (seg_head_s *)addr;

            // The predecessor tail of the new segment header is exactly the
            // previous tail
            new_seg->prev_seg_tail = temp;

            // On the contrary, the free following size of the previous
            // segment is now simply the offset
            new_seg->prev_seg_tail->free_following = offset;

            // The address of the new segment needs to be the address of the
            // previous tail, plus the size of the previous tail, plus the
            // number of new free following bytes
            ASSERT((uint8_t *)new_seg ==
                   (uint8_t *)new_seg->prev_seg_tail +
                       sizeof(*new_seg->prev_seg_tail) +
                       new_seg->prev_seg_tail->free_following);

            // The tail of the new segment is located at an offset of the
            // size of the segment header, plus the user size
            new_seg->next_seg_tail =
                (seg_tail_s *)((uint8_t *)new_seg + sizeof(*new_seg) +
                               effective_size);

            // The new segment user space size is the desired size
            new_seg->seg_size = size;

            // The predecessor of the new segment tail is the new segment
            // header, quite obviously
            new_seg->next_seg_tail->prev_seg_head = new_seg;

            // The successor of the new segment tail is the old successor of
            // the previous tail
            new_seg->next_seg_tail->next_seg_head = temp->next_seg_head;

            // THe number of free following bytes is the old free folloying
            // bytes number of the previous segment, minus the new necessary
            // size (with overhead) together with the new segment offset
            new_seg->next_seg_tail->free_following =
                old_free_size - (offset + new_size);

            // The successor of the predecessor of the new head needs to
            // point to the new head
            new_seg->prev_seg_tail->next_seg_head = new_seg;

            // Integrity check. The sum of the previous segment free following
            // bytes, plus the new segment size (with overhead), plus the
            // consecutive number of free bytes, needs to add up exactly to the
            // old free bytes size when the current segment did not exist yet!
            ASSERT(new_seg->prev_seg_tail->free_following + new_size +
                       new_seg->next_seg_tail->free_following ==
                   old_free_size);

            // The predecessor, of the successor, of the new segment tail,
            // points to the new segment tail
            new_seg->next_seg_tail->next_seg_head->prev_seg_tail =
                new_seg->next_seg_tail;

            // Also, the new segment address, plus the entire segment size
            // (with overhead), plus the subsequent number of free bytes
            // needs to be smaller than the end of the storage table,
            // otherwise something went wrong and we can expect nice
            // SEGFAULTS we all like.
            ASSERT((uint8_t *)new_seg + new_size +
                       new_seg->next_seg_tail->free_following <=
                   start->end_addr);
        }

    } else {
        // This case means there is currently no segment allocated. This does
        // not mean that the storage list has a size of size 0, however. On the
        // contrary, from subtracting storage table header address (plus storage
        // table header size) of the storage table tail address, we get the
        // current number of free bytes in the storage table. As such, we need
        // to set the subsequent number of free bytes appropriately!

        pr_info("Head empty");

        // As described above, to obtain the "empty" size of the storage table,
        // subtract the
        int free_size =
            (uint8_t *)start->end_addr - ((uint8_t *)start + sizeof(*start));

        // Store the distance of the new segment location compared to
        // the table start
        int offset = addr - ((uint8_t *)start + sizeof(*start));

        // The new address needs to point after the storage table start
        // Extremely important check. If this assertion is violated, this
        // means addr points *into* the storage table header and tries to
        // allocate space where it is absolutely not possible. In this case,
        // the allocator function *really* messed up somewhere, which is
        // really bad
        // This assertion should not trigger as the validity of the address
        // is checked above already. Regardless, this assertion should stay
        // in cases the code is changed
        ASSERT(offset >= 0);

        // Check if the relative free size, that is free size minus offset, is
        // larger than the desired size
        if (free_size - offset >= new_size) {

            // The new segment needs to point at the given address
            new_seg = (seg_head_s *)addr;

            // The tail of the new segment is located at the address of the new
            // segment header, plus the size of the new segment header, plus the
            // user space size
            new_seg->next_seg_tail =
                (seg_tail_s *)((uint8_t *)new_seg + sizeof(*new_seg) +
                               effective_size);

            // The user space size is the desired size
            new_seg->seg_size = size;

            // The previous segment header of the new tail is the segment header
            // of the new segment obviously
            new_seg->next_seg_tail->prev_seg_head = new_seg;

            // Since there is only one segment, the predecessor of the new
            // segment points to the new segment tail
            new_seg->prev_seg_tail = new_seg->next_seg_tail;

            // Also, the successor of the new segment tail is equal to the new
            // segment header
            new_seg->next_seg_tail->next_seg_head = new_seg;

            // The number of free bytes following the new segment tail is equal
            // to the old empty storage table size minus the desired size (with
            // overhead) - minus the offset!
            new_seg->next_seg_tail->free_following =
                free_size - (new_size + offset);

            // We created a new segment, as such the first segment pointer of
            // the storage table header needs to be updated properly
            start->first_seg = new_seg;

            // Now it's assertion time... let's see where this goes

            // The new segment address has to equal to the storage table start
            // address, plus storage table start size, plus the offset.
            // Otherwise, well, something above went wrong clearly
            ASSERT((uint8_t *)new_seg ==
                   (uint8_t *)start + sizeof(*start) + offset);

            // The tail of the new segment is located exactly user size plus the
            // size of the segment header away from the new segment header
            // address
            ASSERT((uint8_t *)new_seg->next_seg_tail - (uint8_t *)new_seg ==
                   (int)(sizeof(*new_seg) + effective_size));

            // The address constructed by taking the new segment address, adding
            // the new segment size (with overhead) and adding the subsequent
            // free size has to be equal to the end address of the
            // storage table
            ASSERT((uint8_t *)new_seg + new_size +
                       new_seg->next_seg_tail->free_following ==
                   start->end_addr);

            // Similarly, the address of the new segment tail, plus the segment
            // tail size, plus the number of free following bytes needs to be
            // equal to the end address of the storage table
            ASSERT((uint8_t *)new_seg->next_seg_tail +
                       sizeof(*new_seg->next_seg_tail) +
                       new_seg->next_seg_tail->free_following ==
                   start->end_addr);

        } else {

            // The size of the storage table is not large enough. But don't fret
            // - this case will be properly handled by the calling function by
            // simply expanding the table, so not an error

            pr_warning("Storage table not large enough, consider expanding");
        }
    }

    // If the new segment address is a valid address, congratulations, you
    // successfully managed to allocate a new segment.
    if (new_seg) {
        pr_info("Successfully added entry with user size %zu %zu", size,
                new_seg->seg_size);

        // For Nextfit, set the last allocated tail address to the tail of
        // the just allocated segment
        set_last_addr(new_seg->next_seg_tail);

        // We are again only interested in the usable address for the user,
        // as such we return the new segment address (pointing to the
        // header), plus the size of the new segment header
        return (uint8_t *)new_seg + sizeof(*new_seg);
    } else {
        pr_info("Could not add entry");

        // Otherwise, well, either addr is an invalid pointer (this is really,
        // really bad and should never happen, at all, thus the assertions
        // *will* cause program termination in that case), the gap where addr
        // tries to allocate is not small enough or the table is not large
        // enough
        return nullptr;
    }

    // Now we considered every single case, I think, this function should also
    // properly handle next_fit calls now. Hopefully, that is...
}

// This function removes a segment, and especially considers specialy cases like
// the segment to be removed is the only segment left. Also, pointers are
// redirected appropriately
void remove_segment(uint8_t *addr) {

    // Check that addr is valid
    ASSERT(addr >= (uint8_t *)start + sizeof(*start));
    ASSERT(addr < (uint8_t *)start->end_addr);

    // addr points to user space, this means the segment head is addr minus the
    // size of the segment header
    seg_head_s *old =
        (seg_head_s *)((uint8_t *)addr - sizeof(struct seg_head_s));

    // As some sanity check, check that old points to an address smaller than
    // the end addr of the storage table
    ASSERT((uint8_t *)old < (uint8_t *)start->end_addr);

    // Also assert that the old segment header address points to a segment
    // located after the storage table header
    ASSERT((uint8_t *)old >= (uint8_t *)start + sizeof(*start));

    pr_info("Valid address");

    // If the corresponding segment does not belong to the first segment, simply
    // repoint pointers. This also means the list has more than 2 allocated
    // segments
    if (start->first_seg != old) {

        // Consider the previous tail as a variable and store it for managing
        seg_tail_s *pred = old->prev_seg_tail;

        // For Nextfit, set the last allocated tail address to the tail of
        // the previous segment. Only update if last_addr points to the end of
        // the segment to be removed
        if (old->next_seg_tail == get_last_addr()) {
            set_last_addr(pred);
        }

        // Since we remove the current segment, the number of free bytes gets
        // updated to the size of the old segment, plus the number of free bytes
        // after the old segment
        pred->free_following +=
            sizeof(struct seg_head_s) + round_up(old->seg_size, ALIGNMENT) +
            sizeof(struct seg_tail_s) + old->next_seg_tail->free_following;

        // The next head of the previous tail is the next head of the old
        // segments tails next head
        pred->next_seg_head = old->next_seg_tail->next_seg_head;

        // Similarly, the predecessor of the new successor of pred is pred
        // itself
        pred->next_seg_head->prev_seg_tail = pred;

        // Now we updated all variables and can do some assertions

        // The number of free bytes after pred should not exceed the storage
        // table limits
        ASSERT((uint8_t *)pred + sizeof(struct seg_tail_s) +
                   pred->free_following <=
               start->end_addr);

        // Either, we removed the last segment, and as such, the number of free
        // following bytes need to match up exactly with the end of the storage
        // table.
        // Or, we removed some previous segment, and we know there are more than
        // 1 segments in this case, so, the number of free bytes need to match
        // up with the beginning of the next segment's head
        ASSERT((uint8_t *)pred + sizeof(*pred) + pred->free_following <=
                   start->end_addr ||
               (uint8_t *)pred + sizeof(*pred) + pred->free_following ==
                   (uint8_t *)pred->next_seg_head);

        // If the now previous element is the last element, check if we can
        // shrink the allocated storage
        if (pred == start->first_seg->prev_seg_tail) {

            // Check if the free storage after the last segment is larger than a
            // page size
            if ((int)pred->free_following - (int)PAGE_SIZE > 0) {

                size_t old_free = pred->free_following;

                // We want to shrink the allocated storage by the max multiple
                // of PAGE_SIZE that is still smaller than the free following
                // space.
                int to_shrink =
                    (int)(floor((double)pred->free_following / PAGE_SIZE)) *
                    PAGE_SIZE;

                // to_shrink should be a positive number to avoid confusion
                ASSERT(to_shrink > 0);

                // Make sure the to be shrunken size is actually smaller than
                // the free following size, otherwise expect heap corruption!
                ASSERT((size_t)to_shrink <= pred->free_following);

                if (sbrk(-to_shrink) == (void *)-1) {
                    // If the returned value is -1, sbrk failed. This should not
                    // really happen as we are shrinking the storage, so abort.

                    pr_error("sbrk error: %s", strerror(errno));

                    abort();
                }

                // Update free following of last segment
                pred->free_following -= to_shrink;

                // Update tail pointer
                start->end_addr -= to_shrink;

                ASSERT(pred->free_following == old_free - to_shrink);
                ASSERT((uint8_t *)pred + sizeof(*pred) + pred->free_following ==
                       start->end_addr);
            }
        }

        pr_info("Successfully free entry");
    } else {

        // In this case, old points to the first segment

        // If old is the only segment at all, we are lucky and only need to set
        // the first segment entry of the storage table header to nullptr
        if (old->prev_seg_tail == old->next_seg_tail) {

            // This means only one segment is allocated because the pointers are
            // circular. As such, for nextfit, we set the last_addr to nullptr.
            // Only update if last_addr points to the tail of the to be removed
            // segment
            if (old->next_seg_tail == get_last_addr()) {
                set_last_addr(nullptr);
            }
            pr_info("Start equals head");

            // Simple sanity check: Make sure that the next head of the
            // current tail is the current head again
            ASSERT(old->next_seg_tail->next_seg_head = old);

            // Set start head to nullptr
            start->first_seg = nullptr;

            // Since the only left element has been removed, we can safely reset
            // the list.
            if (reset_list(start)) {
                pr_error("Failed to reset list");
                abort();
            }
        } else {

            // Since we are removing the very first segment, this means
            // pointers of the last segment need to be updated, too

            // Store the old segment user size
            size_t seg_size = old->seg_size;

            // Store the number of trailing free bytes after the current
            // segment
            size_t trailing_free = old->next_seg_tail->free_following;

            // Calculate the distance between old segment start and the
            // beginning of the table
            size_t offset = (uint8_t *)old -
                            ((uint8_t *)start + sizeof(struct seg_list_head_s));

            // Find the last tail
            seg_tail_s *end = find_last_tail(start);

            // For Nextfit, set the last allocated tail address to the tail
            // of the previous segment
            // Only update if last_addr points to the tail of the to be removed
            // segment
            if (old->next_seg_tail == get_last_addr()) {
                set_last_addr(end);
            }

            // Move the next header pointer of the last segment tail one up
            // because segment old is to be removed
            end->next_seg_head = old->next_seg_tail->next_seg_head;

            // Point the previous tail pointer of the next segment header to
            // the last segment tail
            end->next_seg_head->prev_seg_tail = end;

            // Reassign the first segment pointer of the storage table
            // header since old was the first segment
            start->first_seg = end->next_seg_head;

            // Check that the distances match up: The distance between first
            // segment and segment table beginning needs to be equal to the
            // old segment offset towards the beginning, plus the old
            // segment size, plus the number of old trailing bytes after the
            // old segment
            ASSERT((uint8_t *)start->first_seg -
                       ((uint8_t *)start + sizeof(struct seg_list_head_s)) ==
                   (int)(offset + +sizeof(struct seg_head_s) +
                         round_up(seg_size, ALIGNMENT) +
                         sizeof(struct seg_tail_s) + trailing_free));
        }
    }
}

// This function shrinks a segment *by* size many bytes, which is useful for
// realloc. Note that this function only does useful things on valid addr
// pointers
int shrink_segment(uint8_t *addr, size_t size) {

    // Since addr points to user data beginning, to obtain the header,
    // subtract the header size
    seg_head_s *header = (seg_head_s *)(addr - sizeof(struct seg_head_s));

    // If we are trying to shrink more bytes than currently are assigned to
    // user data, this is not a valid operation and we need to abort
    if (size > header->seg_size) {
        pr_error("Sorry, invalid arguments");
        return ERROR;
    }

    // Store the old segment tail address
    seg_tail_s *old_addr = header->next_seg_tail;

    // Store the successor of the old segment tail
    seg_head_s *next = header->next_seg_tail->next_seg_head;

    // Store the number of free bytes after the old segment tail
    size_t free_size = header->next_seg_tail->free_following;

    size_t effective_size = round_up(header->seg_size - size, ALIGNMENT);

    // We "move" the tail size many bytes to the front relative to the old
    // address
    seg_tail_s *shifted =
        (seg_tail_s *)((uint8_t *)header + sizeof(*header) + effective_size);

    // The predecessor of the new tail is same as before, the current header
    shifted->prev_seg_head = header;

    // THe succcessor of the new tail is same as before, the next head
    shifted->next_seg_head = next;

    // The number of free following bytes has increased by size many bytes
    // because we moved the tail size many bytes closer to the header!
    shifted->free_following =
        free_size + ((uint8_t *)old_addr - (uint8_t *)shifted);

    // Update the next segment tail of the segment header to the shifted
    // tail
    header->next_seg_tail = shifted;

    // Since we shrunk the segment by size many bytes, it is only logically
    // to assume the usable user data shrinks by size many bytes, too
    header->seg_size -= size;

    // Update the preceding segment tail of the next header to the new,
    // shifted tail
    next->prev_seg_tail = shifted;

    // If we add the header size and usable user size to the header address,
    // we need to obtain the tail address
    ASSERT((uint8_t *)header + sizeof(struct seg_head_s) +
               round_up(header->seg_size, ALIGNMENT) ==
           (uint8_t *)header->next_seg_tail);

    // If we add the tail size and number of subsequent free bytes to the
    // new table, we should obtain the same value as adding the old free
    // size of the old segment, plus the tail size, to the old tail address
    ASSERT((uint8_t *)header->next_seg_tail + sizeof(struct seg_tail_s) +
               header->next_seg_tail->free_following ==
           (uint8_t *)old_addr + sizeof(struct seg_tail_s) + free_size);

    // Since we are shrinking a segment, the free following segment size
    // needs to be the old free following segment size plus the desired size
    // to shrink by
    ASSERT(header->next_seg_tail->free_following ==
           free_size + ((uint8_t *)old_addr - (uint8_t *)shifted));
    if (old_addr == get_last_addr()) {
        set_last_addr(header->next_seg_tail);
    }

    return SUCCESS;
}

// This function works very similarly to shrink_segment, but instead expands
// segment by size
int expand_segment(uint8_t *addr, size_t size) {

    // It is assumed addr points to user space, so to obtain the header
    // subtract by the header size
    seg_head_s *header = (seg_head_s *)(addr - sizeof(struct seg_head_s));

    // Store the old free following size, we need that later
    size_t free_size = header->next_seg_tail->free_following;

    size_t effective_size = round_up(header->seg_size + size, ALIGNMENT);

    // If we are trying to expand larger than there is space available, this
    // is an invalid operation
    if (effective_size > round_up(free_size, ALIGNMENT) +
                             round_up(header->seg_size, ALIGNMENT)) {
        pr_error("Sorry, invalid arguments. %zu is smaller than %zu", free_size,
                 size);
        return ERROR;
    }

    // Store the old current segment tail addres
    seg_tail_s *old_addr = header->next_seg_tail;

    // Store the old header address of the very next header
    seg_head_s *next = header->next_seg_tail->next_seg_head;

    // Add a new "shifted" header tail shifted by a size of, well, size
    // bytes away from the old header tail
    seg_tail_s *shifted =
        (seg_tail_s *)((uint8_t *)header + sizeof(*header) + effective_size);

    // Fill the new shifted tail with the information of the old tail

    // The previous header obviously is the current segment header
    shifted->prev_seg_head = header;

    // The next header is the old next header we stored earlier
    shifted->next_seg_head = next;

    // The number of free_following bytes is the old number of free
    // following bytes minus the size size we shrunk by!
    shifted->free_following =
        free_size - ((uint8_t *)shifted - (uint8_t *)old_addr);

    // Now, let's point the subsequent tail of the header to the next
    // segment
    header->next_seg_tail = shifted;

    // The header segment size is the old size, plus the size we expanded by
    // Note the new segment bytes are left unmodified, if you want to have a
    // defined state call calloc... but this is of no relevance in this
    // low-level function here and up to the user, that is the calling
    // function
    header->seg_size += size;

    // Point the preceding tail of the old next header to the new shifted
    // tail!
    next->prev_seg_tail = shifted;

    // It's assertion time yet again

    // The new tail address needs to be the old header address, plus the
    // segment header size, plus the user data size
    ASSERT((uint8_t *)header + sizeof(struct seg_head_s) +
               round_up(header->seg_size, ALIGNMENT) ==
           (uint8_t *)header->next_seg_tail);

    // If we add the new tail address, the tail size and the new subsequent
    // free_following bytes number, this should add up to the old tail
    // address, plus old tail size, plus old subsequent free size.
    ASSERT((uint8_t *)header->next_seg_tail + sizeof(struct seg_tail_s) +
               header->next_seg_tail->free_following ==
           (uint8_t *)old_addr + sizeof(struct seg_tail_s) + free_size);

    // Also, the number of free-following bytes now needs to match with the
    // old number of free following bytes minus the desired size we expanded
    // by. Quite important, otherwise the storage becomes really
    // inconsistent
    ASSERT(header->next_seg_tail->free_following ==
           free_size - ((uint8_t *)shifted - (uint8_t *)old_addr));

    if (old_addr == get_last_addr()) {
        set_last_addr(header->next_seg_tail);
    }
    // Now we are done

    return SUCCESS;
}

// This function is called when no gap has been found by any allocator
// function, or if the list just has been initialized. This function expands
// the storage table by as many bytes necessary, but not more. Especially,
// we take the current free space at the end of the storage table into
// account and subtract that size from the to be expanded size. size merely
// denotes the user space size
uint8_t *expand_list(size_t size) {

    // If the list is not initialized yet, this is bad because at this point
    // this should never happen. Fix your implementation
    if (!start) {
        pr_error("List empty");
        return nullptr;
    }

    size_t effective_size = round_up(size, ALIGNMENT);

    ASSERT(effective_size % ALIGNMENT == 0);

    // The size of the new segment is the desired user space size, plus
    // header size, plus tail size But, this will not be the size we expand
    // the list by, as this could be too large, see later
    int totalsize = (int)(effective_size + sizeof(struct seg_head_s) +
                          sizeof(struct seg_tail_s));

    // If the list is empty, expand list by the total size, minus the
    // current list size to obtain a fitting table
    if (!start->first_seg) {

        pr_info("List is empty");

        // As explained earlier, we want to expand the table by the total
        // size, minus the current (free) table size. The current (free)
        // table size is obtained by the address of the storage header
        // start, plus storage header size, from the storage tail address.
        int to_expand = totalsize - ((uint8_t *)start->end_addr -
                                     ((uint8_t *)start + sizeof(*start)));

        // It only makes sense eto expand the list if it needs to be
        // expanded, otherwise, the allocator function messed up somewhere
        // as it should have found a gap
        ASSERT(to_expand > 0);

        // Similarly, make sure the current table size is, in fact, smaller
        // than the desired size (with overhead)
        ASSERT((uint8_t *)start->end_addr -
                   ((uint8_t *)start + sizeof(*start)) <
               totalsize);

        // We only want to expand by pages, not by some smaller values to avoid
        // frequent syscalls
        size_t num_pages = (size_t)ceil((double)to_expand / PAGE_SIZE);

        // Now we actually ask the system for more storage of necessary size
        if (sbrk(num_pages * PAGE_SIZE) == (void *)-1) {
            // If the returned value is -1, sbrk failed, maybe storage is
            // full and you should swap with mmap, who knows. We don't need
            // to care at this point, the only thing we know is that in this
            // implementation, we cannot proceed

            pr_error("sbrk error: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }

        pr_info("Expanded list by %d", num_pages * PAGE_SIZE);

        // Since we expanded the list, the tail pointer needs to be updated
        // by the expanded size
        start->end_addr += num_pages * PAGE_SIZE;

        return (uint8_t *)start + sizeof(*start);
    }

    // In this case, the list is not empty. As such, we need to expand the
    // list by the desired size, minus the last following free bytes size of
    // the last segment

    // Search for the last tail of the list
    seg_tail_s *end = find_last_tail(start);

    // Since we assume the list is not empty there *needs* to be a last
    // tail, otherwise something went terribly wrong
    ASSERT(end);

    // The number of subsequent free bytes is already stored in the tail
    // entry, fetch this value
    size_t trailing_free = end->free_following;

    // The trailing free bytes needs to be smaller than the total size,
    // otherwise the allocator function messed up somewhere by missing this
    // gap
    ASSERT((int)trailing_free < totalsize);

    // Thus, we only need to fetch the desired number of bytes (including
    // offset) - minus the current free number of bytes till table end
    int to_expand = totalsize - (int)trailing_free;

    // Very simple assertion: The number of bytes by which we want to expand
    // the table, plus the current number of free following bytes, needs to
    // match the total size of bytes we need for our new segment
    ASSERT(to_expand + (int)end->free_following == totalsize);

    // We only want to expand by pages, not by some smaller values to avoid
    // frequent syscalls
    size_t num_pages = (size_t)ceil((double)to_expand / PAGE_SIZE);

    pr_info("Expanding by size %d", to_expand);

    if (sbrk(num_pages * PAGE_SIZE) == (void *)-1) {
        // In this case, sbrk failed to allocate and we need to abort

        pr_error("sbrk error: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    pr_info("Expanded list by %d", to_expand);

    // Update the tail pointer by the number of bytes we expanded the list
    // by
    start->end_addr += num_pages * PAGE_SIZE;

    // Update free following bytes counter of last tail by the number of
    // bytes we expanded the table with
    end->free_following += num_pages * PAGE_SIZE;

    // Check that the end_addr pointer is still valid: The address of the
    // last tail, plus the tail size, plus the number of free following
    // bytes matches the end address of the storage table
    ASSERT((uint8_t *)end + sizeof(*end) + end->free_following ==
           start->end_addr);

    return (uint8_t *)end + sizeof(*end);
}

// Search for a gap or expand the table if no gap is found
uint8_t *find_free_seg(size_t size) {

    // If table is not initialized, set up table and expand it
    if (!start) {

        start = create_list();

        // In the unlikely case sbrk failed, return nullptr
        if (!start) {
            return nullptr;
        }
        return expand_list(size);
    }

    pr_info("Start already initialized");

    // If table is initialized, search for a gap with one of the alloc
    // algorithms set previously
    uint8_t *new_addr = g_alloc_function(start, size);

    // If no gap has been found, the table needs to be expanded and the
    // beginning of the storage table will be returned
    if (!new_addr) {
        pr_warning("Did not find a gap");
        return expand_list(size);
    }

    // Return address which points to the first unallocated storage either after
    // some tail of another segment, or after the storage table header. new_addr
    // does NOT point to some kind of actually usable storage area for the user,
    // this happens later in add_entry.
    return new_addr;
}

// This function sets the allocation function pointer being used by
// find_free_seg. Only really useful for debugging purposes
void set_alloc_function(sched_strat_e strat) {
    switch (strat) {
    case BEST_FIT:
        g_alloc_function = &best_fit;
        break;
    case FIRST_FIT:
        g_alloc_function = &first_fit;
        break;
    case NEXT_FIT:
        g_alloc_function = &next_fit;
        break;
    case WORST_FIT:
        g_alloc_function = &worst_fit;
        break;
    }
}

// This function completely erases the storage table by clearing the last_addr
// value for next fit, clearing the first segment pointer, by moving the tail of
// the storage list to the beginning, and by resetting the program break. Only
// useful for debugging.
void clear_alloc_storage() {

    // Reset last_addr value used by next_fit
    set_last_addr(nullptr);

    // Reset header, tail and program break of storage table header. If
    // reset_list failed, brk failed and we abort.
    if (reset_list(start)) {

        pr_error("Unusual sbreak error: %s", strerror(errno));

        abort();
    }
}
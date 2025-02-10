#include "alloc/storage.h"
#include "alloc/types.h"

#include <stdlib.h>

// This function copies one (1) byte from one address to another
static int copy_byte(uint8_t *addr_old, uint8_t *addr_new) {

    // Discard undefined addresses
    if (!addr_old | !addr_new) {

        pr_error("Invalid pointers");

        return ERROR;
    }

    // If the old address is the same as the new address, nothing has to be done
    if (addr_old == addr_new) {

        // pr_info("Nothing to do");

        return SUCCESS;
    }

    // Read the byte of the old memory location where addr_old points to
    uint8_t v_old = read_byte(addr_old);

    // Set the byte at the memory where addr_new points to to the old byte
    set_byte(addr_new, v_old);

    return SUCCESS;
}

// Simple wrapper function which reads a byte of a memory location where addr
// points to. The rationale for this function is to avoid direct (possibly
// undefined) reads on not uint8_t* pointers, thus to make clear we are only
// reading one byte, not a struct etc.
uint8_t read_byte(const uint8_t *addr) {

    // addr might be null. We don't separately handle this case here, except
    // returning 0, this case should be handled in the handling copy_byte
    // function.
    if (!addr) {

        pr_error(
            "Undefined behaviour. Check case handling in calling functions");

        abort();
    }

    // Return byte
    return *addr;
}

// Simple wrapper function which sets a byte of a memory location where addr
// points to. The rationale for this function is to avoid direct (possibly
// undefined) writes on not uint8_t* pointers, thus to make clear we are setting
// one byte, and not one struct etc.
void set_byte(uint8_t *addr, uint8_t v) {

    // addr might be nullptr, but we don't handle this case here as it should be
    // handled in the calling function copy_byte, anything else is undefined
    // behaviour anyways.
    if (!addr) {

        pr_error(
            "Undefined behaviour, check case handling in caling functions");
        abort();
    }

    *addr = v;
}

// This function sets size many bytes of the memory location where addr points
// to to zero
int set_mem_zero(uint8_t *addr, size_t size) {

    // size many bytes are set to zero in a loop
    for (size_t i = 0; i < size; i++) {

        // Set byte at offset location to zero
        set_byte(addr + i, 0);
    }

    return SUCCESS;
}

// This function copies size many bytes from a old memory location to a new
// memory location.

// Note that if the new address is in the range [old_addr,
// old_addr+size), we would overwrite the old storage wile writing because we
// are reading in increasing memory address. In such case, yes, we could simply
// read backwards, but such case distinction goes way beyond the scope of what
// this function should do, since it should *never* happen that the new address
// points in mentioned range above.

// The only reasonable case being covered here is that [new_addr, new_addr +
// size)\cap[old_addr, old_addr+size) is not empty *and* that new_addr<old_addr.
// This should not happen either based on the framework, but at least this case
// makes more sense if we think about e.g. moving one storage segment to a
// beginning of a gap (which this storage framework does not either)
int copy_mem(uint8_t *old_addr, uint8_t *new_addr, size_t size) {

    // Discard invalid addresses
    if (!old_addr || !new_addr) {

        pr_error("Undefined behaviour");

        return ERROR;
    }

    // If new_addr is in the range of to be copied data, don't copy
    if (old_addr + size > new_addr && new_addr > old_addr) {

        pr_error("Invalid new address. Expect heap corruption");

        return ERROR;
    }

    // Read size many bytes from old_addr
    for (size_t i = 0; i < size; i++) {

        // Copy one byte each per offset
        if (copy_byte(old_addr + i, new_addr + i)) {

            return ERROR;
        }
    }

    return SUCCESS;
}
#include <stddef.h>
#include <assert.h>
#include <sys/mman.h>
#include <stdatomic.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <linux/futex.h>
#include <sys/syscall.h>

#include "fastpipe_ring.h"

#define SPIN_COUNT  100

/*
 * Calculate the total size of a fastpipe ring buffer.
 * Arguments:
 *     uint32_t capacity - Total number of elements in buffer.
 *     uint32_t element_size - Size of individual elements.
 * Returns:
 *     size_t - Total size of the fastpipe_ring being described.
 */
static size_t fastpipe_ring_size(uint32_t capacity, uint32_t element_size) {
    return sizeof(struct fastpipe_ring) + capacity * element_size;
}

/*
 * Create a fastpipe ring buffer.
 * Arguments:
 *     int fd - A file descriptor to back the buffer.
 *     uint32_t capacity - How many total elements can be stored in the buffer (must be power of two).
 *     uint32_t element_size - How large are the elements in the buffer?
 *     int initialize - Is the buffer being created or attached?
 * Returns:
 *     fastpipe_ring * - Pointer to a new fastpipe_ring.
 */
struct fastpipe_ring *fastpipe_ring_create(int fd, uint32_t capacity, uint32_t element_size, int initialize) {
    // Make sure the capacity is a power of two
    assert((capacity & (capacity - 1)) == 0);

    // Initialize the file for backing
    size_t size = fastpipe_ring_size(capacity, element_size);
    if(initialize) {
        if(ftruncate(fd, size) == -1)
            return NULL;
    }

    /* Allocate the memory backing the fastpipe ring buffer. */
    struct fastpipe_ring *r = mmap(
        NULL,
        size,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        fd,
        0
    );

    // Make sure the map returns valid memory
    if(r == MAP_FAILED)
        return NULL;

    // If we aren't initializing, wait until the initializer is ready
    if(!initialize) {
        while(atomic_load_explicit(&r->ready, memory_order_acquire) == 0);
    }

    // Initialize buffer metadata if creating
    if(initialize) {
        r->capacity = capacity;
        r->element_size = element_size;
        r->indexer = capacity - 1;

        atomic_store(&r->head, 0);
        atomic_store(&r->tail, 0);
        atomic_store(&r->not_empty, 0);
        atomic_store(&r->not_full, 0);

        atomic_store_explicit(&r->ready, 1, memory_order_release);
    }

    return r;
}

/*
 * Destroy an instance of a fastpipe ring buffer.
 * Arguments:
 *     fastpipe_ring *r - Pointer to the ring to destroy.
 */
void fastpipe_ring_destroy(struct fastpipe_ring *r) {
    /* Unmap the memory backing the fastpipe ring buffer */
    size_t size = fastpipe_ring_size(r->capacity, r->element_size);
    munmap(r, size);
}

/*
 * Attempt push to a fastpipe ring buffer.
 * Arguments:
 *     fastpipe_ring *r - Push to this ring.
 *     const void *msg - Push this message.
 * Returns:
 *     int - 0 on success, -1 on failure.
 */
int try_fastpipe_ring_push(struct fastpipe_ring *r, const void *msg) {
    // Load the head and tail of the buffer
    uint32_t h = atomic_load_explicit(&r->head, memory_order_relaxed);
    uint32_t t = atomic_load_explicit(&r->tail, memory_order_acquire);

    // Calculate the next position of a potentially empty slot
    uint32_t next = (h + 1) & r->indexer;
    if(next == t)
        return -1;

    // This is where the data is stored.
    char *buffer = (char *)(r + 1);

    // Copy the data into the next slot and update the head
    memcpy(buffer + next * r->element_size, msg, r->element_size);
    atomic_store_explicit(&r->head, next, memory_order_release);
    return 0;    
}

/*
 * Attempt pop from a fastpipe ring buffer.
 * Arguments:
 *     fastpipe_ring *r - Pop from this ring.
 *     void *msg - Store the message here.
 * Returns:
 *     int - 0 on success, -1 on failure.
 */
int try_fastpipe_ring_pop(struct fastpipe_ring *r, void *msg) {
    // Load the head and tail of the buffer
    uint32_t t = atomic_load_explicit(&r->tail, memory_order_relaxed);
    uint32_t h = atomic_load_explicit(&r->head, memory_order_acquire);

    // Check if the buffer is empty
    if(t == h)
        return -1;

    // Calculate the next position of a filled spot
    uint32_t next = (t + 1) & r->indexer;
    
    // This is where the data is stored
    char *buffer = (char *)(r + 1);
    
    // Copy the data to the user and update the tail
    memcpy(msg, buffer + next * r->element_size, r->element_size);
    atomic_store_explicit(&r->tail, next, memory_order_release);
    return 0;
}

/*
 * Blocking push (push to fastipe ring buffer only when able to).
 * Arguments:
 *     fastpipe_ring *r - Push to this ring.
 *     const void *msg - Push this message.
 * Returns:
 *     int - 0 on success, -1 on failure.
 */
int fastpipe_ring_push(struct fastpipe_ring *r, const void *msg) {
    // This is where the data is stored
    char *buffer = (char *)(r + 1);

    while(1) {
        // Load the head and tail of the buffer
        uint32_t h = atomic_load_explicit(&r->head, memory_order_relaxed);
        uint32_t t = atomic_load_explicit(&r->tail, memory_order_acquire);

        // Transition state
        int was_empty = (h == t);

        // Calculate the next position of a potentially empty slot
        uint32_t next = (h + 1) & r->indexer;
        if(next != t) {
            // Copy the data into the next slot and update the head
            memcpy(buffer + next * r->element_size, msg, r->element_size);
            atomic_store_explicit(&r->head, next, memory_order_release);

            // Wake consumer (data available)
            if(was_empty) {
                atomic_fetch_add(&r->not_empty, 1);
                syscall(SYS_futex, &r->not_empty, FUTEX_WAKE, 1);
            }

            return 0;   
        }

        // Wait for consumer
        uint32_t val = atomic_load(&r->not_full);
        syscall(SYS_futex, &r->not_full, FUTEX_WAIT, val);
    }
}

/*
 * Blocking pop (pop from fastpipe ring buffer only when able to).
 * Arguments:
 *     fastpipe_ring *r - Pop from this ring.
 *     void *msg - Pop to this address.
 * Returns:
 *     int - 0 on success, -1 on failure.
 */
int fastpipe_ring_pop(struct fastpipe_ring *r, void *msg) {
    // This is where the data is stored
    char *buffer = (char *)(r + 1);

    while(1) {
        // Load the head and tail of the buffer
        uint32_t t = atomic_load_explicit(&r->tail, memory_order_relaxed);
        uint32_t h = atomic_load_explicit(&r->head, memory_order_acquire);
        
        // Check if the buffer is not empty
        if(t != h) {
            // Calculate the next position of a filled spot
            uint32_t next = (t + 1) & r->indexer;
            
            // Transition state
            int was_full = (next == h);
            
            // Copy the data to the user and update the tail
            memcpy(msg, buffer + next * r->element_size, r->element_size);
            atomic_store_explicit(&r->tail, next, memory_order_release);

            // Wake the producer (empty slots available)
            if(was_full) {
                atomic_fetch_add(&r->not_full, 1);
                syscall(SYS_futex, &r->not_full, FUTEX_WAKE, 1);
            }

            return 0;
        }

        // Wait for producer
        uint32_t val = atomic_load(&r->not_empty);
        syscall(SYS_futex, &r->not_empty, FUTEX_WAIT, val);
    }
}

/*
 * Zero copy API reserve new slot.
 * Arguments:
 *     struct fastpipe_ring *r - Reserve a slot from this ring.
 * Returns:
 *     void * - Pointer to the slot.
 */
void *fastpipe_ring_reserve(struct fastpipe_ring *r) {
    // This is where the data is stored
    char *buffer = (char *)(r + 1);

    while(1) {
        // Load the head and tail of the buffer
        uint32_t h = atomic_load(&r->head);
        uint32_t t = atomic_load(&r->tail);

        // Ensure the buffer is not full
        uint32_t next = (h + 1) & r->indexer;
        if(next != t)
            return buffer + next * r->element_size;

        // Wait for consumer
        uint32_t val = atomic_load(&r->not_full);
        syscall(SYS_futex, &r->not_full, FUTEX_WAIT, val);
    }
}

/*
 * Zero copy API to commit new slot.
 * Arguments:
 *     struct fastpipe_ring *r - Commit the slot to this ring.
 */
void fastpipe_ring_commit(struct fastpipe_ring *r) {
    // Load the head and tail of the buffer
    uint32_t h = atomic_load(&r->head);
    uint32_t t = atomic_load(&r->tail);

    // Calculate the next avaiable slot
    uint32_t next = (h + 1) & r->indexer;

    // Transition state
    int was_empty = (h == t);

    // Update the head position
    atomic_store_explicit(&r->head, next, memory_order_release);

    // Wake the consumer (data available)
    if(was_empty) {
        atomic_fetch_add(&r->not_empty, 1);
        syscall(SYS_futex, &r->not_empty, FUTEX_WAKE, 1);
    }
}

/*
 * Zero copy API to peek new slot.
 * Arguments:
 *     struct fastpipe_ring *r - Peek a slot from this ring.
 * Returns:
 *     void * - Pointer to the slot.
 */
void *fastpipe_ring_peek(struct fastpipe_ring *r) {
    // This is where the data is stored
    char *buffer = (char *)(r + 1);

    while(1) {
        // Load the head and tail of the buffer
        uint32_t t = atomic_load(&r->tail);
        uint32_t h = atomic_load(&r->head);

        // Ensure the buffer is not empty
        if(t != h) {
            // Calculate the next available slot
            uint32_t next = (t + 1) & r->indexer;
            return buffer + next * r->element_size;
        }

        // Wait for producer
        uint32_t val = atomic_load(&r->not_empty);
        syscall(SYS_futex, &r->not_empty, FUTEX_WAIT, val);
    }
}

/*
 * Zero copy API to release new slot.
 * Arguments:
 *     struct fastpipe_ring *r - Release a slot from this ring.
 */
void fastpipe_ring_release(struct fastpipe_ring *r) {
    // Load the head and tail of the buffer
    uint32_t t = atomic_load(&r->tail);
    uint32_t h = atomic_load(&r->head);

    // Calculate the next available slot
    uint32_t next = (t + 1) & r->indexer;

    // Transition state
    int was_full = (next == h);

    atomic_store_explicit(&r->tail, next, memory_order_release);

    // Wake up the producer (slots available)
    if(was_full) {
        atomic_fetch_add(&r->not_full, 1);
        syscall(SYS_futex, &r->not_full, FUTEX_WAKE, 1);
    }
}
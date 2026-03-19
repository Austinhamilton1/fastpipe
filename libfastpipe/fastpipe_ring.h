#ifndef FASTPIPE_RING_H
#define FASTPIPE_RING_H

#include <stdint.h>

#define CACHE_SIZE  64

struct fastpipe_ring {
    // Fastpipe ring buffer metadata
    uint32_t    capacity;
    uint32_t    element_size;
    uint32_t    indexer;
    uint32_t    ready;

    // Padding to align on cachelines
    char        _pad1[CACHE_SIZE-16];

    // Producer pushes to this index
    uint32_t    head;

    // Padding to align on cacheline
    char        _pad2[CACHE_SIZE-4];

    // Consumer pops from this index
    uint32_t    tail;

    // Padding to align on cacheline
    char        _pad3[CACHE_SIZE-4];

    // Signifies the buffer is no longer empty (reader can read)
    uint32_t    not_empty;

    // Padding to align on cacheline
    char        _pad4[CACHE_SIZE-4];

    // Signifies the buffer is no longer full (writer can write)
    uint32_t    not_full;
};

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
struct fastpipe_ring *fastpipe_ring_create(int fd, uint32_t capacity, uint32_t element_size, int initialize);

/*
 * Destroy an instance of a fastpipe ring buffer.
 * Arguments:
 *     fastpipe_ring *r - Pointer to the ring to destroy.
 */
void fastpipe_ring_destroy(struct fastpipe_ring *r);

/*
 * Attempt push to a fastpipe ring buffer.
 * Arguments:
 *     fastpipe_ring *r - Push to this ring.
 *     const void *msg - Push this message.
 * Returns:
 *     int - 0 on success, -1 on failure.
 */
int try_fastpipe_ring_push(struct fastpipe_ring *r, const void *msg);

/*
 * Attempt pop from a fastpipe ring buffer.
 * Arguments:
 *     fastpipe_ring *r - Pop from this ring.
 *     void *msg - Store the message here.
 * Returns:
 *     int - 0 on success, -1 on failure.
 */
int try_fastpipe_ring_pop(struct fastpipe_ring *r, void *msg);

/*
 * Blocking push (push to fastipe ring buffer only when able to).
 * Arguments:
 *     fastpipe_ring *r - Push to this ring.
 *     const void *msg - Push this message.
 * Returns:
 *     int - 0 on success, -1 on failure.
 */
int fastpipe_ring_push(struct fastpipe_ring *r, const void *msg);

/*
 * Blocking pop (pop from fastpipe ring buffer only when able to).
 * Arguments:
 *     fastpipe_ring *r - Pop from this ring.
 *     void *msg - Pop to this address.
 * Returns:
 *     int - 0 on success, -1 on failure.
 */
int fastpipe_ring_pop(struct fastpipe_ring *r, void *msg);

/*
 * Zero copy API reserve new slot.
 * Arguments:
 *     struct fastpipe_ring *r - Reserve a slot from this ring.
 * Returns:
 *     void * - Pointer to the slot.
 */
void *fastpipe_ring_reserve(struct fastpipe_ring *r);

/*
 * Zero copy API to commit new slot.
 * Arguments:
 *     struct fastpipe_ring *r - Commit the slot to this ring.
 */
void fastpipe_ring_commit(struct fastpipe_ring *r);

/*
 * Zero copy API to peek new slot.
 * Arguments:
 *     struct fastpipe_ring *r - Peek a slot from this ring.
 * Returns:
 *     void * - Pointer to the slot.
 */
void *fastpipe_ring_peek(struct fastpipe_ring *r);

/*
 * Zero copy API to release new slot.
 * Arguments:
 *     struct fastpipe_ring *r - Release a slot from this ring.
 */
void fastpipe_ring_release(struct fastpipe_ring *r);

#endif
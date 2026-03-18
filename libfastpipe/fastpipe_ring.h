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
 * Push to a fastpipe ring buffer.
 * Arguments:
 *     fastpipe_ring *r - Push to this ring.
 *     const void *msg - Push this message.
 * Returns:
 *     int - 0 on success, -1 on failure.
 */
int fastpipe_ring_push(struct fastpipe_ring *r, const void *msg);

/*
 * Pop from a fastpipe ring buffer.
 * Arguments:
 *     fastpipe_ring *r - Pop from this ring.
 *     void *msg - Store the message here.
 * Returns:
 *     int - 0 on success, -1 on failure.
 */
int fastpipe_ring_pop(struct fastpipe_ring *r, void *msg);

#endif
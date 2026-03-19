#ifndef FASTPIPE_H
#define FASTPIPE_H

#include <stdint.h>

#include "fastpipe_ring.h"

#define MAX_FASTPIPE_NAME 128

/* Is this the procuer or consumer process? */
enum ROLE {
    PRODUCER,
    CONSUMER,
};

/* Data structure for fastpipe metadata. */
struct fastpipe {
    // Is this the producer or consumer?
    enum ROLE role;

    // Pipe file descriptor and pipe name
    int fd;
    char name[MAX_FASTPIPE_NAME];

    // Virtual ring buffer addresses
    struct fastpipe_ring *ring;
};

/*
 * Create a new fastpipe.
 * Arguments:
 *     const char *name - The (unique) name of the fastpipe.
 * Returns:
 *     struct fastpipe * - Newly created fastpipe.
 */
struct fastpipe *fastpipe_create(const char *name);

/*
 * Free up a fastpipe.
 * Arguments:
 *     struct fastpipe *fastpipe - Fastpipe to destroy.
 */
void fastpipe_destroy(struct fastpipe *fastpipe);

/*
 * Bind a fastpipe to the current process.
 * Arguments:
 *     struct fastpipe *fastpipe - Bind this pipe.
 *     uint32_t capacity - Max values in the fastpipe (must be a power of two).
 *     uint32_t element_size - Size of the elements in the fastpipe.
 *     enum ROLE role - Bind from producer or consumer side.
 * Returns:
 *     int - 0 on success, -1 on failure.
 */
int fastpipe_bind(struct fastpipe *fastpipe, uint32_t capacity, uint32_t element_size, enum ROLE role);

/*
 * Push a message to the fastpipe.
 * Arguments:
 *     struct fastpipe *fastpipe - Push to this fastpipe.
 *     const void *msg - Push this message.
 *     int block - Should this push block?
 * Returns:
 *     int - 0 on success, -1 on failure.
 */
int fastpipe_push(struct fastpipe *fastpipe, const void *msg, int block);

/*
 * Pop a message from the fastpipe.
 * Arguments:
 *     struct fastpipe *fastpipe - Pop from this fastpipe.
 *     void *msg - Pop message to this address.
 *     int block - Should this pop block?
 * Returns:
 *     int - 0 on success, -1 on failure.
 */
int fastpipe_pop(struct fastpipe *fastpipe, void *msg, int block);

/*
 * Zero copy API reserve new slot.
 * Arguments:
 *     struct fastpipe *fastpipe - Reserve a slot from this fastpipe.
 * Returns:
 *     void * - Pointer to the slot.
 */
void *fastpipe_reserve(struct fastpipe *fastpipe);

/*
 * Zero copy API to commit new slot.
 * Arguments:
 *     struct fastpipe *fastpipe - Commit the slot to this fastpipe.
 */
void fastpipe_commit(struct fastpipe *fastpipe);

/*
 * Zero copy API to peek new slot.
 * Arguments:
 *     struct fastpipe *fastpipe - Peek a slot from this fastpipe.
 * Returns:
 *     void * - Pointer to the slot.
 */
void *fastpipe_peek(struct fastpipe *fastpipe);

/*
 * Zero copy API to release new slot.
 * Arguments:
 *     struct fastpipe_ring *r - Release a slot from this fastpipe.
 */
void fastpipe_release(struct fastpipe *fastpipe);

#endif
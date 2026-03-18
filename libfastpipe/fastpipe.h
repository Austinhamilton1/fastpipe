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

    // Ring buffer metadata
    uint32_t capacity;
    uint32_t element_size;

    // Virtual ring buffer addresses
    struct fastpipe_ring *ring;
    
};

/*
 * Create a new fastpipe.
 * Arguments:
 *     const char *name - The (unique) name of the fastpipe.
 *     uint32_t capacity - Max capacity of the fastpipe (must be power of two)
 *     uint32_t element_size - Size of elements in the fastpipe.
 * Returns:
 *     struct fastpipe * - Newly created fastpipe.
 */
struct fastpipe *fastpipe_create(const char *name, uint32_t capacity, uint32_t element_size);

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
 *     enum ROLE role - Bind from producer or consumer side.
 * Returns:
 *     int - 0 on success, -1 on failure.
 */
int fastpipe_bind(struct fastpipe *fastpipe, enum ROLE role);

/*
 * Push a message to the fastpipe.
 * Arguments:
 *     struct fastpipe *fastpipe - Push to this fastpipe.
 *     const void *msg - Push this message.
 * Returns:
 *     int - 0 on success, -1 on failure.
 */
int fastpipe_push(struct fastpipe *fastpipe, const void *msg);

/*
 * Pop a message from the fastpipe.
 * Arguments:
 *     struct fastpipe *fastpipe - Pop from this fastpipe.
 *     void *msg - Pop message to this address.
 * Returns:
 *     int - 0 on success, -1 on failure.
 */
int fastpipe_pop(struct fastpipe *fastpipe, void *msg);

#endif
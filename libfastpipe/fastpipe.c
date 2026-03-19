#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include "fastpipe.h"

/*
 * Create a new fastpipe.
 * Arguments:
 *     const char *name - The (unique) name of the fastpipe.
 *     uint32_t capacity - Max capacity of the fastpipe (must be power of two)
 *     uint32_t element_size - Size of elements in the fastpipe.
 * Returns:
 *     struct fastpipe * - Newly created fastpipe.
 */
struct fastpipe *fastpipe_create(const char *name, uint32_t capacity, uint32_t element_size) {
    // Ensure capacity is a power of two
    assert((capacity & (capacity - 1)) == 0);

    char fastpipe_name[MAX_FASTPIPE_NAME] = { 0 };
    snprintf(fastpipe_name, MAX_FASTPIPE_NAME - 6, "/tmp/%s", name);

    // Allocate the new fastpipe
    struct fastpipe *fastpipe = malloc(sizeof(struct fastpipe));
    if(!fastpipe) 
        return NULL;

    // Instantiate the new fastpipe
    memcpy(fastpipe->name, fastpipe_name, MAX_FASTPIPE_NAME);
    fastpipe->capacity = capacity;
    fastpipe->element_size = element_size;

    return fastpipe;
}

/*
 * Free up a fastpipe.
 * Arguments:
 *     struct fastpipe *fastpipe - Fastpipe to destroy.
 */
void fastpipe_destroy(struct fastpipe *fastpipe) {
    // Ensure a valid fastpipe was passed in
    if(!fastpipe)
        return;

    // Free the fastpipe_ring memory
    if(fastpipe->ring)
        fastpipe_ring_destroy(fastpipe->ring);

    // Free the backing file
    if(fastpipe->fd >= 0)
        close(fastpipe->fd);

    // Free the fastpipe allocation
    free(fastpipe);
}

/*
 * Bind a fastpipe to the current process.
 * Arguments:
 *     struct fastpipe *fastpipe - Bind this pipe.
 *     enum ROLE role - Bind from producer or consumer side.
 * Returns:
 *     int - 0 on success, -1 on failure.
 */
int fastpipe_bind(struct fastpipe *fastpipe, enum ROLE role) {
    // Ensure valid fastpipe
    if(!fastpipe)
        return -1;

    fastpipe->role = role;

    /* Check the status of the backing file to determine if the ring buffer needs to be created or attached */
    int fd = open(fastpipe->name, O_RDWR | O_CREAT | O_EXCL, 0666);
    if(fd > 0) {
        fastpipe->fd = fd;
        fastpipe->ring = fastpipe_ring_create(fastpipe->fd, fastpipe->capacity, fastpipe->element_size, 1);
    } else {
        fastpipe->fd = open(fastpipe->name, O_RDWR);
        if(fastpipe->fd < 0)
            return -1;
        fastpipe->ring = fastpipe_ring_create(fastpipe->fd, fastpipe->capacity, fastpipe->element_size, 0);
    }

    return fastpipe->ring ? 0 : -1;
}

/*
 * Push a message to the fastpipe.
 * Arguments:
 *     struct fastpipe *fastpipe - Push to this fastpipe.
 *     const void *msg - Push this message.
 *     int block - Should this push block?
 * Returns:
 *     int - 0 on success, -1 on failure.
 */
int fastpipe_push(struct fastpipe *fastpipe, const void *msg, int block) {
    // Ensure valid fastpipe
    if(!fastpipe)
        return -1;

    // Ensure the correct role is pushing
    if(fastpipe->role != PRODUCER)
        return -1;

    // Ensure valid ring
    if(!fastpipe->ring) 
        return -1;

    // Push the message to the ring
    if(block)
        return fastpipe_ring_push(fastpipe->ring, msg);
    else
        return try_fastpipe_ring_push(fastpipe->ring, msg);
}

/*
 * Pop a message from the fastpipe.
 * Arguments:
 *     struct fastpipe *fastpipe - Pop from this fastpipe.
 *     void *msg - Pop message to this address.
 *     int block - Should this pop block?
 * Returns:
 *     int - 0 on success, -1 on failure.
 */
int fastpipe_pop(struct fastpipe *fastpipe, void *msg, int block) {
    // Ensure valid fastpipe
    if(!fastpipe)
        return -1;

    // Ensure the correct role is pushing
    if(fastpipe->role != CONSUMER)
        return -1;

    // Ensure valid ring
    if(!fastpipe->ring) 
        return -1;

    // Push the message to the ring
    if(block)
        return fastpipe_ring_pop(fastpipe->ring, msg);
    else
        return try_fastpipe_ring_pop(fastpipe->ring, msg);
}

/*
 * Zero copy API reserve new slot.
 * Arguments:
 *     struct fastpipe *fastpipe - Reserve a slot from this fastpipe.
 * Returns:
 *     void * - Pointer to the slot.
 */
void *fastpipe_reserve(struct fastpipe *fastpipe) {
    // Ensure valid fastpipe
    if(!fastpipe)
        return NULL;

    // Ensure the correct role is reserving
    if(fastpipe->role != PRODUCER)
        return NULL;

    // Ensure valid ring
    if(!fastpipe->ring)
        return NULL;

    return fastpipe_ring_reserve(fastpipe->ring);
}

/*
 * Zero copy API to commit new slot.
 * Arguments:
 *     struct fastpipe *fastpipe - Commit the slot to this fastpipe.
 */
void fastpipe_commit(struct fastpipe *fastpipe) {
    // Ensure valid fastpipe
    if(!fastpipe)
        return;

    // Ensure the correct role is reserving
    if(fastpipe->role != PRODUCER)
        return;

    // Ensure valid ring
    if(!fastpipe->ring)
        return;

    fastpipe_ring_commit(fastpipe->ring);
}

/*
 * Zero copy API to peek new slot.
 * Arguments:
 *     struct fastpipe *fastpipe - Peek a slot from this fastpipe.
 * Returns:
 *     void * - Pointer to the slot.
 */
void *fastpipe_peek(struct fastpipe *fastpipe) {
    // Ensure valid fastpipe
    if(!fastpipe)
        return NULL;

    // Ensure the correct role is reserving
    if(fastpipe->role != CONSUMER)
        return NULL;

    // Ensure valid ring
    if(!fastpipe->ring)
        return NULL;

    return fastpipe_ring_peek(fastpipe->ring);
}

/*
 * Zero copy API to release new slot.
 * Arguments:
 *     struct fastpipe_ring *r - Release a slot from this fastpipe.
 */
void fastpipe_release(struct fastpipe *fastpipe) {
    // Ensure valid fastpipe
    if(!fastpipe)
        return;

    // Ensure the correct role is reserving
    if(fastpipe->role != CONSUMER)
        return;

    // Ensure valid ring
    if(!fastpipe->ring)
        return;

    fastpipe_ring_release(fastpipe->ring);
}
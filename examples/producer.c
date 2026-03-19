#include <stdio.h>
#include <errno.h>
#include <stdint.h>

#include "fastpipe.h"

int main(int argc, char **argv) {
    uint32_t capacity = 128;
    uint32_t element_size = sizeof(int);
    int err;

    struct fastpipe *pipe = fastpipe_create("test");
    if(!pipe) {
        perror("producer: could not create fastpipe.");
        return -1;
    }

    err = fastpipe_bind(pipe, capacity, element_size, PRODUCER);
    if(err) {
        perror("producer: could not bind fastpipe");
        fastpipe_destroy(pipe);
        return -1;
    }

    for(int i = 0; i < 512; i++) {
        fastpipe_push(pipe, &i, 0);
        printf("Sent %d\n", i);
    }

    fastpipe_destroy(pipe);

    return 0;
}
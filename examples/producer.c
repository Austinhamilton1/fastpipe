#include <stdio.h>
#include <errno.h>
#include <stdint.h>

#include "fastpipe.h"

int main(int argc, char **argv) {
    int messages[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    uint32_t capacity = 16;
    uint32_t element_size = sizeof(int);
    int err;

    struct fastpipe *pipe = fastpipe_create("test", capacity, element_size);
    if(!pipe) {
        perror("producer: could not create fastpipe.");
        return -1;
    }

    err = fastpipe_bind(pipe, PRODUCER);
    if(err) {
        perror("producer: could not bind fastpipe");
        fastpipe_destroy(pipe);
        return -1;
    }

    for(int i = 0; i < 10; i++) {
        err = fastpipe_push(pipe, &messages[i]);
        if(err) {
            perror("producer: could not push value");
            fastpipe_destroy(pipe);
            return -1;
        }
        printf("Pushed %d\n", messages[i]);
    }

    fastpipe_destroy(pipe);

    return 0;
}
#include <stdio.h>
#include <errno.h>
#include <stdint.h>

#include "fastpipe.h"

int main(int argc, char **argv) {
    uint32_t capacity = 16;
    uint32_t element_size = sizeof(int);
    int err;

    struct fastpipe *pipe = fastpipe_create("test", capacity, element_size);
    if(!pipe) {
        perror("consumer: could not create fastpipe.");
        return -1;
    }

    err = fastpipe_bind(pipe, CONSUMER);
    if(err) {
        perror("consumer: could not bind fastpipe");
        fastpipe_destroy(pipe);
        return -1;
    }

    int recv_count = 0;
    while(recv_count < 10) {
        int received;
        err = fastpipe_pop(pipe, &received);
        if(err) {
            continue;
        }
        printf("Received %d\n", received);
        recv_count++;
    }

    fastpipe_destroy(pipe);

    return 0;
}
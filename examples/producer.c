#include <stdio.h>
#include <errno.h>
#include <stdint.h>

#include "fastpipe.h"

int main(int argc, char **argv) {
    uint32_t capacity = 128;
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

    int sent_messages = 0;
    while(sent_messages < 512) {
        int message = sent_messages + 1;
        if(fastpipe_push(pipe, &message) < 0)
            continue;
        printf("Sent %d\n", message);
        sent_messages++;
    }

    fastpipe_destroy(pipe);

    return 0;
}
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>

#include "fastpipe.h"

#define NUM_MESSAGES 1 << 25
#define LARGE_MESSAGES 1 << 5
#define LARGE_MESSAGE_SIZE 1 << 12

int main(int argc, char **argv) {
    int fd[2];
    struct timespec start, end;
    double pipe_elapsed_time, 
        fastpipe_elapsed_time,
        fastpipe_large_elapsed_time,
        fastpipe_large_zero_copy_time;
    pid_t pid;

    clock_gettime(CLOCK_MONOTONIC, &start);
    // Create a Linux pipe as a base test
    if(pipe(fd) == -1) return -1;
    if((pid = fork()) == 0) {
        // Child: Read from pipe
        close(fd[1]);
        int message;
        for(int i = 0; i < NUM_MESSAGES; i++) {
            read(fd[0], &message, sizeof(message));
        }
        close(fd[0]);
        _exit(0);
    } else {
        // Parent: Write to pipe
        close(fd[0]);
        for(int i = 0; i < NUM_MESSAGES; i++) {
            int message = i + 1;
            write(fd[1], &message, sizeof(message));
        }
        close(fd[1]);
        waitpid(pid, NULL, 0);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    pipe_elapsed_time = 
        (end.tv_sec - start.tv_sec) +
        (end.tv_nsec - start.tv_nsec) / 1e9; 

    printf("Pipe Execution time: %f seconds\n", pipe_elapsed_time);

    unlink("/tmp/test");

    clock_gettime(CLOCK_MONOTONIC, &start);
    if((pid = fork()) == 0) {
        // Child process: Read from pipe
        struct fastpipe *pipe = fastpipe_create("test", 1 << 15, sizeof(int));
        fastpipe_bind(pipe, CONSUMER);
        int message;
        for(int i = 0; i < NUM_MESSAGES; i++) {
            fastpipe_pop(pipe, &message, 1);
        }
        fastpipe_destroy(pipe);
        _exit(0);
    } else {
        // Parent process: Write to pipe
        struct fastpipe *pipe = fastpipe_create("test", 1 << 15, sizeof(int));
        fastpipe_bind(pipe, PRODUCER);
        int message;
        for(int i = 0; i < NUM_MESSAGES; i++) {
            fastpipe_push(pipe, &i, 1);
        }
        fastpipe_destroy(pipe);
        waitpid(pid, NULL, 0);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    fastpipe_elapsed_time = 
        (end.tv_sec - start.tv_sec) +
        (end.tv_nsec - start.tv_nsec) / 1e9;
        
    printf("FastPipe Execution time: %f seconds\n", fastpipe_elapsed_time);

    unlink("/tmp/test");

    clock_gettime(CLOCK_MONOTONIC, &start);
    if((pid = fork()) == 0) {
        // Child process: Read from pipe
        struct fastpipe *pipe = fastpipe_create("test", 1 << 10, LARGE_MESSAGE_SIZE);
        fastpipe_bind(pipe, CONSUMER);
        char *message = malloc(LARGE_MESSAGE_SIZE);
        for(int i = 0; i < NUM_MESSAGES; i++) {
            fastpipe_pop(pipe, message, 1);
        }
        free(message);
        fastpipe_destroy(pipe);
        _exit(0);
    } else {
        // Parent process: Write to pipe
        struct fastpipe *pipe = fastpipe_create("test", 1 << 10, LARGE_MESSAGE_SIZE);
        fastpipe_bind(pipe, PRODUCER);
        char *message = malloc(LARGE_MESSAGE_SIZE);
        for(int i = 0; i < NUM_MESSAGES; i++) {
            fastpipe_push(pipe, message, 1);
        }
        free(message);
        fastpipe_destroy(pipe);
        waitpid(pid, NULL, 0);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    fastpipe_large_elapsed_time = 
        (end.tv_sec - start.tv_sec) +
        (end.tv_nsec - start.tv_nsec) / 1e9;

    unlink("/tmp/test");

    printf("Large Data FastPipe Execution time: %f seconds\n", fastpipe_large_elapsed_time);
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    if((pid = fork()) == 0) {
        // Child process: Read from pipe
        struct fastpipe *pipe = fastpipe_create("test", 1 << 10, LARGE_MESSAGE_SIZE);
        fastpipe_bind(pipe, CONSUMER);
        char *message = malloc(LARGE_MESSAGE_SIZE);
        for(int i = 0; i < LARGE_MESSAGES; i++) {
            char *slot = fastpipe_peek(pipe);
            *slot = *message;
            fastpipe_release(pipe);
        }
        free(message);
        fastpipe_destroy(pipe);
        _exit(0);
    } else {
        // Parent process: Write to pipe
        struct fastpipe *pipe = fastpipe_create("test", 1 << 10, LARGE_MESSAGE_SIZE);
        fastpipe_bind(pipe, PRODUCER);
        char *message = malloc(LARGE_MESSAGE_SIZE);
        for(int i = 0; i < LARGE_MESSAGES; i++) {
            char *slot = fastpipe_reserve(pipe);
            *slot = *message;
            fastpipe_commit(pipe);
        }
        free(message);
        fastpipe_destroy(pipe);
        waitpid(pid, NULL, 0);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    fastpipe_large_zero_copy_time = 
        (end.tv_sec - start.tv_sec) +
        (end.tv_nsec - start.tv_nsec) / 1e9;

    printf("Large Data FastPipe Zero Copy Execution time: %f seconds\n", fastpipe_large_zero_copy_time);
    return 0;
}
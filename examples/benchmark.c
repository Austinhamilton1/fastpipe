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
        fastpipe_zero_copy_time,
        fastpipe_large_elapsed_time,
        fastpipe_large_zero_copy_time;
    pid_t pid;
    struct fastpipe *fpipe;

    // Create a Linux pipe as a base test
    if(pipe(fd) == -1) return -1;

    clock_gettime(CLOCK_MONOTONIC, &start);
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

    unlink("/tmp/int-pipe");
    fpipe = fastpipe_create("int-pipe");

    clock_gettime(CLOCK_MONOTONIC, &start);
    if((pid = fork()) == 0) {
        // Child process: Read from pipe
        fastpipe_bind(fpipe, 1 << 15, sizeof(int), CONSUMER);
        int message;
        for(int i = 0; i < NUM_MESSAGES; i++) {
            fastpipe_pop(fpipe, &message, 1);
        }
        _exit(0);
    } else {
        // Parent process: Write to pipe
        fastpipe_bind(fpipe, 1 << 15, sizeof(int), PRODUCER);
        int message;
        for(int i = 0; i < NUM_MESSAGES; i++) {
            fastpipe_push(fpipe, &i, 1);
        }
        waitpid(pid, NULL, 0);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    fastpipe_elapsed_time = 
        (end.tv_sec - start.tv_sec) +
        (end.tv_nsec - start.tv_nsec) / 1e9;
        
    fastpipe_destroy(fpipe);

    printf("FastPipe Execution time: %f seconds\n", fastpipe_elapsed_time);

    unlink("/tmp/int-pipe");

    fpipe = fastpipe_create("int-pipe");

    clock_gettime(CLOCK_MONOTONIC, &start);
    if((pid = fork()) == 0) {
        // Child process: Read from pipe
        fastpipe_bind(fpipe, 1 << 15, sizeof(int), CONSUMER);
        int message;
        for(int i = 0; i < NUM_MESSAGES; i++) {
            int *slot = fastpipe_peek(fpipe);
            message = *slot;
            fastpipe_release(fpipe);
        }
        _exit(0);
    } else {
        // Parent process: Write to pipe
        fastpipe_bind(fpipe, 1 << 15, sizeof(int), PRODUCER);
        int message;
        for(int i = 0; i < NUM_MESSAGES; i++) {
            int *slot = fastpipe_reserve(fpipe);
            message = *slot;
            fastpipe_commit(fpipe);
        }
        waitpid(pid, NULL, 0);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    fastpipe_zero_copy_time = 
        (end.tv_sec - start.tv_sec) +
        (end.tv_nsec - start.tv_nsec) / 1e9;

    fastpipe_destroy(fpipe);
        
    printf("FastPipe Zero Copy Execution time: %f seconds\n", fastpipe_zero_copy_time);


    unlink("/tmp/large-pipe");

    fpipe = fastpipe_create("large-pipe");
    clock_gettime(CLOCK_MONOTONIC, &start);
    if((pid = fork()) == 0) {
        // Child process: Read from pipe
        fastpipe_bind(fpipe, 1 << 10, LARGE_MESSAGE_SIZE, CONSUMER);
        char *message = malloc(LARGE_MESSAGE_SIZE);
        for(int i = 0; i < NUM_MESSAGES; i++) {
            fastpipe_pop(fpipe, message, 1);
            char c = message[LARGE_MESSAGE_SIZE - 1];
            if(c != 0) {
                printf("Invalid value\n");
                break;
            }
        }
        free(message);
        _exit(0);
    } else {
        // Parent process: Write to pipe
        fastpipe_bind(fpipe, 1 << 10, LARGE_MESSAGE_SIZE, PRODUCER);
        char *message = malloc(LARGE_MESSAGE_SIZE);
        message[LARGE_MESSAGE_SIZE - 1] = 0;
        for(int i = 0; i < NUM_MESSAGES; i++) {
            fastpipe_push(fpipe, message, 1);
        }
        free(message);
        waitpid(pid, NULL, 0);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    fastpipe_large_elapsed_time = 
        (end.tv_sec - start.tv_sec) +
        (end.tv_nsec - start.tv_nsec) / 1e9;

    fastpipe_destroy(fpipe);

    unlink("/tmp/large-pipe");

    printf("Large Data FastPipe Execution time: %f seconds\n", fastpipe_large_elapsed_time);
    
    fpipe = fastpipe_create("large-pipe");
    clock_gettime(CLOCK_MONOTONIC, &start);
    if((pid = fork()) == 0) {
        // Child process: Read from pipe
        fastpipe_bind(fpipe, 1 << 10, LARGE_MESSAGE_SIZE, CONSUMER);
        for(int i = 0; i < LARGE_MESSAGES; i++) {
            char *slot = fastpipe_peek(fpipe);
            char c = slot[LARGE_MESSAGE_SIZE - 1];
            fastpipe_release(fpipe);
            if(c != 0) {
                printf("Invalid value\n");
                break;
            }
        }
        _exit(0);
    } else {
        // Parent process: Write to pipe
        fastpipe_bind(fpipe, 1 << 10, LARGE_MESSAGE_SIZE, PRODUCER);
        for(int i = 0; i < LARGE_MESSAGES; i++) {
            char *slot = fastpipe_reserve(fpipe);
            slot[LARGE_MESSAGE_SIZE - 1] = 0;
            fastpipe_commit(fpipe);
        }
        waitpid(pid, NULL, 0);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    fastpipe_large_zero_copy_time = 
        (end.tv_sec - start.tv_sec) +
        (end.tv_nsec - start.tv_nsec) / 1e9;

    fastpipe_destroy(fpipe);

    printf("Large Data FastPipe Zero Copy Execution time: %f seconds\n", fastpipe_large_zero_copy_time);
    return 0;
}
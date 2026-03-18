#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sched.h>

#include "fastpipe.h"

#define NUM_MESSAGES 1 << 25

int main(int argc, char **argv) {
    int fd[2];
    struct timespec start, end;
    double pipe_elapsed_time, fastpipe_elapsed_time;
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

    unlink("/tmp/test");

    clock_gettime(CLOCK_MONOTONIC, &start);
    if((pid = fork()) == 0) {
        // Child process: Read from pipe
        struct fastpipe *pipe = fastpipe_create("test", 1 << 15, sizeof(int));
        fastpipe_bind(pipe, CONSUMER);
        int message;
        int recv_count = 0;
        while(recv_count < NUM_MESSAGES) {
            int res = fastpipe_pop(pipe, &message);
            if(res < 0) {
                sched_yield();
                continue;
            }
            recv_count++;
        }
        fastpipe_destroy(pipe);
        _exit(0);
    } else {
        // Parent process: Write to pipe
        struct fastpipe *pipe = fastpipe_create("test", 1 << 15, sizeof(int));
        fastpipe_bind(pipe, PRODUCER);
        int message;
        int send_count = 0;
        while(send_count < NUM_MESSAGES) {
            message = send_count + 1;
            int res = fastpipe_push(pipe, &message);
            if(res < 0) {
                sched_yield();
                continue;
            }
            send_count++;
        }
        fastpipe_destroy(pipe);
        waitpid(pid, NULL, 0);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    fastpipe_elapsed_time = 
        (end.tv_sec - start.tv_sec) +
        (end.tv_nsec - start.tv_nsec) / 1e9; 

    printf("Pipe Execution time: %f seconds\n", pipe_elapsed_time);
    printf("FastPipe Execution time: %f seconds\n", fastpipe_elapsed_time);

    return 0;
}
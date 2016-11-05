#include <iostream>
#include <fstream>
#include <cstring>
#include <wait.h>
#include <cassert>


#include "util.h"



static int child_func(void *arg) {

    char* params[10];
    int status;
    int input_pd = ((int*)arg)[0];
    int output_pd = ((int*)arg)[1];
    aucont::start_context ctx;

    read(input_pd, &status, sizeof(int));
    if (status != EXIT_SUCCESS) {
        exit(EXIT_SUCCESS);
    }
    read(input_pd, &ctx, sizeof(aucont::start_context));

    if (ctx.is_daemon) daemonize_after_fork(); // TODO mv to prepare container

    status = aucont::prepare_container(ctx);
    write(output_pd, &status, sizeof(int));
    if (status != EXIT_SUCCESS) {
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < ctx.argc; ++i) params[i] = ctx.argv[i];
    params[ctx.argc] = NULL;

    execv(params[0], params);



    return EXIT_SUCCESS;
}



#define STACK_SIZE (1024 * 1024)    /* Stack size for cloned child */

int main(int argc, char *argv[]) {

    int status;
    int pipefd1[2];
    int pipefd2[2];
    int pipefd3[2];
    int& output_pd = pipefd2[1];
    int& input_pd = pipefd1[0];
    aucont::start_context ctx;
    bool is_fs_defined = false;

    if (pipe(pipefd1) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    if (pipe(pipefd2) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pipefd3[0] = pipefd2[0];
    pipefd3[1] = pipefd1[1];

    ctx.is_daemon = false;
    ctx.argc = 0;
    strcpy(ctx.hostname, "container");

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-d") == 0) {
            ctx.is_daemon = true;
        }
        else if (!is_fs_defined) {
            strcpy(ctx.fs, argv[i]);
            is_fs_defined = !is_fs_defined;
        }
        else {
            strcpy(ctx.argv[ctx.argc++], argv[i]);
        }
    }

    assert(is_fs_defined);

    void *stack;                    /* Start of stack buffer */
    void *stackTop;                 /* End of stack buffer */
    pid_t pid;


    // TODO create method clone
    /* Allocate stack for child */

    stack = malloc(STACK_SIZE);
    if (stack == NULL) errExit("malloc");
    stackTop = stack + STACK_SIZE;  /* Assume stack grows downward */


    pid = clone(child_func, stackTop, CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNS | SIGCHLD, pipefd3);
    if (pid == -1) errExit("clone");
    close(pipefd1[1]);
    close(pipefd2[0]);

    ctx.pid = pid;
    status = aucont::prepare_environment(ctx);
    write(output_pd, &status, sizeof(int));
    if (status != EXIT_SUCCESS) {
        exit(EXIT_FAILURE);
    }

    write(output_pd, &ctx, sizeof(aucont::start_context));
    read(input_pd, &status, sizeof(int));

    if (status != EXIT_SUCCESS) {
        aucont::clear_environment(ctx);
        exit(EXIT_FAILURE);
    }

    if (!ctx.is_daemon) {
        if (waitpid(pid, NULL, 0) == -1) errExit("waitpid");
        aucont::remove_container(ctx);
    } else {
        std::cout << pid << std::endl;
    }

    return EXIT_SUCCESS;
}
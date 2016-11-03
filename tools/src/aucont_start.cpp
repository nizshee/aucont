#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <cstring>
#include <wait.h>
#include <cassert>


#include "util.h"



static int child_func(void *arg) {

    aucont::start_context* ctx = (aucont::start_context*) arg;

    if (ctx->is_daemon) daemonize_after_fork();

    for (int i = 0; i < aucont::todo_size; ++i) {
        long status = aucont::todo[i].make(ctx);
        if (status < 0) {
            write(ctx->output_descriptor, &i, sizeof(int));
            write(ctx->output_descriptor, aucont::todo[i].name, strlen(aucont::todo[i].name));
            write(ctx->output_descriptor, ": ", 2);
            write(ctx->output_descriptor, strerror(errno), strlen(strerror(errno)));
            write(ctx->output_descriptor, "\n", 1);
            close(ctx->output_descriptor);
            exit(EXIT_FAILURE);
        }
    }

    // UTS
    if (sethostname(ctx->hostname, strlen(ctx->hostname)) == -1) errExit("sethostname");

    int success = -1;
    write(ctx->output_descriptor, &success, sizeof(int));
    close(ctx->output_descriptor);

    char* params[10];
    for (int i = 0; i < ctx->argc; ++i) params[i] = ctx->argv[i];
    params[ctx->argc] = NULL;

    execv(params[0], params);

    return 0;
}

#define STACK_SIZE (1024 * 1024)    /* Stack size for cloned child */

int main(int argc, char *argv[]) {

    aucont::check_directory();

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    aucont::start_context* ctx = (aucont::start_context *) malloc(sizeof(aucont::start_context));
    ctx->is_daemon = false;
    ctx->argc = 0;
    bool fs = false;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-d") == 0) {
            ctx->is_daemon = true;
        }
        else if (!fs) {
            strcpy(ctx->fs, argv[i]);
            fs = !fs;
        }
        else {
            strcpy(ctx->argv[ctx->argc++], argv[i]);
        }
    }

    assert(fs);

    ctx->output_descriptor = pipefd[1];
    strcpy(ctx->hostname, "container");
//    strcpy(ctx->fs, "/test/rootfs");
//    strcpy(ctx->argv[0], "/bin/hostname");
//    ctx->argc = 1;



    void *stack;                    /* Start of stack buffer */
    void *stackTop;                 /* End of stack buffer */
    pid_t pid;

    /* Allocate stack for child */

    stack = malloc(STACK_SIZE);
    if (stack == NULL) errExit("malloc");
    stackTop = stack + STACK_SIZE;  /* Assume stack grows downward */


    pid = clone(child_func, stackTop, CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNS | SIGCHLD, ctx);
    if (pid == -1) errExit("clone");
    close(pipefd[1]);


    int result;
    char buf;
    if (read(pipefd[0], &result, sizeof(int)) > 0) {
        if (result >= 0) {
            while (read(pipefd[0], &buf, 1) > 0) write(STDOUT_FILENO, &buf, 1);
            for (int j = result - 1; j >= 0; --j) {
                aucont::todo[j].cancel(ctx);
            }
            close(pipefd[0]);
            exit(EXIT_FAILURE);
        }
    } else {
        std::cerr << "no response from container" << std::endl;
        close(pipefd[0]);
        exit(EXIT_FAILURE);
    }
    close(pipefd[0]);


    aucont::create_context(pid, *ctx); // TODO kill if not created

    if (!ctx->is_daemon) {
        if (waitpid(pid, NULL, 0) == -1) errExit("waitpid");
        aucont::remove_context(pid);
    } else {
        std::cout << pid << std::endl;
    }

    exit(EXIT_SUCCESS);
}


void pre_start(std::ostream&& os) { // initialize container and publish id

    // create home directory for aucont
    struct stat st = {0};
    if (stat(aucont::home_dir.c_str(), &st) == -1 && mkdir(aucont::home_dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0) {
        os << "Can't create home directory for aucont." << std::endl;
        exit(1);
    }

    int res = unshare(CLONE_NEWUTS);
    if (res < 0) {
        os << "Can't create namespaces" << std::endl;
        exit(1);
    }

    // Change hostname
//    system("hostname container");

    std::string hostname;
    std::fstream host("/proc/sys/kernel/hostname");
    host >> hostname;
    os << hostname << std::endl;

//    char hostname[1024];
//    gethostname(hostname, 1024);
//
//    os << hostname << std::endl;

    // post pid
    pid_t pid = getpid();
    std::ofstream file(aucont::home_dir + "/" + std::to_string(pid));
    file << pid << std::endl;
    file.close();

    os << pid << std::endl;
}

void pre_stop() {
    pid_t pid = getpid();
    std::remove((aucont::home_dir + "/" + std::to_string(pid)).c_str());
}

//int main() {
//
//    std::cout << "start" << std::endl;
//    std::ostream os(std::cout.rdbuf());
//
//
//    pre_start(std::move(os));
//    pre_stop();
//
////    std::ofstream ofs("test.txt");
////    ofs << "Writing to a basic_ofstream object..." << std::endl;
////    ofs.close();
////
////    int posix_handle = fileno(::fopen("test.txt", "r"));
////
////    __gnu_cxx::stdio_filebuf<char> filebuf(posix_handle, std::ios::in); // 1
////    std::istream is(&filebuf); // 2
////
////    std::string line;
////    is >> line;
//////    getline(is, line);
////    std::cout << "line: " << line << std::endl;
//
//
//    return 0;
//}

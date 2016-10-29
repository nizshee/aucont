#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <ext/stdio_filebuf.h>
#include <cstring>
#include <sys/utsname.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <wait.h>
#include <linux/unistd.h>


#include "util.h"



#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                               } while (0)


static aucont::action todo[] =
        {
            aucont::mount_rootfs,
            aucont::mkdir_oldroot,
            aucont::pivot_root,
            aucont::chdir_after_pivot,
            aucont::umount_after_pivot,
            aucont::mount_proc,
            aucont::mount_dev
        };

static int child_func(void *arg) {
    struct utsname uts;

    aucont::start_context* ctx = (aucont::start_context*) arg;

    if (ctx->is_daemon) daemonize_after_fork();

    // mount

    for (int i = 0; i < 7; ++i) {
        long status = todo[i].make(ctx);
        if (status < 0) {
            for (int j = i - 1; j >= 0; --j) {
                todo[j].canel(ctx);
            }
            write(ctx->output_descriptor, "0", 1);
            write(ctx->output_descriptor, todo[i].name, strlen(todo[i].name));
            write(ctx->output_descriptor, ": ", 2);
            write(ctx->output_descriptor, strerror(errno), strlen(strerror(errno)));
            write(ctx->output_descriptor, "\n", 1);
            close(ctx->output_descriptor);
            exit(EXIT_FAILURE);
        }
    }

//    std::string fs(ctx->fs);
//    std::string old(fs + "/.pivot_root");
//
//    if (mount(fs.c_str(), fs.c_str(), "bind", MS_BIND | MS_REC, NULL) < 0) errExit("mount");
//
//    mkdir(old.c_str(), 0777);
//
////    status = system(("pivot_root " + fs + " " + old).c_str());
//    if (syscall(SYS_pivot_root, fs.c_str(), old.c_str())) {
//        umount(fs.c_str());
//        errExit("pivot_root");
//    }
//
//    if (chdir("/") < 0) {
//        umount((old + fs).c_str());
//        errExit("chdir");
//    };



    // UTS
    if (sethostname(ctx->hostname, strlen(ctx->hostname)) == -1) errExit("sethostname");

//    status = mount("proc", "/proc", "proc", 0, nullptr);
//    std::cout << "mount " << status << std::endl;

    if (uname(&uts) == -1) errExit("uname");
    printf("uts.nodename in child:  %s\n", uts.nodename);


    // Write data for parent
//    std::string id = std::to_string(getuid());
//    write(ctx->output_descriptor, id.c_str(), id.length());
//    write(ctx->output_descriptor, "\n", 1);
//    id = std::to_string(getgid());
//    write(ctx->output_descriptor, id.c_str(), id.length());
//    write(ctx->output_descriptor, "\n", 1);


    system("ls /");
    system("ls /proc");

    write(ctx->output_descriptor, "1", 1);
    close(ctx->output_descriptor);
    sleep(10);

    return 0;
}

#define STACK_SIZE (1024 * 1024)    /* Stack size for cloned child */

int main(int argc, char *argv[]) {

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    aucont::start_context* ctx = (aucont::start_context *) malloc(sizeof(aucont::start_context));
    ctx->output_descriptor = pipefd[1];
    ctx->is_daemon = false;
    strcpy(ctx->hostname, "container");
    strcpy(ctx->fs, "/test/rootfs");

    void *stack;                    /* Start of stack buffer */
    void *stackTop;                 /* End of stack buffer */
    pid_t pid;
    struct utsname uts;

    /* Allocate stack for child */

    stack = malloc(STACK_SIZE);
    if (stack == NULL) errExit("malloc");
    stackTop = stack + STACK_SIZE;  /* Assume stack grows downward */


    pid = clone(child_func, stackTop, CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWNS | SIGCHLD, ctx);
    if (pid == -1) errExit("clone");
    close(pipefd[1]);
    printf("clone() returned %ld\n", (long) pid);


    char buf;
    if (read(pipefd[0], &buf, 1) > 0) {
        if (buf == '0') {
            while (read(pipefd[0], &buf, 1) > 0) write(STDOUT_FILENO, &buf, 1);
            exit(EXIT_FAILURE);
        }
    } else {
        std::cerr << "no response from container" << std::endl;
        exit(EXIT_FAILURE);
    }

    close(pipefd[0]);




    if (uname(&uts) == -1) errExit("uname");
    printf("uts.nodename in parent: %s\n", uts.nodename);

    if (!ctx->is_daemon) {
        if (waitpid(pid, NULL, 0) == -1) errExit("waitpid");
    }

    exit(EXIT_SUCCESS);
}


void pre_start(std::ostream&& os) { // initialize container and publish id

    // create home directory for aucont
    struct stat st = {0};
    if (stat(aucont::dir.c_str(), &st) == -1 && mkdir(aucont::dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0) {
        os << "Can't create home directory for aucont." << std::endl;
        exit(1);
    }

    int res = unshare(CLONE_NEWUTS);
    if (res < 0) {
        os << "Can't create namespaces" << std::endl;
        exit(1);
    }

    // Change hostname
    system("hostname container");

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
    std::ofstream file(aucont::dir + "/" + std::to_string(pid));
    file << pid << std::endl;
    file.close();

    os << pid << std::endl;
}

void pre_stop() {
    pid_t pid = getpid();
    std::remove((aucont::dir + "/" + std::to_string(pid)).c_str());
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

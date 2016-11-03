
#ifndef AUCONT_UTIL_H
#define AUCONT_UTIL_H

#include <cstdlib>
#include <fstream>
#include <iostream>

extern "C" {
#include <unistd.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <fcntl.h>
}

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                               } while (0)

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

namespace aucont {
    const std::string home_dir = "/tmp/aucont";
    typedef struct _start_context {
        int output_descriptor;
        bool is_daemon;
        char hostname[100];
        char fs[50];
        char argv[10][50];
        uint8_t argc;
    } start_context;

    void create_context(pid_t pid, start_context context) {
        std::string dir = home_dir + "/" + std::to_string(pid);
        if (mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0) errExit("mkdir for context");
        std::ofstream ofstream(dir + "/" + "context");
        ofstream.write((char*) &context, sizeof(start_context));
    }

    void read_context(pid_t pid, start_context& context) {
        std::string dir = home_dir + "/" + std::to_string(pid); // TODO exists
        std::ifstream ifstream(dir + "/" + "context");
        ifstream.read((char*) &context, sizeof(start_context));
    }

    void remove_context(pid_t pid) {
        std::string dir = home_dir + "/" + std::to_string(pid); // TODO
        remove((dir + "/context").c_str());
        rmdir(dir.c_str());
    }

    enum PREPARE_STAGE {
        FULL,
        MOUNT,
        MKDIR,
        NOTHING
    };

    long prepare_ctx(pid_t, start_context*);
    void clear_ctx(pid_t, start_context*, PREPARE_STAGE);

    /**
     * Starts host to prepare environment for container.
     */
    long prepare_ctx(pid_t pid, start_context* ctx) {
        FILE *ptr;
        struct stat st = {0};
        std::string container_dir(home_dir + "/" + std::to_string(pid));
        std::string context(container_dir + "/context");

        if (stat(aucont::home_dir.c_str(), &st) == -1 &&
            mkdir(aucont::home_dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0) {
            perror("mkdir aucont");
            return EXIT_FAILURE;
        }

        if (mkdir(container_dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0) {
            perror("mkdir container");
            clear_ctx(pid, ctx, NOTHING);
            return EXIT_FAILURE;
        }

        // TODO bind to container directory
        if (mount(ctx->fs, ctx->fs, "bind", MS_BIND | MS_REC, NULL) < 0) {
            perror("mount fs directory");
            clear_ctx(pid, ctx, MKDIR);
            return EXIT_FAILURE;
        }

        if ((ptr = fopen(context.c_str(), "wb")) == NULL ||
            fwrite(ctx, sizeof(start_context), 1, ptr) < 0 ||
            fclose(ptr)) {
            perror("write context");
            clear_ctx(pid, ctx, MOUNT);
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    void clear_ctx(pid_t pid, start_context* ctx, PREPARE_STAGE stage=FULL) {
        std::string container_dir(home_dir + "/" + std::to_string(pid));

        switch (stage) {
            case FULL:

            case MOUNT:
                if (umount(ctx->fs) < 0) perror("unmount fs");
            case MKDIR:
                if (rmdir(container_dir.c_str()) < 0) perror("rmdir container");
            case NOTHING:
                break;
        }
    }

    typedef struct _start_params {
        bool is_daemon = false;
        int cpu = 100;
        char* host = nullptr;
    } start_params;

    typedef struct _action {
        const char* name;
        long (*make)(start_context* context);
        void (*cancel)(start_context* context);
    } action;

    void empty_cancel(start_context* context) {}

    long mount_rootfs_make(start_context* context) {
        return mount(context->fs, context->fs, "bind", MS_BIND | MS_REC, NULL);
    }

    void mount_rootfs_cancel(start_context* context) {
        umount(context->fs);
    }

    long mkdir_oldroot_make(start_context* context) {
        std::string old(std::string(context->fs) + "/.pivot_root");
        return mkdir(old.c_str(), 0777);
    }

    void mkdir_oldroot_cancel(start_context* context) {
        std::string old(std::string(context->fs) + "/.pivot_root");
        rmdir(old.c_str());
    }

    long pivot_root_make(start_context* context) {
        std::string old(std::string(context->fs) + "/.pivot_root");
        return syscall(SYS_pivot_root, context->fs, old.c_str());
    }

    long chdir_after_pivot_make(start_context* context) {
        return chdir("/");
    }

    long umount_after_pivot_make(start_context* context) {
        long status;
        status = umount2("/.pivot_root", MNT_DETACH);
        if (status < 0) return status;
        return rmdir("/.pivot_root");
    }

    long mount_proc_make(start_context* context) {
        return mount("proc", "/proc", "proc", 0, "");
    }

    long mount_sys_make(start_context* context) {
        return mount("sys", "/sys", "sysfs", 0, "");
    }

    long mount_dev_make(start_context* context) {
        return mount("udev", "/dev", "devtmpfs", MS_NOSUID | MS_STRICTATIME, "mode=755");
    }

    action mount_rootfs = {"mount new root", &mount_rootfs_make, &mount_rootfs_cancel};

    action mkdir_oldroot = {"mkdir for old root", &mkdir_oldroot_make, &mkdir_oldroot_cancel};

    action pivot_root = {"pivot root", &pivot_root_make, &empty_cancel};

    action chdir_after_pivot = {"chdir after pivot", &chdir_after_pivot_make, &empty_cancel};

    action umount_after_pivot = {"umount after pivot", &umount_after_pivot_make, &empty_cancel};

    action mount_proc = {"mount proc", &mount_proc_make, &empty_cancel};

    action mount_dev = {"mount dev", &mount_dev_make, &empty_cancel};

    action mount_sys = {"mount sys", &mount_sys_make, &empty_cancel};

    static action todo[] =
            {
                    mount_rootfs,
                    mkdir_oldroot,
                    pivot_root,
                    chdir_after_pivot,
                    umount_after_pivot,
                    mount_proc,
                    mount_dev,
                    mount_sys
            };

    static int todo_size = sizeof(todo)/sizeof(*todo);

    void clear_res(start_context* ctx, int from=todo_size) {
        for (int j = from - 1; j >= 0; --j) {
            aucont::todo[j].cancel(ctx);
        }
    }


    void check_directory() {
        struct stat st = {0};
        if (stat(aucont::home_dir.c_str(), &st) == -1 &&
                mkdir(aucont::home_dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0) {
            std::cout << "Can't create home directory for aucont." << std::endl;
            exit(EXIT_FAILURE);
        }
    }
}


static void daemonize_after_fork() {
    pid_t pid, sid;
    int fd;

    /* already a daemon */
    if (getppid() == 1) return;

    /* Fork off the parent process */
//    pid = fork();
//    if (pid < 0) {
//        exit(EXIT_FAILURE);
//    }
//
//    if (pid > 0) {
//        exit(EXIT_SUCCESS); /*Killing the Parent Process*/
//    }

    /* At this point we are executing as the child process */

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        std::cout << "setsid" << std::endl;
        exit(EXIT_FAILURE);
    }

    /* Change the current working directory. */
    if ((chdir("/")) < 0) {
        std::cout << "chdir" << std::endl;
        exit(EXIT_FAILURE);
    }

    fd = open("/dev/null", O_RDWR, 0);

    if (fd != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);

        if (fd > 2) {
            close (fd);
        }
    }

    /*resettign File Creation Mask */
    umask(027);
}


#endif //AUCONT_UTIL_H

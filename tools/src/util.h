
#ifndef AUCONT_UTIL_H
#define AUCONT_UTIL_H

#include <cstdlib>

#include <iostream>
#include <cstring>


extern "C" {
#include <signal.h>
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

    std::string container_dir(pid_t pid) {
        return home_dir + "/" + std::to_string(pid);
    }

    std::string container_ctx(pid_t pid) {
        return container_dir(pid) + "/ctx";
    }

    std::string container_old(const char* fs) {
        return std::string(fs) + "/.old_root";
    }

    typedef struct _start_context {
        pid_t pid;
        bool is_daemon;
        char hostname[100];
        char fs[50];
        char argv[10][50];
        uint8_t argc;
    } start_context;

    int load_ctx(pid_t pid, start_context& ctx) {
        FILE* ptr;
        if ((ptr = fopen(container_ctx(pid).c_str(), "rb")) == NULL ||
            fread(&ctx, sizeof(start_context), 1, ptr) < 0 ||
            fclose(ptr)) {
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }


    enum ENVIRONMENT_STAGE {
        E_FULL,
        E_CONTAINER,
        E_NOTHING
    };

    void clear_environment(start_context&, ENVIRONMENT_STAGE);

    /**
     * Starts on host to prepare environment for container.
     */
    int prepare_environment(start_context& ctx) {
        FILE *ptr;
        struct stat st = {0};

        if (stat(aucont::home_dir.c_str(), &st) == -1 &&
            mkdir(aucont::home_dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0) {
            perror("mkdir aucont");
            return EXIT_FAILURE;
        }

        if (mkdir(container_dir(ctx.pid).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0) {
            perror("mkdir container");
            clear_environment(ctx, E_NOTHING);
            return EXIT_FAILURE;
        }

        if ((ptr = fopen(container_ctx(ctx.pid).c_str(), "wb")) == NULL ||
            fwrite(&ctx, sizeof(start_context), 1, ptr) < 0 ||
            fclose(ptr)) {
            perror("write context");
            clear_environment(ctx, E_FULL);
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }


    void clear_environment(start_context& ctx, ENVIRONMENT_STAGE stage=E_FULL) {

        switch (stage) {
            case E_FULL:
                if (remove(container_ctx(ctx.pid).c_str()) < 0) perror("rm context");
            case E_CONTAINER:
                if (rmdir(container_dir(ctx.pid).c_str()) < 0) perror("rmdir container");
            case E_NOTHING:
                break;
        }
    }

    int remove_container(start_context& ctx) {
        kill(ctx.pid, SIGKILL);
        clear_environment(ctx);
        return EXIT_SUCCESS;
    }



    enum CONTAINER_STAGE {
        C_FULL,
        C_RMDIR,
        C_UMOUNT,
        C_PIVOT,
        C_MOUNT,
        C_MKDIR,
        C_NOTHING
    };

    void clear_container(start_context&, CONTAINER_STAGE);

    int prepare_container(start_context& ctx) {

        if (mkdir(container_old(ctx.fs).c_str(), 0777) < 0) {
            perror("mkdir old root");
            clear_container(ctx, C_NOTHING);
            return EXIT_FAILURE;
        }

        if (mount(ctx.fs, ctx.fs, "bind", MS_BIND | MS_REC, NULL) < 0) {
            perror("mount fs");
            clear_container(ctx, C_MKDIR);
            return EXIT_FAILURE;
        }

        if (syscall(SYS_pivot_root, ctx.fs, container_old(ctx.fs).c_str()) < 0) {
            perror("pivot");
            clear_container(ctx, C_MOUNT);
            return EXIT_FAILURE;
        }

        if (umount2("/.old_root", MNT_DETACH) < 0) {
            perror("umount old root");
            clear_container(ctx, C_PIVOT);
            return EXIT_FAILURE;
        }

        if (rmdir("/.old_root") < 0) {
            perror("rmdir old root");
            clear_container(ctx, C_UMOUNT);
        }

        if (mount("proc", "/proc", "proc", 0, "") < 0) {
            perror("proc");
            return EXIT_FAILURE;
        }

        if (mount("sys", "/sys", "sysfs", 0, "") < 0) {
            perror("sys");
            return EXIT_FAILURE;
        }

        if (mount("udev", "/dev", "devtmpfs", MS_NOSUID | MS_STRICTATIME, "mode=755") < 0) {
            perror("udev");
            return EXIT_FAILURE;
        }

        if (sethostname(ctx.hostname, strlen(ctx.hostname)) < 0) {
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }

    void clear_container(start_context& ctx, CONTAINER_STAGE stage=C_FULL) {
        bool need_to_delete_old_root = true;
        bool need_to_umount = true;
        std::string to_umount = ctx.fs;
        std::string to_rmdir = container_old(ctx.fs);
        switch (stage) {
            case C_FULL:
            case C_RMDIR:
                need_to_delete_old_root = false;
            case C_UMOUNT:
                need_to_umount = false;
            case C_PIVOT:
                to_umount = "/";
                to_rmdir = "/.old_root";
            case C_MOUNT:
                if (need_to_umount && umount(to_umount.c_str()) < 0) perror("unmount fs");
            case C_MKDIR:
                if (need_to_delete_old_root && rmdir(container_old(ctx.fs).c_str()) < 0) perror("rmdir old root");
            case C_NOTHING:
                break;
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

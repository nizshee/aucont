
#ifndef AUCONT_UTIL_H
#define AUCONT_UTIL_H

#include <cstdlib>

extern "C" {
#include <unistd.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include <fcntl.h>
}

namespace aucont {
    const std::string dir = "/tmp/aucont";
    typedef struct _start_context {
        int output_descriptor;
        bool is_daemon;
        char hostname[100];
        char fs[100];
    } start_context;

    typedef struct _start_params {
        bool is_daemon = false;
        int cpu = 100;
        char* host = nullptr;
    } start_params;

    typedef struct _action {
        const char* name;
        long (*make)(start_context* context);
        void (*canel)(start_context* context);
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
        return umount2("/.pivot_root", MNT_DETACH);
    }

    long mount_proc_make(start_context* context) {
        return mount("proc", "/proc", "proc", 0, "");
    }

    long mount_dev_make(start_context* context) {
        return mount("tmpfs", "/dev", "tmpfs", MS_NOSUID | MS_STRICTATIME, "mode=755");
    }

    action mount_rootfs = {"mount new root", &mount_rootfs_make, &mount_rootfs_cancel};

    action mkdir_oldroot = {"mkdir for old root", &mkdir_oldroot_make, &mkdir_oldroot_cancel};

    action pivot_root = {"pivot root", &pivot_root_make, &empty_cancel};

    action chdir_after_pivot = {"chdir after pivot", &chdir_after_pivot_make, &empty_cancel};

    action umount_after_pivot = {"umount after pivot", &umount_after_pivot_make, &empty_cancel};

    action mount_proc = {"mount proc", &mount_proc_make, &empty_cancel};

    action mount_dev = {"mount dev", &mount_dev_make, &empty_cancel};

}


#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

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

#pragma once

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
#include <netinet/in.h>
#include <arpa/inet.h>
}

#define err_exit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                               } while (0)

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1


namespace aucont {

    std::string exec(const char* cmd);

    const std::string home_dir = "/tmp/aucont";

    const std::string cgroup_dir = home_dir + "/cg";

    std::string container_dir(pid_t pid);

    std::string container_cgroup_dir(pid_t pid);

    std::string container_ctx(pid_t pid);

    std::string container_sys(const char* fs);

    std::string container_proc(const char* fs);

    std::string container_old(const char* fs);

    typedef struct _start_context {
        pid_t pid;
        bool is_daemon;
        char hostname[100];
        char fs[100];
        char argv[5][100];
        uint8_t argc;
        int cpu;
        bool is_net;
        struct in_addr ip;
    } start_context;

    int load_ctx(pid_t pid, start_context& ctx);

    void start_container(aucont::start_context&);

    void remove_container(start_context& ctx, int signal=SIGTERM);

    enum ENVIRONMENT_STAGE {
        E_FULL,
        E_MAP,
        E_CTX,
        E_CONTAINER,
        E_CGROUP,
        E_NOTHING
    };

    /**
     * Starts in host to prepare environment for container.
     */
    int prepare_environment(start_context&);

    /**
     * Clear after prepare_environment.
     */
    void clear_environment(start_context&, ENVIRONMENT_STAGE stage=E_FULL);


    enum CONTAINER_STAGE {
        C_FULL,
        C_RMDIR,
        C_UMOUNT,
        C_PIVOT,
        C_MOUNT,
        C_MKDIR,
        C_SYS,
        C_PROC,
        C_NOTHING
    };

    /**
     * Starts in container to prepare namespaces and groups.
     */
    int prepare_container(start_context&);

    /**
     * Clear after prepare_container.
     */
    void clear_container(start_context&, CONTAINER_STAGE stage = C_FULL);

}

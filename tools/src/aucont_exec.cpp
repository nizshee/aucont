
#include <iostream>
#include <sstream>
#include <array>
#include <fstream>

#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include <sys/wait.h>
#include "include/util.h"


void unshare(std::string pid, std::string ns) {
    std::string f = "/proc/" + pid + "/ns/" + ns;
    int fd = open(f.c_str(), O_RDONLY);
    if (fd < 0) {
        err_exit(ns.c_str());
    }
    if (setns(fd, 0) < 0) {
        close(fd);
        err_exit(ns.c_str());
    }
    close(fd);
}


int main(int argc, char* argv[]) {

    if (argc < 3) {
        err_exit("not enough arguments");
    }

    std::string cont_pid_str = std::string(argv[1]);

    unshare(cont_pid_str, "user");
    unshare(cont_pid_str, "pid");

    auto cmd_pid = fork();
    if (cmd_pid < 0) {
        err_exit("fork");
    } else if (cmd_pid > 0) {
        if (waitpid(cmd_pid, NULL, 0) < 0) {
            err_exit("waitpid");
        }
        exit(0);
    }

    unshare(cont_pid_str, "net");
    unshare(cont_pid_str, "ipc");
    unshare(cont_pid_str, "uts");
    unshare(cont_pid_str, "mnt");

    char* params[10];
    for (int i = 0; i < argc - 2; ++i) {
        params[i] = argv[i + 2];
    }
    params[argc - 2] = NULL;
    if (execv(params[0], params) < 0) {
        err_exit("exec");
    }
    return 0;
}

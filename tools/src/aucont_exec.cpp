#include "util.h"

#include <iostream>
#include <string>
#include <sstream>

#include <stdio.h>
#include <errno.h>
#include <sched.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>


using namespace std;

int main(int argc, char *argv[]) {

    if (argc <= 2) {
        std::cout << "no container id" << std::endl;
        exit(1);
    }

    int count = 0;
    char* ps[10];
    char params[10][50];
    for (int i = 2; i < argc; ++i) {
        ps[count] = params[count];
        strcpy(params[count++], argv[i]);
    }
    ps[count] = NULL;


    string ns = string("/proc/") + argv[1] + "/ns/";
    int ns_mnt = open((ns + "mnt").c_str(), O_RDONLY, O_CLOEXEC);
    int ns_uts = open((ns + "uts").c_str(), O_RDONLY, O_CLOEXEC);
    int ns_pid = open((ns + "pid").c_str(), O_RDONLY, O_CLOEXEC);

    if (setns(ns_pid, 0) < 0) errExit("pid");
    close(ns_pid);
    if (setns(ns_mnt, 0) < 0) errExit("mount");
    close(ns_mnt);
    if (setns(ns_uts, 0) < 0) errExit("uts");
    close(ns_uts);



    execv(argv[2], ps);

    return 0;
}


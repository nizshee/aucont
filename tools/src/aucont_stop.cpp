#include <iostream>
#include <fstream>
#include <signal.h>

#include "util.h"

extern "C" {
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        std::cout << "no container id" << std::endl;
        exit(1);
    }

    pid_t pid = std::stoi(argv[1]);
    aucont::start_context ctx;
    aucont::read_context(pid, ctx);
    kill(pid, SIGKILL);
    aucont::clear_res(&ctx);
    aucont::remove_context(pid);
//    daemonize_after_fork();
//    std::ofstream os;
//    os.open("/tmp/aucont.log");
//
//    os << getppid() << std::endl;

//    os.close();
    return 0;
}

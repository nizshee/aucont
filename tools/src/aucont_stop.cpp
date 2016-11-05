#include <iostream>
#include <fstream>
#include <signal.h>

#include "util.h"

extern "C" {
}

int main(int argc, char *argv[]) {
    pid_t pid;
    aucont::start_context ctx;

    if (argc < 2) {
        std::cout << "no container id" << std::endl;
        exit(1);
    }

    pid = std::stoi(argv[1]);
    if (aucont::load_ctx(pid, ctx) != EXIT_SUCCESS) {
        perror("load context");
        exit(EXIT_FAILURE);
    }

    aucont::remove_container(ctx);

    return 0;
}

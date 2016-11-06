#include <iostream>

#include "include/util.h"


int main(int argc, char* argv[]) {
    aucont::start_context ctx;

    if (argc != 2 && argc != 3) {
        err_exit("2 or 3 arguments");
        exit(1);
    }

    pid_t pid = atoi(argv[1]);


    if (aucont::load_ctx(pid, ctx) != EXIT_SUCCESS) {
        err_exit("can't find container");
    }
    if (argc == 3) {
        aucont::remove_container(ctx, atoi(argv[2]));
    }
    else {
        aucont::remove_container(ctx);
    }

    return 0;
}

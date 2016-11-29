#include <iostream>
#include <cstring>
#include <cassert>

#include "include/util.h"


int main(int argc, const char* argv[]) {

    aucont::start_context ctx;
    ctx.is_daemon = false;
    ctx.is_net = false;
    ctx.cpu = 100;
    ctx.argc = 0;
    strcpy(ctx.hostname, "container");

    bool is_fs_defined = false;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--cpu") == 0) {
            ctx.cpu = std::stoi(argv[++i]);
            assert(ctx.cpu > 0 && ctx.cpu <= 100);
        }
        else if (strcmp(argv[i], "--net") == 0) {
            struct in_addr addr;
            assert(inet_aton(argv[++i], &addr));
            ctx.is_net = true;
            ctx.ip = addr;
        }
        else if (strcmp(argv[i], "-d") == 0) {
            ctx.is_daemon = true;
        }
        else if (!is_fs_defined) {
            strcpy(ctx.fs, argv[i]);
            is_fs_defined = !is_fs_defined;
        }
        else {
            strcpy(ctx.argv[ctx.argc++], argv[i]);
        }
    }

    assert(is_fs_defined);

    aucont::start_container(ctx);

    return 0;
}

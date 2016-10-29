#include <iostream>
#include <fstream>

#include "util.h"

extern "C" {
}

int main() {

    daemonize_after_fork();
    std::ofstream os;
    os.open("/tmp/aucont.log");

    os << getppid() << std::endl;

    os.close();
    return 0;
}

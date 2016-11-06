#include <iostream>
#include <sstream>
#include <iomanip>

#include <csignal>
#include <dirent.h>
#include <cstring>

#include "include/util.h"


int main(int argc, char* argv[]) {

    dirent* dp;
    DIR* dirp = opendir(aucont::home_dir.c_str());

    if (dirp == NULL) {
        return 0;
    }

    while ((dp = readdir(dirp)) != NULL) {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0) {
            std::cout << dp->d_name << std::endl;
        }
    }

    closedir(dirp);
    return 0;
}

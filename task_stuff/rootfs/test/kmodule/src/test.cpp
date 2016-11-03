#include <cstdio>

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

/* Flags for sys_finit_module: */
#define MODULE_INIT_IGNORE_MODVERSIONS 1
#define MODULE_INIT_IGNORE_VERMAGIC 2
int finit_module(int fd, const char *param_values, int flags)
{
    return syscall(SYS_finit_module, fd, param_values, flags);
}

int main(int argc, char **argv)
{
    char *mod_path = argv[1];
    int mod_fd = open(mod_path, O_RDONLY);
    if (mod_fd == -1) return 1;
    int ret = finit_module(mod_fd, "",
        MODULE_INIT_IGNORE_MODVERSIONS |
        MODULE_INIT_IGNORE_VERMAGIC
    );
    if (ret != -1) return 1;
    if (errno != EPERM) return 1;
    return 0;
}

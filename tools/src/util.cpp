#include <fstream>
#include <wait.h>
#include <memory>
#include <sstream>

#include "include/util.h"


std::string aucont::exec(const char* cmd) {
    char buffer[128];
    std::string result = "";
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer, 128, pipe.get()) != NULL)
            result += buffer;
    }
    return result;
}

std::string aucont::container_dir(pid_t pid) {
    return home_dir + "/" + std::to_string(pid);
}

std::string aucont::container_cgroup_dir(pid_t pid) {
    return cgroup_dir + "/" + std::to_string(pid);
}

std::string aucont::container_ctx(pid_t pid) {
    return container_dir(pid) + "/ctx";
}

std::string aucont::container_sys(const char* fs) {
    return std::string(fs) + "/sys";
}

std::string aucont::container_proc(const char* fs) {
    return std::string(fs) + "/proc";
}

std::string aucont::container_old(const char* fs) {
    return std::string(fs) + "/.old_root";
}

int aucont::load_ctx(pid_t pid, start_context& ctx) {
    FILE* ptr;
    if ((ptr = fopen(container_ctx(pid).c_str(), "rb")) == NULL ||
        fread(&ctx, sizeof(start_context), 1, ptr) < 0 ||
        fclose(ptr)) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void aucont::remove_container(start_context& ctx, int signal) {
    clear_environment(ctx);
    if (kill(ctx.pid, signal) < 0 && errno != ESRCH) {
        err_exit("kill");
    }
}

int aucont::prepare_environment(start_context& ctx) {
    FILE *ptr;
    struct stat st = {0};

    if (stat(aucont::home_dir.c_str(), &st) == -1 &&
        mkdir(aucont::home_dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0) {
        perror("(environment) mkdir aucont");
        return EXIT_FAILURE;
    }

    if (stat(aucont::cgroup_dir.c_str(), &st) == -1 &&
        mkdir(aucont::cgroup_dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0) {
        perror("(environment) mkdir cgroup");
        return EXIT_FAILURE;
    }

    if (ctx.is_net) {

        std::string host_iname = "aucont" + std::to_string(ctx.pid) + "h";
        std::string cont_iname = "aucont" + std::to_string(ctx.pid) + "c";
        std::string host_ip = inet_ntoa(inet_makeaddr(inet_netof(ctx.ip), inet_lnaof(ctx.ip) + 1));

        std::stringstream ss;
        ss << "sudo ip link add " << host_iname << " type veth peer name " << cont_iname << " && " << std::endl;
        ss << "sudo ip link set " << cont_iname << " netns " << ctx.pid << " && " << std::endl;
        ss << "sudo ip addr add " << host_ip << "/24 dev " << host_iname << " && " << std::endl;
        ss << "sudo ip link set " << host_iname << " up && " << std::endl;
        ss << "sudo sysctl net.ipv4.conf.all.forwarding=1 > /dev/null" << std::endl;

        std::cerr << ss.str() << std::endl;
        if (system(ss.str().c_str()) < 0) {
            perror("(environment) ip");
            return EXIT_FAILURE;
        }
    }

    if (ctx.cpu < 100 ) {

        std::string opts =  aucont::exec("mount | grep cgroup | grep cpu | grep -o \"(.*)\" | head -n 1");
        if (opts == "") {
            opts = "cpu";
        } else {
            opts = opts.substr(1, opts.length() - 3);
        }

        std::cerr << opts << std::endl;

        if (system(("sudo mount -t cgroup -o " + opts + " aucont_cg " + cgroup_dir).c_str()) < 0) {
            perror("(environment) mount cgroup");
            return EXIT_FAILURE;
        }

        if (system(("sudo mkdir -p " + aucont::container_cgroup_dir(ctx.pid)).c_str()) < 0) {
            perror("(environment) mkdir container cgroup");
            clear_environment(ctx, E_NOTHING);
            return EXIT_FAILURE;
        }

        std::string uid = std::to_string(getuid());
        std::string gid = std::to_string(getgid());
        if (system(("sudo chown -R " + uid + ":" + gid + " " + aucont::container_cgroup_dir(ctx.pid)).c_str()) < 0) {
            perror("(environment) chown container cgroup");
            clear_environment(ctx, E_CGROUP);
            return EXIT_FAILURE;
        }

        int period = 1000000;
        int cores = (int) sysconf(_SC_NPROCESSORS_ONLN);
        int quota = (period * cores * ctx.cpu) / 100;

        if ((ptr = fopen((container_cgroup_dir(ctx.pid) + "/cpu.cfs_period_us").c_str(), "wb")) == NULL ||
            fprintf(ptr, "%d\n", period) < 0 ||
            fclose(ptr)) {
            perror("(environment) write period");
            clear_environment(ctx, E_CGROUP);
            return EXIT_FAILURE;
        }

        if ((ptr = fopen((container_cgroup_dir(ctx.pid) + "/cpu.cfs_quota_us").c_str(), "wb")) == NULL ||
            fprintf(ptr, "%d\n", quota) < 0 ||
            fclose(ptr)) {
            perror("(environment) write quota");
            clear_environment(ctx, E_CGROUP);
            return EXIT_FAILURE;
        }

        if ((ptr = fopen((container_cgroup_dir(ctx.pid) + "/tasks").c_str(), "wb")) == NULL ||
            fprintf(ptr, "%d\n", ctx.pid) < 0 ||
            fclose(ptr)) {
            perror("(environment) write task");
            clear_environment(ctx, E_CGROUP);
            return EXIT_FAILURE;
        }

    }

    if (mkdir(container_dir(ctx.pid).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0) {
        perror("(environment) mkdir container");
        clear_environment(ctx, E_CGROUP);
        return EXIT_FAILURE;
    }

    if ((ptr = fopen(container_ctx(ctx.pid).c_str(), "wb")) == NULL ||
        fwrite(&ctx, sizeof(start_context), 1, ptr) < 0 ||
        fclose(ptr)) {
        perror("(environment) write context");
        clear_environment(ctx, E_CONTAINER);
        return EXIT_FAILURE;
    }

    std::string pid = std::to_string(ctx.pid);
    if ((ptr = fopen(("/proc/" + pid + "/setgroups").c_str(), "wb")) == NULL ||
        fprintf(ptr, "deny") < 0 ||
        fclose(ptr)) {
        perror("(environment) setgroups");
        clear_environment(ctx, E_CONTAINER);
        return EXIT_FAILURE;
    }

    if ((ptr = fopen(("/proc/" + pid + "/uid_map").c_str(), "wb")) == NULL ||
        fprintf(ptr, "0 %d 1", getuid()) < 0 ||
        fclose(ptr)) {
        perror("(environment) uid_map");
        clear_environment(ctx, E_CONTAINER);
        return EXIT_FAILURE;
    }

    if ((ptr = fopen(("/proc/" + pid + "/gid_map").c_str(), "wb")) == NULL ||
        fprintf(ptr, "0 %d 1", getgid()) < 0 ||
        fclose(ptr)) {
        perror("(environment) uid_map");
        clear_environment(ctx, E_CONTAINER);
        return EXIT_FAILURE;
    }


    return EXIT_SUCCESS;
}

void aucont::clear_environment(start_context& ctx, ENVIRONMENT_STAGE stage) {
    switch (stage) {
        case E_FULL:
        case E_MAP:
        case E_CTX:
            if (remove(container_ctx(ctx.pid).c_str()) < 0) perror("(cl environment) rm context");
        case E_CONTAINER:
            if (rmdir(container_dir(ctx.pid).c_str()) < 0) perror("(cl environment) rmdir container");
        case E_CGROUP:
            if (ctx.cpu < 100) { // TODO
                umount(container_cgroup_dir(ctx.pid).c_str());
                if (system(("sudo rmdir " + container_cgroup_dir(ctx.pid)).c_str()) < 0 )
                    perror("(cl environment) rmdir cgroup");
            }
        case E_NOTHING:
            break;
    }
}

int aucont::prepare_container(start_context& ctx) {

    if (sethostname(ctx.hostname, strlen(ctx.hostname)) < 0) {
        perror("(container) hostname");
        clear_container(ctx, C_NOTHING);
        return EXIT_FAILURE;
    }

    if (ctx.is_net) {
        std::string cont_iname = "aucont" + std::to_string(ctx.pid) + "c";
        std::string host_ip = inet_ntoa(inet_makeaddr(inet_netof(ctx.ip), inet_lnaof(ctx.ip) + 1));
        std::string cont_ip = inet_ntoa(inet_makeaddr(inet_netof(ctx.ip), inet_lnaof(ctx.ip)));

        std::stringstream ss;
        ss << "ip addr add " << cont_ip << "/24 dev " << cont_iname << " && " << std::endl;
        ss << "ip link set " << cont_iname << " up && " << std::endl;
        ss << "ip route add default via " << host_ip << " && " << std::endl;
        ss << "ip link set lo up" << std::endl;

        std::cerr << ss.str() << std::endl;

        if (system(ss.str().c_str()) < 0) {
            perror("(environment) ip");
            return EXIT_FAILURE;
        }
    }

    if (mount(NULL, "/", NULL, MS_PRIVATE | MS_REC, NULL) < 0) {
        perror("(container) private");
        clear_container(ctx, C_NOTHING);
        return EXIT_FAILURE;
    }

    if (mount(NULL, container_proc(ctx.fs).c_str(), "proc", MS_NOSUID | MS_NOEXEC | MS_NODEV, NULL) < 0) {
        perror("(container) proc");
        clear_container(ctx, C_NOTHING);
        return EXIT_FAILURE;
    }

    if (mount(NULL, container_sys(ctx.fs).c_str(), "sysfs", MS_NOSUID | MS_NOEXEC | MS_NODEV, NULL) < 0) {
        perror("(container) sys");
        clear_container(ctx, C_PROC);
        return EXIT_FAILURE;
    }

    if (mkdir(container_old(ctx.fs).c_str(), 0777) < 0) {
        perror("(container) mkdir");
        clear_container(ctx, C_SYS);
        return EXIT_FAILURE;
    }

    if (mount(ctx.fs, ctx.fs, "bind", MS_BIND | MS_REC, NULL) < 0) {
        perror("(container) mount");
        clear_container(ctx, C_MKDIR);
        return EXIT_FAILURE;
    }

    if (syscall(SYS_pivot_root, ctx.fs, container_old(ctx.fs).c_str()) < 0) {
        perror("(container) pivot");
        clear_container(ctx, C_MOUNT);
        return EXIT_FAILURE;
    }

    if (chdir("/") != 0) {
        perror("(container) chdir");
        clear_container(ctx, C_PIVOT);
        return EXIT_FAILURE;
    }

    if (umount2("/.old_root", MNT_DETACH) < 0) {
        perror("(container) umount");
        clear_container(ctx, C_PIVOT);
        return EXIT_FAILURE;
    }

    if (rmdir("/.old_root") < 0) {
        perror("(container) rmdir");
        clear_container(ctx, C_UMOUNT);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void aucont::clear_container(start_context& ctx, CONTAINER_STAGE stage) {
    std::string fs = ctx.fs;
    bool need_to_remove_old_root = true;
    bool need_to_umount_old_root = true;
    switch (stage) {
        case C_FULL:
        case C_RMDIR:
            need_to_remove_old_root = false;
        case C_UMOUNT:
        case C_PIVOT:
            fs = "/";
            need_to_umount_old_root = false;
        case C_MOUNT:
            if (need_to_umount_old_root) {
                if (umount(container_old(fs.c_str()).c_str()) < 0) perror("(cl container) umount");
            }
        case C_MKDIR:
            if (need_to_remove_old_root) {
                if (rmdir(container_old(fs.c_str()).c_str()) < 0) perror("(cl container) rmdir");
            }
        case C_SYS:
            if (umount(container_sys(fs.c_str()).c_str()) < 0) perror("(cl container) sys");
        case C_PROC:
            if (umount(container_proc(fs.c_str()).c_str()) < 0) perror("(cl container) proc");
        case C_NOTHING:
            break;
    }
}


void daemonize_after_fork() {

    if (setsid() < 0) {
        err_exit("(daemonize) setsid");
    }

    if (chdir("/") < 0) {
        err_exit("(daemonize) chdir");
    }

    int fd = open("/dev/null", O_RDWR, 0);

    if (fd != -1) {
        if (dup2(fd, STDIN_FILENO) < 0) err_exit("(daemonize) dup stdin");
        if (dup2(fd, STDOUT_FILENO) < 0) err_exit("(daemonize) dup stdout");
        if (dup2(fd, STDERR_FILENO) < 0) err_exit("(daemonize) dup stderr");
        if (fd > 2 && close(fd) < 0) {
            err_exit("(daemonize) close fd");
        }
    }

    umask(027);
}


int container_main(void *arg) {
    pid_t pid;
    int status;
    int input_fd = ((int*) arg)[0];
    close(((int*) arg)[1]);
    close(((int*) arg)[2]);
    int output_fd = ((int*) arg)[3];

    aucont::start_context ctx;
    read(input_fd, &ctx, sizeof(aucont::start_context));

    if (ctx.is_daemon) {
        daemonize_after_fork();
    }

    if (unshare(CLONE_NEWPID) < 0) {
        err_exit("(main) unshare");
    }

    pid = fork();
    if (pid < 0) {
        err_exit("(main) fork");
    } else if (pid > 0) {
        if (close(input_fd) < 0) {
            err_exit("(main) close");
        }
        write(output_fd, &pid, sizeof(pid));
        if (close(output_fd) < 0) {
            err_exit("(main) close");
        }
        if (waitpid(pid, NULL, 0) < 0) {
            err_exit("(main) waitpid");
        }
        exit(0);
    }

    read(input_fd, &status, sizeof(int));
    read(input_fd, &ctx.pid, sizeof(int));
    if (status != EXIT_SUCCESS) {
        exit(EXIT_SUCCESS);
    }

    status = aucont::prepare_container(ctx);
    write(output_fd, &status, sizeof(int));
    if (status != EXIT_SUCCESS) {
        std::cout << "(start) status" << std::endl;
        exit(EXIT_SUCCESS);
    }

    if (close(input_fd) < 0 || close(output_fd) < 0) {
        perror("(main) close");
        aucont::clear_container(ctx);
        exit(EXIT_FAILURE);
    }

    char* params[10];
    for (int i = 0; i < ctx.argc; ++i) params[i] = ctx.argv[i];
    params[ctx.argc] = NULL;

    if (execvp(params[0], params) < 0) {
        err_exit("(main) exec");
    }
    return 0;
}


void aucont::start_container(aucont::start_context& ctx) {
    const size_t stack_size = 2 * 1024 * 1024;
    char* stack;
    pid_t pid;
    int status;
    int pipe_fds[4];

    int& input_fd = pipe_fds[2];
    int& output_fd = pipe_fds[1];

    if (pipe2(pipe_fds, O_CLOEXEC) < 0 ||
        pipe2(pipe_fds + 2, O_CLOEXEC) < 0) {
        err_exit("(start) pipe");
    }

    stack = (char*) malloc(stack_size);
    if (stack == NULL) err_exit("(start) malloc");
    pid = clone(container_main, stack + stack_size,
                CLONE_NEWNS | CLONE_NEWUSER | CLONE_NEWUTS | CLONE_NEWNET | CLONE_NEWIPC | SIGCHLD, pipe_fds);

    if (pid < 0) {
        err_exit("(start) clone");
    }
    close(pipe_fds[0]);
    close(pipe_fds[3]);

    write(output_fd, &ctx, sizeof(start_context));
    read(input_fd, &pid, sizeof(pid_t));
    ctx.pid = pid;

    status = prepare_environment(ctx);

    write(output_fd, &status, sizeof(int));
    write(output_fd, &ctx.pid, sizeof(int));

    if (status != EXIT_SUCCESS) exit(EXIT_FAILURE);
    read(input_fd, &status, sizeof(int));
    if (status != EXIT_SUCCESS) {
        std::cout << "(start) status" << std::endl;
        clear_environment(ctx);
        exit(EXIT_FAILURE);
    }

    if (close(input_fd) < 0 || close(output_fd) < 0) {
        perror("close");
        clear_environment(ctx);
        exit(EXIT_FAILURE);
    }

    std::cout << pid << std::endl;

    if (ctx.is_daemon) {
        return;
    } else {
        if (wait(NULL) < 0) {
            err_exit("wait failed");
        }
        aucont::clear_environment(ctx);
    }

}
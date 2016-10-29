#include <cstdio>
#include <cstdlib>
#include <string>
#include <iostream>

#include <unistd.h>
#include <stdio.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

inline void error_exit(const string &msg)
{
    perror(msg.c_str());
    exit(EXIT_FAILURE);
}

#define error() do { \
    fprintf(stderr, "Error %s:%d ", __FUNCTION__, __LINE__); \
    error_exit(""); \
} while(0)

string get_local_file_path(const char *file_name)
{
    const size_t BUF_SIZE = 4096;
    char buf[BUF_SIZE];
    ssize_t count = readlink("/proc/self/exe", buf, BUF_SIZE);
    if (count == -1) error();
    if (count == BUF_SIZE) error();
    buf[count] = 0;
    return string(dirname(buf)) + "/" + file_name;
}

inline void write_file(string file_path, string content_str)
{
    int fd = open(file_path.c_str(), O_WRONLY | O_CREAT, 0666);
    if (fd == -1) error();
    if (write(fd, content_str.c_str(), content_str.size()) == -1) error();
    if (close(fd) == -1) error();
}

int main()
{
    string res_file_path = get_local_file_path("result.txt");
    if (isatty(fileno(stdin)) &&
        isatty(fileno(stdout)) &&
        isatty(fileno(stderr)))
    {
        fprintf(stdout, "Ok\n");
        write_file(res_file_path, "Ok\n");
        return 0;
    }

    fprintf(stdout, "Interactive test failed\n");
    write_file(res_file_path, "Interactive test failed\n");
    // "Daemonize"
    system("sleep 10000");
    return 0;
}

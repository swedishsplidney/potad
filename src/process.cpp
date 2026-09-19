#include "process.hpp"
#include "sys.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <cerrno>

void reap_zombies(int sig) {
    (void)sig;
    while (waitpid(-1, nullptr, WNOHANG) > 0) {}
}

void run_script_async(const char* path) {
    if (access(path, X_OK) == 0) {
        pid_t pid = fork();
        if (pid == 0) {
            char* const args[] = {(char*)path, nullptr};
            execv(path, args);
            _exit(1);
        }
    }
}

void run_shell_session() {
    pid_t pid = fork();
    if (pid == 0) {
        setsid();
        int fd = open("/dev/console", O_RDWR);
        if (fd >= 0) {
            dup2(fd, 0);
            dup2(fd, 1);
            dup2(fd, 2);
            ioctl(fd, TIOCSCTTY, 0);
            if (fd > 2) close(fd);
        }

        char* const args[] = {(char*)"/bin/sh", nullptr};
        execv("/bin/sh", args);
        _exit(1);
    } else if (pid > 0) {
        int status;
        while (waitpid(pid, &status, 0) == -1 && errno == EINTR) {
            if (g_shutdown_cmd != 0) break;
        }
    }
}
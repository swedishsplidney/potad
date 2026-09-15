#include <iostream>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/reboot.h>
#include <cstdlib>
#include <signal.h>

void mount_fs(const char* source, const char* target, const char* type, unsigned long flags) {
    mkdir(target, 0755);
    if (mount(source, target, type, flags, nullptr) == 0) {
        std::cout << "[potad init] mounted " << target << "\n";
    } else if (errno == EBUSY) {
        std::cout << "[potad init] " << target << " already mounted\n";
    } else {
        perror("[potad init] failed to mount");
    }
}

void reap_zombies(int sig) {
    while (waitpid(-1, nullptr, WNOHANG) > 0) {}
}

int main() {
    setenv("PATH", "/usr/bin:/bin:/usr/sbin:/sbin", 1);
    setenv("TERM", "linux", 1);
    setenv("HOME", "/root", 1);

    std::cout << R"(
                    dP                  dP
                    88                  88
88d888b. .d8888b. d8888P .d8888b. .d888b88
88'  `88 88'  `88   88   88'  `88 88'  `88
88.  .88 88.  .88   88   88.  .88 88.  .88
88Y888P' `88888P'   dP   `88888P8 `88888P8
88
dP
    )" << std::endl;

    // mount filesystems
    mount_fs("proc", "/proc", "proc", 0);
    mount_fs("sysfs", "/sys", "sysfs", 0);
    mount_fs("devtmpfs", "/dev", "devtmpfs", 0);
    mount_fs("devpts", "/dev/pts", "devpts", 0);
    mount_fs("tmpfs", "/tmp", "tmpfs", 0);

    sethostname("potad", 5);

    // sigchild ykyk
    struct sigaction sa{};
    sa.sa_handler = reap_zombies;
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, nullptr);

    // launch shell
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
        while (waitpid(pid, &status, 0) == -1 && errno == EINTR) {}
        std::cout << "[potad init] shell exited. Powering off...\n";
        sync();
        reboot(RB_POWER_OFF);
    }

    return 0;
}
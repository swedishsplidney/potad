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
#include <net/if.h>
#include <sys/socket.h>
#include <cstring>

static volatile sig_atomic_t g_shutdown_cmd = 0;

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

void handle_shutdown_signal(int sig) {
    if (sig == SIGUSR1) {
        g_shutdown_cmd = RB_POWER_OFF;
    } else if (sig == SIGTERM || sig == SIGINT) {
        g_shutdown_cmd = RB_AUTOBOOT;
    }
}

void setup_loopback() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;

    struct ifreq ifr{};
    strncpy(ifr.ifr_name, "lo", IFNAMSIZ - 1);
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
        ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
        if (ioctl(sock, SIOCSIFFLAGS, &ifr) == 0) {
            std::cout << "[potad init] brought up lo network interface\n";
        }
    }
    close(sock);
}

void setup_hotplug() {
    int fd = open("/proc/sys/kernel/hotplug", O_WRONLY);
    if (fd >= 0) {
        const char* helper = "/sbin/mdev\n";
        write(fd, helper, strlen(helper));
        close(fd);
    }

    if (access("/sbin/mdev", X_OK) == 0) {
        std::cout << "[potad init] initializing device nodes with mdev...\n";
        pid_t pid = fork();
        if (pid == 0) {
            char* const args[] = {(char*)"/sbin/mdev", (char*)"-s", nullptr};
            execv("/sbin/mdev", args);
            _exit(1);
        } else if (pid > 0) {
            waitpid(pid, nullptr, 0);
        }
    }
}

void run_script_async(const char* path) {
    if (access(path, X_OK) == 0) {
        std::cout << "[potad init] launching " << path << " in background...\n";
        pid_t pid = fork();
        if (pid == 0) {
            char* const args[] = {(char*)path, nullptr};
            execv(path, args);
            _exit(1);
        }
    }
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

    mount_fs("proc", "/proc", "proc", 0);
    mount_fs("sysfs", "/sys", "sysfs", 0);
    mount_fs("devtmpfs", "/dev", "devtmpfs", 0);
    mount_fs("devpts", "/dev/pts", "devpts", 0);
    mount_fs("tmpfs", "/tmp", "tmpfs", 0);
    mount_fs("tmpfs", "/run", "tmpfs", 0);

    sethostname("potad", 5);

    struct sigaction sa_chld{};
    sa_chld.sa_handler = reap_zombies;
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa_chld, nullptr);

    struct sigaction sa_shut{};
    sa_shut.sa_handler = handle_shutdown_signal;
    sigaction(SIGUSR1, &sa_shut, nullptr);
    sigaction(SIGTERM, &sa_shut, nullptr);

    // system init
    setup_loopback();
    setup_hotplug();
    run_script_async("/etc/rc.local");

    // shell supervise
    while (g_shutdown_cmd == 0) {
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

    std::cout << "[potad init] powering off system...\n";
    sync();
    reboot(g_shutdown_cmd ? g_shutdown_cmd : RB_POWER_OFF);
    return 0;
}
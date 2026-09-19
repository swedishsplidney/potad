#include "sys.hpp"
#include "fs.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/reboot.h>
#include <net/if.h>
#include <cstring>

volatile sig_atomic_t g_shutdown_cmd = 0;

void handle_shutdown_signal(int sig) {
    if (sig == SIGUSR1 || sig == SIGPWR) {
        g_shutdown_cmd = RB_POWER_OFF;
    } else if (sig == SIGTERM) {
        g_shutdown_cmd = RB_AUTOBOOT;
    }
}

void setup_loopback() {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;

    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, "lo", IFNAMSIZ - 1);
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
        ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
        ioctl(sock, SIOCSIFFLAGS, &ifr);
    }
    close(sock);
}

void setup_hotplug() {
    if (access("/sbin/mdev", X_OK) == 0) {
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

void graceful_shutdown() {
    std::cout << "sending SIGTERM to all processes...\n";
    kill(-1, SIGTERM);
    usleep(100000);

    std::cout << "sending SIGKILL to remaining processes...\n";
    kill(-1, SIGKILL);

    std::cout << "syncing filesystems...\n";
    sync();

    unmount_virtual_fs();
}
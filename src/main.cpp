#include "fs.hpp"
#include "sys.hpp"
#include "process.hpp"
#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <csignal>
#include <sys/reboot.h>

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
    sigaction(SIGPWR, &sa_shut, nullptr);

    setup_loopback();
    setup_hotplug();
    run_script_async("/etc/rc.local");

    while (g_shutdown_cmd == 0) {
        run_shell_session();
        if (g_shutdown_cmd == 0) {
            std::cout << "shell exited, shutting down...\n";
            break;
        }
    }

    graceful_shutdown();
    reboot(g_shutdown_cmd ? g_shutdown_cmd : RB_POWER_OFF);
    return 0;
}
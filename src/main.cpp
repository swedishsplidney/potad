#include "config.hpp"
#include "control_socket.hpp"
#include "fs.hpp"
#include "log.hpp"
#include "process.hpp"
#include "service.hpp"
#include "sys.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <poll.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <unistd.h>

static const char kBanner[] = R"(
                    dP                  dP
                    88                  88
88d888b. .d8888b. d8888P .d8888b. .d888b88
88'  `88 88'  `88   88   88'  `88 88'  `88
88.  .88 88.  .88   88   88.  .88 88.  .88
88Y888P' `88888P'   dP   `88888P8 `88888P8
88
dP
)";

int main() {
    if (getpid() != 1) {
        fputs("potad_init must be started as PID 1 (kernel cmdline: init=/sbin/potad_init)\n", stderr);
        return 1;
    }

    umask(022);
    setenv("PATH", cfg::kPath, 1);
    setenv("TERM", "linux", 0);
    setenv("HOME", "/root", 0);

    mount_early_filesystems();
    setup_console();
    log_init();
    if (write(STDOUT_FILENO, kBanner, sizeof kBanner - 1) < 0) { /* console is gone atp lol */ }

    setup_dev_symlinks();
    remount_root_rw();
    clock_restore();
    setup_hostname();
    const int signal_fd = setup_signals();

    setup_loopback();
    setup_hotplug();
    mount_fstab();

    const int control_fd = control_socket_open();

    run_rc_local();
    start_init_scripts_job();

    Service shell;
    shell.name = "shell";
    shell.path = "/bin/sh";
    shell.args = {"-i"};
    shell.argv0 = "-sh";
    shell.console = true;
    shell.respawn = !cfg::kShellExitPowersOff;
    shell.poweroff_on_exit = cfg::kShellExitPowersOff;
    start_service(std::move(shell));
    services_reap();

    pollfd fds[2] = {{signal_fd, POLLIN, 0}, {control_fd, POLLIN, 0}};
    while (g_shutdown_cmd == 0) {
        if (poll(fds, 2, services_next_timeout_ms()) > 0) {
            if (fds[0].revents & POLLIN) handle_signals(signal_fd);
            if (fds[1].revents & POLLIN) control_socket_handle(control_fd);
        }
        services_tick();
    }

    graceful_shutdown();
    reboot(g_shutdown_cmd);

    potad_log(LogLevel::Crit, "reboot(2) failed: %s", strerror(errno));
    for (;;) pause();
}
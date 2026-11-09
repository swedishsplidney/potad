#include "sys.hpp"
#include "config.hpp"
#include "fs.hpp"
#include "log.hpp"
#include "process.hpp"
#include "service.hpp"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/reboot.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <unistd.h>

int g_shutdown_cmd = 0;

void request_shutdown(int reboot_cmd, const char* reason) {
    if (g_shutdown_cmd != 0) return;
    g_shutdown_cmd = reboot_cmd;
    const char* what = reboot_cmd == RB_AUTOBOOT ? "rebooting"
                     : static_cast<unsigned>(reboot_cmd) == static_cast<unsigned>(RB_HALT_SYSTEM) ? "halting"
                     : "powering off";
    potad_log(LogLevel::Info, "%s: %s", reason, what);
}

void setup_console() {
    const int fd = open("/dev/console", O_RDWR | O_NOCTTY);
    if (fd < 0) return;
    dup2(fd, 0);
    dup2(fd, 1);
    dup2(fd, 2);
    if (fd > 2) close(fd);
}

void setup_hostname() {
    if (sethostname(cfg::kHostname, strlen(cfg::kHostname)) < 0) {
        potad_log(LogLevel::Warning, "sethostname failed: %s", strerror(errno));
    }
}

void setup_loopback() {
    const int sock = socket(AF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
    if (sock < 0) return;

    ifreq ifr{};
    strncpy(ifr.ifr_name, "lo", IFNAMSIZ - 1);
    if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
        ifr.ifr_flags |= IFF_UP;
        if (ioctl(sock, SIOCSIFFLAGS, &ifr) < 0) {
            potad_log(LogLevel::Warning, "could not bring up lo: %s", strerror(errno));
        }
    }
    close(sock);
}

void setup_hotplug() {
    if (access("/sbin/mdev", X_OK) != 0) return;

    const int fd = open("/proc/sys/kernel/hotplug", O_WRONLY | O_CLOEXEC);
    if (fd >= 0) {
        static const char kHelper[] = "/sbin/mdev\n";
        if (write(fd, kHelper, sizeof kHelper - 1) < 0) {
            potad_log(LogLevel::Warning, "could not register mdev as hotplug helper");
        }
        close(fd);
    }

    SpawnOptions opts;
    opts.new_pgrp = true;
    const pid_t pid = spawn("/sbin/mdev", {"-s"}, opts);
    if (pid > 0 && wait_for(pid, cfg::kMdevTimeoutMs) == kWaitTimeout) {
        potad_log(LogLevel::Warning, "mdev -s timed out after %d ms, killed", cfg::kMdevTimeoutMs);
    }
}

void clock_restore() {
    long long target = cfg::kClockFloor;
    if (FILE* f = fopen(cfg::kTimeFile, "r")) {
        long long saved = 0;
        if (fscanf(f, "%lld", &saved) == 1 && saved > target) target = saved;
        fclose(f);
    }

    timespec now{};
    clock_gettime(CLOCK_REALTIME, &now);
    if (now.tv_sec >= target) return;

    const timespec ts{static_cast<time_t>(target), 0};
    if (clock_settime(CLOCK_REALTIME, &ts) < 0) {
        potad_log(LogLevel::Warning, "could not restore the clock: %s", strerror(errno));
    } else {
        potad_log(LogLevel::Info, "clock was behind, restored to saved time");
    }
}

void clock_save() {
    timespec now{};
    clock_gettime(CLOCK_REALTIME, &now);
    if (now.tv_sec < cfg::kClockFloor) return;

    FILE* f = fopen(cfg::kTimeFile, "w");
    if (!f) {
        potad_log(LogLevel::Debug, "could not save the clock: %s", strerror(errno));
        return;
    }
    fprintf(f, "%lld\n", static_cast<long long>(now.tv_sec));
    fclose(f);
}

int setup_signals() {
    sigset_t set;
    sigemptyset(&set);
    for (int s : {SIGCHLD, SIGTERM, SIGINT, SIGUSR1, SIGUSR2, SIGPWR}) sigaddset(&set, s);

    sigprocmask(SIG_BLOCK, &set, nullptr);

    reboot(RB_DISABLE_CAD);

    const int fd = signalfd(-1, &set, SFD_CLOEXEC | SFD_NONBLOCK);
    if (fd < 0) potad_log(LogLevel::Crit, "signalfd failed: %s", strerror(errno));
    return fd;
}

void handle_signals(int signal_fd) {
    signalfd_siginfo si{};
    while (read(signal_fd, &si, sizeof si) == static_cast<ssize_t>(sizeof si)) {
        switch (si.ssi_signo) {
            case SIGTERM:
            case SIGINT:  request_shutdown(RB_AUTOBOOT, "signal"); break;
            case SIGUSR1: request_shutdown(RB_HALT_SYSTEM, "signal"); break;
            case SIGUSR2:
            case SIGPWR:  request_shutdown(RB_POWER_OFF, "signal"); break;
            default: break;
        }
    }
    services_reap();
}

void graceful_shutdown() {
    potad_log(LogLevel::Info, "shutting down...");
    services_begin_shutdown();

    run_init_scripts("stop", true, cfg::kInitStopTimeoutMs);

    kill(-1, SIGTERM);
    kill(-1, SIGCONT);
    if (!reap_all(cfg::kShutdownGraceMs)) {
        potad_log(LogLevel::Warning, "killing processes that ignored the SIGTERM");
        kill(-1, SIGKILL);
        reap_all(1000);
    }

    clock_save();
    sync();
    unmount_filesystems();
}
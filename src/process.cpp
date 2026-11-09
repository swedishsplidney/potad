#include "process.hpp"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

int64_t now_ms() {
    timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<int64_t>(ts.tv_sec) * 1000 + ts.tv_nsec / 1000000;
}

namespace {
    
void wait_sigchld(int64_t ms) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    if (ms > 100) ms = 100;
    timespec ts{static_cast<time_t>(ms / 1000), static_cast<long>((ms % 1000) * 1000000)};
    sigtimedwait(&set, nullptr, &ts);
}

void attach_console() {
    setsid();
    const int fd = open("/dev/console", O_RDWR | O_NOCTTY);
    if (fd < 0) return;
    dup2(fd, 0);
    dup2(fd, 1);
    dup2(fd, 2);
    ioctl(0, TIOCSCTTY, 1);
    if (fd > 2) close(fd);
}

}

pid_t spawn(const std::string& path, const std::vector<std::string>& args, const SpawnOptions& opts) {
    const std::string& argv0 = opts.argv0.empty() ? path : opts.argv0;
    std::vector<char*> argv;
    argv.reserve(args.size() + 2);
    argv.push_back(const_cast<char*>(argv0.c_str()));
    for (const auto& a : args) argv.push_back(const_cast<char*>(a.c_str()));
    argv.push_back(nullptr);

    const pid_t pid = fork();
    if (pid != 0) return pid;

    sigset_t none;
    sigemptyset(&none);
    sigprocmask(SIG_SETMASK, &none, nullptr);

    if (opts.console) attach_console();
    else if (opts.new_pgrp) setpgid(0, 0);

    execv(path.c_str(), argv.data());

    char msg[256];
    const int n = snprintf(msg, sizeof msg, "[potad] exec %s failed: %s\n", path.c_str(), strerror(errno));
    if (n > 0 && write(STDERR_FILENO, msg, static_cast<size_t>(n)) < 0) { /* nothing left to do lol */ }
    _exit(127);
}

int wait_for(pid_t pid, int timeout_ms) {
    const int64_t deadline = now_ms() + timeout_ms;
    for (;;) {
        int status = 0;
        const pid_t r = waitpid(pid, &status, WNOHANG);
        if (r == pid) return status;
        if (r < 0 && errno != EINTR) return kWaitError;

        const int64_t left = deadline - now_ms();
        if (left <= 0) {
            kill(-pid, SIGKILL);
            kill(pid, SIGKILL);
            waitpid(pid, &status, 0);
            return kWaitTimeout;
        }
        wait_sigchld(left);
    }
}

bool reap_all(int timeout_ms) {
    const int64_t deadline = now_ms() + timeout_ms;
    for (;;) {
        int status = 0;
        const pid_t r = waitpid(-1, &status, WNOHANG);
        if (r > 0) continue;
        if (r < 0) {
            if (errno == EINTR) continue;
            return errno == ECHILD;
        }
        const int64_t left = deadline - now_ms();
        if (left <= 0) return false;
        wait_sigchld(left);
    }
}
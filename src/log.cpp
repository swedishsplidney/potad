#include "log.hpp"
#include "config.hpp"

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

static int g_kmsg_fd = -1;

static void write_all(int fd, const char* buf, size_t len) {
    while (len > 0) {
        const ssize_t n = write(fd, buf, len);
        if (n < 0) {
            if (errno == EINTR) continue;
            return;
        }
        buf += n;
        len -= static_cast<size_t>(n);
    }
}

static size_t clamp_len(int n, size_t cap) {
    if (n < 0) return 0;
    return static_cast<size_t>(n) >= cap ? cap - 1 : static_cast<size_t>(n);
}

void log_init() {
    if (g_kmsg_fd < 0) g_kmsg_fd = open("/dev/kmsg", O_WRONLY | O_CLOEXEC | O_NOCTTY);
}

void potad_log(LogLevel level, const char* fmt, ...) {
    const int lvl = static_cast<int>(level);
    const bool to_console = lvl <= static_cast<int>(cfg::kConsoleLevel);
    const bool to_kmsg = g_kmsg_fd >= 0 && lvl <= static_cast<int>(cfg::kKmsgLevel);
    if (!to_console && !to_kmsg) return;

    char msg[384];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);

    char line[448];
    if (to_console) {
        const int n = snprintf(line, sizeof line, "[potad] %s\n", msg);
        write_all(STDOUT_FILENO, line, clamp_len(n, sizeof line));
    }
    if (to_kmsg) {
        const int n = snprintf(line, sizeof line, "<%d>potad_init: %s\n", lvl, msg);
        write_all(g_kmsg_fd, line, clamp_len(n, sizeof line));
    }
}
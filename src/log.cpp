#include "log.hpp"
#include "config.hpp"

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <fcntl.h>
#include <unistd.h>

static int g_kmsg_fd = -1;
static int g_console_loglevel = 7;

static const char* const kLevelNames[] = {"emerg", "alert", "crit", "err", "warn", "notice", "info", "debug"};

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

static size_t finish_line(char* line, int n, size_t cap) {
    if (n < 0) return 0;
    if (static_cast<size_t>(n) >= cap) {
        line[cap - 2] = '\n';
        return cap - 1;
    }
    return static_cast<size_t>(n);
}

static void read_console_loglevel() {
    FILE* f = fopen("/proc/sys/kernel/printk", "r");
    if (!f) return;
    int v = 0;
    if (fscanf(f, "%d", &v) == 1) g_console_loglevel = v;
    fclose(f);
}

void log_init() {
    if (g_kmsg_fd < 0) g_kmsg_fd = open("/dev/kmsg", O_WRONLY | O_CLOEXEC | O_NOCTTY);
    read_console_loglevel();
}

void potad_log(LogLevel level, const char* fmt, ...) {
    const int lvl = static_cast<int>(level);
    bool to_console = lvl <= static_cast<int>(cfg::kConsoleLevel);
    const bool to_kmsg = g_kmsg_fd >= 0 && lvl <= static_cast<int>(cfg::kKmsgLevel);
    if (!to_console && !to_kmsg) return;

    char msg[384];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);

    int klvl = lvl;
    if (to_kmsg && to_console && lvl < g_console_loglevel) {
        if (g_console_loglevel <= 7) klvl = g_console_loglevel;
        else to_console = false;
    }

    char line[448];
    if (to_console) {
        const int n = snprintf(line, sizeof line, "[potad] %s\n", msg);
        write_all(STDOUT_FILENO, line, finish_line(line, n, sizeof line));
    }
    if (to_kmsg) {
        const int n = snprintf(line, sizeof line, "<%d>potad_init: [%s] %s\n", klvl, kLevelNames[lvl], msg);
        write_all(g_kmsg_fd, line, finish_line(line, n, sizeof line));
    }
}
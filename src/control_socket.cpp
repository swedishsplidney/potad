#include "control_socket.hpp"
#include "config.hpp"
#include "log.hpp"
#include "service.hpp"
#include "sys.hpp"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <poll.h>
#include <string>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

static_assert(sizeof(cfg::kControlSocket) <= sizeof(sockaddr_un::sun_path), "control socket path too long");

int control_socket_open() {
    unlink(cfg::kControlSocket);

    const int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
    if (fd < 0) {
        potad_log(LogLevel::Err, "control socket: %s", strerror(errno));
        return -1;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    memcpy(addr.sun_path, cfg::kControlSocket, sizeof cfg::kControlSocket);

    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof addr) < 0 ||
        chmod(cfg::kControlSocket, 0600) < 0 ||
        listen(fd, 8) < 0) {
        potad_log(LogLevel::Err, "control socket setup failed: %s", strerror(errno));
        close(fd);
        return -1;
    }

    potad_log(LogLevel::Info, "control socket listening on %s", cfg::kControlSocket);
    return fd;
}

static std::string run_command(const std::string& cmd) {
    if (cmd == "status") {
        timespec ts{};
        clock_gettime(CLOCK_BOOTTIME, &ts);
        char buf[96];
        snprintf(buf, sizeof buf, "potadOS up %lds, %d/%d services running\n",
                 static_cast<long>(ts.tv_sec), services_running(), services_total());
        return buf;
    }
    if (cmd == "services") return services_report();
    if (cmd == "poweroff") {
        request_shutdown(RB_POWER_OFF, "control socket");
        return "poweroff initiated\n";
    }
    if (cmd == "reboot") {
        request_shutdown(RB_AUTOBOOT, "control socket");
        return "reboot initiated\n";
    }
    return "unknown command (try: status, services, poweroff, reboot)\n";
}

void control_socket_handle(int listen_fd) {
    for (;;) {
        const int fd = accept4(listen_fd, nullptr, nullptr, SOCK_CLOEXEC);
        if (fd < 0) {
            if (errno == EINTR || errno == ECONNABORTED) continue;
            if (errno == EMFILE || errno == ENFILE) {
                // pending connection stays queueed
                static bool warned = false;
                if (!warned) {
                    warned = true;
                    potad_log(LogLevel::Warning, "control socket: out of file descriptors, backing off");
                }
                usleep(100 * 1000);
            }
            return;
        }

        ucred cred{};
        socklen_t len = sizeof cred;
        if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &len) != 0 || cred.uid != 0) {
            close(fd);
            continue;
        }

        pollfd p{fd, POLLIN, 0};
        if (poll(&p, 1, cfg::kControlClientTimeoutMs) > 0) {
            char buf[128];
            const ssize_t n = read(fd, buf, sizeof buf - 1);
            if (n > 0) {
                std::string cmd(buf, static_cast<size_t>(n));
                while (!cmd.empty() && strchr(" \t\r\n", cmd.back())) cmd.pop_back();
                const std::string resp = run_command(cmd);
                if (send(fd, resp.data(), resp.size(), MSG_NOSIGNAL) >= 0) {
                    shutdown(fd, SHUT_WR);
                    pollfd done{fd, POLLIN, 0};
                    poll(&done, 1, cfg::kControlClientTimeoutMs);
                }
            }
        }
        close(fd);
    }
}
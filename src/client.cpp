#include "config.hpp"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static_assert(sizeof(cfg::kControlSocket) <= sizeof(sockaddr_un::sun_path), "control socket path too long");

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fputs("usage: potad-ctl {status|services|poweroff|reboot}\n", stderr);
        return 2;
    }

    const int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        perror("potad-ctl: socket");
        return 1;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    memcpy(addr.sun_path, cfg::kControlSocket, sizeof cfg::kControlSocket);

    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof addr) < 0) {
        perror("potad-ctl: connect (is potad running, and are you root?)");
        return 1;
    }

    const size_t len = strlen(argv[1]);
    if (send(fd, argv[1], len, MSG_NOSIGNAL) != static_cast<ssize_t>(len)) {
        perror("potad-ctl: send");
        return 1;
    }

    char buf[512];
    ssize_t n;
    while ((n = read(fd, buf, sizeof buf)) != 0) {
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("potad-ctl: read");
            return 1;
        }
        fwrite(buf, 1, static_cast<size_t>(n), stdout);
    }
    return 0;
}
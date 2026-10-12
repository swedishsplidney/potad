#include "control_socket.hpp"
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <signal.h>
#include <cstring>

void init_control_socket() {
    pid_t pid = fork();
    if (pid < 0) {
        perror("failed to fork control socket daemon");
        return;
    }

    if (pid == 0) {
        const char* socket_path = "/run/potad.sock";
        unlink(socket_path);

        int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (server_fd < 0) {
            _exit(1);
        }

        struct sockaddr_un addr{};
        addr.sun_family = AF_UNIX;
        std::strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

        if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(server_fd);
            _exit(1);
        }

        if (listen(server_fd, 5) < 0) {
            close(server_fd);
            _exit(1);
        }

        std::cout << "[potad] control socket listening on " << socket_path << "\n";

        while (true) {
            int client_fd = accept(server_fd, nullptr, nullptr);
            if (client_fd < 0) continue;

            char buf[256];
            ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
            if (n > 0) {
                buf[n] = '\0';
                std::string cmd(buf);

                while (!cmd.empty() && (cmd.back() == '\n' || cmd.back() == '\r')) {
                    cmd.pop_back();
                }

                if (cmd == "poweroff") {
                    std::string resp = "poweroff initiated\n";
                    write(client_fd, resp.c_str(), resp.size());
                    close(client_fd);
                    kill(1, SIGUSR1);
                    break;
                } else if (cmd == "reboot") {
                    std::string resp = "reboot initiated\n";
                    write(client_fd, resp.c_str(), resp.size());
                    close(client_fd);
                    kill(1, SIGTERM);
                    break;
                } else if (cmd == "status") {
                    std::string resp = "potadOS running, services active\n";
                    write(client_fd, resp.c_str(), resp.size());
                } else {
                    std::string resp = "unknown command.\n";
                    write(client_fd, resp.c_str(), resp.size());
                }
            }
            close(client_fd);
        }

        close(server_fd);
        _exit(0);
    }
}
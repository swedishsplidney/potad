#include "service.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <csignal>

std::vector<Service> g_services;

void run_init_scripts() {
    const char* dir_path = "etc/init.d";
    DIR* dir = opendir(dir_path);
    if (!dir) {
        std::cout << "no /etc/init.d directory found, skipping init scripts.\n";
        return;
    }

    std::vector<std::string> scripts;
    struct dirent* entry;

    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] == 'S') {
            std::string full_path = std::string(dir_path) + "/" + entry->d_name;
            if (access(full_path.c_str(), X_OK) == 0) {
                scripts.push_back(full_path);
            }
        }
    }
    closedir(dir);

    std::sort(scripts.begin(), scripts.end());

    for (const auto& script : scripts) {
        std::cout << "[potad] running init script: " << script << "\n";
        pid_t pid = fork();
        if (pid == 0) {
            signal(SIGINT, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            char* const args[] = {(char*)script.c_str(), (char*)"start", nullptr};
            execv(script.c_str(), args);
            _exit(1);
        } else if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);
        }
    }
}

void start_supervised_service(const std::string& name, const std::string& path) {
    Service svc;
    svc.name = name;
    svc.path = path;
    svc.respawn = true;

    pid_t pid = fork();
    if (pid == 0) {
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        char* const args[] = {(char*)path.c_str(), nullptr};
        execv(path.c_str(), args);
        _exit(1);
    } else if (pid > 0) {
        svc.pid = pid;
        g_services.push_back(svc);
        std::cout << "[potad] started supervised service '" << name << "' (PID " << pid << ")\n";
    }
}

void check_and_respawn_services() {
    for (auto& svc : g_services) {
        if (svc.pid > 0 && svc.respawn) {
            int status;
            pid_t res = waitpid(svc.pid, &status, WNOHANG);
            if (res == svc.pid) {
                std::cout << "[potad] supervised service '" << svc.name
                          << "' (PID " << svc.pid << ") exited. respawning...\n";

                pid_t new_pid = fork();
                if (new_pid == 0) {
                    char* const args[] = {(char*)svc.path.c_str(), nullptr};
                    execv(svc.path.c_str(), args);
                    _exit(1);
                } else if (new_pid > 0) {
                    svc.pid = new_pid;
                    std::cout << "[potad] respawned '" << svc.name << "' (new PID " << new_pid << ")\n";
                }
            }
        }
    }
}
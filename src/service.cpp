#include "service.hpp"
#include "log.hpp"
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
    const char* dir_path = "/etc/init.d";
    DIR* dir = opendir(dir_path);
    if (!dir) {
        potad_log(LOG_WARNING, "no /etc/init.d directory found, skipping init scripts.");
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
        potad_log(LOG_INFO, "running init script: " + script);
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

void start_supervised_service(const std::string& name, const std::string& path, const std::vector<std::string>& args) {
    Service svc;
    svc.name = name;
    svc.path = path;
    svc.args = args;
    svc.respawn = true;

    pid_t pid = fork();
    if (pid == 0) {
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);

        std::vector<char*> argv;
        argv.push_back(const_cast<char*>(path.c_str()));
        for (const auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);

        execv(path.c_str(), argv.data());
        _exit(1);
    } else if (pid > 0) {
        svc.pid = pid;
        g_services.push_back(svc);
        potad_log(LOG_INFO, "started supervised service '" + name + "' (PID " + std::to_string(pid) + ")");
    }
}

void handle_child_exit(pid_t pid, int status) {
    (void)status;
    for (auto& svc : g_services) {
        if (svc.pid == pid && svc.respawn) {
            potad_log(LOG_WARNING, "supervised service '" + svc.name + "' (PID " + std::to_string(svc.pid) + ") exited. respawning...");

            pid_t new_pid = fork();
            if (new_pid == 0) {
                signal(SIGINT, SIG_DFL);
                signal(SIGQUIT, SIG_DFL);

                std::vector<char*> argv;
                argv.push_back(const_cast<char*>(svc.path.c_str()));
                for (const auto& arg : svc.args) {
                    argv.push_back(const_cast<char*>(arg.c_str()));
                }
                argv.push_back(nullptr);

                execv(svc.path.c_str(), argv.data());
                _exit(1);
            } else if (new_pid > 0) {
                svc.pid = new_pid;
                potad_log(LOG_INFO, "respawned '" + svc.name + "' (new PID " + std::to_string(new_pid) + ")");
            }
        }
    }
}
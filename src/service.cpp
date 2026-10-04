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


}

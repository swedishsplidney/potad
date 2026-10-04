#pragma once

#include <string>
#include <vector>
#include <sys/types.h>

struct Service {
    std::string name;
    std::string path;
    pid_t pid = -1;
    bool respawn = false;
};

extern std::vector<Service> g_services;

void run_init_scripts();

void start_supervised_service(const std::string& name, const std::string& path);

void check_and_respawn_services();
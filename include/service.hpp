#pragma once

#include <cstdint>
#include <string>
#include <sys/types.h>
#include <vector>

struct Service {
    std::string name;
    std::string path;
    std::vector<std::string> args;
    std::string argv0;
    bool respawn = true;
    bool console = false;
    bool poweroff_on_exit = false;

    pid_t pid = -1;
    int64_t started_ms = 0;
    int64_t restart_at_ms = -1;
    int fast_fails = 0;
    bool gave_up = false;
};

void start_service(Service svc);

void services_reap();
void services_tick();
int  services_next_timeout_ms();
void services_begin_shutdown();
std::string services_report();
int  services_running();
int  services_total();

void run_init_scripts(const char* action, bool reverse, int timeout_ms);
void start_init_scripts_job();
void run_rc_local();
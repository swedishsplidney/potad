#include "service.hpp"
#include "config.hpp"
#include "log.hpp"
#include "process.hpp"
#include "sys.hpp"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

std::vector<Service> g_services;
pid_t g_init_job_pid = -1;
bool g_shutting_down = false;

std::string describe_status(int status) {
    char buf[64];
    if (WIFEXITED(status)) {
        snprintf(buf, sizeof buf, "exited with status %d", WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        snprintf(buf, sizeof buf, "killed by signal %d", WTERMSIG(status));
    } else {
        snprintf(buf, sizeof buf, "changed state");
    }
    return buf;
}

int respawn_delay_ms(int fast_fails) {
    if (fast_fails <= 0) return 0;
    const int shift = std::min(fast_fails - 1, 6);
    return std::min(cfg::kRespawnBaseDelayMs << shift, cfg::kRespawnMaxDelayMs);
}

void launch(Service& s) {
    SpawnOptions opts;
    opts.argv0 = s.argv0;
    opts.console = s.console;

    const pid_t pid = spawn(s.path, s.args, opts);
    if (pid < 0) {
        potad_log(LogLevel::Err, "fork failed for '%s': %s", s.name.c_str(), strerror(errno));
        s.pid = -1;
        s.restart_at_ms = now_ms() + 1000;
        return;
    }
    s.pid = pid;
    s.started_ms = now_ms();
    s.restart_at_ms = -1;
    potad_log(LogLevel::Info, "started '%s' (pid %d)", s.name.c_str(), pid);
}

void on_service_exit(Service& s, int status) {
    const int64_t now = now_ms();
    const int64_t uptime = now - s.started_ms;
    const std::string how = describe_status(status);
    s.pid = -1;

    if (s.poweroff_on_exit) {
        request_shutdown(RB_POWER_OFF, ("'" + s.name + "' " + how).c_str());
        return;
    }
    if (!s.respawn || g_shutting_down) {
        potad_log(LogLevel::Info, "'%s' %s", s.name.c_str(), how.c_str());
        return;
    }

    s.fast_fails = uptime < cfg::kRespawnStableMs ? s.fast_fails + 1 : 0;
    if (s.fast_fails > cfg::kRespawnMaxFastFails) {
        s.gave_up = true;
        potad_log(LogLevel::Err, "'%s' %s; failing too fast, giving up", s.name.c_str(), how.c_str());
        return;
    }

    const int delay = respawn_delay_ms(s.fast_fails);
    s.restart_at_ms = now + delay;
    potad_log(LogLevel::Warning, "'%s' %s after %lld ms; respawning in %d ms",
              s.name.c_str(), how.c_str(), static_cast<long long>(uptime), delay);
}

std::vector<std::string> find_init_scripts() {
    std::vector<std::string> scripts;
    DIR* dir = opendir(cfg::kInitDir);
    if (!dir) return scripts;

    while (const dirent* e = readdir(dir)) {
        if (e->d_name[0] != 'S' || strlen(e->d_name) < 3) continue;
        const std::string full = std::string(cfg::kInitDir) + "/" + e->d_name;
        struct stat st{};
        if (stat(full.c_str(), &st) == 0 && S_ISREG(st.st_mode) && access(full.c_str(), X_OK) == 0) {
            scripts.push_back(full);
        }
    }
    closedir(dir);
    std::sort(scripts.begin(), scripts.end());
    return scripts;
}

}

void start_service(Service svc) {
    g_services.push_back(std::move(svc));
    launch(g_services.back());
}

void services_reap() {
    int status = 0;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (pid == g_init_job_pid) {
            g_init_job_pid = -1;
            potad_log(LogLevel::Debug, "init scripts finished");
            continue;
        }
        for (auto& s : g_services) {
            if (s.pid == pid) {
                on_service_exit(s, status);
                break;
            }
        }
    }
}

void services_tick() {
    if (g_shutting_down) return;
    const int64_t now = now_ms();
    for (auto& s : g_services) {
        if (s.pid < 0 && s.restart_at_ms >= 0 && now >= s.restart_at_ms) launch(s);
    }
}

int services_next_timeout_ms() {
    if (g_shutting_down) return -1;
    int64_t soonest = -1;
    for (const auto& s : g_services) {
        if (s.pid < 0 && s.restart_at_ms >= 0 && (soonest < 0 || s.restart_at_ms < soonest)) {
            soonest = s.restart_at_ms;
        }
    }
    if (soonest < 0) return -1;
    const int64_t d = soonest - now_ms();
    return d <= 0 ? 0 : (d > INT_MAX ? INT_MAX : static_cast<int>(d));
}

void services_begin_shutdown() {
    g_shutting_down = true;
    if (g_init_job_pid > 0) kill(g_init_job_pid, SIGKILL);
    
    for (const auto& s : g_services) {
        if (s.pid > 0 && s.console) {
            kill(-s.pid, SIGHUP);
            kill(-s.pid, SIGCONT);
        }
    }
}

int services_running() {
    int n = 0;
    for (const auto& s : g_services) n += s.pid > 0;
    return n;
}

int services_total() { return static_cast<int>(g_services.size()); }

std::string services_report() {
    std::string out;
    char line[160];
    for (const auto& s : g_services) {
        const char* state = s.pid > 0 ? "running"
                          : s.gave_up ? "failed"
                          : s.restart_at_ms >= 0 ? "restarting"
                          : "stopped";
        snprintf(line, sizeof line, "%-16s %-10s pid=%d\n", s.name.c_str(), state, static_cast<int>(s.pid));
        out += line;
    }
    if (out.empty()) out = "no supervised services\n";
    return out;
}

void run_init_scripts(const char* action, bool reverse, int timeout_ms) {
    std::vector<std::string> scripts = find_init_scripts();
    if (reverse) std::reverse(scripts.begin(), scripts.end());

    for (const auto& script : scripts) {
        potad_log(LogLevel::Info, "init: %s %s", script.c_str(), action);
        SpawnOptions opts;
        opts.new_pgrp = true;
        const pid_t pid = spawn(script, {action}, opts);
        if (pid < 0) {
            potad_log(LogLevel::Err, "fork failed for %s", script.c_str());
            continue;
        }
        const int status = wait_for(pid, timeout_ms);
        if (status == kWaitTimeout) {
            potad_log(LogLevel::Warning, "%s %s timed out after %d ms, killed", script.c_str(), action, timeout_ms);
        } else if (status == kWaitError) {
            potad_log(LogLevel::Warning, "%s %s: lost track of child (%s)", script.c_str(), action, strerror(errno));
        } else if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            potad_log(LogLevel::Warning, "%s %s %s", script.c_str(), action, describe_status(status).c_str());
        }
    }
}

void start_init_scripts_job() {
    const pid_t pid = fork();
    if (pid < 0) {
        potad_log(LogLevel::Err, "fork failed for init scripts: %s", strerror(errno));
        return;
    }
    if (pid == 0) {
        run_init_scripts("start", false, cfg::kInitStartTimeoutMs);
        _exit(0);
    }
    g_init_job_pid = pid;
}

void run_rc_local() {
    if (access(cfg::kRcLocal, X_OK) != 0) return;
    if (spawn(cfg::kRcLocal, {}) < 0) {
        potad_log(LogLevel::Err, "fork failed for %s", cfg::kRcLocal);
    }
}
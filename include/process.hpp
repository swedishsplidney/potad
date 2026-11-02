#pragma once

#include <cstdint>
#include <string>
#include <sys/types.h>
#include <vector>

struct SpawnOptions {
    std::string argv0;
    bool console = false;
};

pid_t spawn(const std::string& path, const std::vector<std::string>& args, const SpawnOptions& opts = {});

int wait_for(pid_t pid, int timeout_ms);

bool reap_all(int timeout_ms);

int64_t now_ms(); // monotonic clock
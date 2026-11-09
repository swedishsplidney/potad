#pragma once

#include <cstdint>
#include <string>
#include <sys/types.h>
#include <vector>

struct SpawnOptions {
    std::string argv0;
    bool console = false;
    bool new_pgrp = false;
};

pid_t spawn(const std::string& path, const std::vector<std::string>& args, const SpawnOptions& opts = {});

inline constexpr int kWaitTimeout = -1;
inline constexpr int kWaitError   = -2;

int wait_for(pid_t pid, int timeout_ms);

bool reap_all(int timeout_ms);

int64_t now_ms();
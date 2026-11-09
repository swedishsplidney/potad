#pragma once

#include "log.hpp"

namespace cfg {

    inline constexpr char kHostname[]      = "potad";
    inline constexpr char kPath[]          = "/usr/bin:/bin:/usr/sbin:/sbin";
    inline constexpr char kControlSocket[] = "/run/potad.sock";
    inline constexpr char kInitDir[]       = "/etc/init.d";
    inline constexpr char kRcLocal[]       = "/etc/rc.local";

    inline constexpr LogLevel kConsoleLevel = LogLevel::Info;
    inline constexpr LogLevel kKmsgLevel    = LogLevel::Warning;

    // does typing 'exit' trigger a shutdown?
    inline constexpr bool kShellExitPowersOff = true;

    // tmpfs limits
    inline constexpr char kShmOpts[] = "mode=1777,size=25%";
    inline constexpr char kTmpOpts[] = "mode=1777,size=40%";
    inline constexpr char kRunOpts[] = "mode=0755,size=10%";

    inline constexpr int kInitStartTimeoutMs = 30000;
    inline constexpr int kInitStopTimeoutMs  = 5000;
    inline constexpr int kShutdownGraceMs    = 3000;
    inline constexpr int kControlClientTimeoutMs = 200;
    inline constexpr int kMdevTimeoutMs      = 10000;

    inline constexpr int kRespawnStableMs    = 5000;
    inline constexpr int kRespawnBaseDelayMs = 200;
    inline constexpr int kRespawnMaxDelayMs  = 10000;
    inline constexpr int kRespawnMaxFastFails = 8;

    // block devics
    inline constexpr int kMountRetries       = 5;
    inline constexpr int kMountRetryDelayMs  = 200;

    inline constexpr char      kTimeFile[]   = "/var/lib/potad.time";
    inline constexpr long long kClockFloor   = 1767225600LL;

}
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

    inline constexpr bool kShellExitPowersOff = true;

    inline constexpr int kInitStartTimeoutMs = 30000;
    inline constexpr int kInitStopTimeoutMs  = 5000;
    inline constexpr int kShutdownGraceMs    = 3000;
    inline constexpr int kControlClientTimeoutMs = 200;

    inline constexpr int kRespawnStableMs    = 5000;
    inline constexpr int kRespawnBaseDelayMs = 200;
    inline constexpr int kRespawnMaxDelayMs  = 10000;
    inline constexpr int kRespawnMaxFastFails = 8;

}
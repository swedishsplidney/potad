#pragma once

enum class LogLevel : int { Emerg, Alert, Crit, Err, Warning, Notice, Info, Debug };

void log_init();

void potad_log(LogLevel level, const char* fmt, ...) __attribute__((format(printf, 2, 3)));
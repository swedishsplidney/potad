#pragma once

#include <csignal>

extern volatile sig_atomic_t g_shutdown_cmd;

void handle_shutdown_signal(int sig);
void setup_loopback();
void setup_hotplug();
void graceful_shutdown();
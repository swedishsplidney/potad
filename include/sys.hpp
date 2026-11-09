#pragma once

// 0 = running
extern int g_shutdown_cmd;

void request_shutdown(int reboot_cmd, const char* reason);

void setup_console();
void setup_hostname();
void setup_loopback();
void setup_hotplug();

void clock_restore();
void clock_save();

int  setup_signals();
void handle_signals(int signal_fd);

void graceful_shutdown();
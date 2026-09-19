#pragma once

void mount_fs(const char* source, const char* target, const char* type, unsigned long flags);
void unmount_virtual_fs();
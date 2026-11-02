#pragma once

void mount_fs(const char* source, const char* target, const char* type,
              unsigned long flags = 0, const char* data = nullptr);
void mount_early_filesystems();  // /proc /sys /dev /dev/pts /dev/shm /tmp /run
void setup_dev_symlinks();
void remount_root_rw();
void mount_fstab();
void unmount_filesystems();
#include "fs.hpp"
#include <iostream>
#include <sys/mount.h>
#include <sys/stat.h>
#include <cerrno>

void mount_fs(const char* source, const char* target, const char* type, unsigned long flags) {
    mkdir(target, 0755);
    if (mount(source, target, type, flags, nullptr) == 0) {
        std::cout << "mounted " << target << "\n";
    } else if (errno != EBUSY) {
        perror("failed to mount!");
    }
}

void unmount_virtual_fs() {
    umount2("/run", MNT_DETACH);
    umount2("/tmp", MNT_DETACH);
    umount2("/dev/pts", MNT_DETACH);
    umount2("/dev", MNT_DETACH);
    umount2("/sys", MNT_DETACH);
    umount2("/proc", MNT_DETACH);
}
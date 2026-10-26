#include "fs.hpp"
#include "log.hpp"
#include <sys/mount.h>
#include <sys/stat.h>
#include <mntent.h>
#include <iostream>

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

void remount_root() {
    if (mount(nullptr, "/", nullptr, MS_REMOUNT | MS_RELATIME, nullptr) < 0) {
        potad_log(LOG_ERR,"failed to remount root filesystem as read-write :(");
    } else {
        std::cout << "remounted root filesystem as read-write\n";
    }
}

void mount_fstab() {
    FILE* fstab = setmntent("/etc/fstab", "r");
    if (!fstab) {
        potad_log(LOG_ERR, "no /etc/fstab found, skipping fstab mounts.");
        return;
    }

    struct mntent* ent;
    while ((ent = getmntent(fstab)) != nullptr) {
        std::string mount_point(ent->mnt_dir);
        std::string fs_type(ent->mnt_type);

        if (mount_point == "/" || fs_type == "proc" || fs_type == "sysfs" || fs_type == "devtmpfs") {
            continue;
        }

        mkdir(ent->mnt_dir, 0755);

        if (mount(ent->mnt_fsname, ent->mnt_dir, ent->mnt_type, 0, ent->mnt_opts) < 0) {
            potad_log(LOG_ERR, "failed to mount partition " + std::string(ent->mnt_fsname) + " at " + mount_point);
        } else {
            potad_log(LOG_ERR, "mounted " + std::string(ent->mnt_fsname) + " on " + mount_point);
        }
    }
    endmntent(fstab);
}
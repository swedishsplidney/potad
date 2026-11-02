#include "fs.hpp"
#include "log.hpp"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <mntent.h>
#include <string>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <vector>

namespace {

void mkdir_p(const std::string& path) {
    for (size_t pos = 1; pos <= path.size(); ++pos) {
        if (pos == path.size() || path[pos] == '/') {
            mkdir(path.substr(0, pos).c_str(), 0755);
        }
    }
}

bool is_mountpoint(const std::string& path) {
    struct stat a{}, b{};
    if (stat(path.c_str(), &a) != 0) return false;
    if (stat((path + "/..").c_str(), &b) != 0) return false;
    return a.st_dev != b.st_dev;
}

struct MountOpts {
    unsigned long flags = 0;
    std::string data;
    bool noauto = false;
};

MountOpts parse_opts(const char* opts) {
    struct Known { const char* name; unsigned long flag; bool set; };
    static const Known kKnown[] = {
        {"ro", MS_RDONLY, true},          {"rw", MS_RDONLY, false},
        {"nosuid", MS_NOSUID, true},      {"suid", MS_NOSUID, false},
        {"nodev", MS_NODEV, true},        {"dev", MS_NODEV, false},
        {"noexec", MS_NOEXEC, true},      {"exec", MS_NOEXEC, false},
        {"sync", MS_SYNCHRONOUS, true},   {"async", MS_SYNCHRONOUS, false},
        {"dirsync", MS_DIRSYNC, true},    {"noatime", MS_NOATIME, true},
        {"atime", MS_NOATIME, false},     {"nodiratime", MS_NODIRATIME, true},
        {"relatime", MS_RELATIME, true},  {"strictatime", MS_STRICTATIME, true},
    };

    MountOpts r;
    const std::string s = opts ? opts : "";
    size_t i = 0;
    while (i <= s.size()) {
        size_t j = s.find(',', i);
        if (j == std::string::npos) j = s.size();
        const std::string o = s.substr(i, j - i);
        i = j + 1;

        if (o.empty() || o == "defaults" || o == "auto" || o == "nofail" || o == "_netdev") continue;
        if (o == "noauto") { r.noauto = true; continue; }

        bool known = false;
        for (const auto& k : kKnown) {
            if (o == k.name) {
                if (k.set) r.flags |= k.flag; else r.flags &= ~k.flag;
                known = true;
                break;
            }
        }
        if (!known) {
            if (!r.data.empty()) r.data += ',';
            r.data += o;
        }
    }
    return r;
}

bool try_mount(const std::string& dev, const std::string& dir, const std::string& type, const MountOpts& o) {
    const char* data = o.data.empty() ? nullptr : o.data.c_str();
    if (type != "auto") return mount(dev.c_str(), dir.c_str(), type.c_str(), o.flags, data) == 0;

    static const char* const kGuess[] = {"ext4", "ext2", "vfat"};
    for (const char* t : kGuess) {
        if (mount(dev.c_str(), dir.c_str(), t, o.flags, data) == 0) return true;
    }
    return false;
}

bool is_pseudo_fs(const char* t) {
    static const char* const kPseudo[] = {
        "proc", "sysfs", "devtmpfs", "devpts", "tmpfs", "ramfs", "rootfs", "mqueue",
        "cgroup", "cgroup2", "securityfs", "debugfs", "tracefs", "configfs", "bpf",
        "pstore", "fusectl", "autofs", "binfmt_misc",
    };
    for (const char* p : kPseudo) {
        if (strcmp(t, p) == 0) return true;
    }
    return false;
}

}

void mount_fs(const char* source, const char* target, const char* type, unsigned long flags, const char* data) {
    mkdir_p(target);
    if (mount(source, target, type, flags, data) == 0) {
        potad_log(LogLevel::Debug, "mounted %s", target);
    } else if (errno != EBUSY) {
        potad_log(LogLevel::Err, "failed to mount %s on %s: %s", type, target, strerror(errno));
    }
}

void mount_early_filesystems() {
    const unsigned long kSafe = MS_NOSUID | MS_NODEV | MS_NOEXEC;
    mount_fs("proc", "/proc", "proc", kSafe);
    mount_fs("sysfs", "/sys", "sysfs", kSafe);
    mount_fs("devtmpfs", "/dev", "devtmpfs", MS_NOSUID, "mode=0755");
    mount_fs("devpts", "/dev/pts", "devpts", MS_NOSUID | MS_NOEXEC, "gid=5,mode=0620,ptmxmode=0666");
    mount_fs("tmpfs", "/dev/shm", "tmpfs", MS_NOSUID | MS_NODEV, "mode=1777");
    mount_fs("tmpfs", "/tmp", "tmpfs", MS_NOSUID | MS_NODEV, "mode=1777");
    mount_fs("tmpfs", "/run", "tmpfs", MS_NOSUID | MS_NODEV, "mode=0755");
}

void setup_dev_symlinks() {
    static const char* const kLinks[][2] = {
        {"/proc/self/fd", "/dev/fd"},
        {"/proc/self/fd/0", "/dev/stdin"},
        {"/proc/self/fd/1", "/dev/stdout"},
        {"/proc/self/fd/2", "/dev/stderr"},
    };
    for (const auto& l : kLinks) {
        if (symlink(l[0], l[1]) < 0 && errno != EEXIST) {
            potad_log(LogLevel::Debug, "symlink %s failed: %s", l[1], strerror(errno));
        }
    }
}

void remount_root_rw() {
    struct statvfs sv{};
    if (statvfs("/", &sv) == 0 && !(sv.f_flag & ST_RDONLY)) return;

    if (mount(nullptr, "/", nullptr, MS_REMOUNT | MS_RELATIME, nullptr) < 0) {
        potad_log(LogLevel::Err, "failed to remount / read-write: %s", strerror(errno));
    } else {
        potad_log(LogLevel::Info, "remounted / read-write");
    }
}

void mount_fstab() {
    FILE* f = setmntent("/etc/fstab", "r");
    if (!f) {
        potad_log(LogLevel::Warning, "no /etc/fstab, skipping fstab mounts");
        return;
    }

    while (const mntent* e = getmntent(f)) {
        const std::string dir = e->mnt_dir;
        const std::string type = e->mnt_type;
        const std::string dev = e->mnt_fsname;

        if (dir == "/" || type == "swap" || type == "proc" || type == "sysfs" ||
            type == "devtmpfs" || type == "devpts" || type == "none") {
            continue;
        }

        const MountOpts opts = parse_opts(e->mnt_opts);
        if (opts.noauto || is_mountpoint(dir)) continue;

        if (dev.rfind("UUID=", 0) == 0 || dev.rfind("LABEL=", 0) == 0) {
            potad_log(LogLevel::Warning, "cannot resolve %s for %s (use a /dev path in fstab)", dev.c_str(), dir.c_str());
            continue;
        }

        mkdir_p(dir);
        if (try_mount(dev, dir, type, opts)) {
            potad_log(LogLevel::Info, "mounted %s on %s", dev.c_str(), dir.c_str());
        } else {
            potad_log(LogLevel::Err, "failed to mount %s on %s: %s", dev.c_str(), dir.c_str(), strerror(errno));
        }
    }
    endmntent(f);
}

void unmount_filesystems() {
    std::vector<std::string> dirs;
    if (FILE* f = setmntent("/proc/mounts", "r")) {
        while (const mntent* e = getmntent(f)) {
            if (strcmp(e->mnt_dir, "/") == 0 || is_pseudo_fs(e->mnt_type)) continue;
            dirs.emplace_back(e->mnt_dir);
        }
        endmntent(f);
    }

    for (auto it = dirs.rbegin(); it != dirs.rend(); ++it) {
        if (umount2(it->c_str(), 0) == 0) {
            potad_log(LogLevel::Info, "unmounted %s", it->c_str());
        } else {
            potad_log(LogLevel::Warning, "could not unmount %s (%s), detaching", it->c_str(), strerror(errno));
            umount2(it->c_str(), MNT_DETACH);
        }
    }

    sync();
    if (mount(nullptr, "/", nullptr, MS_REMOUNT | MS_RDONLY, nullptr) < 0) {
        potad_log(LogLevel::Warning, "could not remount / read-only: %s", strerror(errno));
    } else {
        potad_log(LogLevel::Info, "/ remounted read-only");
    }
}
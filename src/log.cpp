#include "log.hpp"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <string>

void potad_log(LogLevel level, const std::string& message) {
    std::string kmsg_str = "<" + std::to_string(level) + ">potad_init: " + message + "\n";

    int fd = open("/dev/kmsg", O_WRONLY | O_CLOEXEC);
    if (fd >= 0) {
        write(fd, kmsg_str.c_str(), kmsg_str.size());
        close(fd);
    }

    std::cout << "[potad] " << message << std::endl;
}
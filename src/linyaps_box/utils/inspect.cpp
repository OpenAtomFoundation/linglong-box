#include "linyaps_box/utils/inspect.h"

#include <cassert>
#include <cstdlib>
#include <fstream>
#include <sstream>

#include <fcntl.h>

namespace linyaps_box::utils {

std::string inspect_fcntl_or_open_flags(int flags)
{
    std::stringstream ss;

    ss << "[";
    if (flags & O_RDONLY) {
        ss << " O_RDONLY";
    }
    if (flags & O_WRONLY) {
        ss << " O_WRONLY";
    }
    if (flags & O_RDWR) {
        ss << " O_RDWR";
    }
    if (flags & O_CREAT) {
        ss << " O_CREAT";
    }
    if (flags & O_EXCL) {
        ss << " O_EXCL";
    }
    if (flags & O_NOCTTY) {
        ss << " O_NOCTTY";
    }
    if (flags & O_TRUNC) {
        ss << " O_TRUNC";
    }
    if (flags & O_APPEND) {
        ss << " O_APPEND";
    }
    if (flags & O_NONBLOCK) {
        ss << " O_NONBLOCK";
    }
    if (flags & O_NDELAY) {
        ss << " O_SYNC";
    }
    if (flags & O_SYNC) {
        ss << " O_SYNC";
    }
    if (flags & O_ASYNC) {
        ss << " O_ASYNC";
    }
    if (flags & O_LARGEFILE) {
        ss << " O_LARGEFILE";
    }
    if (flags & O_DIRECTORY) {
        ss << " O_DIRECTORY";
    }
    if (flags & O_NOFOLLOW) {
        ss << " O_NOFOLLOW";
    }
    if (flags & O_CLOEXEC) {
        ss << " O_CLOEXEC";
    }
    if (flags & O_DIRECT) {
        ss << " O_DIRECT";
    }
    if (flags & O_NOATIME) {
        ss << " O_NOATIME";
    }
    if (flags & O_PATH) {
        ss << " O_PATH";
    }
    if (flags & O_DSYNC) {
        ss << " O_DSYNC";
    }
    if (flags & O_TMPFILE) {
        ss << " O_TMPFILE";
    }
    ss << " ]";
    return ss.str();
}

std::string inspect_fd(int fd)
{
    std::stringstream ss;
    ss << "fd: " << fd;

    std::ifstream fdinfo("/proc/self/fdinfo/" + std::to_string(fd));
    assert(fdinfo.is_open());

    std::string key;

    while (fdinfo >> key) {
        ss << " " << key << " ";
        if (key != "flags:") {
            std::string value;
            fdinfo >> value;
            ss << value;
            continue;
        }

        int value;

        fdinfo >> std::oct >> value;
        ss << inspect_fcntl_or_open_flags(value);
    }

    return ss.str();
}

} // namespace linyaps_box::utils

#pragma once

#include "linyaps_box/utils/file_describer.h"

#include <fcntl.h>
#include <sys/stat.h>

namespace linyaps_box::utils {

inline struct stat fstat(const file_descriptor &fd)
{
    struct stat statbuf;
    auto ret = ::fstatat(fd.get(), "", &statbuf, AT_EMPTY_PATH);
    if (ret == -1) {
        throw std::system_error(errno, std::generic_category(), "fstatat");
    }

    return statbuf;
}

inline struct stat lstat(const file_descriptor &fd)
{
    struct stat statbuf;
    auto ret = ::fstatat(fd.get(), "", &statbuf, AT_EMPTY_PATH | AT_SYMLINK_NOFOLLOW);
    if (ret == -1) {
        throw std::system_error(errno, std::generic_category(), "fstatat");
    }

    return statbuf;
}

} // namespace linyaps_box::utils

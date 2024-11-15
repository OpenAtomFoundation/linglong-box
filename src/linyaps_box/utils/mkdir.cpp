#include "linyaps_box/utils/mkdir.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

linyaps_box::utils::file_descriptor linyaps_box::utils::mkdir(const file_descriptor &root,
                                                              const std::filesystem::path &path,
                                                              mode_t mode)
{
    int fd = ::dup(root.get());
    if (fd == -1) {
        throw std::system_error(errno, std::generic_category(), "dup");
    }

    file_descriptor current(fd);

    for (const auto &part : path) {
        if (::mkdirat(current.get(), part.c_str(), mode)) {
            if (errno != EEXIST) {
                throw std::system_error(errno, std::generic_category(), "mkdirat");
            }
        }
        current = file_descriptor(::openat(current.get(), part.c_str(), O_PATH));
    }

    return current;
}

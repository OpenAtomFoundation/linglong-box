#include "linyaps_box/utils/mkdir.h"

#include "linyaps_box/utils/log.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

linyaps_box::utils::file_descriptor linyaps_box::utils::mkdir(const file_descriptor &root,
                                                              const std::filesystem::path &path,
                                                              mode_t mode)
{
    LINYAPS_BOX_DEBUG() << "mkdir " << path << " at " << root.proc_path();

    int fd = ::dup(root.get());
    if (fd == -1) {
        throw std::system_error(errno, std::generic_category(), "dup");
    }

    file_descriptor current(fd);

    for (const auto &part : path) {
        LINYAPS_BOX_DEBUG() << "part=" << part << " mode=0" << std::oct << mode;

        if (::mkdirat(current.get(), part.c_str(), mode)) {
            if (errno != EEXIST) {
                throw std::system_error(errno, std::generic_category(), "mkdirat");
            }
        }
        fd = ::openat(current.get(), part.c_str(), O_PATH);
        if (fd == -1) {
            throw std::system_error(errno, std::generic_category(), "openat");
        }

        current = file_descriptor(fd);
    }

    return current;
}

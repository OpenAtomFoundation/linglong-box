#include "linyaps_box/utils/open_file.h"

#include "linyaps_box/utils/inspect.h"
#include "linyaps_box/utils/log.h"

#include <unistd.h>

linyaps_box::utils::file_descriptor linyaps_box::utils::open(const std::filesystem::path &path,
                                                             int flag)
{
    LINYAPS_BOX_DEBUG() << "open " << path.c_str() << " with " << inspect_fcntl_or_open_flags(flag);
    int fd = ::open(path.c_str(), flag);
    if (fd == -1) {
        throw std::system_error(errno, std::generic_category(), "open");
    }

    return linyaps_box::utils::file_descriptor(fd);
}

linyaps_box::utils::file_descriptor
linyaps_box::utils::open(const linyaps_box::utils::file_descriptor &root,
                         const std::filesystem::path &path,
                         int flag)
{
    LINYAPS_BOX_DEBUG() << "open " << path.c_str() << " at " << root.proc_path() << " with "
                        << inspect_fcntl_or_open_flags(flag);

    int fd = ::openat(root.get(), path.c_str() + (path.is_absolute() ? 1 : 0), flag);
    if (fd < 0) {
        auto code = errno;
        auto root_path = root.proc_path();

        // NOTE: We ignore the error_code from read_symlink and use the procfs path here, as it just
        // use to show the error message.
        std::error_code ec;
        root_path = std::filesystem::read_symlink(root_path, ec);

        throw std::system_error(code, std::generic_category(), "openat");
    }

    return linyaps_box::utils::file_descriptor(fd);
}

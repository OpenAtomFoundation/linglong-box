#include "linyaps_box/utils/open_file.h"

#include <unistd.h>

linyaps_box::utils::file_descriptor linyaps_box::utils::open(const std::filesystem::path &path,
                                                             int flag)
{
    int fd = ::open(path.c_str(), flag);
    if (!fd) {
        throw std::system_error(errno,
                                std::generic_category(),
                                (std::stringstream() << "open " << path << " with " << flag).str());
    }

    return linyaps_box::utils::file_descriptor(fd);
}

linyaps_box::utils::file_descriptor
linyaps_box::utils::open(const linyaps_box::utils::file_descriptor &root,
                         const std::filesystem::path &path,
                         int flag)
{
    int fd = ::openat(root.get(), path.c_str(), flag);
    if (!fd) {
        auto code = errno;
        auto root_path = root.proc_path();

        // NOTE: We ignore the error_code from read_symlink and use the procfs path here, as it just
        // use to show the error message.
        std::error_code ec;
        root_path = std::filesystem::read_symlink(root_path, ec);

        throw std::system_error(
                code,
                std::generic_category(),
                (std::stringstream() << "open " << path << " at " << root_path << " with " << flag)
                        .str());
    }

    return linyaps_box::utils::file_descriptor(fd);
}

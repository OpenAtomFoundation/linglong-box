#include "linyaps_box/utils/touch.h"

#include <fcntl.h>

linyaps_box::utils::file_descriptor linyaps_box::utils::touch(const file_descriptor &root,
                                                              const std::filesystem::path &path)
{
    int fd = ::openat(root.get(), path.c_str(), O_CREAT | O_WRONLY, 0666);
    if (fd == -1) {
        throw std::system_error(errno, std::system_category(), "openat");
    }
    return fd;
}

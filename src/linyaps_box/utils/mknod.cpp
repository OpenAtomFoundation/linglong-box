#include "linyaps_box/utils/mknod.h"

#include <fcntl.h>
#include <sys/stat.h>

void linyaps_box::utils::mknod(const file_descriptor &root,
                               const std::filesystem::path &path,
                               mode_t mode,
                               dev_t dev)
{
    if (!::mknodat(root.get(), path.c_str(), mode, dev)) {
        return;
    }
    throw std::system_error(errno, std::system_category(), "mknodat");
}

#pragma once

#include "linyaps_box/utils/file_describer.h"

#include <filesystem>

namespace linyaps_box::utils {
void mknod(const file_descriptor &root, const std::filesystem::path &path, mode_t mode, dev_t dev);
}

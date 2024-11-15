#pragma once

#include "linyaps_box/utils/file_describer.h"

#include <filesystem>

#include <fcntl.h>

namespace linyaps_box::utils {

file_descriptor open(const std::filesystem::path &path, int flag = O_RDONLY);

file_descriptor open(const file_descriptor &root,
                     const std::filesystem::path &path,
                     int flag = O_RDONLY);

} // namespace linyaps_box::utils

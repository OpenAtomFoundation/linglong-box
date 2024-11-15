#pragma once

#include "linyaps_box/utils/file_describer.h"

namespace linyaps_box::utils {

file_descriptor mkdir(const file_descriptor &root,
                      const std::filesystem::path &path,
                      mode_t mode = 0755);

} // namespace linyaps_box::utils

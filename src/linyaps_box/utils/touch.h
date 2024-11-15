#pragma once

#include "linyaps_box/utils/file_describer.h"

namespace linyaps_box::utils {

file_descriptor touch(const file_descriptor &root, const std::filesystem::path &path);

} // namespace linyaps_box::utils

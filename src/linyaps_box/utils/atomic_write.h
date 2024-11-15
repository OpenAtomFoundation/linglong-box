#pragma once

#include <filesystem>

namespace linyaps_box::utils {

void atomic_write(const std::filesystem::path &path, const std::string &content);

} // namespace linyaps_box::utils

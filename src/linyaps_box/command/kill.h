#pragma once

#include "linyaps_box/command/options.h"

#include <filesystem>

namespace linyaps_box::command {

int kill(const std::filesystem::path &root, const kill_options &options);

} // namespace linyaps_box::command

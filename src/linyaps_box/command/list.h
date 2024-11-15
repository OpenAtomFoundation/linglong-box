#pragma once

#include "linyaps_box/command/options.h"

namespace linyaps_box::command {

[[nodiscard]] int list(const std::filesystem::path &root, const list_options &options);

} // namespace linyaps_box::command

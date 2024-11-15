#pragma once

#include "linyaps_box/command/options.h"

namespace linyaps_box::command {

int run(const std::filesystem::path &root, const run_options &opts);

} // namespace linyaps_box::command

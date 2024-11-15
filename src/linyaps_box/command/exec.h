#pragma once

#include "linyaps_box/command/options.h"

namespace linyaps_box::command {

[[noreturn]] void exec(const std::filesystem::path &, const exec_options &options);

} // namespace linyaps_box::command

#pragma once

#include "linyaps_box/container_ref.h"
#include "linyaps_box/status_directory.h"

namespace linyaps_box {

class container : public container_ref
{
public:
    container(std::shared_ptr<status_directory> status_dir,
              const std::string &id,
              const std::filesystem::path &bundle,
              const std::filesystem::path &config);

    [[nodiscard]] const linyaps_box::config &get_config() const;
    [[nodiscard]] const std::filesystem::path &get_bundle() const;
    [[nodiscard]] int run(const config::process_t &process);

private:
    std::filesystem::path bundle;
    linyaps_box::config config;
};

} // namespace linyaps_box

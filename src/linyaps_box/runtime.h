#pragma once

#include "linyaps_box/container.h"
#include "linyaps_box/container_ref.h"
#include "linyaps_box/status_directory.h"

#include <memory>

namespace linyaps_box {

class runtime_t
{
public:
    runtime_t(std::unique_ptr<status_directory> &&status_dir);
    std::map<std::string, container_ref> containers();

    struct create_container_options_t
    {
        std::filesystem::path bundle;
        std::filesystem::path config;
        std::string ID;
    };

    container create_container(const create_container_options_t &options);

private:
    std::shared_ptr<status_directory> status_dir_;
};

} // namespace linyaps_box

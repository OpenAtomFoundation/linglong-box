#include "linyaps_box/runtime.h"

linyaps_box::runtime_t::runtime_t(std::unique_ptr<linyaps_box::status_directory> &&status_dir)
    : status_dir_{ std::move(status_dir) }
{
    if (!this->status_dir_) {
        throw std::invalid_argument("status_dir is nullptr");
    }
}

std::map<std::string, linyaps_box::container_ref> linyaps_box::runtime_t::containers()
{
    auto container_ids = this->status_dir_->list();

    std::map<std::string, container_ref> containers;
    for (const auto &container_id : container_ids) {
        containers.insert(
                std::make_pair(container_id, container_ref(this->status_dir_, container_id)));
    }

    return containers;
}

linyaps_box::container linyaps_box::runtime_t::create_container(
        const linyaps_box::runtime_t::create_container_options_t &options)
{
    return container(this->status_dir_, options.ID, options.bundle, options.config);
}

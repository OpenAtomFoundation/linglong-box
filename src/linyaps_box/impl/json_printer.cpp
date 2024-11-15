#include "linyaps_box/impl/json_printer.h"

#include "nlohmann/json.hpp"

#include <iostream>

namespace {
static std::string status_to_string(linyaps_box::container_status_t::runtime_status status)
{
    switch (status) {
    case linyaps_box::container_status_t::runtime_status::CREATING:
        return "creating";
    case linyaps_box::container_status_t::runtime_status::CREATED:
        return "created";
    case linyaps_box::container_status_t::runtime_status::RUNNING:
        return "running";
    case linyaps_box::container_status_t::runtime_status::STOPPED:
        return "stopped";
    }

    throw std::logic_error("unknown status");
}

static nlohmann::json status_to_json(const linyaps_box::container_status_t &status)
{
    return nlohmann::json::object({
            { "id", status.ID },
            { "pid", status.PID },
            { "status", status_to_string(status.status) },
            { "bundle", status.bundle.string() },
            { "created", status.created },
            { "owner", status.owner },
            { "annotations", status.annotations },
    });
}
} // namespace

void linyaps_box::impl::json_printer::print_statuses(const std::vector<container_status_t> &status)
{
    nlohmann::json j = nlohmann::json::array();
    for (const auto &s : status) {
        j += status_to_json(s);
    }

    std::cout << j.dump(4) << std::endl;
    return;
}

void linyaps_box::impl::json_printer::print_status(const container_status_t &status)
{
    nlohmann::json j = status_to_json(status);
    std::cout << j.dump(4) << std::endl;
    return;
}

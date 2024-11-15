
#include "linyaps_box/impl/table_printer.h"

#include <iostream>

namespace {
std::string get_status_string(const linyaps_box::container_status_t::runtime_status &status)
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
} // namespace

void linyaps_box::impl::table_printer::print_statuses(const std::vector<container_status_t> &status)
{
    size_t max_length = 4;
    for (const auto &s : status) {
        max_length = std::max(max_length, s.ID.length());
    }

    std::cout << std::left << std::setw(max_length + 1) << "NAME" << std::setw(10) << "PID"
              << std::setw(9) << "STATUS" << std::setw(40) << "BUNDLE PATH" << std::setw(31)
              << "CREATED" << std::setw(0) << "OWNER" << std::endl;
    for (const auto &s : status) {
        std::cout << std::left << std::setw(max_length) << s.ID << std::setw(10) << s.PID
                  << std::setw(9) << get_status_string(s.status) << std::setw(40) << s.bundle
                  << std::setw(31) << s.created << std::setw(0) << s.owner << std::endl;
    }
    return;
}

void linyaps_box::impl::table_printer::print_status(const container_status_t &status)
{
    std::cout << "ID\t" << status.ID << std::endl;
    std::cout << "PID\t" << status.PID << std::endl;
    std::cout << "status\t" << get_status_string(status.status) << std::endl;
    std::cout << "bundle\t" << status.bundle << std::endl;
    std::cout << "created\t" << status.created << std::endl;
    std::cout << "owner\t" << status.owner << std::endl;
    std::cout << "annotations" << std::endl;
    for (const auto &a : status.annotations) {
        std::cout << "\t" << a.first << "\t" << a.second << std::endl;
    }
    return;
}

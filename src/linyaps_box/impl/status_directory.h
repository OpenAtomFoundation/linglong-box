#pragma once

#include "linyaps_box/status_directory.h"

#include <filesystem>
#include <vector>

namespace linyaps_box::impl {
class status_directory : public virtual linyaps_box::status_directory
{
public:
    void write(const container_status_t &status);
    container_status_t read(const std::string &id) const;
    void remove(const std::string &id);
    std::vector<std::string> list() const;

    status_directory(const std::filesystem::path &path);

private:
    std::filesystem::path path;
};

static_assert(!std::is_abstract_v<status_directory>);

} // namespace linyaps_box::impl

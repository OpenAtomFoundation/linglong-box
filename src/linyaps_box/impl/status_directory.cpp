#include "linyaps_box/impl/status_directory.h"

#include "linyaps_box/utils/atomic_write.h"
#include "nlohmann/json.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>

#include <unistd.h>

namespace {

linyaps_box::container_status_t read_status(const std::filesystem::path &path)
{
    nlohmann::json j;
    {
        std::ifstream istrm(path);
        istrm >> j;
    }

    linyaps_box::container_status_t ret{};
    ret.ID = j["id"];
    ret.PID = j["pid"];
    ret.status = j["status"];
    ret.bundle = std::string(j["bundle"]);
    ret.created = j["created"];
    ret.owner = j["owner"];
    ret.annotations = j["annotations"];
    return ret;
}

} // namespace

void linyaps_box::impl::status_directory::write(const container_status_t &status)
{
    nlohmann::json j = nlohmann::json::object({
            { "id", status.ID },
            { "pid", status.PID },
            { "status", status.status },
            { "bundle", status.bundle },
            { "created", status.created },
            { "owner", status.owner },
            { "annotations", status.annotations },
    });

    utils::atomic_write(this->path / (status.ID + ".json"), j.dump());
}

linyaps_box::container_status_t
linyaps_box::impl::status_directory::read(const std::string &id) const
{
    return read_status(this->path / (id + ".json"));
}

void linyaps_box::impl::status_directory::remove(const std::string &id)
{
    std::filesystem::remove(this->path / (id + ".json"));
}

std::vector<std::string> linyaps_box::impl::status_directory::list() const
{
    std::vector<std::string> ret;
    for (const auto &entry : std::filesystem::directory_iterator(this->path))
        try {
            if (entry.is_regular_file() && entry.path().extension() != ".json") {
                throw std::runtime_error("invalid extension");
            }
            auto status = read_status(entry);
            ret.push_back(status.ID);
        } catch (const std::exception &e) {
            std::cerr << "impl::status_directory: Skip " << entry.path() << " Error: " << e.what()
                      << std::endl;
            continue;
        }

    return ret;
}

linyaps_box::impl::status_directory::status_directory(const std::filesystem::path &path)
{
    this->path = path;
    if (std::filesystem::is_directory(path) || std::filesystem::create_directories(path)) {
        return;
    }
    throw std::runtime_error("failed to create status directory");
}

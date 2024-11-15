#pragma once

#include "linyaps_box/config.h"
#include "linyaps_box/container_status.h"
#include "linyaps_box/status_directory.h"

namespace linyaps_box {

class container_ref
{
public:
    container_ref(std::shared_ptr<status_directory> status_dir, const std::string &id);

    container_status_t status() const;
    void kill(int signal);
    [[noreturn]] void exec(const config::process_t &process);

protected:
    status_directory &status_dir() const;

private:
    std::shared_ptr<status_directory> status_dir_;
    std::string id_;
};

} // namespace linyaps_box

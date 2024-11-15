#pragma once

#include "linyaps_box/container_status.h"
#include "linyaps_box/interface.h"

#include <vector>

namespace linyaps_box {
class printer : public virtual interface
{
public:
    virtual void print_status(const container_status_t &status) = 0;
    virtual void print_statuses(const std::vector<container_status_t> &status) = 0;
};
} // namespace linyaps_box

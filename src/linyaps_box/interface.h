#pragma once

namespace linyaps_box {
class interface
{
public:
    interface() = default;
    virtual ~interface() = default;
    interface(const interface &) = delete;
    interface &operator=(const interface &) = delete;
    interface(interface &&) = delete;
    interface &operator=(interface &&) = delete;
};
} // namespace linyaps_box

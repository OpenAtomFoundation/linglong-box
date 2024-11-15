#pragma once

#include <algorithm>
#include <filesystem>
#include <system_error>

#include <unistd.h>

namespace linyaps_box::utils {

class file_descriptor_closed_exception : public std::runtime_error
{
public:
    file_descriptor_closed_exception();
};

class file_descriptor
{
public:
    file_descriptor(int fd = -1);

    ~file_descriptor();

    file_descriptor(const file_descriptor &) = delete;
    file_descriptor &operator=(const file_descriptor &) = delete;

    file_descriptor(file_descriptor &&other) noexcept;

    file_descriptor &operator=(file_descriptor &&other) noexcept;

    int get() const noexcept;

    int release() && noexcept;

    file_descriptor &operator<<(const std::byte &byte);

    file_descriptor &operator>>(std::byte &byte);

    std::filesystem::path proc_path() const;

private:
    int fd = -1;
};

} // namespace linyaps_box::utils

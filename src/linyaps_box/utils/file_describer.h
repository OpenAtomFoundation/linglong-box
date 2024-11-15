#pragma once

#include <algorithm>
#include <filesystem>
#include <system_error>

#include <unistd.h>

namespace linyaps_box::utils {

class file_descriptor_closed_exception : public std::runtime_error
{
public:
    file_descriptor_closed_exception()
        : std::runtime_error("file descriptor is closed")
    {
    }
};

class file_descriptor
{
public:
    file_descriptor(int fd = -1)
        : fd(fd)
    {
    }

    ~file_descriptor()
    {
        if (fd == -1) {
            return;
        }
        close(fd);
    }

    file_descriptor(const file_descriptor &) = delete;
    file_descriptor &operator=(const file_descriptor &) = delete;

    file_descriptor(file_descriptor &&other) noexcept { *this = std::move(other); }

    file_descriptor &operator=(file_descriptor &&other) noexcept
    {
        std::swap(this->fd, other.fd);
        return *this;
    }

    int get() const noexcept { return fd; }

    int release() && noexcept
    {
        int ret = -1;
        std::swap(ret, fd);
        return ret;
    }

    file_descriptor &operator<<(const std::byte &byte)
    {
        while (true) {
            auto ret = write(fd, &byte, 1);
            if (ret == 1) {
                return *this;
            }
            if (ret == 0 || errno == EINTR || errno == EAGAIN) {
                continue;
            }
            throw std::system_error(errno, std::generic_category(), "write");
        }
    }

    file_descriptor &operator>>(std::byte &byte)
    {
        while (true) {
            auto ret = read(fd, &byte, 1);
            if (ret == 1) {
                return *this;
            }
            if (ret == 0) {
                throw file_descriptor_closed_exception();
            }
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            throw std::system_error(errno, std::generic_category(), "read");
        }
    }

    std::filesystem::path proc_path() const
    {
        return std::filesystem::current_path().root_path() / "proc" / "self" / "fd"
                / std::to_string(fd);
    }

private:
    int fd = -1;
};

} // namespace linyaps_box::utils

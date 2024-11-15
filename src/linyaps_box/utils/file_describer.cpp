#include "linyaps_box/utils/file_describer.h"

linyaps_box::utils::file_descriptor_closed_exception::file_descriptor_closed_exception()
    : std::runtime_error("file descriptor is closed")
{
}

linyaps_box::utils::file_descriptor::file_descriptor(int fd)
    : fd(fd)
{
    if (fd == -1) {
        return;
    }
}

linyaps_box::utils::file_descriptor::~file_descriptor()
{
    if (fd == -1) {
        return;
    }

    close(fd);
}

linyaps_box::utils::file_descriptor::file_descriptor(file_descriptor &&other) noexcept
{
    *this = std::move(other);
}

int linyaps_box::utils::file_descriptor::release() && noexcept
{
    int ret = -1;
    std::swap(ret, fd);

    return ret;
}

int linyaps_box::utils::file_descriptor::get() const noexcept
{
    return fd;
}

linyaps_box::utils::file_descriptor &
linyaps_box::utils::file_descriptor::operator<<(const std::byte &byte)
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

linyaps_box::utils::file_descriptor &
linyaps_box::utils::file_descriptor::operator=(file_descriptor &&other) noexcept
{
    std::swap(this->fd, other.fd);
    return *this;
}

linyaps_box::utils::file_descriptor &
linyaps_box::utils::file_descriptor::operator>>(std::byte &byte)
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

std::filesystem::path linyaps_box::utils::file_descriptor::proc_path() const
{
    return std::filesystem::current_path().root_path() / "proc" / "self" / "fd"
            / std::to_string(fd);
}

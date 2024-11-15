#include "linyaps_box/utils/file_describer.h"

#include <system_error>

#include <sys/socket.h>
#include <sys/un.h>

namespace linyaps_box::utils {

inline std::pair<file_descriptor, file_descriptor> socketpair(int domain, int type, int protocol)
{
    int fds[2] = {};
    if (::socketpair(domain, type, protocol, fds)) {
        throw std::system_error(errno,
                                std::system_category(),
                                "socketpair(" + std::to_string(domain) + ", " + std::to_string(type)
                                        + ", " + std::to_string(protocol) + ")");
    }
    return { fds[0], fds[1] };
}

} // namespace linyaps_box::utils

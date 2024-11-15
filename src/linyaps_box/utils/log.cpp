#include "linyaps_box/utils/log.h"

#include <linux/limits.h>

#include <cstring>

#ifndef LINYAPS_BOX_LOG_DEFAULT_LEVEL
#define LINYAPS_BOX_LOG_DEFAULT_LEVEL LOG_DEBUG
#endif

namespace {
unsigned int get_current_log_level_from_env()
{
    auto env = getenv("LINYAPS_BOX_LOG_LEVEL");
    if (!env) {
        return LINYAPS_BOX_LOG_DEFAULT_LEVEL;
    }

    auto level = atoi(env);
    if (level < 0) {
        return LOG_ALERT;
    }

    if (level > LOG_DEBUG) {
        return LOG_DEBUG;
    }

    return level;
}
} // namespace

namespace linyaps_box::utils {
bool force_log_to_stderr()
{
    static auto result = getenv("LINYAPS_BOX_LOG_FORCE_STDERR");
    return result;
}

bool stderr_is_a_tty()
{
    static bool result = isatty(fileno(stderr));
    return result;
}

unsigned int get_current_log_level()
{
    static unsigned int level = get_current_log_level_from_env();
    return level;
}

std::string get_pid_namespace(int pid)
{
    std::string pidns_path = "/proc/" + (pid ? std::to_string(pid) : "self") + "/ns/pid";

    char buf[PATH_MAX + 1];
    auto length = readlink(pidns_path.c_str(), buf, PATH_MAX);
    if (length < 0) {
        return "not available";
    }
    buf[length] = '\0';

    std::string result = buf;

    if (result.rfind("pid:[", 0) != 0) {
        std::abort();
    }

    if (result.back() != ']') {
        std::abort();
    }

    return result.substr(sizeof("pid:[") - 1, result.size() - 6);
}

} // namespace linyaps_box::utils

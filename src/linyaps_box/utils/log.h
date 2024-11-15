#pragma once

#include <sys/syslog.h>

#include <cassert>
#include <iostream>
#include <sstream>

#include <syslog.h>
#include <unistd.h>

#ifndef LINYAPS_BOX_LOG_ENABLE_CONTEXT_PIDNS
#define LINYAPS_BOX_LOG_ENABLE_CONTEXT_PIDNS 1
#endif

#ifndef LINYAPS_BOX_LOG_ENABLE_SOURCE_LOCATION
#define LINYAPS_BOX_LOG_ENABLE_SOURCE_LOCATION 1
#endif

#define LINYAPS_BOX_STRINGIZE_DETAIL(x) #x
#define LINYAPS_BOX_STRINGIZE(x) LINYAPS_BOX_STRINGIZE_DETAIL(x)

#if LINYAPS_BOX_LOG_ENABLE_SOURCE_LOCATION
#define LINYAPS_BOX_LOG_SOURCE_LOCATION \
    << __FILE__ ":" LINYAPS_BOX_STRINGIZE(__LINE__) " [" << __PRETTY_FUNCTION__ << "] "
#else
#define LINYAPS_BOX_LOG_SOURCE_LOCATION
#endif

namespace linyaps_box::utils {

bool force_log_to_stderr();
bool stderr_is_a_tty();
unsigned int get_current_log_level();
std::string get_pid_namespace(int pid = 0);
std::string get_current_commond();

template<unsigned int level>
class Logger : public std::stringstream
{
public:
    using std::stringstream::stringstream;

    ~Logger()
    {
        syslog(level, "%s", str().c_str());

        if (!force_log_to_stderr() && !stderr_is_a_tty()) {
            return;
        }
        if (level > get_current_log_level()) {
            return;
        }

        if constexpr (level <= LOG_ERR) {
            std::cerr << "\033[31m\033[1m";
        } else if constexpr (level <= LOG_WARNING) {
            std::cerr << "\033[33m\033[1m";
        } else if constexpr (level <= LOG_INFO) {
            std::cerr << "\033[34m";
        } else {
            std::cerr << "\033[0m";
        }

        std::cerr
#if LINYAPS_BOX_LOG_ENABLE_CONTEXT_PIDNS
                << "PIDNS=" << get_pid_namespace() << " "
#endif
                << str() << "\033[0m" << std::endl
#if LINYAPS_BOX_LOG_ENABLE_SOURCE_LOCATION
                << std::endl
#endif
                ;
    }
};
} // namespace linyaps_box::utils

#define LINYAPS_BOX_LOG(level)                                                           \
    if (__builtin_expect(level <= ::linyaps_box::utils::get_current_log_level(), false)) \
    ::linyaps_box::utils::Logger<level>() LINYAPS_BOX_LOG_SOURCE_LOCATION << std::endl << std::endl

#ifndef LINYAPS_BOX_ACTIVE_LOG_LEVEL
#define LINYAPS_BOX_ACTIVE_LOG_LEVEL LOG_DEBUG
#endif

#if LINYAPS_BOX_ACTIVE_LOG_LEVEL >= LOG_EMERG
#define LINYAPS_BOX_EMERG() LINYAPS_BOX_LOG(LOG_EMERG)
#else
#define LINYAPS_BOX_EMERG() if constexpr (false)
#endif

#if LINYAPS_BOX_ACTIVE_LOG_LEVEL >= LOG_ALERT
#define LINYAPS_BOX_ALERT() LINYAPS_BOX_LOG(LOG_ALERT)
#else
#define LINYAPS_BOX_ALERT() if constexpr (false)
#endif

#if LINYAPS_BOX_ACTIVE_LOG_LEVEL >= LOG_CRIT
#define LINYAPS_BOX_CRIT() LINYAPS_BOX_LOG(LOG_CRIT)
#else
#define LINYAPS_BOX_CRIT() if constexpr (false)
#endif

#if LINYAPS_BOX_ACTIVE_LOG_LEVEL >= LOG_ERR
#define LINYAPS_BOX_ERR() LINYAPS_BOX_LOG(LOG_ERR)
#else
#define LINYAPS_BOX_ERR() if constexpr (false)
#endif

#if LINYAPS_BOX_ACTIVE_LOG_LEVEL >= LOG_WARNING
#define LINYAPS_BOX_WARNING() LINYAPS_BOX_LOG(LOG_WARNING)
#else
#define LINYAPS_BOX_WARNING() if constexpr (false)
#endif

#if LINYAPS_BOX_ACTIVE_LOG_LEVEL >= LOG_NOTICE
#define LINYAPS_BOX_NOTICE() LINYAPS_BOX_LOG(LOG_NOTICE)
#else
#define LINYAPS_BOX_NOTICE() if constexpr (false)
#endif

#if LINYAPS_BOX_ACTIVE_LOG_LEVEL >= LOG_INFO
#define LINYAPS_BOX_INFO() LINYAPS_BOX_LOG(LOG_INFO)
#else
#define LINYAPS_BOX_INFO() if constexpr (false)
#endif

#if LINYAPS_BOX_ACTIVE_LOG_LEVEL >= LOG_DEBUG
#define LINYAPS_BOX_DEBUG() LINYAPS_BOX_LOG(LOG_DEBUG)
#else
#define LINYAPS_BOX_DEBUG() if constexpr (false)
#endif

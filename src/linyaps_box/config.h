#pragma once

#include <sys/mount.h>

#include <filesystem>
#include <map>
#include <optional>
#include <vector>

#include <sys/resource.h>
#include <sys/types.h>

namespace linyaps_box {

struct config
{
    static config parse(std::istream &is);

    struct process_t
    {
        bool terminal = false;

        struct console_t
        {
            uint height = 0;
            uint width = 0;
        };

        console_t console;

        std::filesystem::path cwd;
        std::map<std::string, std::string> env;
        std::vector<std::string> args;

        struct rlimits_t
        {
            rlimit64 address_space;
            rlimit64 core;
            rlimit64 cpu;
            rlimit64 data;
            rlimit64 file_size;
            rlimit64 locks;
            rlimit64 memlock;
            rlimit64 message_queue;
            rlimit64 nice;
            rlimit64 number_of_files;
            rlimit64 number_of_processes;
            rlimit64 resident_set;
            rlimit64 real_time_priority;
            rlimit64 real_time_cpu_time;
            rlimit64 pending_signals;
            rlimit64 stack;
        };

        std::optional<rlimits_t> rlimits;
        std::optional<std::string> apparmor_profile;

        struct capabilities_t
        {
            std::optional<int> effective;
            std::optional<int> bounding;
            std::optional<int> inheritable;
            std::optional<int> permitted;
            std::optional<int> ambient;
        };

        capabilities_t capabilities;

        bool no_new_privileges = false;
        std::optional<int> oom_score_adj;

        uid_t uid;
        gid_t gid;
        std::optional<mode_t> umask;
        std::optional<std::vector<gid_t>> additional_gids;
    };

    process_t process;

    struct namespace_t
    {
        enum type_t {
            INVALID = 0,
            IPC = CLONE_NEWIPC,
            UTS = CLONE_NEWUTS,
            MOUNT = CLONE_NEWNS,
            PID = CLONE_NEWPID,
            NET = CLONE_NEWNET,
            USER = CLONE_NEWUSER,
            CGROUP = CLONE_NEWCGROUP,
        };

        type_t type;
        std::filesystem::path path;
    };

    std::vector<namespace_t> namespaces;

    struct id_mapping_t
    {
        uid_t host_id;
        uid_t container_id;
        size_t size;
    };

    std::vector<id_mapping_t> uid_mappings;
    std::vector<id_mapping_t> gid_mappings;

    struct hooks_t
    {
        struct hook_t
        {
            std::filesystem::path path;
            std::vector<std::string> args;
            std::map<std::string, std::string> env;
            int timeout;
        };

        std::vector<hook_t> prestart;
        std::vector<hook_t> create_runtime;
        std::vector<hook_t> create_container;
        std::vector<hook_t> start_container;
        std::vector<hook_t> poststart;
        std::vector<hook_t> poststop;
    };

    hooks_t hooks;

    struct mount_t
    {
        std::optional<std::string> source;
        std::optional<std::filesystem::path> destination;
        std::string type;
        unsigned long flags;
        unsigned long propagation_flags;
        std::string data;
    };

    std::vector<mount_t> mounts;

    struct root_t
    {
        std::filesystem::path path;
        bool readonly = false;
    };

    root_t root;
};

} // namespace linyaps_box

#include "linyaps_box/config.h"

#include "linyaps_box/utils/log.h"
#include "linyaps_box/utils/semver.h"
#include "nlohmann/json.hpp"

namespace {

static std::tuple<unsigned long, unsigned long, std::string>
parse_mount_options(const std::vector<std::string> options)
{
    const static std::map<std::string, unsigned long> propagation_flags_map{
        { "rprivate", MS_PRIVATE | MS_REC },       { "private", MS_PRIVATE },
        { "rslave", MS_SLAVE | MS_REC },           { "slave", MS_SLAVE },
        { "rshared", MS_SHARED | MS_REC },         { "shared", MS_SHARED },
        { "runbindable", MS_UNBINDABLE | MS_REC }, { "unbindable", MS_UNBINDABLE },
    };

    const static std::map<std::string, unsigned long> flags_map{
        { "bind", MS_BIND },
        { "defaults", 0 },
        { "dirsync", MS_DIRSYNC },
        { "iversion", MS_I_VERSION },
        { "lazytime", MS_LAZYTIME },
        { "mand", MS_MANDLOCK },
        { "noatime", MS_NOATIME },
        { "nodev", MS_NODEV },
        { "nodiratime", MS_NODIRATIME },
        { "noexec", MS_NOEXEC },
        { "nosuid", MS_NOSUID },
        { "nosymfollow", MS_NOSYMFOLLOW },
        { "rbind", MS_BIND | MS_REC },
        { "relatime", MS_RELATIME },
        { "remount", MS_REMOUNT },
        { "ro", MS_RDONLY },
        { "silent", MS_SILENT },
        { "strictatime", MS_STRICTATIME },
        { "sync", MS_SYNCHRONOUS },
    };

    const static std::map<std::string, unsigned long> unset_flags_map{
        { "async", MS_SYNCHRONOUS },
        { "atime", MS_NOATIME },
        { "dev", MS_NODEV },
        { "diratime", MS_NODIRATIME },
        { "exec", MS_NOEXEC },
        { "loud", MS_SILENT },
        { "noiversion", MS_I_VERSION },
        { "nolazytime", MS_LAZYTIME },
        { "nomand", MS_MANDLOCK },
        { "norelatime", MS_RELATIME },
        { "nostrictatime", MS_STRICTATIME },
        { "rw", MS_RDONLY },
        { "suid", MS_NOSUID },
        { "symfollow", MS_NOSYMFOLLOW },
    };

    unsigned long flags = 0;
    unsigned long propagation_flags = 0;
    std::stringstream data;

    for (const auto &opt : options) {
        if (auto it = flags_map.find(opt); it != flags_map.end()) {
            flags |= it->second;
            continue;
        }
        if (auto it = unset_flags_map.find(opt); it != unset_flags_map.end()) {
            flags &= ~it->second;
            continue;
        }
        if (auto it = propagation_flags_map.find(opt); it != propagation_flags_map.end()) {
            propagation_flags &= it->second;
            continue;
        }
        data << "," << opt;
    }
    auto str = data.str();
    if (!str.empty()) {
        str = str.substr(1);
    }

    return { flags, propagation_flags, str };
}

const auto ptr = ""_json_pointer;

static linyaps_box::config parse_1_2_0(const nlohmann::json &j)
{
    auto semver = linyaps_box::utils::semver(j[ptr / "ociVersion"].get<std::string>());
    if (!linyaps_box::utils::semver("1.2.0").is_compatible_with(semver)) {
        throw std::runtime_error("unsupported OCI version: " + semver.to_string());
    }

    linyaps_box::config cfg;

    {
        if (j.contains(ptr / "process" / "terminal")) {
            cfg.process.terminal = j[ptr / "process" / "terminal"].get<bool>();
        }

        // https://github.com/opencontainers/runtime-spec/blob/09fcb39bb7185b46dfb206bc8f3fea914c674779/config.md?plain=1#L245
        if (cfg.process.terminal && j.contains(ptr / "process" / "consoleSize")) {
            cfg.process.console.height = j[ptr / "process" / "consoleSize" / "height"].get<uint>();
            cfg.process.console.width = j[ptr / "process" / "consoleSize" / "width"].get<uint>();
        }

        cfg.process.cwd = j[ptr / "process" / "cwd"].get<std::string>();

        if (j.contains(ptr / "process" / "env")) {
            auto env = j[ptr / "process" / "env"].get<std::vector<std::string>>();
            for (const auto &e : env) {
                auto pos = e.find('=');
                if (pos == std::string::npos) {
                    throw std::runtime_error("invalid env entry: " + e);
                }
                cfg.process.env[e.substr(0, pos)] = e.substr(pos + 1);
            }
        }

        cfg.process.args = j[ptr / "process" / "args"].get<std::vector<std::string>>();

        // TODO: rlimits

        // TODO: apparmorProfile

        if (j.contains(ptr / "process" / "noNewPrivileges")) {
            cfg.process.no_new_privileges = j[ptr / "process" / "noNewPrivileges"].get<bool>();
        }

        if (j.contains(ptr / "process" / "oomScoreAdj")) {
            cfg.process.oom_score_adj = j[ptr / "process" / "oomScoreAdj"].get<int>();
        }

        cfg.process.uid = j[ptr / "process" / "user" / "uid"].get<uid_t>();
        cfg.process.gid = j[ptr / "process" / "user" / "gid"].get<gid_t>();
        if (j.contains(ptr / "process" / "user" / "umask")) {
            cfg.process.umask = j[ptr / "process" / "user" / "umask"].get<mode_t>();
        }
        if (j.contains(ptr / "process" / "user" / "additionalGids")) {
            cfg.process.additional_gids =
                    j[ptr / "process" / "user" / "additionalGids"].get<std::vector<gid_t>>();
        }
    }

    if (j.contains(ptr / "linux" / "namespaces")) {
        for (const auto &ns : j[ptr / "linux" / "namespaces"]) {
            linyaps_box::config::namespace_t n;
            if (!ns.contains("type")) {
                throw std::runtime_error("property `type` is REQUIRED for linux namespaces");
            }

            auto type = ns["type"].get<std::string>();
            if (type == "pid") {
                n.type = linyaps_box::config::namespace_t::type_t::PID;
            } else if (type == "network") {
                n.type = linyaps_box::config::namespace_t::type_t::NET;
            } else if (type == "ipc") {
                n.type = linyaps_box::config::namespace_t::type_t::IPC;
            } else if (type == "uts") {
                n.type = linyaps_box::config::namespace_t::type_t::UTS;
            } else if (type == "mount") {
                n.type = linyaps_box::config::namespace_t::type_t::MOUNT;
            } else if (type == "user") {
                n.type = linyaps_box::config::namespace_t::type_t::USER;
            } else if (type == "cgroup") {
                n.type = linyaps_box::config::namespace_t::type_t::CGROUP;
            } else {
                throw std::runtime_error("unsupported namespace type: " + type);
            }

            if (ns.contains("path")) {
                n.path = ns["path"].get<std::string>();
            }

            cfg.namespaces.push_back(n);
        }
    } else {
        LINYAPS_BOX_WARNING() << "No namespaces found";
    }

    if (j.contains(ptr / "linux" / "uidMappings")) {
        std::vector<linyaps_box::config::id_mapping_t> uid_mappings;
        for (const auto &m : j[ptr / "linux" / "uidMappings"]) {
            linyaps_box::config::id_mapping_t id_mapping;
            id_mapping.host_id = m["hostID"].get<uid_t>();
            id_mapping.container_id = m["containerID"].get<uid_t>();
            id_mapping.size = m["size"].get<size_t>();
            uid_mappings.push_back(id_mapping);
        }
        cfg.uid_mappings = uid_mappings;
    } else {
        LINYAPS_BOX_WARNING() << "No uidMappings found";
    }

    if (j.contains(ptr / "linux" / "gidMappings")) {
        std::vector<linyaps_box::config::id_mapping_t> gid_mappings;
        for (const auto &m : j[ptr / "linux" / "gidMappings"]) {
            linyaps_box::config::id_mapping_t id_mapping;
            id_mapping.host_id = m["hostID"].get<uid_t>();
            id_mapping.container_id = m["containerID"].get<uid_t>();
            id_mapping.size = m["size"].get<size_t>();
            gid_mappings.push_back(id_mapping);
        }
        cfg.gid_mappings = gid_mappings;
    } else {
        LINYAPS_BOX_WARNING() << "No gidMappings found";
    }

    if (j.contains(ptr / "hooks")) {
        auto hooks = j[ptr / "hooks"];
        auto get_hooks =
                [&](const std::string &key) -> std::vector<linyaps_box::config::hooks_t::hook_t> {
            std::vector<linyaps_box::config::hooks_t::hook_t> result;
            if (!hooks.contains(key)) {
                return result;
            }

            for (const auto &h : hooks[key]) {
                linyaps_box::config::hooks_t::hook_t hook;
                hook.path = h["path"].get<std::string>();
                hook.args = h["args"].get<std::vector<std::string>>();
                if (h.contains("env")) {
                    for (const auto &e : h["env"].get<std::vector<std::string>>()) {
                        auto pos = e.find('=');
                        if (pos == std::string::npos) {
                            throw std::runtime_error("invalid env entry: " + e);
                        }
                        hook.env[e.substr(0, pos)] = e.substr(pos + 1);
                    }
                }
                if (h.contains("timeout")) {
                    hook.timeout = h["timeout"].get<int>();
                }
                result.push_back(hook);
            }

            return result;
        };

        cfg.hooks.prestart = get_hooks("prestart");
        cfg.hooks.create_runtime = get_hooks("createRuntime");
        cfg.hooks.create_container = get_hooks("createContainer");
        cfg.hooks.start_container = get_hooks("startContainer");
        cfg.hooks.poststart = get_hooks("poststart");
        cfg.hooks.poststop = get_hooks("poststop");
    }

    if (j.contains(ptr / "mounts")) {
        std::vector<linyaps_box::config::mount_t> mounts;
        for (const auto &m : j[ptr / "mounts"]) {
            linyaps_box::config::mount_t mount;
            if (m.contains("source")) {
                mount.source = m["source"].get<std::string>();
            }
            if (m.contains("destination")) {
                mount.destination = m["destination"].get<std::string>();
            }
            mount.type = m["type"].get<std::string>();

            const auto it = m.find("options");
            if (it != m.end()) {
                auto options = it->get<std::vector<std::string>>();
                std::tie(mount.flags, mount.propagation_flags, mount.data) =
                        parse_mount_options(options);
            }

            mounts.push_back(mount);
        }
        cfg.mounts = mounts;
    }

    cfg.root.path = j[ptr / "root" / "path"].get<std::string>();

    if (j.contains(ptr / "root" / "readonly")) {
        cfg.root.readonly = j[ptr / "root" / "readonly"].get<bool>();
    }

    return cfg;
}

} // namespace

linyaps_box::config linyaps_box::config::parse(std::istream &is)
{
    auto j = nlohmann::json::parse(is);
    return parse_1_2_0(j);
}

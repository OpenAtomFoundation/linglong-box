#include "linyaps_box/container_ref.h"

#include <signal.h>

linyaps_box::container_ref::container_ref(std::shared_ptr<status_directory> status_dir,
                                          const std::string &id)
    : status_dir_(status_dir)
    , id_(id)
{
}

linyaps_box::container_status_t linyaps_box::container_ref::status() const
{
    return this->status_dir_->read(this->id_);
}

void linyaps_box::container_ref::kill(int signal)
{
    auto pid = this->status().PID;

    if (!::kill(pid, signal)) {
        return;
    }

    throw std::system_error(errno,
                            std::generic_category(),
                            (std::stringstream() << "kill " << signal << " " << pid).str());
}

void linyaps_box::container_ref::exec(const linyaps_box::config::process_t &process)
{
    auto target = std::to_string(this->status().PID);
    auto wd = process.cwd.string();

    std::vector<const char *> argv{
        "nsenter",
        "--target",
        target.c_str(),
        "--user",
        "--mount",
        "--pid",

        // FIXME:
        // Old nsenter command do not support --wdns,
        // so we have to implement nsenter by ourself in the future.
        "--wdns",
        wd.c_str(),

        "--preserve-credentials",
        "--",
    };

    for (const auto &arg : process.args) {
        argv.push_back(arg.c_str());
    }
    argv.push_back(nullptr);

    // FIXME:
    // We only handle the command arguments for now
    // here are some other fields in process we need to consider:
    // terminal
    // console.height
    // console.width
    // cwd
    // env
    // rlimits
    // apparmor_profile
    // capabilities
    // no_new_privileges
    // oom_score_adj

    ::execvp("nsenter", const_cast<char **>(argv.data()));

    std::stringstream ss;

    ss << "execvp nsenter with arguments:";
    for (const auto &arg : argv) {
        ss << " " << arg;
    }

    throw std::system_error(errno, std::generic_category(), std::move(ss).str());
}

linyaps_box::status_directory &linyaps_box::container_ref::status_dir() const
{
    return *this->status_dir_;
}

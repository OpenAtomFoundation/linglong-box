#include "linyaps_box/container.h"

#include "linyaps_box/utils/file_describer.h"
#include "linyaps_box/utils/fstat.h"
#include "linyaps_box/utils/mkdir.h"
#include "linyaps_box/utils/mknod.h"
#include "linyaps_box/utils/open_file.h"
#include "linyaps_box/utils/socketpair.h"
#include "linyaps_box/utils/touch.h"

#include <linux/magic.h>
#include <sys/mount.h>
#include <sys/statfs.h>
#include <sys/sysmacros.h>

#include <cassert>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>

#include <dirent.h>
#include <grp.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

enum class sync_message : uint8_t {
    REQUEST_CONFIGURE_USER_NAMESPACE,
    USER_NAMESPACE_CONFIGURED,
    PRESTART_HOOKS_EXECUTED,
    CREATE_RUNTIME_HOOKS_EXECUTED,
    CREATE_CONTAINER_HOOKS_EXECUTED,
};

std::stringstream &&operator<<(std::stringstream &&os, sync_message message)
{
    switch (message) {
    case sync_message::REQUEST_CONFIGURE_USER_NAMESPACE: {
        os << "REQUEST_CONFIGURE_USER_NAMESPACE";
    } break;
    case sync_message::USER_NAMESPACE_CONFIGURED: {
        os << "USER_NAMESPACE_CONFIGURED";
    } break;
    case sync_message::PRESTART_HOOKS_EXECUTED: {
        os << "PRESTART_HOOKS_EXECUTED";
    } break;
    case sync_message::CREATE_RUNTIME_HOOKS_EXECUTED: {
        os << "CREATE_RUNTIME_HOOKS_EXECUTED";
    } break;
    case sync_message::CREATE_CONTAINER_HOOKS_EXECUTED: {
        os << "CREATE_CONTAINER_HOOKS_EXECUTED";
    } break;
    default: {
        assert(false);
        os << "UNKNOWN " << (uint8_t)message;
    } break;
    }
    return std::move(os);
}

class unexpected_sync_message : public std::logic_error
{
public:
    unexpected_sync_message(sync_message excepted, sync_message actual)
        : std::logic_error((std::stringstream() << "unexpected sync message: expected " << excepted
                                                << " got " << actual)
                                   .str())
    {
    }
};

static void execute_hook(const linyaps_box::config::hooks_t::hook_t &hook)
{
    auto pid = fork();
    if (pid < 0) {
        throw std::system_error(errno, std::generic_category(), "fork");
    }

    if (pid == 0) {
        [&]() noexcept {
            std::vector<const char *> c_args;
            c_args.push_back(hook.path.c_str());
            for (const auto &arg : hook.args) {
                c_args.push_back(arg.c_str());
            }
            c_args.push_back(nullptr);

            std::vector<std::string> envs;
            for (const auto &env : hook.env) {
                envs.push_back(env.first + "=" + env.second);
            }

            std::vector<const char *> c_env;
            for (const auto &env : envs) {
                c_env.push_back(env.c_str());
            }

            execvpe(c_args[0],
                    const_cast<char *const *>(c_args.data()),
                    const_cast<char *const *>(c_env.data()));

            std::cerr << "execvp: " << strerror(errno) << " errno=" << errno << std::endl;
            exit(1);
        }();
    }

    int status = 0;

    pid_t ret = -1;
    while (ret == -1) {
        ret = waitpid(pid, &status, 0);
        if (ret != -1) {
            break;
        }
        if (errno == EINTR && errno == EAGAIN) {
            continue;
        }
        throw std::system_error(errno,
                                std::generic_category(),
                                (std::stringstream() << "waitpid " << pid).str());
    }

    if (WIFEXITED(status)) {
        return;
    }

    throw std::runtime_error((std::stringstream() << "hook terminated by signal" << WTERMSIG(status)
                                                  << " with " << WEXITSTATUS(status))
                                     .str());
}

struct clone_fn_args
{
    const linyaps_box::container *container;
    const linyaps_box::config::process_t *process;
    int socket;
};

// NOTE: All function in this namespace are running in the container namespace.
namespace container_ns {

static void configure_container_namespaces(linyaps_box::utils::file_descriptor &socket)
{
    socket << std::byte{ 0 }; // NOTE: request configure user_namespace
    std::byte byte;
    socket >> byte; // NOTE: user_namespace configured
}

struct delay_readonly_mount_t
{
    linyaps_box::utils::file_descriptor destination_fd;
    unsigned long flags;
};

static void do_delay_readonly_mount(const delay_readonly_mount_t &mount)
{
    assert(mount.destination_fd.get() != -1);
    assert(mount.flags & (MS_BIND | MS_REMOUNT | MS_RDONLY));

    int ret = ::mount(nullptr,
                      mount.destination_fd.proc_path().c_str(),
                      nullptr,
                      mount.flags,
                      nullptr);
    if (ret == 0) {
        return;
    }

    throw std::system_error(errno, std::generic_category(), "mount");
}

[[nodiscard]] static linyaps_box::utils::file_descriptor create_destination_file(
        const linyaps_box::utils::file_descriptor &root, const std::filesystem::path &destination)
{
    const auto &parent = linyaps_box::utils::mkdir(root, destination.parent_path());
    return linyaps_box::utils::touch(parent, destination.filename());
}

[[nodiscard]] static linyaps_box::utils::file_descriptor create_destination_directory(
        const linyaps_box::utils::file_descriptor &root, const std::filesystem::path &destination)
{
    return linyaps_box::utils::mkdir(root, destination);
}

template<bool file = false>
[[nodiscard]] linyaps_box::utils::file_descriptor ensure_mount_destination(
        const linyaps_box::utils::file_descriptor &root, const linyaps_box::config::mount_t &mount)
try {
    assert(mount.destination.has_value());
    auto open_flag = O_PATH;
    if (mount.flags | MS_NOSYMFOLLOW) {
        open_flag |= O_NOFOLLOW;
    }
    return linyaps_box::utils::open(root, mount.destination.value(), open_flag);
} catch (const std::system_error &e) {
    if (e.code().value() != ENOENT) {
        throw;
    }

    // NOTE: Automatically create destination is not a part of the OCI runtime
    // spec, as it requires implementation to follow the behivor of mount(8).
    // But both crun and runc does this.

    if constexpr (file) {
        return create_destination_file(root, mount.destination.value());
    } else {
        return create_destination_directory(root, mount.destination.value());
    }
}

static void do_propagation_mount(const linyaps_box::utils::file_descriptor &destination,
                                 const unsigned long &flags)
{
    assert(destination.get() != -1);

    if (!flags) {
        return;
    }

    int ret = ::mount(nullptr, destination.proc_path().c_str(), nullptr, flags, nullptr);
    if (ret == 0) {
        return;
    }

    throw std::system_error(errno, std::generic_category(), "mount");
}

static void do_bind_mount(const linyaps_box::utils::file_descriptor &root,
                          const linyaps_box::config::mount_t &mount)
{
    assert(mount.flags & MS_BIND);
    assert(mount.source.has_value());
    assert(mount.destination.has_value());

    auto open_flag = O_PATH;
    if (mount.flags & MS_NOSYMFOLLOW) {
        open_flag |= O_NOFOLLOW;
    }
    auto source_fd = linyaps_box::utils::open(mount.source.value(), open_flag);
    auto source_stat = linyaps_box::utils::lstat(source_fd);

    linyaps_box::utils::file_descriptor destination_fd;
    if (S_ISDIR(source_stat.st_mode)) {
        destination_fd = ensure_mount_destination(root, mount);
    } else {
        destination_fd = ensure_mount_destination<true>(root, mount);
    }

    auto bind_flags = mount.flags & (MS_BIND | MS_REC);

    auto ret = ::mount(source_fd.proc_path().c_str(),
                       destination_fd.proc_path().c_str(),
                       mount.type.c_str(),
                       bind_flags,
                       mount.data.c_str());
    if (ret < 0) {
        throw std::system_error(errno, std::generic_category(), "mount");
    }

    bind_flags = mount.flags | MS_REMOUNT;

    ret = ::mount(source_fd.proc_path().c_str(),
                  destination_fd.proc_path().c_str(),
                  mount.type.c_str(),
                  bind_flags,
                  mount.data.c_str());
    if (ret < 0) {
        throw std::system_error(errno, std::generic_category(), "mount");
    }

    if (!mount.propagation_flags) {
        return;
    }

    do_propagation_mount(destination_fd, mount.propagation_flags);
}

[[nodiscard]] static std::optional<delay_readonly_mount_t>
do_mount(const linyaps_box::utils::file_descriptor &root, const linyaps_box::config::mount_t &mount)
{
    if (mount.flags & MS_BIND) {
        do_bind_mount(root, mount);
        return std::nullopt;
    }

    // if ...

    linyaps_box::utils::file_descriptor destination_fd = ensure_mount_destination(root, mount);
    assert(!mount.source.has_value());

    auto mount_flags = mount.flags;

    std::optional<delay_readonly_mount_t> delay_readonly_mount;
    // NOTE: runc
    if (mount.type == "tmpfs" && mount.flags & MS_RDONLY) {
        delay_readonly_mount =
                delay_readonly_mount_t{ std::move(destination_fd),
                                        mount_flags | MS_RDONLY | MS_REMOUNT | MS_BIND };
        mount_flags &= ~MS_RDONLY;
    }

    int ret = ::mount(mount.source.value().c_str() ? mount.source.value().c_str() : nullptr,
                      destination_fd.proc_path().c_str(),
                      mount.type.c_str(),
                      mount_flags,
                      mount.data.c_str());
    if (ret < 0) {
        throw std::system_error(errno, std::generic_category(), "mount");
    }

    do_propagation_mount(destination_fd, mount.propagation_flags);

    return delay_readonly_mount;
}

bool directory_is_empty(const std::filesystem::path &path)
{
    DIR *dir = opendir(path.c_str());
    if (dir == NULL) {
        throw std::system_error(errno, std::generic_category(), "opendir");
    }

    class defer_close
    {
    public:
        DIR *dir;

        ~defer_close()
        {
            if (dir == nullptr) {
                return;
            }
            closedir(dir);
        }
    } _{ dir };

    int n = 0;
    while (readdir(dir)) {
        if (++n > 2)
            break;
    }

    if (n <= 2)
        return true;
    return false;
}

class mounter
{
public:
    mounter(linyaps_box::utils::file_descriptor root)
        : root(std::move(root))
    {
    }

    void mount(const linyaps_box::config::mount_t &mount)
    {
        auto delay_mount = do_mount(root, mount);
        if (!delay_mount.has_value()) {
            return;
        }

        remounts.push_back(std::move(delay_mount).value());
    }

    void finalize()
    {
        this->configure_default_filesystems();
        this->configure_default_devices();
        for (const auto &remount : remounts) {
            do_delay_readonly_mount(remount);
        }
    }

private:
    linyaps_box::utils::file_descriptor root;
    std::vector<delay_readonly_mount_t> remounts;

    // https://github.com/opencontainers/runtime-spec/blob/09fcb39bb7185b46dfb206bc8f3fea914c674779/config-linux.md#default-filesystems
    void configure_default_filesystems()
    {
        do {
            auto proc = linyaps_box::utils::open(root, "proc");
            struct statfs buf;
            int ret = ::statfs(proc.proc_path().c_str(), &buf);
            if (ret) {
                throw std::system_error(errno, std::generic_category(), "statfs");
            }

            if (buf.f_type == PROC_SUPER_MAGIC) {
                break;
            }

            linyaps_box::config::mount_t mount;
            mount.source = "proc";
            mount.type = "proc";
            mount.destination = "/proc";
            this->mount(mount);
        } while (0);

        do {
            auto sys = linyaps_box::utils::open(root, "sys");
            struct statfs buf;
            int ret = ::statfs(sys.proc_path().c_str(), &buf);
            if (ret) {
                throw std::system_error(errno, std::generic_category(), "statfs");
            }

            if (buf.f_type == SYSFS_MAGIC) {
                break;
            }

            linyaps_box::config::mount_t mount;
            mount.source = "sysfs";
            mount.type = "sysfs";
            mount.destination = "/sys";
            mount.flags = MS_NOSUID | MS_NOEXEC | MS_NODEV;
            try {
                this->mount(mount);
            } catch (const std::system_error &e) {
                if (e.code().value() != EPERM) {
                    throw;
                }

                // NOTE: fallback to bind mount
                mount.source = "/sys";
                mount.type = "bind";
                mount.destination = "/sys";
                mount.flags = MS_BIND | MS_REC | MS_NOSUID | MS_NOEXEC | MS_NODEV;
                this->mount(mount);
            }
        } while (0);

        do {
            auto dev = linyaps_box::utils::open(root, "dev");
            struct statfs buf;
            int ret = ::statfs(dev.proc_path().c_str(), &buf);
            if (ret) {
                throw std::system_error(errno, std::generic_category(), "statfs");
            }

            if (buf.f_type == TMPFS_MAGIC) {
                break;
            }

            if (!directory_is_empty(dev.proc_path())) {
                break;
            }

            linyaps_box::config::mount_t mount;
            mount.source = "tmpfs";
            mount.destination = "/dev";
            mount.type = "tmpfs";
            mount.flags = MS_NOSUID | MS_STRICTATIME | MS_NODEV;
            mount.data = "mode=755,size=65536k";
            this->mount(mount);
        } while (0);

        do {
            try {
                auto pts = linyaps_box::utils::open(root, "dev/pts");
                break;
            } catch (const std::system_error &e) {
                if (e.code().value() != ENOENT) {
                    throw;
                }
            }

            linyaps_box::config::mount_t mount;
            mount.source = "devpts";
            mount.destination = "/dev/pts";
            mount.type = "devpts";
            mount.flags = MS_NOSUID | MS_NOEXEC;
            mount.data = "newinstance,ptmxmode=0666,mode=0620";
            this->mount(mount);
        } while (0);

        do {
            try {
                auto shm = linyaps_box::utils::open(root, "dev/shm");
                break;
            } catch (const std::system_error &e) {
                if (e.code().value() != ENOENT) {
                    throw;
                }
            }

            linyaps_box::config::mount_t mount;
            mount.source = "shm";
            mount.destination = "/dev/shm";
            mount.type = "tmpfs";
            mount.flags = MS_NOSUID | MS_NOEXEC | MS_NODEV;
            mount.data = "mode=1777,size=65536k";
            this->mount(mount);
        } while (0);
    }

    void configure_deivce(const std::filesystem::path &destination, mode_t mode, dev_t dev)
    {
        assert(destination.is_absolute());

        std::optional<linyaps_box::utils::file_descriptor> destination_fd;
        try {
            destination_fd = linyaps_box::utils::open(root, destination.relative_path(), O_PATH);
        } catch (const std::system_error &e) {
            if (e.code().value() != ENOENT) {
                throw;
            }
        }

        if (!destination_fd.has_value())
            try {
                linyaps_box::utils::mknod(root, destination.relative_path(), mode, dev);
            } catch (const std::system_error &e) {
                if (e.code().value() != EPERM) {
                    throw;
                }
            }

        auto stat = linyaps_box::utils::lstat(*destination_fd);
        if (S_ISCHR(stat.st_mode) && major(stat.st_dev) == 1 && minor(stat.st_dev) == 3) {
            return;
        }

        // NOTE: fallback to bind mount host device into container

        linyaps_box::config::mount_t mount;
        mount.source = destination;
        mount.destination = destination;
        mount.type = "bind";
        mount.flags = MS_BIND | MS_REC | MS_NOSUID | MS_NOEXEC | MS_NODEV;
        this->mount(mount);
    }

    void configure_default_devices()
    {
        this->configure_deivce("/dev/null", 0666, makedev(1, 3));
        this->configure_deivce("/dev/zero", 0666, makedev(1, 5));
        this->configure_deivce("/dev/full", 0666, makedev(1, 7));
        this->configure_deivce("/dev/random", 0666, makedev(1, 8));
        this->configure_deivce("/dev/urandom", 0666, makedev(1, 9));
        this->configure_deivce("/dev/tty", 0666, makedev(5, 0));

        // TODO Handle `/dev/console`;

        // TODO Handle `/dev/ptmx`;
    }
};

static void configure_mounts(const linyaps_box::container &container)
{
    if (container.get_config().mounts.empty()) {
        return;
    }

    std::unique_ptr<mounter> m;

    {
        auto bundle = linyaps_box::utils::open(container.get_bundle(), O_PATH);
        m = std::make_unique<mounter>(
                linyaps_box::utils::open(bundle, container.get_config().root.path, O_PATH));
    }

    for (const auto &mount : container.get_config().mounts) {
        m->mount(mount);
    }

    m->finalize();
}

[[noreturn]] static void execute_process(const linyaps_box::config::process_t &process)
{
    std::vector<const char *> c_args;
    for (const auto &arg : process.args) {
        c_args.push_back(arg.c_str());
    }
    c_args.push_back(nullptr);

    std::vector<std::string> envs;
    for (const auto &env : process.env) {
        envs.push_back(env.first + "=" + env.second);
    }
    std::vector<const char *> c_env;
    for (const auto &env : envs) {
        c_env.push_back(env.c_str());
    }

    auto ret = chdir(process.cwd.c_str());
    if (ret) {
        throw std::system_error(errno, std::generic_category(), "chdir");
    }

    ret = setgid(process.uid);
    if (ret) {
        throw std::system_error(errno, std::generic_category(), "setgid");
    }

    if (process.additional_gids) {
        ret = setgroups(process.additional_gids->size(), process.additional_gids->data());
        if (ret) {
            throw std::system_error(errno, std::generic_category(), "setgroups");
        }
    }

    ret = setuid(process.uid);
    if (ret) {
        throw std::system_error(errno, std::generic_category(), "setuid");
    }

    execvpe(c_args[0],
            const_cast<char *const *>(c_args.data()),
            const_cast<char *const *>(c_env.data()));

    throw std::system_error(errno, std::generic_category(), "execvpe");
}

static void prestart_hooks(const linyaps_box::container &container,
                           linyaps_box::utils::file_descriptor &socket)
{

    if (container.get_config().hooks.prestart.empty()) {
        return;
    }

    for (const auto &hook : container.get_config().hooks.prestart) {
        execute_hook(hook);
    }

    socket << std::byte{ 0 };
}

static void wait_create_runtime_result(const linyaps_box::container &container,
                                       linyaps_box::utils::file_descriptor &socket)
{
    if (container.get_config().hooks.create_runtime.empty()) {
        return;
    }

    std::byte byte;
    socket >> byte;
}

static void create_container_hooks(const linyaps_box::container &container,
                                   linyaps_box::utils::file_descriptor &socket)
{
    if (container.get_config().hooks.create_container.empty()) {
        return;
    }

    for (const auto &hook : container.get_config().hooks.create_container) {
        execute_hook(hook);
    }

    socket << std::byte{ 0 };
}

static void start_container_hooks(const linyaps_box::container &container,
                                  linyaps_box::utils::file_descriptor &socket)
{
    if (container.get_config().hooks.start_container.empty()) {
        return;
    }

    for (const auto &hook : container.get_config().hooks.start_container) {
        execute_hook(hook);
    }

    socket << std::byte{ 0 };
}

static void close_other_fds(const std::multiset<int> &except_fds)
{
    auto tmp = except_fds;
    tmp.insert(0);
    tmp.insert(~0U);
    for (auto fd = tmp.begin(); std::next(fd) != tmp.end(); fd++) {
        auto low = *fd;
        auto high = *std::next(fd) - 1;
        if (low >= high) {
            continue;
        }
        auto ret = close_range(low, high, 0);
        if (ret) {
            throw std::system_error(errno, std::generic_category(), "close_range");
        }
    }
}

static int clone_fn(void *data) noexcept
try {
    auto args = *static_cast<clone_fn_args *>(data);

    close_other_fds({ STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO, args.socket });

    auto &container = *args.container;
    auto &process = *args.process;
    auto socket = linyaps_box::utils::file_descriptor(args.socket);

    configure_container_namespaces(socket);
    configure_mounts(container);
    prestart_hooks(container, socket);
    wait_create_runtime_result(container, socket);
    create_container_hooks(container, socket);
    start_container_hooks(container, socket);
    execute_process(process);
} catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return -1;
} catch (...) {
    std::cerr << "unknown error" << std::endl;
    return -1;
}

} // namespace container_ns

// NOTE: All function in this namespace are running in the runtime namespace.
namespace runtime_ns {

[[nodiscard]] static int
generate_clone_flag(std::vector<linyaps_box::config::namespace_t> namespaces)
{
    int flag = SIGCHLD;
    int setted_namespaces = 0;

    for (const auto &ns : namespaces) {
        switch (ns.type) {
        case linyaps_box::config::namespace_t::IPC: {
            flag |= CLONE_NEWIPC;
        } break;
        case linyaps_box::config::namespace_t::UTS: {
            flag |= CLONE_NEWUTS;
        } break;
        case linyaps_box::config::namespace_t::MOUNT: {
            flag |= CLONE_NEWNS;
        } break;
        case linyaps_box::config::namespace_t::PID: {
            flag |= CLONE_NEWPID;
        } break;
        case linyaps_box::config::namespace_t::NET: {
            flag |= CLONE_NEWNET;
        } break;
        case linyaps_box::config::namespace_t::USER: {
            flag |= CLONE_NEWUSER;
        } break;
        case linyaps_box::config::namespace_t::CGROUP: {
            flag |= CLONE_NEWCGROUP;
        } break;
        default: {
            throw std::invalid_argument("invalid namespace");
        }
        }

        if (setted_namespaces & ns.type) {
            throw std::invalid_argument("duplicate namespace");
        }
        setted_namespaces |= ns.type;
    }

    return flag;
}

class child_stack
{
public:
    child_stack()
    {
        this->stack_low = mmap(nullptr,
                               LINYAPS_BOX_CLONE_CHILD_STACK_SIZE,
                               PROT_READ | PROT_WRITE,
                               MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK,
                               -1,
                               0);
        if (this->stack_low == MAP_FAILED) {
            throw std::runtime_error("mmap child stack failed");
        }
    }

    ~child_stack() noexcept
    {
        if (this->stack_low == MAP_FAILED) {
            return;
        }

        if (!munmap(this->stack_low, LINYAPS_BOX_CLONE_CHILD_STACK_SIZE)) {
            return;
        }

        const auto code = errno;
        std::cerr << "munmap: " << strerror(code) << std::endl;
        assert(false);
    }

    [[nodiscard]] void *top() const noexcept
    {
        if constexpr (LINYAPS_BOX_STACK_GROWTH_DOWN) {
            return (char *)this->stack_low + LINYAPS_BOX_CLONE_CHILD_STACK_SIZE;
        } else {
            return (char *)this->stack_low - LINYAPS_BOX_CLONE_CHILD_STACK_SIZE;
        }
    }

private:
    void *stack_low;
};

static std::tuple<int, linyaps_box::utils::file_descriptor> start_container_process(
        const linyaps_box::container &container, const linyaps_box::config::process_t &process)
{
    auto sockets = linyaps_box::utils::socketpair(AF_UNIX, SOCK_SEQPACKET, 0);
    int clone_flag = runtime_ns::generate_clone_flag(container.get_config().namespaces);

    clone_fn_args args = { &container, &process, std::move(sockets.second).release() };

    child_stack stack;
    int child_pid = clone(container_ns::clone_fn, stack.top(), clone_flag, (void *)&args);
    if (child_pid < 0) {
        throw std::runtime_error("clone failed");
    } else if (child_pid == 0) {
        throw std::logic_error("clone should not return in child");
    }

    return { child_pid, std::move(sockets.second) };
}

static void execute_user_namespace_helper(const std::vector<std::string> &args)
{
    auto pid = fork();
    if (pid < 0) {
        throw std::runtime_error("fork failed");
    }

    if (pid == 0) {
        std::vector<const char *> c_args;

        for (const auto &arg : args) {
            c_args.push_back(arg.c_str());
        }

        c_args.push_back(nullptr);
        execvp(c_args[0], const_cast<char *const *>(c_args.data()));

        throw std::system_error(errno, std::generic_category(), "execvp");
    }

    int status = 0;
    auto ret = -1;
    while (ret == -1) {
        ret = waitpid(pid, &status, 0);

        if (ret != -1) {
            break;
        }

        if (errno == EINTR || errno == EAGAIN) {
            continue;
        }

        throw std::system_error(errno, std::generic_category(), "waitpid");
    }

    if (WIFEXITED(status)) {
        return;
    }

    throw std::runtime_error("user_namespace helper exited abnormally");
}

static void
configure_gid_mapping(pid_t pid, const std::vector<linyaps_box::config::id_mapping_t> &gid_mappings)
{
    if (gid_mappings.size() == 0) {
        return;
    }

    std::vector<std::string> args;
    args.push_back("newgidmap");
    args.push_back(std::to_string(pid));
    for (const auto &mapping : gid_mappings) {
        args.push_back(std::to_string(mapping.host_id));
        args.push_back(std::to_string(mapping.container_id));
        args.push_back(std::to_string(mapping.size));
    }

    execute_user_namespace_helper(args);
}

static void
configure_uid_mapping(pid_t pid, const std::vector<linyaps_box::config::id_mapping_t> &uid_mappings)
{
    if (uid_mappings.size() == 0) {
        return;
    }

    std::vector<std::string> args;
    args.push_back("newuidmap");
    args.push_back(std::to_string(pid));
    for (const auto &mapping : uid_mappings) {
        args.push_back(std::to_string(mapping.host_id));
        args.push_back(std::to_string(mapping.container_id));
        args.push_back(std::to_string(mapping.size));
    }

    execute_user_namespace_helper(args);
}

static void configure_container_cgroup(const linyaps_box::container &)
{
    // TODO
}

static void configure_container_namespaces(const linyaps_box::container &container,
                                           linyaps_box::utils::file_descriptor &socket)
{
    std::byte byte;
    socket >> byte;
    {
        auto message = sync_message(byte);
        if (message != sync_message::REQUEST_CONFIGURE_USER_NAMESPACE) {
            throw unexpected_sync_message(sync_message::REQUEST_CONFIGURE_USER_NAMESPACE, message);
        }
    }

    const auto &config = container.get_config();

    if (std::find_if(config.namespaces.cbegin(),
                     config.namespaces.cend(),
                     [](const linyaps_box::config::namespace_t &ns) -> bool {
                         return ns.type == linyaps_box::config::namespace_t::USER;
                     })
        != config.namespaces.end()) {

        auto pid = container.status().PID;

        configure_gid_mapping(pid, config.gid_mappings);
        configure_uid_mapping(pid, config.uid_mappings);
    }

    configure_container_cgroup(container);

    byte = std::byte(sync_message::USER_NAMESPACE_CONFIGURED);
    socket << byte;
}

static void wait_prestart_result(const linyaps_box::container &container,
                                 linyaps_box::utils::file_descriptor &socket)
{
    if (container.get_config().hooks.prestart.empty()) {
        return;
    }

    std::byte byte;
    socket >> byte;
    auto message = sync_message(byte);
    if (message == sync_message::PRESTART_HOOKS_EXECUTED) {
        return;
    }
    throw unexpected_sync_message(sync_message::PRESTART_HOOKS_EXECUTED, message);
}

static void create_runtime_hooks(const linyaps_box::container &container,
                                 linyaps_box::utils::file_descriptor &socket)
{
    if (container.get_config().hooks.create_runtime.empty()) {
        return;
    }

    for (const auto &hook : container.get_config().hooks.create_runtime) {
        execute_hook(hook);
    }

    socket << std::byte(sync_message::CREATE_RUNTIME_HOOKS_EXECUTED);
}

static void wait_create_container_result(const linyaps_box::container &container,
                                         linyaps_box::utils::file_descriptor &socket)
{
    if (container.get_config().hooks.create_container.empty()) {
        return;
    }

    std::byte byte;
    socket >> byte;
    auto message = sync_message(byte);
    if (message == sync_message::CREATE_CONTAINER_HOOKS_EXECUTED) {
        return;
    }
    throw unexpected_sync_message(sync_message::CREATE_CONTAINER_HOOKS_EXECUTED, message);
}

static void wait_socket_close(linyaps_box::utils::file_descriptor &socket)
try {
    std::byte byte;
    socket >> byte;
} catch (const linyaps_box::utils::file_descriptor_closed_exception &e) {
    return;
}

static void poststart_hooks(const linyaps_box::container &container)
{
    if (container.get_config().hooks.poststart.empty()) {
        return;
    }

    for (const auto &hook : container.get_config().hooks.poststart) {
        execute_hook(hook);
    }
}

static void poststop_hooks(const linyaps_box::container &container) noexcept
{
    if (container.get_config().hooks.poststop.empty()) {
        return;
    }

    for (const auto &hook : container.get_config().hooks.poststart)
        try {
            execute_hook(hook);
        } catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
}

[[nodiscard]] static int wait_container_process(pid_t pid)
{
    int status = 0;

    pid_t ret = -1;
    while (ret == -1) {
        ret = waitpid(pid, &status, 0);

        if (ret != -1) {
            break;
        }

        if (errno == EINTR || errno == EAGAIN) {
            continue;
        }

        throw std::system_error(errno, std::generic_category(), "waitpid");
    }
    return WEXITSTATUS(status);
}

} // namespace runtime_ns

} // namespace

linyaps_box::container::container(std::shared_ptr<status_directory> status_dir,
                                  const std::string &id,
                                  const std::filesystem::path &bundle,
                                  const std::filesystem::path &config)
    : container_ref(std::move(status_dir), id)
    , bundle(bundle)
{
    std::ifstream ifs(config);
    this->config = linyaps_box::config::parse(ifs);

    {
        auto status = this->status();
        status.status = container_status_t::runtime_status::CREATING;
        this->status_dir().write(status);
    }
}

const linyaps_box::config &linyaps_box::container::get_config() const
{
    return this->config;
}

const std::filesystem::path &linyaps_box::container::get_bundle() const
{
    return this->bundle;
}

int linyaps_box::container::run(const config::process_t &process)
{
    auto [child_pid, socket] = runtime_ns::start_container_process(*this, process);

    {
        auto status = this->status();
        assert(status.status == container_status_t::runtime_status::CREATING);
        status.PID = child_pid;
        status.status = container_status_t::runtime_status::CREATED;
        this->status_dir().write(status);
    }

    runtime_ns::configure_container_namespaces(*this, socket);
    runtime_ns::wait_prestart_result(*this, socket);
    runtime_ns::create_runtime_hooks(*this, socket);
    runtime_ns::wait_create_container_result(*this, socket);
    runtime_ns::wait_socket_close(socket);
    runtime_ns::poststart_hooks(*this);
    auto container_process_exit_code = runtime_ns::wait_container_process(this->status().PID);

    {
        auto status = this->status();
        assert(status.status == container_status_t::runtime_status::STOPPED);
        status.PID = child_pid;
        this->status_dir().write(status);
    }

    runtime_ns::poststop_hooks(*this);

    return container_process_exit_code;
}

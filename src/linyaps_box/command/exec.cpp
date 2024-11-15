#include "linyaps_box/command/exec.h"

#include "linyaps_box/impl/status_directory.h"
#include "linyaps_box/runtime.h"
#include "linyaps_box/status_directory.h"

void linyaps_box::command::exec(const std::filesystem::path &root, const struct exec_options &opts)
{
    std::unique_ptr<status_directory> dir;
    dir = std::make_unique<impl::status_directory>(root);

    runtime_t runtime(std::move(dir));

    auto container_refs = runtime.containers();
    auto container = container_refs.find(opts.ID);
    if (container == container_refs.end()) {
        throw std::runtime_error("container not found");
    }

    config::process_t proc;
    proc.cwd = opts.cwd;
    proc.args = opts.command;
    // TODO

    container->second.exec(proc);
}

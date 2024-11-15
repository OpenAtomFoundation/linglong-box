#include "linyaps_box/command/options.h"

#include "CLI/CLI.hpp"

#include <csignal>

#include <unistd.h>

linyaps_box::command::options linyaps_box::command::parse(int argc, char *argv[]) noexcept
{
    auto app = std::make_unique<CLI::App>();

    app->name("ll-box");
    app->description("A simple OCI runtime implementation "
                     "focused on desktop applications.");

    linyaps_box::command::options options;

    auto default_root = std::filesystem::current_path().root_path() / "run" / "user"
            / std::to_string(geteuid()) / "linglong" / "box";

    if (getenv("XDG_RUNTIME_DIR")) {
        default_root = std::filesystem::path{ getenv("XDG_RUNTIME_DIR") } / "linglong" / "box";
    }

    app->add_option("--root", options.root, "Root directory for storage of container state")
            ->default_val(default_root);

    app->require_subcommand();

    auto cmd_list = app->add_subcommand("list", "List know containers");

    cmd_list->add_option("-f,--format", options.list.output_format, "Specify the output format")
            ->default_val(list_options::output_format_t::table)
            ->default_str("table")
            ->type_name("FORMAT")
            ->transform(
                    CLI::CheckedTransformer(std::map<std::string, list_options::output_format_t>{
                            { "table", list_options::output_format_t::table },
                            { "json", list_options::output_format_t::json },
                    }));

    auto cmd_run = app->add_subcommand("run", "Create and immediately start a container");

    cmd_run->add_option("CONTAINER", options.run.ID, "The container ID")->required();

    cmd_run->add_option("-b,--bundle", options.run.bundle, "Path to the OCI bundle")
            ->default_val(".");

    cmd_run->add_option("-f,--config", options.run.config, "Override the configuration file to use")
            ->default_val("config.json");

    auto cmd_exec = app->add_subcommand("exec", "Exec a command in a running container");

    cmd_exec->add_option("-u,--user",
                         options.exec.user,
                         "Specify the user, "
                         "for example `1000` for UID=1000 "
                         "or `1000:1000` for UID=1000 and GID=1000")
            ->type_name("UID[:GID]");
    cmd_exec->add_option("--cwd", options.exec.cwd, "Current working directory.");
    cmd_exec->add_option("CONTAINER", options.exec.ID, "Container ID")->required();
    cmd_exec->add_option("COMMAND", options.exec.command, "Command to execute")->required();

    auto cmd_kill =
            app->add_subcommand("kill", "Send the specified signal to the container init process");

    cmd_kill->add_option("--signal", options.kill.signal)
            ->type_name("SIGNAL")
            ->default_val(SIGTERM)
            ->default_str("TERM")
            ->transform(CLI::CheckedTransformer(std::map<std::string, int>{
                    { "TERM", SIGTERM },
                    { "STOP", SIGSTOP },
            }));

    cmd_kill->add_option("CONTAINER", options.kill.container, "The container ID")->required();

    // argv = app->ensure_utf8(argv);

    try {
        app->parse(argc, argv);
    } catch (const CLI::ParseError &e) {
        options.return_code = app->exit(e);
    }

    if (cmd_list->parsed()) {
        options.command = options::command_t::list;
    } else if (cmd_run->parsed()) {
        options.command = options::command_t::run;
    } else if (cmd_exec->parsed()) {
        options.command = options::command_t::exec;
    } else if (cmd_kill->parsed()) {
        options.command = options::command_t::kill;
    }

    return options;
}

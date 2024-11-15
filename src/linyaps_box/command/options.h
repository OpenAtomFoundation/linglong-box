#pragma once

#include <filesystem>
#include <optional>
#include <vector>

namespace linyaps_box::command {

struct list_options
{
    enum class output_format_t {
        table,
        json,
    } output_format = output_format_t::table;
};

struct exec_options
{
    std::string user;
    std::string cwd;
    std::string ID;
    std::vector<std::string> command;
};

struct run_options
{
    std::string ID;
    std::string bundle;
    std::string config;
};

struct kill_options
{
    std::string container;
    int signal;
};

struct options
{
    enum class command_t {
        not_set,
        list,
        exec,
        run,
        kill,
    } command;

    std::filesystem::path root;
    std::optional<int> return_code;

    list_options list;
    exec_options exec;
    run_options run;
    kill_options kill;
};

// This function parses the command line arguments.
// It might print help or usage to stdout or stderr.
options parse(int argc, char *argv[]) noexcept;

} // namespace linyaps_box::command

#include "linyaps_box/app.h"

#include "linyaps_box/command/exec.h"
#include "linyaps_box/command/kill.h"
#include "linyaps_box/command/list.h"
#include "linyaps_box/command/run.h"
#include "linyaps_box/utils/log.h"

#include <iostream>
#include <stdexcept>

namespace linyaps_box {

// The main function of the ll-box,
// it is the entry point.
// Command line arguments are parsed according to
// https://github.com/opencontainers/runtime-tools/blob/v0.9.0/docs/command-line-interface.md
// Extended commands and options should be compatible with crun.
int main(int argc, char **argv) noexcept
try {
    LINYAPS_BOX_DEBUG() << "linyaps box called with" << [=]() {
        std::stringstream result;
        for (int i = 0; i < argc; ++i) {
            result << " \"";
            for (char *c = argv[i]; *c; ++c) {
                if (*c == '\\') {
                    result << "\\\\";
                } else if (*c == '"') {
                    result << "\\\"";
                } else {
                    result << *c;
                }
            }
            result << "\"";
        }
        return result.str();
    }();

    command::options options;
    options = command::parse(argc, argv);

    if (options.return_code) {
        return *options.return_code;
    }

    switch (options.command) {
    case command::options::command_t::list: {
        return command::list(options.root, options.list);
    }
    case command::options::command_t::run: {
        command::run(options.root, options.run);
        throw std::logic_error("unreachable");
    }
    case command::options::command_t::exec: {
        command::exec(options.root, options.exec);
        throw std::logic_error("unreachable");
    }
    case command::options::command_t::kill: {
        return command::kill(options.root, options.kill);
    }
    case command::options::command_t::not_set:
    default: {
        throw std::logic_error("unreachable");
    }
    }
} catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return -1;
} catch (...) {
    std::cerr << "Error: unknown" << std::endl;
    return -1;
}

} // namespace linyaps_box

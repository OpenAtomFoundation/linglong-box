#include "linyaps_box/command/list.h"

#include "linyaps_box/impl/json_printer.h"
#include "linyaps_box/impl/status_directory.h"
#include "linyaps_box/impl/table_printer.h"
#include "linyaps_box/runtime.h"

int linyaps_box::command::list(const std::filesystem::path &root,
                               const struct list_options &options)
{
    std::unique_ptr<status_directory> dir;
    dir = std::make_unique<impl::status_directory>(root);

    runtime_t runtime(std::move(dir));

    auto containers = runtime.containers();

    std::unique_ptr<printer> printer;
    if (options.output_format == list_options::output_format_t::json) {
        printer = std::make_unique<impl::json_printer>();
    } else {
        printer = std::make_unique<impl::table_printer>();
    }

    std::vector<container_status_t> statuses;
    for (const auto &container : containers) {
        statuses.push_back(container.second.status());
    }

    printer->print_statuses(statuses);
    return 0;
}

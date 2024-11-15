#include "linyaps_box/utils/atomic_write.h"

#include <fstream>

void linyaps_box::utils::atomic_write(const std::filesystem::path &path, const std::string &content)
{
    std::filesystem::path temp_path = path;
    temp_path += ".tmp";
    std::ofstream temp_file(temp_path);
    if (!temp_file.is_open()) {
        throw std::runtime_error("failed to open temporary file");
    }
    temp_file << content;
    temp_file.close();

    std::filesystem::rename(temp_path, path);
}

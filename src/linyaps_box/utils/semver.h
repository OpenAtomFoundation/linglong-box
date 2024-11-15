#pragma once

#include <string>

namespace linyaps_box::utils {

class semver
{
public:
    semver(unsigned int major,
           unsigned int minor,
           unsigned int patch,
           const std::string &prerelease = "",
           const std::string &build = "");

    semver(const std::string &str);

    unsigned int major;
    unsigned int minor;
    unsigned int patch;

    const std::string &prerelease() const;
    const std::string &build() const;

    std::string to_string() const;
    bool is_compatible_with(const semver &other) const;

private:
    std::string prerelease_;
    std::string build_;
};

} // namespace linyaps_box::utils

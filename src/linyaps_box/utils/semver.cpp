#include "linyaps_box/utils/semver.h"

#include <stdexcept>

linyaps_box::utils::semver::semver(const std::string &str)
{
    auto end = str.find('.');
    decltype(end) begin = 0;

    if (end == std::string::npos) {
        throw std::invalid_argument("invalid semver: " + str);
    }

    auto major = std::stoi(str.substr(begin, end));

    begin = end + 1;
    end = str.find('.', begin);
    if (end == std::string::npos) {
        throw std::invalid_argument("invalid semver: " + str);
    }

    auto minor = std::stoi(str.substr(begin, end));

    begin = end + 1;
    end = str.find_first_of("-+", begin);

    auto patch = std::stoi(str.substr(begin, end));

    if (end == std::string::npos) {
        this->major = major;
        this->minor = minor;
        this->patch = patch;
        return;
    }

    if (str[end] == '+') {
        this->major = major;
        this->minor = minor;
        this->patch = patch;
        this->build_ = str.substr(end + 1);
        return;
    }

    begin = end + 1;
    end = str.find('+', begin);

    auto prerelease = str.substr(begin, end);

    if (end == std::string::npos) {
        this->major = major;
        this->minor = minor;
        this->patch = patch;
        this->prerelease_ = prerelease;
        return;
    }

    auto build = str.substr(end + 1);
    this->major = major;
    this->minor = minor;
    this->patch = patch;
    this->prerelease_ = prerelease;
    this->build_ = build;
    return;
}

linyaps_box::utils::semver::semver(unsigned int major,
                                   unsigned int minor,
                                   unsigned int patch,
                                   const std::string &prerelease,
                                   const std::string &build)
    : major(major)
    , minor(minor)
    , patch(patch)
    , prerelease_(prerelease)
    , build_(build)
{
}

const std::string &linyaps_box::utils::semver::prerelease() const
{
    return this->prerelease_;
}

const std::string &linyaps_box::utils::semver::build() const
{
    return this->build_;
}

std::string linyaps_box::utils::semver::to_string() const
{
    return std::to_string(this->major) + "." + std::to_string(this->minor) + "."
            + std::to_string(this->patch) + (this->prerelease_.empty() ? "" : "-")
            + this->prerelease_ + (this->build_.empty() ? "" : "+") + this->build_;
}

bool linyaps_box::utils::semver::is_compatible_with(const semver &other) const
{
    if (this->major != other.major) {
        return false;
    }

    if (this->minor < other.minor) {
        return false;
    }

    if (this->minor > other.minor) {
        return true;
    }

    if (this->patch < other.patch) {
        return false;
    }

    if (this->patch > other.patch) {
        return true;
    }

    // FIXME: handle prerelease

    return true;
}

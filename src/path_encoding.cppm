export module md_archive.path_encoding;

import std;

export namespace md_archive::path_encoding {

std::filesystem::path from_utf8(std::string_view value) {
    std::u8string utf8(value.size(), u8'\0');
    if (!value.empty())
        std::memcpy(utf8.data(), value.data(), value.size());
    return std::filesystem::path(utf8);
}

std::string to_utf8(const std::filesystem::path& path) {
    const auto utf8 = path.u8string();
    std::string value(utf8.size(), '\0');
    if (!utf8.empty())
        std::memcpy(value.data(), utf8.data(), utf8.size());
    return value;
}

std::string generic_to_utf8(const std::filesystem::path& path) {
    const auto utf8 = path.generic_u8string();
    std::string value(utf8.size(), '\0');
    if (!utf8.empty())
        std::memcpy(value.data(), utf8.data(), utf8.size());
    return value;
}

} // namespace md_archive::path_encoding

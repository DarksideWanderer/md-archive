export module md_archive.frontmatter;

import std;

/**
 * @brief Parsed YAML frontmatter metadata for one Markdown document.
 *
 * @details The parser intentionally supports a small, dependency-free subset:
 * `title: ...`, inline tags like `tags: [a, b]`, and block tags like
 * `tags:\n  - a`.
 *
 * 中文：单个 Markdown 文档的 YAML frontmatter 解析结果。
 */
export struct DocumentMeta {
    /// Explicit document title from frontmatter.
    /// frontmatter 中显式提供的文档标题。
    std::string title;

    /// Parsed tag names in source order.
    /// 按源文件顺序解析出的标签名。
    std::vector<std::string> tags;

    /// True when the file starts with `---`.
    /// 文件以 `---` 开头时为 true。
    bool has_frontmatter = false;

    /// True when a non-empty `title:` field is present.
    /// 存在非空 `title:` 字段时为 true。
    bool has_title = false;

    /// True when the opening `---` has a matching closing `---`.
    /// 开始的 `---` 有匹配的结束 `---` 时为 true。
    bool closed_frontmatter = false;
};

/**
 * @brief Parse the top-of-file YAML frontmatter from a Markdown file.
 *
 * @param file_path Markdown file to inspect.
 * @return Parsed metadata. Missing or malformed fields are represented in the
 * returned flags instead of throwing.
 *
 * 中文：解析 Markdown 文件顶部的 YAML frontmatter。
 */
export [[nodiscard]] DocumentMeta parse_frontmatter(const std::filesystem::path& file_path);

namespace fs = std::filesystem;

namespace {

std::string trim(const std::string& s) {
    std::size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
        ++start;
    std::size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
        --end;
    return s.substr(start, end - start);
}

std::string extract_value(const std::string& line) {
    auto colon = line.find(':');
    if (colon == std::string::npos)
        return "";
    return trim(line.substr(colon + 1));
}

std::vector<std::string> parse_inline_array(const std::string& val) {
    std::vector<std::string> result;
    std::string s = trim(val);
    if (s.empty() || s[0] != '[')
        return result;

    s = s.substr(1);
    if (!s.empty() && s.back() == ']')
        s.pop_back();

    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) {
        item = trim(item);
        if (item.size() >= 2) {
            if ((item.front() == '"' && item.back() == '"') ||
                (item.front() == '\'' && item.back() == '\'')) {
                item = item.substr(1, item.size() - 2);
            }
        }
        if (!item.empty())
            result.push_back(item);
    }
    return result;
}

} // namespace

DocumentMeta parse_frontmatter(const fs::path& file_path) {
    DocumentMeta meta;

    std::ifstream in(file_path);
    if (!in.is_open())
        return meta;

    std::string line;
    if (!std::getline(in, line))
        return meta;
    if (trim(line) != "---")
        return meta;

    meta.has_frontmatter = true;

    bool in_tags_block = false;

    while (std::getline(in, line)) {
        std::string trimmed = trim(line);
        if (trimmed == "---") {
            meta.closed_frontmatter = true;
            break;
        }

        if (in_tags_block) {
            if (trimmed.starts_with("- ")) {
                std::string tag = trim(trimmed.substr(2));
                if (tag.size() >= 2 && ((tag.front() == '"' && tag.back() == '"') ||
                                        (tag.front() == '\'' && tag.back() == '\''))) {
                    tag = tag.substr(1, tag.size() - 2);
                }
                if (!tag.empty())
                    meta.tags.push_back(tag);
                continue;
            }
            in_tags_block = false;
        }

        if (trimmed.starts_with("title:")) {
            std::string val = extract_value(trimmed);
            if (val.size() >= 2 &&
                ((val.front() == '"' && val.back() == '"') || (val.front() == '\'' && val.back() == '\''))) {
                val = val.substr(1, val.size() - 2);
            }
            meta.title = val;
            meta.has_title = !val.empty();
            continue;
        }

        if (trimmed.starts_with("tags:")) {
            std::string val = extract_value(trimmed);

            if (val.empty()) {
                in_tags_block = true;
            } else if (val[0] == '[') {
                meta.tags = parse_inline_array(val);
            } else {
                meta.tags.push_back(val);
            }
            continue;
        }
    }

    return meta;
}

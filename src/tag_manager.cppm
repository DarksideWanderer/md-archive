export module md_archive.tag_manager;

import std;
import md_archive.frontmatter;
export import md_archive.config;

/**
 * @brief Coordinates archive copies, tag symlinks, and tag index files.
 *
 * @details TagManager owns the filesystem side effects of md-archive:
 * creating `.archive/`, creating `.tags/`, copying source Markdown files,
 * maintaining symlinks, and rebuilding Markdown index files.
 *
 * 中文：协调归档副本、标签符号链接和标签索引文件。
 */
export class TagManager {
  public:
    /**
     * @brief Create a manager for one validated configuration.
     *
     * The constructor ensures `.tags/` and `.archive/` exist and writes local
     * `.gitignore` files for generated output.
     *
     * 中文：基于已校验配置创建管理器，并确保生成目录存在。
     */
    explicit TagManager(const Config& cfg);

    /**
     * @brief Archive one Markdown file.
     *
     * @param md_file Source Markdown file. It must be inside `Config::workspace`.
     * @param force Replace existing archive copies or title-conflicting tag
     * links when true.
     * @return Tags that were archived; empty means skipped or failed with a
     * user-facing message already printed.
     *
     * 中文：归档单个 Markdown 文件。
     */
    std::vector<std::string> archive(const std::filesystem::path& md_file, bool force = false);

    /**
     * @brief Remove one source document from tag links and archive storage.
     *
     * 中文：从标签链接和归档存储中移除单个源文档。
     */
    void remove(const std::filesystem::path& md_file);

    /**
     * @brief Scan the workspace for Markdown files and archive them.
     *
     * @details Generated `.archive/` and `.tags/` directories are skipped to
     * avoid recursively archiving generated files.
     *
     * @return Number of files archived in this scan.
     *
     * 中文：扫描 workspace 中的 Markdown 文件并归档。
     */
    int scan_all(bool force = false);

    /**
     * @brief Rebuild all tag index Markdown files from tag directories.
     *
     * 中文：根据标签目录重建所有标签索引 Markdown 文件。
     */
    void rebuild_all_indexes();

    /// @return Sorted tag names currently present under `.tags/`.
    /// @return `.tags/` 下当前存在的已排序标签名。
    [[nodiscard]]
    std::vector<std::string> list_tags() const;

    /// @return Source document paths for one tag.
    /// @return 某个标签下对应的源文档路径。
    [[nodiscard]]
    std::vector<std::filesystem::path> list_docs_for_tag(const std::string& tag) const;

    /// @return Unique source document paths represented by all tags.
    /// @return 所有标签共同表示的去重源文档路径。
    [[nodiscard]]
    std::vector<std::filesystem::path> list_all_archived() const;

  private:
    Config cfg;
    std::filesystem::path tags_root;
    std::filesystem::path archive_root;

    static std::string safe_filename(const std::string& title);
    std::optional<std::filesystem::path> copy_to_archive(const std::filesystem::path& md_file);
    bool create_symlink_for_tag(const std::filesystem::path& archive_copy, const std::string& tag,
                                const std::string& doc_title, bool force);
    void update_tag_index(const std::string& tag);

    struct DocInfo {
        std::string title;
        std::filesystem::path rel_path;
    };
    std::vector<DocInfo> collect_docs_for_tag(const std::string& tag) const;
};

namespace fs = std::filesystem;

namespace {

fs::path normalize_path(const fs::path& path) {
    std::error_code ec;
    auto normalized = fs::weakly_canonical(path, ec);
    if (!ec)
        return normalized;
    return fs::absolute(path);
}

bool is_within_directory(const fs::path& path, const fs::path& directory) {
    auto norm_path = normalize_path(path);
    auto norm_dir = normalize_path(directory);
    auto rel = norm_path.lexically_relative(norm_dir);
    return !rel.empty() && *rel.begin() != "..";
}

} // namespace

TagManager::TagManager(const Config& cfg) : cfg(cfg) {
    tags_root = cfg.tags_root();
    archive_root = cfg.archive_root();

    if (!fs::exists(tags_root)) {
        fs::create_directories(tags_root);
        std::ofstream gi(tags_root / ".gitignore");
        gi << "# 自动生成的标签文件，无需提交\n*\n!.gitignore\n";
    }

    if (!fs::exists(archive_root)) {
        fs::create_directories(archive_root);
        std::ofstream gi(archive_root / ".gitignore");
        gi << "# 归档内容副本，无需提交\n*\n!.gitignore\n";
    }
}

std::string TagManager::safe_filename(const std::string& title) {
    static const std::string forbidden = "/\\:*?\"<>|";
    std::string safe;
    for (char c : title) {
        if (forbidden.find(c) != std::string::npos) {
            safe += '_';
        } else if (c == '\n' || c == '\r' || c == '\t') {
            safe += ' ';
        } else {
            safe += c;
        }
    }
    while (!safe.empty() && (safe.front() == ' ' || safe.front() == '.'))
        safe.erase(0, 1);
    while (!safe.empty() && (safe.back() == ' ' || safe.back() == '.'))
        safe.pop_back();
    if (safe.empty())
        safe = "untitled";
    return safe;
}

std::optional<fs::path> TagManager::copy_to_archive(const fs::path& md_file) {
    fs::path rel = fs::relative(md_file, cfg.workspace);
    fs::path dest = archive_root / rel;

    fs::create_directories(dest.parent_path());

    std::error_code ec;
    fs::copy_file(md_file, dest, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        std::cerr << "  错误: 复制到归档失败: " << ec.message() << "\n";
        return std::nullopt;
    }

    return dest;
}

bool TagManager::create_symlink_for_tag(const fs::path& archive_copy, const std::string& tag,
                                        const std::string& doc_title, bool force) {
    fs::path tag_dir = tags_root / tag;
    if (!fs::exists(tag_dir)) {
        fs::create_directories(tag_dir);
    }

    std::string safe_title = safe_filename(doc_title);
    fs::path link_path = tag_dir / (safe_title + ".md");

    if (fs::exists(link_path)) {
        fs::path canonical_new;
        {
            std::error_code ec;
            auto ca = fs::canonical(archive_copy, ec);
            if (!ec)
                canonical_new = ca;
            else
                canonical_new = archive_copy;
        }
        std::error_code ec;
        fs::path resolved_existing = fs::canonical(link_path, ec);
        bool same_target = (!ec && resolved_existing == canonical_new);

        if (!same_target) {
            if (!force) {
                std::cout << "  ⚠️  标题冲突: \"" << safe_title << "\" 已被另一个文件使用，跳过标签 [" << tag
                          << "]（使用 --force 强制替换）\n";
                return false;
            }
            std::cout << "  ⚠️  强制替换: \"" << safe_title << "\" 在标签 [" << tag << "] 中被覆盖\n";
            fs::remove(link_path, ec);
        } else {
            fs::remove(link_path, ec);
        }
    }

    fs::path rel = fs::relative(archive_copy, tag_dir);
    std::error_code ec2;
    fs::create_symlink(rel, link_path, ec2);
    if (ec2) {
        std::ofstream stub(link_path);
        fs::path rel_to_ws = fs::relative(archive_copy, cfg.workspace);
        stub << "# " << doc_title << "\n\n"
             << "→ 归档副本: [" << doc_title << "](" << rel_to_ws.string() << ")\n";
        std::cerr << "  注意: 无法创建符号链接，已创建桩文件: " << link_path << "\n";
    }
    return true;
}

std::vector<TagManager::DocInfo> TagManager::collect_docs_for_tag(const std::string& tag) const {
    std::vector<DocInfo> docs;
    fs::path tag_dir = tags_root / tag;

    if (!fs::exists(tag_dir))
        return docs;

    for (const auto& entry : fs::directory_iterator(tag_dir)) {
        if (!entry.is_symlink() && !entry.is_regular_file())
            continue;
        if (entry.path().extension() != ".md")
            continue;

        DocInfo info;
        info.title = entry.path().stem().string();

        if (entry.is_symlink()) {
            fs::path target = fs::read_symlink(entry.path());
            fs::path abs_target = tag_dir / target;
            std::error_code ec;
            abs_target = fs::canonical(abs_target, ec);
            if (!ec) {
                info.rel_path = fs::relative(abs_target, cfg.workspace);
            } else {
                info.rel_path = target;
            }
        } else {
            info.rel_path = fs::relative(entry.path(), cfg.workspace);
        }

        std::string rel_str = info.rel_path.string();
        std::string archive_prefix = cfg.archive_dir.string() + "/";
        if (rel_str.starts_with(archive_prefix)) {
            rel_str = rel_str.substr(archive_prefix.size());
            info.rel_path = rel_str;
        }

        docs.push_back(info);
    }

    std::sort(docs.begin(), docs.end(), [](const DocInfo& a, const DocInfo& b) { return a.title < b.title; });

    return docs;
}

void TagManager::update_tag_index(const std::string& tag) {
    auto docs = collect_docs_for_tag(tag);
    fs::path index_path = tags_root / (tag + ".md");

    std::ofstream out(index_path);
    out << "# 🏷️ " << tag << "\n\n";

    if (docs.empty()) {
        out << "*此标签下暂无文档*\n\n";
    } else {
        out << "| # | 文档 | 路径 |\n";
        out << "|---|------|------|\n";
        int idx = 1;
        for (const auto& doc : docs) {
            out << "| " << idx++ << " | "
                << "[" << doc.title << "](" << doc.rel_path.string() << ") | "
                << "`" << doc.rel_path.string() << "` |\n";
        }
        out << "\n";
    }

    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    out << "---\n";
    out << "*共 " << docs.size() << " 篇文档 | "
        << "最后更新: " << std::put_time(&tm, "%Y-%m-%d %H:%M") << "*\n";
}

std::vector<std::string> TagManager::archive(const fs::path& md_file, bool force) {
    fs::path source = normalize_path(md_file);
    if (!is_within_directory(source, cfg.workspace)) {
        std::cerr << "错误: 文件不在 workspace 内: " << md_file << "\n";
        return {};
    }

    if (source.extension() != ".md") {
        std::cerr << "错误: 只接受 .md 文件\n";
        return {};
    }

    auto meta = parse_frontmatter(source);
    if (!meta.has_frontmatter) {
        std::cout << "跳过: " << source.filename() << " (缺少顶部 YAML frontmatter)\n";
        return {};
    }
    if (!meta.closed_frontmatter) {
        std::cout << "跳过: " << source.filename() << " (frontmatter 未闭合)\n";
        return {};
    }
    if (meta.tags.empty()) {
        std::cout << "跳过: " << source.filename() << " (缺少 tags)\n";
        return {};
    }
    if (!meta.has_title) {
        std::cout << "跳过: " << source.filename() << " (缺少 title)\n";
        return {};
    }

    fs::path rel = fs::relative(source, cfg.workspace);
    fs::path archive_copy = archive_root / rel;

    if (fs::exists(archive_copy)) {
        if (!force) {
            std::cout << "⚠️  警告: \"" << rel.string() << "\" 已在归档中，跳过（使用 --force 强制替换）\n";
            return {};
        }
        std::cout << "⚠️  强制替换已归档的: " << rel.string() << "\n";

        fs::path canonical_archive = fs::canonical(archive_copy);
        std::set<std::string> affected_tags;
        if (fs::exists(tags_root)) {
            for (const auto& tag_entry : fs::directory_iterator(tags_root)) {
                if (!tag_entry.is_directory())
                    continue;
                fs::path tag_dir = tag_entry.path();
                for (const auto& link_entry : fs::directory_iterator(tag_dir)) {
                    if (!link_entry.is_symlink())
                        continue;
                    std::error_code ec;
                    fs::path resolved = fs::canonical(link_entry.path(), ec);
                    if (!ec && resolved == canonical_archive) {
                        fs::remove(link_entry.path(), ec);
                        affected_tags.insert(tag_dir.filename().string());
                    }
                }
                if (fs::is_empty(tag_dir)) {
                    std::error_code ec;
                    fs::remove(tag_dir, ec);
                }
            }
        }
        for (const auto& t : affected_tags) {
            update_tag_index(t);
        }
    }

    std::cout << "归档: " << source.filename() << "\n";
    std::cout << "  标题: " << meta.title << "\n";
    std::cout << "  标签: ";
    for (std::size_t i = 0; i < meta.tags.size(); ++i) {
        if (i > 0)
            std::cout << ", ";
        std::cout << meta.tags[i];
    }
    std::cout << "\n";

    auto copied = copy_to_archive(source);
    if (!copied) {
        return {};
    }
    archive_copy = *copied;
    std::cout << "  已存档: " << rel.string() << "\n";

    for (const auto& tag : meta.tags) {
        create_symlink_for_tag(archive_copy, tag, meta.title, force);
    }

    for (const auto& tag : meta.tags) {
        update_tag_index(tag);
    }

    return meta.tags;
}

void TagManager::remove(const fs::path& md_file) {
    fs::path source = normalize_path(md_file);
    if (!is_within_directory(source, cfg.workspace)) {
        std::cerr << "错误: 文件不在 workspace 内: " << md_file << "\n";
        return;
    }

    auto meta = parse_frontmatter(source);
    if (meta.tags.empty()) {
        std::cout << "文件无标签，无需移除\n";
        return;
    }

    std::string safe_title = safe_filename(meta.title);
    bool removed_any = false;

    for (const auto& tag : meta.tags) {
        fs::path tag_dir = tags_root / tag;
        fs::path link_path = tag_dir / (safe_title + ".md");

        if (fs::exists(link_path)) {
            std::cout << "  移除: " << tag << " / " << safe_title << ".md\n";
            std::error_code ec;
            fs::remove(link_path, ec);
            removed_any = true;

            if (fs::is_empty(tag_dir)) {
                fs::remove(tag_dir, ec);
            }

            update_tag_index(tag);
        }
    }

    if (removed_any) {
        fs::path rel = fs::relative(source, cfg.workspace);
        std::string rel_str = rel.string();
        std::string archive_prefix = cfg.archive_dir.string() + "/";
        if (rel_str.starts_with(archive_prefix)) {
            rel_str = rel_str.substr(archive_prefix.size());
            rel = rel_str;
        }
        fs::path archive_copy = archive_root / rel;
        std::error_code ec;
        if (fs::exists(archive_copy)) {
            fs::remove(archive_copy, ec);
            std::cout << "  已删除存档副本: " << (cfg.archive_dir / rel).string() << "\n";

            auto parent = archive_copy.parent_path();
            if (parent != archive_root && fs::exists(parent) && fs::is_empty(parent)) {
                fs::remove(parent, ec);
            }
        }
        std::cout << "已从归档中移除: " << source.filename() << "\n";
    }
}

int TagManager::scan_all(bool force) {
    int count = 0;
    std::cout << "扫描工作区: " << cfg.workspace << "\n";

    std::error_code ec;
    fs::recursive_directory_iterator it(cfg.workspace, fs::directory_options::skip_permission_denied, ec);
    fs::recursive_directory_iterator end;
    for (; it != end; it.increment(ec)) {
        if (ec) {
            std::cerr << "跳过: 无法访问路径 (" << ec.message() << ")\n";
            ec.clear();
            continue;
        }
        const auto& entry = *it;

        if (entry.is_directory(ec)) {
            auto path = normalize_path(entry.path());
            if (path == normalize_path(tags_root) || path == normalize_path(archive_root)) {
                it.disable_recursion_pending();
                continue;
            }
        }

        if (entry.is_regular_file() && entry.path().extension() == ".md") {
            auto tags = archive(entry.path(), force);
            if (!tags.empty())
                ++count;
        }
    }

    std::cout << "\n扫描完成，归档了 " << count << " 个文件\n";
    return count;
}

void TagManager::rebuild_all_indexes() {
    std::set<std::string> tags;

    if (fs::exists(tags_root)) {
        for (const auto& entry : fs::directory_iterator(tags_root)) {
            if (entry.is_directory()) {
                tags.insert(entry.path().filename().string());
            }
        }
    }

    for (const auto& tag : tags) {
        std::cout << "  重建索引: " << tag << "\n";
        update_tag_index(tag);
    }
    std::cout << "已重建 " << tags.size() << " 个标签索引\n";
}

std::vector<std::string> TagManager::list_tags() const {
    std::vector<std::string> tags;
    if (!fs::exists(tags_root))
        return tags;

    for (const auto& entry : fs::directory_iterator(tags_root)) {
        if (entry.is_directory()) {
            tags.push_back(entry.path().filename().string());
        }
    }
    std::sort(tags.begin(), tags.end());
    return tags;
}

std::vector<fs::path> TagManager::list_docs_for_tag(const std::string& tag) const {
    auto docs = collect_docs_for_tag(tag);
    std::vector<fs::path> paths;
    for (const auto& doc : docs) {
        paths.push_back(cfg.workspace / doc.rel_path);
    }
    return paths;
}

std::vector<fs::path> TagManager::list_all_archived() const {
    std::set<fs::path> unique;
    auto tags = list_tags();
    for (const auto& tag : tags) {
        auto docs = collect_docs_for_tag(tag);
        for (const auto& doc : docs) {
            unique.insert(cfg.workspace / doc.rel_path);
        }
    }
    return std::vector<fs::path>(unique.begin(), unique.end());
}

export module md_archive.tag_manager;

import std;
import md_archive.path_encoding;
import md_archive.content_hash;
import md_archive.archive_store;
import md_archive.frontmatter;
export import md_archive.config;

/**
 * @brief Coordinates hash-addressed archive objects and source tag links.
 *
 * @details TagManager owns the filesystem side effects of md-archive:
 * creating `.archive/`, creating `.tags/`, deduplicating source Markdown
 * content, maintaining source links, and migrating legacy archives.
 *
 * 中文：协调哈希寻址归档对象和源文件标签链接，并迁移旧版归档。
 */
export class TagManager {
  public:
    struct ListedDoc {
        std::string title;
        std::filesystem::path path;
    };
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
     * @brief Normalize tag links and remove obsolete root overview files.
     *
     * 中文：整理标签符号链接并删除旧版根级索引文件。
     */
    void rebuild_all_links();

    /// @return Sorted tag names currently present under `.tags/`.
    /// @return `.tags/` 下当前存在的已排序标签名。
    [[nodiscard]]
    std::vector<std::string> list_tags() const;

    /// @return Source document paths for one tag.
    /// @return 某个标签下对应的源文档路径。
    [[nodiscard]]
    std::vector<std::filesystem::path> list_docs_for_tag(const std::string& tag) const;

    /// Return display titles together with their effective source or archive paths.
    [[nodiscard]]
    std::vector<ListedDoc> list_doc_entries_for_tag(const std::string& tag) const;

    /// @return Unique source document paths represented by all tags.
    /// @return 所有标签共同表示的去重源文档路径。
    [[nodiscard]]
    std::vector<std::filesystem::path> list_all_archived() const;

  private:
    Config cfg;
    std::filesystem::path tags_root;
    std::filesystem::path archive_root;
    std::unique_ptr<md_archive::ArchiveStore> archive_store;

    static std::string safe_filename(const std::string& title);
    std::optional<std::filesystem::path> copy_to_archive(const std::filesystem::path& md_file);
    bool create_tag_entry(const std::filesystem::path& archive_copy, const std::string& tag,
                          const std::string& doc_title, bool force,
                          bool report_forced_replacement = true);
    void remove_root_tag_overview(const std::string& tag);
    void normalize_all_links(bool verbose);
    void restore_links_from_index();

    struct DocInfo {
        std::string title;
        std::filesystem::path rel_path;
        std::filesystem::path archive_rel_path;
    };
    std::vector<DocInfo> collect_docs_for_tag(const std::string& tag) const;
};

namespace fs = std::filesystem;
using md_archive::path_encoding::from_utf8;
using md_archive::path_encoding::generic_to_utf8;
using md_archive::path_encoding::to_utf8;

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

constexpr std::string_view target_marker = "<!-- md-archive-target: ";

bool windows_reserved_component(std::string value) {
    std::ranges::transform(value, value.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    const auto dot = value.find('.');
    if (dot != std::string::npos)
        value.resize(dot);
    if (value == "CON" || value == "PRN" || value == "AUX" || value == "NUL")
        return true;
    return value.size() == 4 &&
           (value.starts_with("COM") || value.starts_with("LPT")) &&
           value[3] >= '1' && value[3] <= '9';
}

bool valid_tag_name(const std::string& tag) {
    if (tag.empty() || tag == "." || tag == ".." || tag.back() == ' ' || tag.back() == '.' ||
        windows_reserved_component(tag))
        return false;
    constexpr std::string_view forbidden = "/\\:*?\"<>|";
    return std::ranges::none_of(tag, [&](unsigned char c) {
        return c < 0x20 || forbidden.find(static_cast<char>(c)) != std::string_view::npos;
    });
}

std::optional<fs::path> tag_entry_target(const fs::path& entry, const fs::path& tag_dir,
                                         const fs::path& archive_root = {}) {
    std::error_code ec;
    if (fs::is_symlink(fs::symlink_status(entry, ec)))
        return tag_dir / fs::read_symlink(entry, ec);

    std::ifstream in(entry, std::ios::binary);
    if (!in)
        return std::nullopt;
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

    const auto marker = content.rfind(target_marker);
    if (marker != std::string::npos) {
        const auto begin = marker + target_marker.size();
        const auto end = content.find(" -->", begin);
        if (end != std::string::npos)
            return tag_dir / from_utf8(content.substr(begin, end - begin));
    }

    // Git checks out mode-120000 links as one-line text when core.symlinks=false.
    while (!content.empty() && (content.back() == '\n' || content.back() == '\r'))
        content.pop_back();
    if (!content.empty() && content.find('\n') == std::string::npos && content.ends_with(".md"))
        return tag_dir / from_utf8(content);

    // A hard link is the privilege-free Windows fallback. Find the matching
    // archive file by comparing filesystem identities.
    if (!archive_root.empty() && fs::exists(archive_root)) {
        fs::recursive_directory_iterator it(
            archive_root, fs::directory_options::skip_permission_denied, ec);
        const fs::recursive_directory_iterator end;
        for (; it != end; it.increment(ec)) {
            if (ec) {
                ec.clear();
                continue;
            }
            if (!it->is_regular_file(ec)) {
                ec.clear();
                continue;
            }
            if (fs::equivalent(entry, it->path(), ec) && !ec)
                return it->path();
            ec.clear();

            // Git preserves hard-linked files as two independent regular
            // files. Content equality lets a macOS checkout recover the
            // archive target and recreate its native symbolic link.
            if (fs::file_size(it->path(), ec) != content.size() || ec) {
                ec.clear();
                continue;
            }
            std::ifstream candidate(it->path(), std::ios::binary);
            if (!candidate)
                continue;
            std::string candidate_content((std::istreambuf_iterator<char>(candidate)),
                                          std::istreambuf_iterator<char>());
            if (candidate_content == content)
                return it->path();
        }
    }

    return std::nullopt;
}

} // namespace

TagManager::TagManager(const Config& cfg) : cfg(cfg) {
    tags_root = cfg.tags_root();
    archive_root = cfg.archive_root();

    if (!fs::exists(tags_root)) {
        fs::create_directories(tags_root);
    }
    {
        std::ofstream gi(tags_root / ".gitignore");
        gi << "# Generated platform-specific tag entries; do not commit.\n"
              "*\n!.gitignore\n";
    }

    if (!fs::exists(archive_root)) {
        fs::create_directories(archive_root);
    }
    {
        std::ofstream gi(archive_root / ".gitignore", std::ios::trunc);
        gi << "# 1.0 portable hash archive: commit index.tsv and objects/\n"
              "*\n!.gitignore\n!index.tsv\n!objects/\n!objects/**\n";
    }

    archive_store = std::make_unique<md_archive::ArchiveStore>(cfg.workspace, archive_root);

    // A clone can materialize links differently on another operating system.
    // Normalize them before every command without requiring a manual rebuild.
    normalize_all_links(false);
    restore_links_from_index();
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
    if (windows_reserved_component(safe))
        safe += '_';
    return safe;
}

std::optional<fs::path> TagManager::copy_to_archive(const fs::path& md_file) {
    auto record = archive_store->store(md_file);
    if (!record) {
        std::cerr << "  错误: 无法计算哈希或写入归档对象\n";
        return std::nullopt;
    }
    return record->object_path;
}

bool TagManager::create_tag_entry(const fs::path& archive_copy, const std::string& tag,
                                  const std::string& doc_title, bool force,
                                  bool report_forced_replacement) {
    fs::path tag_dir = tags_root / from_utf8(tag);
    if (!fs::exists(tag_dir)) {
        fs::create_directories(tag_dir);
    }

    std::string safe_title = safe_filename(doc_title);
    fs::path link_path = tag_dir / from_utf8(safe_title + ".md");

    std::error_code status_error;
    const bool link_path_present =
        fs::symlink_status(link_path, status_error).type() != fs::file_type::not_found;
    if (link_path_present) {
        fs::path canonical_new = normalize_path(archive_copy);
        std::error_code ec;
        auto existing_target = tag_entry_target(link_path, tag_dir, archive_root);
        fs::path resolved_existing = existing_target ? normalize_path(*existing_target) : fs::path{};
        bool same_target = existing_target && resolved_existing == canonical_new;

        if (existing_target && !fs::exists(*existing_target)) {
            fs::remove(link_path, ec);
            same_target = true;
        }

        if (!same_target) {
            if (!force) {
                std::cout << "  ⚠️  标题冲突: \"" << safe_title << "\" 已被另一个文件使用，跳过标签 [" << tag
                          << "]（使用 --force 强制替换）\n";
                return false;
            }
            if (report_forced_replacement)
                std::cout << "  ⚠️  强制替换: \"" << safe_title << "\" 在标签 [" << tag << "] 中被覆盖\n";
            fs::remove(link_path, ec);
        } else {
            fs::remove(link_path, ec);
        }
    }

    const fs::path rel = fs::relative(archive_copy, tag_dir);
    std::error_code link_error;
    fs::create_symlink(rel, link_path, link_error);
    if (link_error) {
        std::error_code hard_link_error;
        fs::create_hard_link(archive_copy, link_path, hard_link_error);
        if (hard_link_error) {
            std::cerr << "  错误: 无法创建符号链接或硬链接: " << to_utf8(link_path) << "\n"
                      << "        符号链接: " << link_error.message() << "\n"
                      << "        硬链接: " << hard_link_error.message() << "\n";
            return false;
        }
        std::cout << "  提示: 符号链接不可用，已创建 Windows 兼容的硬链接\n";
    }
    return true;
}

std::vector<TagManager::DocInfo> TagManager::collect_docs_for_tag(const std::string& tag) const {
    std::vector<DocInfo> docs;
    fs::path tag_dir = tags_root / from_utf8(tag);

    if (!fs::exists(tag_dir))
        return docs;

    for (const auto& entry : fs::directory_iterator(tag_dir)) {
        if (!entry.is_symlink() && !entry.is_regular_file())
            continue;
        if (entry.path().extension() != ".md")
            continue;

        DocInfo info;
        info.title = to_utf8(entry.path().stem());

        if (auto stored_target = tag_entry_target(entry.path(), tag_dir, archive_root)) {
            fs::path abs_target = *stored_target;
            std::error_code ec;
            abs_target = fs::canonical(abs_target, ec);
            if (ec)
                abs_target = normalize_path(*stored_target);

            if (is_within_directory(abs_target, archive_root)) {
                if (auto hash = archive_store->hash_for_object(abs_target)) {
                    auto sources = archive_store->sources_for_hash(*hash);
                    // The object path is an internal storage detail. Keep the
                    // tag entry linked to that durable object when necessary,
                    // but expose the first indexed source path to list/docs,
                    // even when that historical source no longer exists.
                    if (!sources.empty())
                        abs_target = sources.front();
                } else {
                    auto legacy_rel = abs_target.lexically_relative(archive_root);
                    auto legacy_source = cfg.workspace / legacy_rel;
                    if (fs::exists(legacy_source))
                        abs_target = legacy_source;
                }
            }
            info.rel_path = fs::relative(abs_target, cfg.workspace);
            info.archive_rel_path = fs::relative(*stored_target, cfg.workspace);
        } else {
            info.rel_path = fs::relative(entry.path(), cfg.workspace);
            info.archive_rel_path = info.rel_path;
        }

        docs.push_back(info);
    }

    std::sort(docs.begin(), docs.end(), [](const DocInfo& a, const DocInfo& b) { return a.title < b.title; });

    return docs;
}

void TagManager::remove_root_tag_overview(const std::string& tag) {
    fs::path index_path = tags_root / from_utf8(tag + ".md");
    std::error_code ec;
    fs::remove(index_path, ec);
}

std::vector<std::string> TagManager::archive(const fs::path& md_file, bool force) {
    fs::path source = normalize_path(md_file);
    if (!is_within_directory(source, cfg.workspace)) {
        std::cerr << "错误: 文件不在 workspace 内: " << to_utf8(md_file) << "\n";
        return {};
    }

    if (source.extension() != ".md") {
        std::cerr << "错误: 只接受 .md 文件\n";
        return {};
    }

    auto meta = parse_frontmatter(source);
    if (!meta.has_frontmatter) {
        std::cout << "跳过: " << to_utf8(source.filename()) << " (缺少顶部 YAML frontmatter)\n";
        return {};
    }
    if (!meta.closed_frontmatter) {
        std::cout << "跳过: " << to_utf8(source.filename()) << " (frontmatter 未闭合)\n";
        return {};
    }
    if (meta.tags.empty()) {
        std::cout << "跳过: " << to_utf8(source.filename()) << " (缺少 tags)\n";
        return {};
    }
    if (!meta.has_title) {
        std::cout << "跳过: " << to_utf8(source.filename()) << " (缺少 title)\n";
        return {};
    }
    for (const auto& tag : meta.tags) {
        if (!valid_tag_name(tag)) {
            std::cerr << "错误: 标签名不能包含路径分隔符、Windows 保留字符或保留名称: "
                      << tag << "\n";
            return {};
        }
    }

    fs::path rel = fs::relative(source, cfg.workspace);
    auto current_hash = md_archive::sha256_markdown_file(source);
    if (!current_hash) {
        std::cerr << "错误: 无法读取文件并计算 SHA-256\n";
        return {};
    }
    auto known_hash = archive_store->hash_for_source(source);

    if (known_hash) {
        if (!force) {
            std::cout << "⚠️  警告: \"" << to_utf8(rel) << "\" 已在归档中，跳过（使用 --force 强制替换）\n";
            return {};
        }
        std::cout << "⚠️  强制替换已归档的: " << to_utf8(rel) << "\n";

        std::set<std::string> affected_tags;
        if (fs::exists(tags_root)) {
            for (const auto& tag_entry : fs::directory_iterator(tags_root)) {
                if (!tag_entry.is_directory())
                    continue;
                fs::path tag_dir = tag_entry.path();
                for (const auto& link_entry : fs::directory_iterator(tag_dir)) {
                    if (!link_entry.is_symlink() && !link_entry.is_regular_file())
                        continue;
                    std::error_code ec;
                    auto target = tag_entry_target(link_entry.path(), tag_dir, archive_root);
                    fs::path resolved = target ? normalize_path(*target) : fs::path{};
                    bool belongs_to_source = resolved == source ||
                                             fs::equivalent(link_entry.path(), source, ec);
                    ec.clear();
                    if (!belongs_to_source && target) {
                        if (auto target_hash = archive_store->hash_for_object(*target)) {
                            auto aliases = archive_store->sources_for_hash(*target_hash);
                            belongs_to_source = *target_hash == *known_hash && aliases.size() == 1 &&
                                                normalize_path(aliases.front()) == source;
                        }
                    }
                    if (belongs_to_source) {
                        fs::remove(link_entry.path(), ec);
                        affected_tags.insert(to_utf8(tag_dir.filename()));
                    }
                }
                if (fs::is_empty(tag_dir)) {
                    std::error_code ec;
                    fs::remove(tag_dir, ec);
                }
            }
        }
        for (const auto& t : affected_tags) {
            remove_root_tag_overview(t);
        }
    }

    std::cout << "归档: " << to_utf8(source.filename()) << "\n";
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
    const fs::path archive_copy = *copied;
    const auto aliases = archive_store->sources_for_hash(*current_hash);
    std::cout << "  SHA-256: " << *current_hash << "\n";
    std::cout << "  已存档对象: " << to_utf8(fs::relative(archive_copy, cfg.workspace)) << "\n";
    if (aliases.size() > 1)
        std::cout << "  内容重复: 共 " << aliases.size() << " 个源路径共享此备份\n";

    for (const auto& tag : meta.tags) {
        create_tag_entry(source, tag, meta.title, force);
    }

    for (const auto& tag : meta.tags) {
        remove_root_tag_overview(tag);
    }

    // Updating one source that previously shared an object may have removed a
    // tag entry which was also the only visible representation of another
    // source alias. Reconcile immediately so the remaining old-hash aliases
    // and the updated source are both represented after this command returns.
    if (known_hash && force)
        restore_links_from_index();

    return meta.tags;
}

void TagManager::remove(const fs::path& md_file) {
    fs::path source = normalize_path(md_file);
    if (!is_within_directory(source, cfg.workspace)) {
        std::cerr << "错误: 文件不在 workspace 内: " << to_utf8(md_file) << "\n";
        return;
    }

    const auto known_hash = archive_store->hash_for_source(source);
    if (!known_hash) {
        std::cout << "文件不在归档索引中: " << to_utf8(source) << "\n";
        return;
    }

    const fs::path archived = archive_store->object_for_hash(*known_hash);
    const fs::path metadata_source = fs::exists(source) ? source : archived;
    auto meta = parse_frontmatter(metadata_source);
    if (meta.tags.empty()) {
        std::cout << "警告: 无法从源文件或归档对象恢复标签元数据；仅移除路径映射\n";
    }

    std::string safe_title = safe_filename(meta.title);
    bool removed_any = false;
    const auto aliases = archive_store->sources_for_hash(*known_hash);

    for (const auto& tag : meta.tags) {
        if (!valid_tag_name(tag))
            continue;
        fs::path tag_dir = tags_root / from_utf8(tag);
        fs::path link_path = tag_dir / from_utf8(safe_title + ".md");

        std::error_code link_status_error;
        if (fs::symlink_status(link_path, link_status_error).type() != fs::file_type::not_found) {
            auto target = tag_entry_target(link_path, tag_dir, archive_root);
            std::error_code equivalent_error;
            bool points_to_source = (target && normalize_path(*target) == source) ||
                                    fs::equivalent(link_path, source, equivalent_error);
            if (!points_to_source && target && aliases.size() == 1) {
                if (auto target_hash = archive_store->hash_for_object(*target))
                    points_to_source = *target_hash == *known_hash;
            }
            if (!points_to_source)
                continue;
            std::cout << "  移除: " << tag << " / " << safe_title << ".md\n";
            std::error_code ec;
            fs::remove(link_path, ec);
            removed_any = true;

            if (fs::is_empty(tag_dir)) {
                fs::remove(tag_dir, ec);
            }

            remove_root_tag_overview(tag);
        }
    }

    const bool removed_mapping = archive_store->remove_source(source);
    if (removed_mapping) {
        std::cout << "  已更新哈希归档索引；无引用的内容对象已清理\n";
        // If the removed path owned the sole visible entry for a shared title,
        // immediately retarget that entry to a remaining indexed source.
        restore_links_from_index();
    }
    if (removed_mapping || removed_any) {
        std::cout << "已从归档中移除: " << to_utf8(source.filename()) << "\n";
    }
}

int TagManager::scan_all(bool force) {
    int count = 0;
    std::cout << "扫描工作区: " << to_utf8(cfg.workspace) << "\n";

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

void TagManager::normalize_all_links(bool verbose) {
    std::set<std::string> tags;
    struct PendingEntry {
        fs::path archive_copy;
        std::string tag;
        std::string title;
    };
    std::vector<PendingEntry> pending_entries;

    if (fs::exists(tags_root)) {
        for (const auto& entry : fs::directory_iterator(tags_root)) {
            if (entry.is_directory()) {
                const std::string tag = to_utf8(entry.path().filename());
                tags.insert(tag);
                for (const auto& tag_entry : fs::directory_iterator(entry.path())) {
                    if (!tag_entry.is_regular_file() && !tag_entry.is_symlink())
                        continue;
                    if (tag_entry.path().extension() != ".md")
                        continue;
                    if (auto target = tag_entry_target(tag_entry.path(), entry.path(), archive_root)) {
                        fs::path desired = *target;
                        if (is_within_directory(normalize_path(desired), archive_root)) {
                            if (auto hash = archive_store->hash_for_object(desired)) {
                                auto sources = archive_store->sources_for_hash(*hash);
                                auto existing = std::ranges::find_if(
                                    sources, [](const fs::path& path) { return fs::exists(path); });
                                if (existing != sources.end())
                                    desired = *existing;
                            } else {
                                auto legacy_rel = desired.lexically_relative(archive_root);
                                auto legacy_source = cfg.workspace / legacy_rel;
                                if (fs::exists(legacy_source))
                                    desired = legacy_source;
                            }
                        }
                        if (!fs::exists(desired))
                            continue;
                        std::error_code equivalent_error;
                        if (fs::equivalent(tag_entry.path(), desired, equivalent_error) &&
                            !equivalent_error)
                            continue;
                        if (tag_entry.is_symlink() && normalize_path(*target) == normalize_path(desired))
                            continue;
                        pending_entries.push_back(
                            {desired, tag, to_utf8(tag_entry.path().stem())});
                    }
                }
            }
        }
    }

    for (const auto& pending : pending_entries)
        create_tag_entry(pending.archive_copy, pending.tag, pending.title, true, false);

    for (const auto& tag : tags) {
        if (verbose)
            std::cout << "  整理标签: " << tag << "\n";
        remove_root_tag_overview(tag);
    }
    if (verbose)
        std::cout << "已整理 " << tags.size() << " 个标签目录\n";
}

void TagManager::rebuild_all_links() {
    normalize_all_links(true);
    restore_links_from_index();
}

void TagManager::restore_links_from_index() {
    for (const auto& [source_key, hash] : archive_store->entries()) {
        const fs::path source = cfg.workspace / from_utf8(source_key);
        const fs::path archived = archive_store->object_for_hash(hash);

        // The source path is not the durable copy: users may intentionally
        // delete it after archiving. In that case the hash index and object
        // must remain sufficient to reconstruct every .tags entry.
        const fs::path document = fs::exists(source) ? source : archived;
        if (!fs::exists(document) || document.extension() != ".md")
            continue;
        const auto meta = parse_frontmatter(document);
        if (!meta.has_frontmatter || !meta.closed_frontmatter || !meta.has_title || meta.tags.empty())
            continue;
        for (const auto& tag : meta.tags) {
            if (!valid_tag_name(tag))
                continue;
            const fs::path tag_dir = tags_root / from_utf8(tag);
            const fs::path link_path = tag_dir / from_utf8(safe_filename(meta.title) + ".md");
            std::error_code ec;
            const bool missing_entry =
                fs::symlink_status(link_path, ec).type() == fs::file_type::not_found;
            const auto current_target =
                missing_entry ? std::optional<fs::path>{}
                              : tag_entry_target(link_path, tag_dir, archive_root);
            const bool dangling_entry = current_target && !fs::exists(*current_target);
            if (missing_entry || dangling_entry)
                create_tag_entry(document, tag, meta.title, false);
        }
    }
}

std::vector<std::string> TagManager::list_tags() const {
    std::vector<std::string> tags;
    if (!fs::exists(tags_root))
        return tags;

    for (const auto& entry : fs::directory_iterator(tags_root)) {
        if (entry.is_directory()) {
            tags.push_back(to_utf8(entry.path().filename()));
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

std::vector<TagManager::ListedDoc> TagManager::list_doc_entries_for_tag(const std::string& tag) const {
    const auto docs = collect_docs_for_tag(tag);
    std::vector<ListedDoc> result;
    result.reserve(docs.size());
    for (const auto& doc : docs)
        result.push_back({doc.title, cfg.workspace / doc.rel_path});
    return result;
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

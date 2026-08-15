export module md_archive.archive_store;

import std;
import md_archive.content_hash;
import md_archive.path_encoding;

export namespace md_archive {

struct ArchiveRecord {
    std::string hash;
    std::filesystem::path object_path;
    bool object_created = false;
    bool source_already_known = false;
    std::optional<std::string> previous_hash;
};

class ArchiveStore {
  public:
    ArchiveStore(std::filesystem::path workspace, std::filesystem::path root)
        : workspace_(std::move(workspace)), root_(std::move(root)), objects_(root_ / "objects"),
          index_path_(root_ / "index.tsv") {
        std::filesystem::create_directories(objects_);
        recover_object_backups();
        recover_index_backup();
        load();
        migrate_legacy_files();
    }

    std::optional<ArchiveRecord> store(const std::filesystem::path& source) {
        auto hash = sha256_file(source);
        if (!hash)
            return std::nullopt;

        const auto key = source_key(source);
        ArchiveRecord result{*hash, object_path(*hash), false, false, std::nullopt};
        if (auto found = sources_.find(key); found != sources_.end()) {
            result.source_already_known = found->second == *hash;
            if (found->second != *hash)
                result.previous_hash = found->second;
        }

        std::error_code ec;
        const auto stored_hash = sha256_file(result.object_path);
        if (!stored_hash || *stored_hash != *hash) {
            std::filesystem::create_directories(result.object_path.parent_path(), ec);
            if (ec)
                return std::nullopt;
            auto temporary = result.object_path;
            temporary += ".tmp";
            auto backup = result.object_path;
            backup += ".bak";
            std::filesystem::copy_file(source, temporary,
                                       std::filesystem::copy_options::overwrite_existing, ec);
            if (ec)
                return std::nullopt;
            const auto copied_hash = sha256_file(temporary);
            if (!copied_hash || *copied_hash != *hash) {
                std::filesystem::remove(temporary, ec);
                return std::nullopt;
            }

            std::filesystem::remove(backup, ec);
            ec.clear();
            const bool had_object = std::filesystem::exists(result.object_path);
            if (had_object) {
                std::filesystem::rename(result.object_path, backup, ec);
                if (ec) {
                    std::filesystem::remove(temporary);
                    return std::nullopt;
                }
            }
            std::filesystem::rename(temporary, result.object_path, ec);
            if (ec) {
                std::error_code restore_error;
                if (had_object)
                    std::filesystem::rename(backup, result.object_path, restore_error);
                return std::nullopt;
            }
            if (had_object) {
                ec.clear();
                std::filesystem::remove(backup, ec);
            }
            result.object_created = true;
        }

        // Every observed source path is durable archive metadata, even after
        // the user deletes or moves that source. Never infer a move merely
        // from equal content: several historical paths may legitimately share
        // one object. Only an explicit remove_source() may erase a mapping.
        sources_[key] = *hash;
        save();
        if (result.previous_hash)
            prune_object(*result.previous_hash);
        return result;
    }

    std::optional<std::string> hash_for_source(const std::filesystem::path& source) const {
        if (auto found = sources_.find(source_key(source)); found != sources_.end())
            return found->second;
        return std::nullopt;
    }

    std::vector<std::filesystem::path> sources_for_hash(const std::string& hash) const {
        std::vector<std::filesystem::path> result;
        for (const auto& [source, stored_hash] : sources_) {
            if (stored_hash == hash)
                result.push_back(workspace_ / path_encoding::from_utf8(source));
        }
        return result;
    }

    std::optional<std::string> hash_for_object(const std::filesystem::path& object) const {
        const auto filename = path_encoding::to_utf8(object.filename());
        if (filename.size() == 67 && filename.ends_with(".md"))
            return filename.substr(0, 64);
        return std::nullopt;
    }

    std::filesystem::path object_for_hash(const std::string& hash) const {
        return object_path(hash);
    }

    bool remove_source(const std::filesystem::path& source) {
        auto found = sources_.find(source_key(source));
        if (found == sources_.end())
            return false;
        const auto hash = found->second;
        sources_.erase(found);
        save();
        prune_object(hash);
        return true;
    }

    const std::map<std::string, std::string>& entries() const { return sources_; }

  private:
    std::filesystem::path workspace_;
    std::filesystem::path root_;
    std::filesystem::path objects_;
    std::filesystem::path index_path_;
    std::map<std::string, std::string> sources_;
    bool index_writable_ = true;

    static bool valid_hash(const std::string& hash) {
        return hash.size() == 64 && std::ranges::all_of(hash, [](unsigned char c) {
            return std::isdigit(c) || (c >= 'a' && c <= 'f');
        });
    }

    static bool valid_source_key(const std::string& source) {
        if (source.empty())
            return false;
        const auto path = path_encoding::from_utf8(source);
        if (path.empty() || path.is_absolute())
            return false;
        return std::ranges::none_of(path, [](const auto& part) { return part == ".."; });
    }

    std::string source_key(const std::filesystem::path& source) const {
        std::error_code ec;
        auto relative = std::filesystem::relative(source, workspace_, ec);
        if (ec)
            relative = source.lexically_relative(workspace_);
        return path_encoding::generic_to_utf8(relative.lexically_normal());
    }

    std::filesystem::path object_path(const std::string& hash) const {
        return objects_ / path_encoding::from_utf8(hash.substr(0, 2)) /
               path_encoding::from_utf8(hash + ".md");
    }

    void load() {
        std::ifstream input(index_path_, std::ios::binary);
        std::string line;
        std::size_t line_number = 0;
        while (std::getline(input, line)) {
            ++line_number;
            if (line.empty())
                continue;
            std::istringstream row(line);
            std::string hash;
            std::string source;
            if (!(row >> hash >> std::quoted(source)) || !valid_hash(hash) ||
                !valid_source_key(source)) {
                index_writable_ = false;
                std::cerr << "归档索引错误: index.tsv 第 " << line_number
                          << " 行无效；为防止数据丢失，本次禁止改写索引\n";
                continue;
            }
            row >> std::ws;
            if (!row.eof()) {
                index_writable_ = false;
                std::cerr << "归档索引错误: index.tsv 第 " << line_number
                          << " 行包含多余内容；为防止数据丢失，本次禁止改写索引\n";
                continue;
            }
            sources_[source] = hash;
        }
    }

    void recover_index_backup() const {
        auto backup = index_path_;
        backup += ".bak";
        std::error_code ec;
        if (!std::filesystem::exists(index_path_) && std::filesystem::exists(backup)) {
            std::filesystem::rename(backup, index_path_, ec);
            if (ec)
                throw std::filesystem::filesystem_error(
                    "cannot recover archive index backup", backup, index_path_, ec);
        } else if (std::filesystem::exists(index_path_) && std::filesystem::exists(backup)) {
            std::filesystem::remove(backup, ec);
        }
    }

    void recover_object_backups() const {
        std::error_code ec;
        std::filesystem::recursive_directory_iterator it(
            objects_, std::filesystem::directory_options::skip_permission_denied, ec);
        const std::filesystem::recursive_directory_iterator end;
        for (; it != end; it.increment(ec)) {
            if (ec) { ec.clear(); continue; }
            if (!it->is_regular_file(ec)) { ec.clear(); continue; }
            const auto path = it->path();
            if (path.extension() != ".bak")
                continue;
            auto original = path;
            original.replace_extension();
            if (!std::filesystem::exists(original)) {
                std::filesystem::rename(path, original, ec);
                if (ec)
                    throw std::filesystem::filesystem_error(
                        "cannot recover archive object backup", path, original, ec);
            } else {
                std::filesystem::remove(path, ec);
            }
            ec.clear();
        }
    }

    void save() const {
        if (!index_writable_)
            throw std::runtime_error(
                "refusing to overwrite malformed archive index.tsv");
        auto temporary = index_path_;
        temporary += ".tmp";
        auto backup = index_path_;
        backup += ".bak";
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output)
            throw std::filesystem::filesystem_error(
                "cannot create temporary archive index", temporary,
                std::make_error_code(std::errc::io_error));
        for (const auto& [source, hash] : sources_)
            output << hash << ' ' << std::quoted(source) << '\n';
        output.close();
        if (!output)
            throw std::filesystem::filesystem_error(
                "cannot write temporary archive index", temporary,
                std::make_error_code(std::errc::io_error));

        std::error_code ec;
        std::filesystem::remove(backup, ec);
        ec.clear();
        const bool had_index = std::filesystem::exists(index_path_);
        if (had_index) {
            std::filesystem::rename(index_path_, backup, ec);
            if (ec) {
                std::filesystem::remove(temporary);
                throw std::filesystem::filesystem_error(
                    "cannot preserve old archive index", index_path_, backup, ec);
            }
        }
        std::filesystem::rename(temporary, index_path_, ec);
        if (ec) {
            std::error_code restore_error;
            if (had_index)
                std::filesystem::rename(backup, index_path_, restore_error);
            throw std::filesystem::filesystem_error("cannot update archive index", temporary,
                                                    index_path_, ec);
        }
        if (had_index) {
            ec.clear();
            std::filesystem::remove(backup, ec);
        }
    }

    void prune_object(const std::string& hash) {
        const bool referenced = std::ranges::any_of(
            sources_, [&](const auto& item) { return item.second == hash; });
        if (referenced)
            return;
        std::error_code ec;
        const auto path = object_path(hash);
        std::filesystem::remove(path, ec);
        const auto parent = path.parent_path();
        if (!ec && std::filesystem::exists(parent) && std::filesystem::is_empty(parent))
            std::filesystem::remove(parent, ec);
    }

    void migrate_legacy_files() {
        std::vector<std::filesystem::path> legacy;
        std::error_code ec;
        std::filesystem::recursive_directory_iterator it(
            root_, std::filesystem::directory_options::skip_permission_denied, ec);
        const std::filesystem::recursive_directory_iterator end;
        for (; it != end; it.increment(ec)) {
            if (ec) { ec.clear(); continue; }
            if (it->path() == objects_) {
                it.disable_recursion_pending();
                continue;
            }
            if (!it->is_regular_file(ec)) { ec.clear(); continue; }
            if (it->path() == index_path_ || it->path().filename() == ".gitignore" ||
                it->path().extension() != ".md")
                continue;
            legacy.push_back(it->path());
        }

        bool changed = false;
        std::vector<std::filesystem::path> migrated_sources;
        for (const auto& path : legacy) {
            auto hash = sha256_file(path);
            if (!hash)
                continue;
            auto relative = std::filesystem::relative(path, root_, ec);
            if (ec) { ec.clear(); continue; }
            // Be conservative: an arbitrary Markdown file placed under
            // .archive is not enough evidence that it belongs to the legacy
            // path-mirrored format. Only migrate when the corresponding source
            // still exists; otherwise leave the file untouched for recovery.
            const auto legacy_source = workspace_ / relative;
            if (!std::filesystem::exists(legacy_source))
                continue;
            const auto destination = object_path(*hash);
            if (!std::filesystem::exists(destination)) {
                std::filesystem::create_directories(destination.parent_path(), ec);
                ec.clear();
                std::filesystem::copy_file(path, destination,
                                           std::filesystem::copy_options::overwrite_existing, ec);
                if (ec) { ec.clear(); continue; }
            }
            sources_[path_encoding::generic_to_utf8(relative)] = *hash;
            migrated_sources.push_back(path);
            changed = true;
        }
        if (changed) {
            save();
            for (const auto& path : migrated_sources) {
                std::filesystem::remove(path, ec);
                ec.clear();
            }
        }
    }
};

} // namespace md_archive

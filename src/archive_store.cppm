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
        if (!std::filesystem::exists(result.object_path)) {
            std::filesystem::create_directories(result.object_path.parent_path(), ec);
            ec.clear();
            std::filesystem::copy_file(source, result.object_path,
                                       std::filesystem::copy_options::overwrite_existing, ec);
            if (ec)
                return std::nullopt;
            result.object_created = true;
        }

        // If an indexed path disappeared and the same content appeared under
        // a new path, treat it as a move. Existing paths remain as aliases,
        // which represents a real copy without duplicating the object.
        for (auto it = sources_.begin(); it != sources_.end();) {
            if (it->second == *hash && it->first != key &&
                !std::filesystem::exists(workspace_ / path_encoding::from_utf8(it->first)))
                it = sources_.erase(it);
            else
                ++it;
        }
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
        std::string hash;
        std::string source;
        while (input >> hash >> std::quoted(source)) {
            if (hash.size() == 64 && !source.empty())
                sources_[source] = hash;
        }
    }

    void save() const {
        const auto temporary = index_path_.string() + ".tmp";
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        for (const auto& [source, hash] : sources_)
            output << hash << ' ' << std::quoted(source) << '\n';
        output.close();
        std::error_code ec;
        std::filesystem::remove(index_path_, ec);
        ec.clear();
        std::filesystem::rename(temporary, index_path_, ec);
        if (ec)
            throw std::filesystem::filesystem_error("cannot update archive index", temporary,
                                                    index_path_, ec);
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
        for (const auto& path : legacy) {
            auto hash = sha256_file(path);
            if (!hash)
                continue;
            auto relative = std::filesystem::relative(path, root_, ec);
            if (ec) { ec.clear(); continue; }
            const auto destination = object_path(*hash);
            if (!std::filesystem::exists(destination)) {
                std::filesystem::create_directories(destination.parent_path(), ec);
                ec.clear();
                std::filesystem::copy_file(path, destination,
                                           std::filesystem::copy_options::overwrite_existing, ec);
                if (ec) { ec.clear(); continue; }
            }
            sources_[path_encoding::generic_to_utf8(relative)] = *hash;
            std::filesystem::remove(path, ec);
            ec.clear();
            changed = true;
        }
        if (changed)
            save();
    }
};

} // namespace md_archive

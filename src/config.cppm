export module md_archive.config;

import std;
import md_archive.path_encoding;

/**
 * @brief Global configuration overrides parsed before command dispatch.
 *
 * @details These values come from process-level CLI options such as
 * `--config <path>` and `--workspace <path>`. They are intentionally separate
 * from Config so command handlers can decide whether configuration loading is
 * required at all.
 *
 * 中文：命令分发前解析出的全局配置覆盖项。
 *
 * 这些值来自 `--config <path>` 和 `--workspace <path>` 等全局 CLI 参数。
 */
export struct ConfigOptions {
    /// Explicit config file path from `--config`.
    /// 来自 `--config` 的显式配置文件路径。
    std::optional<std::filesystem::path> config_path;

    /// Temporary workspace override from `--workspace`.
    /// 来自 `--workspace` 的临时 workspace 覆盖。
    std::optional<std::filesystem::path> workspace_override;
};

/**
 * @brief Effective archive configuration.
 *
 * @details `workspace` is always normalized to an existing absolute directory.
 * `tags_dir` and `archive_dir` are workspace-relative paths and are validated
 * to reject absolute paths and `..` path escapes.
 *
 * 中文：归档系统最终生效的配置。
 *
 * `workspace` 会被规范化为已存在的绝对目录；`tags_dir` 与 `archive_dir`
 * 必须是 workspace 内部的相对路径。
 */
export struct Config {
    /// Workspace root containing source Markdown files.
    /// 保存源 Markdown 文件的工作区根目录。
    std::filesystem::path workspace;

    /// Tag navigation directory, relative to workspace.
    /// 标签导航目录，相对 workspace。
    std::filesystem::path tags_dir;

    /// Hash-addressed archive object directory, relative to workspace.
    /// 哈希寻址归档对象目录，相对 workspace。
    std::filesystem::path archive_dir;

    /// Config file used to produce this Config, if one was found.
    /// 产生当前配置的配置文件路径；使用内置默认值时为空。
    std::optional<std::filesystem::path> config_path;

    /// True when no config file was found and built-in defaults were used.
    /// 未找到配置文件并使用内置默认值时为 true。
    bool used_default_config = false;

    /// @return Absolute path to the tag navigation root.
    /// @return 标签导航根目录的绝对路径。
    [[nodiscard]]
    std::filesystem::path tags_root() const;

    /// @return Absolute path to the durable archive root.
    /// @return 持久归档根目录的绝对路径。
    [[nodiscard]]
    std::filesystem::path archive_root() const;

    /**
     * @brief Load and validate one config file.
     *
     * @param config_path Config file path.
     * @param workspace_override Optional workspace override from CLI.
     * @return Validated Config, or `std::nullopt` after printing a clear error.
     *
     * 中文：读取并校验单个配置文件。
     */
    [[nodiscard]]
    static std::optional<Config>
    load(const std::filesystem::path& config_path,
         const std::optional<std::filesystem::path>& workspace_override = std::nullopt);

    /**
     * @brief Resolve the effective configuration using the project lookup order.
     *
     * Lookup order: explicit `--config`, current directory, parent directories,
     * user config directory, then built-in defaults.
     *
     * 中文：按项目查找顺序解析最终配置。
     */
    [[nodiscard]]
    static std::optional<Config> resolve(const ConfigOptions& options = {});

    /**
     * @brief Search for `config.ini` from a directory upward to filesystem root.
     *
     * 中文：从指定目录开始向上查找 `config.ini`。
     */
    [[nodiscard]]
    static std::optional<std::filesystem::path> find_config_from(const std::filesystem::path& start_dir);

    /**
     * @brief Return the platform-specific user config path.
     *
     * 中文：返回平台相关的用户配置文件路径。
     */
    [[nodiscard]]
    static std::filesystem::path user_config_path();

    /**
     * @brief Create `config.ini` in a directory.
     *
     * @param directory Target directory.
     * @param force Overwrite an existing config file when true.
     * @return Whether the file was written.
     *
     * 中文：在目录中创建 `config.ini`。
     */
    [[nodiscard]]
    static bool init_config(const std::filesystem::path& directory, bool force);

    /**
     * @brief Print the effective config in a stable machine-readable-ish form.
     *
     * 中文：以稳定、接近机器可读的形式打印最终配置。
     */
    static void print_effective(const Config& cfg);
};

namespace fs = std::filesystem;
using md_archive::path_encoding::from_utf8;
using md_archive::path_encoding::to_utf8;

namespace {

std::string trim(const std::string& s) {
    std::size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    std::size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return s.substr(start, end - start);
}

std::string strip_inline_comment(const std::string& s) {
    bool in_single = false;
    bool in_double = false;
    for (std::size_t i = 0; i < s.size(); ++i) {
        const char c = s[i];
        if (c == '\'' && !in_double) {
            in_single = !in_single;
        } else if (c == '"' && !in_single) {
            in_double = !in_double;
        } else if ((c == '#' || c == ';') && !in_single && !in_double) {
            return trim(s.substr(0, i));
        }
    }
    return trim(s);
}

std::string parse_value(const std::string& line) {
    auto eq = line.find('=');
    if (eq == std::string::npos) {
        return "";
    }
    auto value = strip_inline_comment(line.substr(eq + 1));
    if (value.size() >= 2 &&
        ((value.front() == '"' && value.back() == '"') || (value.front() == '\'' && value.back() == '\''))) {
        value = value.substr(1, value.size() - 2);
    }
    return value;
}

bool path_has_parent_escape(const fs::path& p) {
    for (const auto& part : p) {
        if (part == "..") {
            return true;
        }
    }
    return false;
}

std::optional<fs::path> canonical_existing_directory(const fs::path& path, const std::string& label) {
    std::error_code ec;
    auto absolute = fs::absolute(path, ec);
    if (ec) {
        std::cerr << "配置错误: 无法解析 " << label << ": " << to_utf8(path) << " (" << ec.message() << ")\n";
        return std::nullopt;
    }
    auto canonical = fs::weakly_canonical(absolute, ec);
    if (ec) {
        std::cerr << "配置错误: 无法规范化 " << label << ": " << to_utf8(absolute) << " (" << ec.message() << ")\n";
        return std::nullopt;
    }
    if (!fs::exists(canonical) || !fs::is_directory(canonical)) {
        std::cerr << "配置错误: " << label << " 不存在或不是目录: " << to_utf8(canonical) << "\n";
        return std::nullopt;
    }
    return canonical;
}

bool validate_relative_dir(const fs::path& value, const char* key) {
    if (value.empty()) {
        std::cerr << "配置错误: " << key << " 不能为空\n";
        return false;
    }
    if (value.is_absolute() || path_has_parent_escape(value)) {
        std::cerr << "配置错误: " << key
                  << " 必须是 workspace 内的相对路径，不能使用绝对路径或 '..': " << to_utf8(value) << "\n";
        return false;
    }
    return true;
}

void write_example_config(std::ostream& out, const fs::path& workspace) {
    out << "[archive]\n";
    out << "# Workspace root for your Markdown notes / Markdown 工作区根目录\n";
    out << "workspace = " << to_utf8(workspace) << "\n";
    out << "# Tag navigation directory, relative to workspace / 标签导航目录，相对 workspace\n";
    out << "tags_dir = .tags\n";
    out << "# Hash-addressed archive directory, relative to workspace / 哈希寻址归档目录，相对 workspace\n";
    out << "archive_dir = .archive\n";
}

} // namespace

fs::path Config::tags_root() const {
    return workspace / tags_dir;
}

fs::path Config::archive_root() const {
    return workspace / archive_dir;
}

std::optional<Config> Config::load(const fs::path& requested_config_path,
                                   const std::optional<fs::path>& workspace_override) {
    std::error_code ec;
    fs::path actual_config_path = fs::weakly_canonical(requested_config_path, ec);
    if (ec || !fs::exists(actual_config_path)) {
        std::cerr << "配置错误: 找不到配置文件 " << to_utf8(requested_config_path) << "\n";
        return std::nullopt;
    }

    std::ifstream in(actual_config_path);
    if (!in.is_open()) {
        std::cerr << "配置错误: 无法读取配置文件 " << to_utf8(actual_config_path) << "\n";
        return std::nullopt;
    }

    Config cfg;
    cfg.config_path = actual_config_path;
    cfg.tags_dir = ".tags";
    cfg.archive_dir = ".archive";

    bool in_archive = false;
    bool has_workspace = false;
    std::string line;

    while (std::getline(in, line)) {
        line = strip_inline_comment(trim(line));
        if (line.empty()) {
            continue;
        }

        if (line.front() == '[') {
            in_archive = (line == "[archive]");
            continue;
        }
        if (!in_archive) {
            continue;
        }

        if (line.starts_with("workspace")) {
            cfg.workspace = from_utf8(parse_value(line));
            has_workspace = true;
        } else if (line.starts_with("tags_dir")) {
            cfg.tags_dir = from_utf8(parse_value(line));
        } else if (line.starts_with("archive_dir")) {
            cfg.archive_dir = from_utf8(parse_value(line));
        }
    }

    if (workspace_override) {
        cfg.workspace = *workspace_override;
        has_workspace = true;
    } else if (has_workspace && cfg.workspace.is_relative()) {
        cfg.workspace = actual_config_path.parent_path() / cfg.workspace;
    } else if (!has_workspace) {
        cfg.workspace = actual_config_path.parent_path();
    }

    auto canonical_workspace = canonical_existing_directory(cfg.workspace, "workspace");
    if (!canonical_workspace) {
        return std::nullopt;
    }
    cfg.workspace = *canonical_workspace;

    if (!validate_relative_dir(cfg.tags_dir, "tags_dir") ||
        !validate_relative_dir(cfg.archive_dir, "archive_dir")) {
        return std::nullopt;
    }

    if (cfg.tags_dir == cfg.archive_dir) {
        std::cerr << "配置错误: tags_dir 和 archive_dir 不能相同\n";
        return std::nullopt;
    }

    return cfg;
}

std::optional<Config> Config::resolve(const ConfigOptions& options) {
    if (options.config_path) {
        return load(*options.config_path, options.workspace_override);
    }

    if (auto found = find_config_from(fs::current_path())) {
        return load(*found, options.workspace_override);
    }

    auto user_config = user_config_path();
    if (fs::exists(user_config)) {
        return load(user_config, options.workspace_override);
    }

    Config cfg;
    cfg.workspace = options.workspace_override.value_or(fs::current_path());
    auto canonical_workspace = canonical_existing_directory(cfg.workspace, "workspace");
    if (!canonical_workspace) {
        return std::nullopt;
    }
    cfg.workspace = *canonical_workspace;
    cfg.tags_dir = ".tags";
    cfg.archive_dir = ".archive";
    cfg.used_default_config = true;
    return cfg;
}

std::optional<fs::path> Config::find_config_from(const fs::path& start_dir) {
    std::error_code ec;
    fs::path dir = fs::weakly_canonical(start_dir, ec);
    if (ec) {
        dir = fs::absolute(start_dir);
    }

    while (true) {
        fs::path candidate = dir / "config.ini";
        if (fs::exists(candidate)) {
            return candidate;
        }
        if (dir == dir.root_path()) {
            break;
        }
        auto parent = dir.parent_path();
        if (parent == dir || parent.empty()) {
            break;
        }
        dir = parent;
    }
    return std::nullopt;
}

fs::path Config::user_config_path() {
#ifdef _WIN32
    if (const char* appdata = std::getenv("APPDATA")) {
        return fs::path(appdata) / "md-archive" / "config.ini";
    }
    return fs::path("md-archive") / "config.ini";
#else
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME")) {
        return fs::path(xdg) / "md-archive" / "config.ini";
    }
    if (const char* home = std::getenv("HOME")) {
        return fs::path(home) / ".config" / "md-archive" / "config.ini";
    }
    return fs::path(".config") / "md-archive" / "config.ini";
#endif
}

bool Config::init_config(const fs::path& directory, bool force) {
    std::error_code ec;
    fs::path dir = fs::weakly_canonical(directory, ec);
    if (ec) {
        dir = fs::absolute(directory);
    }

    if (!fs::exists(dir)) {
        std::cerr << "错误: 目录不存在: " << to_utf8(dir) << "\n";
        return false;
    }
    fs::path path = dir / "config.ini";
    if (fs::exists(path) && !force) {
        std::cerr << "错误: config.ini 已存在: " << to_utf8(path) << " (使用 --force 覆盖)\n";
        return false;
    }

    std::ofstream out(path);
    if (!out.is_open()) {
        std::cerr << "错误: 无法写入配置文件: " << to_utf8(path) << "\n";
        return false;
    }
    write_example_config(out, dir);
    std::cout << "已生成配置文件: " << to_utf8(path) << "\n";
    return true;
}

void Config::print_effective(const Config& cfg) {
    std::cout << "config_file = ";
    if (cfg.config_path) {
        std::cout << to_utf8(*cfg.config_path);
    } else {
        std::cout << "(not found; using built-in defaults)";
    }
    std::cout << "\n";
    std::cout << "workspace = " << to_utf8(cfg.workspace) << "\n";
    std::cout << "tags_dir = " << to_utf8(cfg.tags_dir) << "\n";
    std::cout << "archive_dir = " << to_utf8(cfg.archive_dir) << "\n";
}

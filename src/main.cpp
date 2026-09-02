#ifdef _WIN32
// clang-format off: shellapi.h depends on declarations from windows.h.
#include <windows.h>
#include <shellapi.h>
// clang-format on
#else
#include <unistd.h>
#endif

import std;
import md_archive.path_encoding;
import md_archive.config;
import md_archive.tag_manager;

namespace fs = std::filesystem;

using md_archive::path_encoding::from_utf8;
using md_archive::path_encoding::generic_to_utf8;
using md_archive::path_encoding::to_utf8;

#ifdef _WIN32
std::string wide_to_utf8(std::wstring_view value) {
    if (value.empty())
        return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0,
                                         nullptr, nullptr);
    if (size <= 0)
        throw std::runtime_error("无法将 Windows 命令行转换为 UTF-8");
    std::string result(static_cast<std::size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), size,
                        nullptr, nullptr);
    return result;
}

std::vector<std::string> windows_utf8_args() {
    int count = 0;
    LPWSTR* values = CommandLineToArgvW(GetCommandLineW(), &count);
    if (!values)
        throw std::runtime_error("无法读取 Windows 命令行");
    std::vector<std::string> result;
    result.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i)
        result.push_back(wide_to_utf8(values[i]));
    LocalFree(values);
    return result;
}
#endif

namespace {

constexpr int exit_success = 0;
constexpr int exit_invalid_arguments = 2;
constexpr int exit_config_error = 3;
constexpr int exit_filesystem_error = 4;

#ifndef MD_ARCHIVE_VERSION
#define MD_ARCHIVE_VERSION "0.0.0-dev"
#endif

constexpr const char* version = MD_ARCHIVE_VERSION;

struct Colors {
    bool enabled = false;
    std::string_view reset() const {
        return enabled ? "\x1b[0m" : "";
    }
    std::string_view bold() const {
        return enabled ? "\x1b[1m" : "";
    }
    std::string_view green() const {
        return enabled ? "\x1b[32m" : "";
    }
    std::string_view cyan() const {
        return enabled ? "\x1b[36m" : "";
    }
    std::string_view yellow() const {
        return enabled ? "\x1b[33m" : "";
    }
    std::string_view dim() const {
        return enabled ? "\x1b[2m" : "";
    }
};

Colors terminal_colors() {
    Colors colors;
    if (std::getenv("NO_COLOR"))
        return colors;
#ifdef _WIN32
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    colors.enabled = output != INVALID_HANDLE_VALUE && GetConsoleMode(output, &mode) &&
                     SetConsoleMode(output, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#else
    colors.enabled = isatty(STDOUT_FILENO) != 0;
#endif
    return colors;
}

struct ParsedArgs {
    ConfigOptions config_options;
    std::vector<std::string> command_args;
};

void print_usage(const char* prog) {
    std::cout << "md-archive " << version << "\n\n";
    std::cout << "用法 / Usage:\n";
    std::cout << "  " << prog << " [--config <path>] [--workspace <path>] <command> [args]\n\n";
    std::cout << "全局参数 / Global options:\n";
    std::cout << "  --config <path>       使用指定 config.ini\n";
    std::cout << "  --workspace <path>    临时覆盖配置中的 workspace\n";
    std::cout << "  -h, --help            显示帮助\n";
    std::cout << "  --version             显示版本\n\n";
    std::cout << "命令 / Commands:\n";
    std::cout << "  init [--force]             在当前目录生成 config.ini\n";
    std::cout << "  config show                显示最终生效配置\n";
    std::cout << "  config path                显示实际使用的配置文件路径\n";
    std::cout << "  add <file.md> [-f]         归档一个 Markdown 文件\n";
    std::cout << "  scan [--force]             扫描工作区所有 .md 文件并归档\n";
    std::cout << "  list [-exact] [tag]        列出标签；默认包含子标签，-exact 仅精确匹配\n";
    std::cout << "  docs                       列出所有已归档文档\n";
    std::cout << "  search <word>              按标题或文件名搜索\n";
    std::cout << "  search -all <word>         搜索 Markdown 全文\n";
    std::cout << "  remove <file.md>           从归档中移除文件\n";
    std::cout << "  rebuild                    整理标签符号链接并清理旧索引\n";
}

std::optional<ParsedArgs> parse_args(int argc, char* argv[]) {
    ParsedArgs parsed;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--config") {
            if (i + 1 >= argc) {
                std::cerr << "参数错误: --config 需要路径\n";
                return std::nullopt;
            }
            parsed.config_options.config_path = from_utf8(argv[++i]);
        } else if (arg == "--workspace") {
            if (i + 1 >= argc) {
                std::cerr << "参数错误: --workspace 需要路径\n";
                return std::nullopt;
            }
            parsed.config_options.workspace_override = from_utf8(argv[++i]);
        } else {
            parsed.command_args.push_back(arg);
        }
    }
    return parsed;
}

bool has_flag(const std::vector<std::string>& args, const std::string& long_name,
              const std::string& short_name = "") {
    for (const auto& arg : args) {
        if (arg == long_name || (!short_name.empty() && arg == short_name)) {
            return true;
        }
    }
    return false;
}

std::optional<Config> load_config_or_report(const ConfigOptions& options) {
    auto cfg = Config::resolve(options);
    if (!cfg) {
        // Config::resolve() has already reported the concrete lookup, parse, or
        // validation error.  Suggesting `init` here is actively misleading when
        // a config file was found but contains an invalid value.
        return std::nullopt;
    }
    if (cfg->used_default_config) {
        std::cerr
            << "提示: 未找到 config.ini，使用当前目录作为 workspace。可运行 `md-archive init` 固化配置。\n";
    }
    return cfg;
}

std::optional<fs::path> parse_file_arg(const std::vector<std::string>& args) {
    for (const auto& arg : args) {
        if (arg == "--force" || arg == "-f") {
            continue;
        }
        if (!arg.starts_with("-")) {
            return fs::absolute(from_utf8(arg));
        }
    }
    return std::nullopt;
}

std::string normalize_list_tag_arg(std::string tag) {
#ifdef _WIN32
    // MSYS2 may path-convert a relative `A/B` argument before a native program
    // receives it. If that conversion produced an absolute path below the
    // invocation directory, recover the original Unix-style tag spelling.
    const fs::path converted = from_utf8(tag);
    if (converted.is_absolute()) {
        const auto relative = converted.lexically_normal().lexically_relative(fs::current_path());
        if (!relative.empty() && *relative.begin() != "..")
            return generic_to_utf8(relative);
    }
#endif
    return tag;
}

} // namespace

int run_main(int argc, char* argv[]) {
#ifdef _WIN32
    auto utf8_args = windows_utf8_args();
    std::vector<char*> utf8_argv;
    utf8_argv.reserve(utf8_args.size());
    for (auto& arg : utf8_args)
        utf8_argv.push_back(arg.data());
    argc = static_cast<int>(utf8_argv.size());
    argv = utf8_argv.data();
#endif
    auto parsed = parse_args(argc, argv);
    if (!parsed) {
        print_usage(argv[0]);
        return exit_invalid_arguments;
    }

    auto& args = parsed->command_args;
    if (args.empty() || args[0] == "help" || args[0] == "--help" || args[0] == "-h") {
        print_usage(argv[0]);
        return args.empty() ? exit_invalid_arguments : exit_success;
    }
    if (args[0] == "--version" || args[0] == "version") {
        std::cout << "md-archive " << version << "\n";
        return exit_success;
    }

    const std::string cmd = args[0];

    if (cmd == "init") {
        bool force = has_flag(args, "--force", "-f");
        return Config::init_config(fs::current_path(), force) ? exit_success : exit_filesystem_error;
    }

    if (cmd == "config") {
        if (args.size() < 2) {
            std::cerr << "用法: " << argv[0] << " config <show|path>\n";
            return exit_invalid_arguments;
        }
        auto cfg = load_config_or_report(parsed->config_options);
        if (!cfg)
            return exit_config_error;

        if (args[1] == "show") {
            Config::print_effective(*cfg);
            return exit_success;
        }
        if (args[1] == "path") {
            if (cfg->config_path) {
                std::cout << to_utf8(*cfg->config_path) << "\n";
            } else {
                std::cout << "(no config.ini found; using built-in defaults)\n";
            }
            return exit_success;
        }
        std::cerr << "未知 config 子命令: " << args[1] << "\n";
        return exit_invalid_arguments;
    }

    auto cfg = load_config_or_report(parsed->config_options);
    if (!cfg)
        return exit_config_error;
    TagManager tm(*cfg);

    if (cmd == "add") {
        std::vector<std::string> add_args(args.begin() + 1, args.end());
        auto file_path = parse_file_arg(add_args);
        if (!file_path) {
            std::cerr << "用法: " << argv[0] << " add <file.md> [-f|--force]\n";
            return exit_invalid_arguments;
        }
        if (!fs::exists(*file_path)) {
            std::cerr << "错误: 文件不存在: " << *file_path << "\n";
            return exit_filesystem_error;
        }
        bool force = has_flag(args, "--force", "-f");
        tm.archive(*file_path, force);
        return exit_success;
    }

    if (cmd == "remove" || cmd == "rm") {
        if (args.size() < 2) {
            std::cerr << "用法: " << argv[0] << " remove <file.md>\n";
            return exit_invalid_arguments;
        }
        tm.remove(fs::absolute(from_utf8(args[1])));
        return exit_success;
    }

    if (cmd == "scan") {
        bool force = has_flag(args, "--force", "-f");
        tm.scan_all(force);
        return exit_success;
    }

    if (cmd == "list") {
        const bool exact = has_flag(args, "-exact", "--exact");
        std::vector<std::string> list_args;
        for (std::size_t i = 1; i < args.size(); ++i) {
            if (args[i] == "-exact" || args[i] == "--exact")
                continue;
            if (args[i].starts_with("-")) {
                std::cerr << "未知 list 参数: " << args[i] << "\n";
                return exit_invalid_arguments;
            }
            list_args.push_back(args[i]);
        }
        if (list_args.size() > 1 || (exact && list_args.empty())) {
            std::cerr << "用法: " << argv[0] << " list [-exact|--exact] [tag]\n";
            return exit_invalid_arguments;
        }

        if (!list_args.empty()) {
            std::string tag = normalize_list_tag_arg(list_args.front());
            auto docs = tm.list_doc_entries_for_tag(tag, exact);
            std::cout << "标签 [" << tag << "] 下的文档" << (exact ? "（精确匹配）" : "（包含子标签）")
                      << ":\n";
            if (docs.empty()) {
                std::cout << "  (无文档)\n";
            } else {
                for (std::size_t i = 0; i < docs.size(); ++i) {
                    std::cout << "  " << (i + 1) << ". " << docs[i].title << " -> " << to_utf8(docs[i].path)
                              << "\n";
                }
                std::cout << "\n共 " << docs.size() << " 篇\n";
                std::cout << "标签目录: " << generic_to_utf8(cfg->workspace / cfg->tags_dir) << "/" << tag
                          << "\n";
            }
        } else {
            auto tags = tm.list_tags();
            std::cout << "所有标签:\n";
            if (tags.empty()) {
                std::cout << "  (暂无标签)\n";
            } else {
                for (std::size_t i = 0; i < tags.size(); ++i) {
                    auto docs = tm.list_docs_for_tag(tags[i]);
                    std::cout << "  " << (i + 1) << ". " << tags[i] << " (" << docs.size() << " 篇)\n";
                }
                std::cout << "\n共 " << tags.size() << " 个标签\n";
                std::cout << "标签目录: " << generic_to_utf8(cfg->workspace / cfg->tags_dir) << "\n";
            }
        }
        return exit_success;
    }

    if (cmd == "docs") {
        auto docs = tm.list_all_archived();
        std::cout << "所有已归档文档:\n";
        if (docs.empty()) {
            std::cout << "  (暂无归档文档)\n";
        } else {
            for (std::size_t i = 0; i < docs.size(); ++i) {
                auto rel = fs::relative(docs[i], cfg->workspace);
                std::cout << "  " << (i + 1) << ". " << to_utf8(rel) << "\n";
            }
            std::cout << "\n共 " << docs.size() << " 篇\n";
        }
        return exit_success;
    }

    if (cmd == "search") {
        const bool full_text = has_flag(args, "-all") || has_flag(args, "--all");
        std::vector<std::string> words;
        for (std::size_t i = 1; i < args.size(); ++i)
            if (args[i] != "-all" && args[i] != "--all")
                words.push_back(args[i]);
        if (words.size() != 1 || words.front().empty()) {
            std::cerr << "用法: " << argv[0] << " search [-all|--all] <word>\n";
            return exit_invalid_arguments;
        }

        const auto colors = terminal_colors();
        const auto results = tm.search(words.front(), full_text);
        std::cout << colors.bold() << (full_text ? "全文搜索" : "名称搜索") << colors.reset() << ": "
                  << colors.yellow() << words.front() << colors.reset() << "\n\n";
        if (results.empty()) {
            std::cout << "  (没有匹配的文档)\n";
        } else {
            for (std::size_t i = 0; i < results.size(); ++i) {
                const auto rel = fs::relative(results[i].path, cfg->workspace);
                std::cout << colors.green() << colors.bold() << "  " << (i + 1) << ". " << results[i].title
                          << colors.reset() << "\n"
                          << colors.dim() << "     路径  " << to_utf8(rel) << colors.reset() << "\n";
                if (!results[i].tags.empty()) {
                    std::cout << colors.cyan() << "     标签  ";
                    for (std::size_t j = 0; j < results[i].tags.size(); ++j) {
                        if (j)
                            std::cout << ", ";
                        std::cout << results[i].tags[j];
                    }
                    std::cout << colors.reset() << "\n";
                }
                if (!results[i].preview.empty())
                    std::cout << colors.yellow() << "     匹配  " << results[i].preview << colors.reset()
                              << "\n";
                std::cout << "\n";
            }
            std::cout << "共 " << results.size() << " 篇匹配\n";
        }
        return exit_success;
    }

    if (cmd == "rebuild") {
        tm.rebuild_all_links();
        return exit_success;
    }

    std::cerr << "未知命令: " << cmd << "\n";
    print_usage(argv[0]);
    return exit_invalid_arguments;
}

int main(int argc, char* argv[]) {
    try {
        return run_main(argc, argv);
    } catch (const std::filesystem::filesystem_error& error) {
        std::cerr << "文件系统错误: " << error.what() << "\n";
        return exit_filesystem_error;
    } catch (const std::exception& error) {
        std::cerr << "错误: " << error.what() << "\n";
        return exit_filesystem_error;
    }
}

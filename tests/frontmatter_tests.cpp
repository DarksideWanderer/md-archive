import std;
import md_archive.frontmatter;

namespace fs = std::filesystem;

namespace {

int failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << "\n";
        ++failures;
    }
}

fs::path write_note(const fs::path& dir, const std::string& name, const std::string& body) {
    fs::path path = dir / name;
    std::ofstream out(path);
    out << body;
    return path;
}

} // namespace

int main() {
    fs::path dir = fs::temp_directory_path() / "md_archive_frontmatter_tests";
    fs::remove_all(dir);
    fs::create_directories(dir);

    auto inline_tags = parse_frontmatter(write_note(dir, "inline.md",
                                                    "---\n"
                                                    "tags: [算法, 图论]\n"
                                                    "title: Dijkstra 最短路径\n"
                                                    "---\n"
                                                    "# Body\n"));
    expect(inline_tags.has_frontmatter, "inline tags should detect frontmatter");
    expect(inline_tags.closed_frontmatter, "inline tags should close frontmatter");
    expect(inline_tags.has_title, "inline tags should detect title");
    expect(inline_tags.title == "Dijkstra 最短路径", "inline title should parse");
    expect(inline_tags.tags.size() == 2, "inline tags should parse two tags");
    expect(inline_tags.tags[0] == "算法", "first inline tag should match");

    auto block_tags = parse_frontmatter(write_note(dir, "block.md",
                                                   "---\n"
                                                   "tags:\n"
                                                   "  - 数据结构\n"
                                                   "  - 树\n"
                                                   "title: Segment Tree\n"
                                                   "---\n"));
    expect(block_tags.tags.size() == 2, "block tags should parse two tags");
    expect(block_tags.tags[1] == "树", "second block tag should match");

    auto missing_tags = parse_frontmatter(write_note(dir, "missing-tags.md",
                                                     "---\n"
                                                     "title: Missing Tags\n"
                                                     "---\n"));
    expect(missing_tags.has_frontmatter, "missing tags file should still have frontmatter");
    expect(missing_tags.tags.empty(), "missing tags should be empty");

    auto missing_title = parse_frontmatter(write_note(dir, "missing-title.md",
                                                      "---\n"
                                                      "tags: [算法]\n"
                                                      "---\n"));
    expect(!missing_title.has_title, "missing title should be reported");

    auto no_frontmatter = parse_frontmatter(write_note(dir, "plain.md", "# Plain\n"));
    expect(!no_frontmatter.has_frontmatter, "plain markdown should not report frontmatter");

    auto bom = parse_frontmatter(write_note(dir, "bom.md",
                                            "\xEF\xBB\xBF"
                                            "---\n"
                                            "tags: [UTF-8]\n"
                                            "title: BOM note\n"
                                            "---\n"));
    expect(bom.has_frontmatter && bom.closed_frontmatter,
           "UTF-8 BOM should not hide frontmatter");
    expect(bom.has_title && bom.title == "BOM note", "BOM title should parse");

    fs::remove_all(dir);
    return failures == 0 ? 0 : 1;
}

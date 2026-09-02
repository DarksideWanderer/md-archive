import std;
import md_archive.content_hash;

int main() {
    const auto path = std::filesystem::temp_directory_path() / "md_archive_sha256_test.txt";
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << "abc";
    }
    const auto hash = md_archive::sha256_file(path);
    std::error_code ec;
    std::filesystem::remove(path, ec);
    if (!hash || *hash != "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad") {
        std::cerr << "unexpected SHA-256: " << hash.value_or("<read failure>") << "\n";
        return 1;
    }

    const auto lf_path = std::filesystem::temp_directory_path() / "md_archive_lf_test.md";
    const auto crlf_path = std::filesystem::temp_directory_path() / "md_archive_crlf_test.md";
    {
        std::ofstream lf(lf_path, std::ios::binary | std::ios::trunc);
        std::ofstream crlf(crlf_path, std::ios::binary | std::ios::trunc);
        lf << "line 1\nline 2\n";
        crlf << "line 1\r\nline 2\r\n";
    }
    const auto lf_hash = md_archive::sha256_markdown_file(lf_path);
    const auto crlf_hash = md_archive::sha256_markdown_file(crlf_path);
    std::filesystem::remove(lf_path, ec);
    std::filesystem::remove(crlf_path, ec);
    if (!lf_hash || lf_hash != crlf_hash) {
        std::cerr << "normalized Markdown hash differs between LF and CRLF\n";
        return 1;
    }
    return 0;
}

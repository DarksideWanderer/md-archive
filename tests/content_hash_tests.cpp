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
    return 0;
}

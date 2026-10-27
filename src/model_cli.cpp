#include "ps2asset/model/inspect.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;
namespace {
constexpr std::uintmax_t kMaxModelBytes = 64U * 1024U * 1024U;

bool is_mdl_path(const fs::path& path) {
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return extension == ".mdl";
}

bool inspect_path(const fs::path& path) {
    std::error_code ec;
    const auto size = fs::file_size(path, ec);
    if (ec) {
        std::cerr << "ERROR " << path.string() << ": file_size failed: " << ec.message() << '\n';
        return false;
    }
    if (size > kMaxModelBytes) {
        std::cerr << "ERROR " << path.string() << ": exceeds 64 MiB safety limit\n";
        return false;
    }

    std::ifstream input(path, std::ios::binary);
    if (!input) {
        std::cerr << "ERROR " << path.string() << ": cannot open file\n";
        return false;
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    if (!bytes.empty()) {
        input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (input.gcount() != static_cast<std::streamsize>(bytes.size())) {
            std::cerr << "ERROR " << path.string() << ": short read\n";
            return false;
        }
    }

    const auto result = ps2asset::model::inspect_model_bytes(bytes, path.string());
    std::cout << ps2asset::model::format_model_report(result) << "\n";
    return true;
}
}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: ps2asset-model-inspect <model.mdl-or-directory> [more paths...]\n"
                  << "Directories are scanned recursively for .mdl files. Inspection is read-only; "
                     "each file is limited to 64 MiB.\n";
        return 2;
    }

    std::vector<fs::path> paths;
    std::size_t errors = 0;
    for (int i = 1; i < argc; ++i) {
        const fs::path input(argv[i]);
        std::error_code ec;
        if (fs::is_regular_file(input, ec)) {
            paths.push_back(input);
        } else if (fs::is_directory(input, ec)) {
            fs::recursive_directory_iterator it(
                input, fs::directory_options::skip_permission_denied, ec), end;
            for (; !ec && it != end; it.increment(ec)) {
                std::error_code item_error;
                if (it->is_regular_file(item_error) && !item_error && is_mdl_path(it->path())) {
                    paths.push_back(it->path());
                }
            }
            if (ec) {
                std::cerr << "ERROR " << input.string() << ": directory traversal failed: "
                          << ec.message() << '\n';
                ++errors;
            }
        } else {
            std::cerr << "ERROR " << input.string() << ": not a regular file or directory\n";
            ++errors;
        }
    }

    std::sort(paths.begin(), paths.end());
    paths.erase(std::unique(paths.begin(), paths.end()), paths.end());
    if (paths.empty()) {
        std::cerr << "No model files selected.\n";
        return errors == 0 ? 2 : 1;
    }
    for (const auto& path : paths) {
        if (!inspect_path(path)) ++errors;
    }
    std::cout << "SUMMARY\nfiles=" << paths.size() << "\nerrors=" << errors << '\n';
    return errors == 0 ? 0 : 1;
}

#include "ps2asset/inspect.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

struct FileRow {
    std::string path;
    std::uintmax_t size = 0;
    ps2asset::InspectionResult result;
    std::string error;
};

FileRow inspect_path(const fs::path& path) {
    FileRow row;
    row.path = path.string();
    std::error_code ec;
    row.size = fs::file_size(path, ec);
    if (ec) {
        row.error = "file_size failed: " + ec.message();
        return row;
    }

    std::ifstream in(path, std::ios::binary);
    if (!in) {
        row.error = "cannot open file";
        return row;
    }
    const auto want = static_cast<std::streamsize>(std::min<std::uintmax_t>(row.size, 64));
    std::vector<std::uint8_t> prefix(static_cast<std::size_t>(want));
    if (want > 0) {
        in.read(reinterpret_cast<char*>(prefix.data()), want);
        if (in.gcount() != want) {
            row.error = "short read while reading header";
            return row;
        }
    }
    row.result = ps2asset::inspect_header(prefix, row.size);
    return row;
}

void print_row(const FileRow& row) {
    std::cout << row.path << "\t" << row.size << "\t";
    if (!row.error.empty()) {
        std::cout << "ERROR\t" << row.error << '\n';
        return;
    }
    std::cout << row.result.format << '\t';
    for (std::size_t i = 0; i < row.result.details.size(); ++i) {
        if (i) std::cout << "; ";
        std::cout << row.result.details[i];
    }
    std::cout << '\n';
}

int main(int argc, char** argv) {
    bool summary_only = false;
    int arg = 1;
    if (argc > 1 && std::string(argv[1]) == "--summary") {
        summary_only = true;
        ++arg;
    }
    if (arg >= argc) {
        std::cerr << "Usage: ps2asset-inspect [--summary] <file-or-directory> [more paths...]\n";
        return 2;
    }

    std::vector<FileRow> rows;
    for (int i = arg; i < argc; ++i) {
        const fs::path p(argv[i]);
        std::error_code ec;
        if (fs::is_regular_file(p, ec)) {
            rows.push_back(inspect_path(p));
        } else if (fs::is_directory(p, ec)) {
            fs::recursive_directory_iterator it(p, fs::directory_options::skip_permission_denied, ec), end;
            for (; !ec && it != end; it.increment(ec)) {
                if (it->is_regular_file(ec)) rows.push_back(inspect_path(it->path()));
                if (ec) ec.clear();
            }
            if (ec) {
                std::cerr << "Warning: directory traversal issue for " << p << ": " << ec.message() << '\n';
            }
        } else {
            FileRow row;
            row.path = p.string();
            row.error = "path is neither a regular file nor a directory";
            rows.push_back(row);
        }
    }

    std::sort(rows.begin(), rows.end(), [](const FileRow& a, const FileRow& b) {
        return a.path < b.path;
    });

    if (!summary_only) {
        std::cout << "path\tsize_bytes\tformat\tdetails\n";
        for (const auto& row : rows) print_row(row);
    }

    std::map<std::string, std::size_t> counts;
    std::size_t sl01_checked = 0, sl01_matches = 0, errors = 0;
    for (const auto& row : rows) {
        if (!row.error.empty()) { ++errors; continue; }
        ++counts[row.result.format];
        if (row.result.sl01_length_checked) {
            ++sl01_checked;
            if (row.result.sl01_length_matches) ++sl01_matches;
        }
    }
    std::cout << "\nSUMMARY\nfiles_seen=" << rows.size() << "\nerrors=" << errors
              << "\nsl01_length_checks=" << sl01_checked
              << "\nsl01_length_matches=" << sl01_matches << '\n';
    for (const auto& item : counts) std::cout << item.first << "=" << item.second << '\n';
    return errors ? 1 : 0;
}

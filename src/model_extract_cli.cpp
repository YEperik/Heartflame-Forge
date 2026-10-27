#include "ps2asset/model/containers.hpp"
#include "ps2asset/model/inspect.hpp"
#include "ps2asset/io/new_file_transaction.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;
namespace {

constexpr std::uintmax_t kMaxInputBytes = 64U * 1024U * 1024U;

std::string safe_stem(const fs::path& path) {
    std::string stem = path.stem().string();
    for (char& ch : stem) {
        const auto c = static_cast<unsigned char>(ch);
        if (!(std::isalnum(c) || ch == '_' || ch == '-')) ch = '_';
    }
    while (!stem.empty() && stem.front() == '.') stem.erase(stem.begin());
    return stem.empty() ? "model" : stem;
}

std::string hex_offset(std::uint64_t value) {
    std::ostringstream out;
    out << std::uppercase << std::hex << std::setfill('0')
        << std::setw(8) << value;
    return out.str();
}

bool read_input(const fs::path& path, std::vector<std::uint8_t>& bytes) {
    std::error_code ec;
    const auto size = fs::file_size(path, ec);
    if (ec) {
        std::cerr << "ERROR: cannot determine input file size: " << ec.message() << '\n';
        return false;
    }
    if (size > kMaxInputBytes) {
        std::cerr << "ERROR: input exceeds the 64 MiB safety limit\n";
        return false;
    }
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        std::cerr << "ERROR: cannot open input file\n";
        return false;
    }
    bytes.resize(static_cast<std::size_t>(size));
    if (!bytes.empty()) {
        input.read(reinterpret_cast<char*>(bytes.data()),
                   static_cast<std::streamsize>(bytes.size()));
        if (input.gcount() != static_cast<std::streamsize>(bytes.size())) {
            std::cerr << "ERROR: short read\n";
            return false;
        }
    }
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: ps2asset-model-extract-sl01 <input.mdl> <new-output-directory>\n"
                  << "Extracts bounded SL01 ranges only. It does not decode mesh/animation data.\n"
                  << "Input limit: 64 MiB; output directory must not already exist.\n";
        return 2;
    }

    const fs::path input_path(argv[1]);
    const fs::path output_dir(argv[2]);
    if (fs::exists(output_dir)) {
        std::cerr << "ERROR: output directory already exists; refusing to overwrite it\n";
        return 1;
    }

    std::vector<std::uint8_t> bytes;
    if (!read_input(input_path, bytes)) return 1;

    const auto scan = ps2asset::model::scan_embedded_sl01_blocks(bytes);
    if (scan.blocks.empty()) {
        std::cerr << "ERROR: no bounded SL01 blocks were validated\n";
        for (const auto& warning : scan.warnings) std::cerr << "  - " << warning << '\n';
        return 1;
    }

    std::error_code ec;
    if (!output_dir.parent_path().empty()) {
        fs::create_directories(output_dir.parent_path(), ec);
        if (ec) {
            std::cerr << "ERROR: cannot create output parent: " << ec.message() << '\n';
            return 1;
        }
    }
    if (!fs::create_directory(output_dir, ec) || ec) {
        std::cerr << "ERROR: cannot create new output directory: "
                  << (ec ? ec.message() : std::string("directory may already exist")) << '\n';
        return 1;
    }

    const std::string stem = safe_stem(input_path);
    const fs::path manifest_path = output_dir / "manifest.tsv";
    std::ostringstream manifest;
    manifest << "index\tfilename\tsource_offset_hex\tblock_bytes\tsl01_field08\tgap_after_bytes\n";
    std::vector<fs::path> created_files;
    bool manifest_created = false;
    std::size_t written = 0;
    try {
        for (std::size_t i = 0; i < scan.blocks.size(); ++i) {
            const auto& block = scan.blocks[i];
            const std::string filename = stem + "_sl01_" +
                (i < 10U ? "00" : (i < 100U ? "0" : "")) + std::to_string(i) +
                "_offset_" + hex_offset(block.offset) + ".mdl";
            const auto begin = bytes.begin() + static_cast<std::ptrdiff_t>(block.offset);
            const auto end = begin + static_cast<std::ptrdiff_t>(block.total_size);
            std::vector<std::uint8_t> extracted(begin, end);

            const auto check = ps2asset::model::inspect_model_bytes(extracted, filename);
            if (!check.sl01_wrapped || !check.sl01_length_checked || !check.sl01_length_matches) {
                throw std::runtime_error("internal size-invariant check failed for " + filename);
            }

            const fs::path extracted_path = output_dir / filename;
            ps2asset::io::write_new_file_atomically(extracted_path, extracted);
            created_files.push_back(extracted_path);

            manifest << i << '\t' << filename << '\t' << hex_offset(block.offset) << '\t'
                     << block.total_size << '\t' << block.size_field << '\t'
                     << block.gap_after << '\n';
            if (!manifest) throw std::runtime_error("failed while preparing manifest contents");
            ++written;
        }
        ps2asset::io::write_new_file_atomically(manifest_path, [&manifest](std::ostream& out) {
            out << manifest.str();
            if (!out) throw std::runtime_error("failed while writing manifest");
        });
        manifest_created = true;
    } catch (const std::exception& ex) {
        std::cerr << "ERROR: extraction stopped safely: " << ex.what() << '\n';
        std::error_code cleanup_error;
        if (manifest_created) {
            fs::remove(manifest_path, cleanup_error);
            if (cleanup_error) std::cerr << "WARNING: could not remove created manifest: " << cleanup_error.message() << '\n';
        }
        for (const auto& path : created_files) {
            cleanup_error.clear();
            fs::remove(path, cleanup_error);
            if (cleanup_error) std::cerr << "WARNING: could not remove created file " << path.string() << ": " << cleanup_error.message() << '\n';
        }
        cleanup_error.clear();
        fs::remove(output_dir, cleanup_error); // non-recursive: never erase files another process may have added
        if (cleanup_error) {
            std::cerr << "WARNING: output directory was left in place because it is not empty or could not be removed: "
                      << cleanup_error.message() << '\n';
        }
        return 1;
    }

    std::cout << "Validated SL01 blocks: " << scan.blocks.size() << '\n'
              << "Extracted files: " << written << '\n'
              << "Output directory: " << output_dir.string() << '\n'
              << "Manifest: " << manifest_path.string() << '\n'
              << "Note: extracted blocks are validated by declared bounds only; they are not decoded meshes.\n";
    if (scan.rejected_candidates > 0U) {
        std::cout << "Rejected overlapping/out-of-bounds signatures: "
                  << scan.rejected_candidates << '\n';
    }
    return 0;
}

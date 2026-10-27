#include "ps2asset/audio/wav.hpp"
#include "ps2asset/inspect.hpp"

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
namespace {
constexpr std::uintmax_t kMaxInputBytes = 64U * 1024U * 1024U;

std::string header_prefix(const std::vector<std::uint8_t>& bytes) {
    std::string out;
    const std::size_t n = std::min<std::size_t>(bytes.size(), 4096U);
    out.reserve(n);
    for (std::size_t i = 0; i < n; ++i) out.push_back(static_cast<char>(bytes[i]));
    return out;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
        std::cout << "Usage: ps2asset-audio-inspect <audio-or-resource-file>\n"
                     "RIFF/WAVE metadata is parsed; proprietary SDPK/ADSC signatures are only identified.\n"
                     "Read-only inspection; input limit 64 MiB. No playback or decoding.\n";
        return argc == 2 ? 0 : 2;
    }
    const fs::path path(argv[1]);
    std::error_code ec;
    if (!fs::is_regular_file(path, ec) || ec) {
        std::cerr << "ERROR: input is not a regular file\n";
        return 1;
    }
    const auto size = fs::file_size(path, ec);
    if (ec || size > kMaxInputBytes) {
        std::cerr << "ERROR: input size cannot be read or exceeds 64 MiB\n";
        return 1;
    }
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "ERROR: unable to open input\n";
        return 1;
    }
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    if (!bytes.empty()) {
        in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (in.gcount() != static_cast<std::streamsize>(bytes.size())) {
            std::cerr << "ERROR: short read\n";
            return 1;
        }
    }

    const auto wav = ps2asset::audio::inspect_wav_bytes(bytes);
    if (wav.state == ps2asset::audio::WavState::ValidMetadata) {
        std::cout << "file: " << path.string() << '\n' << "size: " << size << '\n'
                  << ps2asset::audio::format_wav_report(wav);
        return 0;
    }
    if (wav.state == ps2asset::audio::WavState::Malformed) {
        std::cerr << "file: " << path.string() << '\n' << ps2asset::audio::format_wav_report(wav);
        return 1;
    }

    const std::size_t prefixSize = std::min<std::size_t>(bytes.size(), 4096U);
    std::vector<std::uint8_t> prefix(bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(prefixSize));
    const auto generic = ps2asset::inspect_header(prefix, static_cast<std::uint64_t>(bytes.size()));
    std::cout << "file: " << path.string() << '\n' << "size: " << size << '\n'
              << "generic format: " << generic.format << '\n';
    for (const auto& detail : generic.details) std::cout << "  " << detail << '\n';
    std::cout << "No known WAV structure; audio codec/container internals were not decoded.\n";
    return 0;
}

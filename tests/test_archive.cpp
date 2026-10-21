#include "ps2asset/archive/zip_reader.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <archive.h>
#include <archive_entry.h>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void addEntry(struct archive* writer, const char* path, const std::vector<std::uint8_t>& bytes,
              bool directory = false) {
    archive_entry* entry = archive_entry_new();
    if (entry == nullptr) throw std::runtime_error("archive_entry_new failed");
    archive_entry_set_pathname(entry, path);
    archive_entry_set_filetype(entry, directory ? AE_IFDIR : AE_IFREG);
    archive_entry_set_perm(entry, directory ? 0755 : 0644);
    archive_entry_set_size(entry, directory ? 0 : static_cast<la_int64_t>(bytes.size()));
    if (archive_write_header(writer, entry) != ARCHIVE_OK) {
        const char* error = archive_error_string(writer);
        archive_entry_free(entry);
        throw std::runtime_error(error != nullptr ? error : "archive_write_header failed");
    }
    if (!directory && !bytes.empty()) {
        const auto written = archive_write_data(writer, bytes.data(), bytes.size());
        if (written != static_cast<la_ssize_t>(bytes.size())) {
            const char* error = archive_error_string(writer);
            archive_entry_free(entry);
            throw std::runtime_error(error != nullptr ? error : "archive_write_data failed");
        }
    }
    archive_entry_free(entry);
}

void createFixture(const std::filesystem::path& path) {
    struct archive* writer = archive_write_new();
    if (writer == nullptr) throw std::runtime_error("archive_write_new failed");
    if (archive_write_set_format_zip(writer) != ARCHIVE_OK ||
        archive_write_open_filename(writer, path.string().c_str()) != ARCHIVE_OK) {
        const char* error = archive_error_string(writer);
        archive_write_free(writer);
        throw std::runtime_error(error != nullptr ? error : "Unable to create ZIP fixture");
    }
    try {
        addEntry(writer, "textures/", {}, true);
        addEntry(writer, "textures/test.tm2", {'T', 'I', 'M', '2'});
        addEntry(writer, "scripts/main.scp", {'S', 'C', 'P'});
    } catch (...) {
        archive_write_close(writer);
        archive_write_free(writer);
        throw;
    }
    if (archive_write_close(writer) != ARCHIVE_OK) {
        const char* error = archive_error_string(writer);
        archive_write_free(writer);
        throw std::runtime_error(error != nullptr ? error : "archive_write_close failed");
    }
    archive_write_free(writer);
}
}  // namespace

int main() {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto fixture = std::filesystem::temp_directory_path() /
        ("ps2asset-archive-test-" + std::to_string(nonce) + ".zip");
    try {
        createFixture(fixture);
        const auto entries = ps2asset::archive::list_zip_entries(fixture.string());
        assert(entries.size() == 3U);
        bool foundTexture = false;
        bool foundScript = false;
        bool foundDirectory = false;
        for (const auto& entry : entries) {
            if (entry.path == "textures/test.tm2") {
                foundTexture = entry.is_regular_file && !entry.is_directory && entry.uncompressed_size == 4U;
            } else if (entry.path == "scripts/main.scp") {
                foundScript = entry.is_regular_file && entry.uncompressed_size == 3U;
            } else if (entry.path == "textures/") {
                foundDirectory = entry.is_directory;
            }
        }
        assert(foundTexture && foundScript && foundDirectory);

        bool limitChecked = false;
        try {
            (void)ps2asset::archive::list_zip_entries(fixture.string(), 2U);
        } catch (const std::exception&) {
            limitChecked = true;
        }
        assert(limitChecked);
        std::filesystem::remove(fixture);
    } catch (...) {
        std::error_code ignored;
        std::filesystem::remove(fixture, ignored);
        throw;
    }
    std::cout << "ZIP archive listing tests passed.\n";
    return 0;
}

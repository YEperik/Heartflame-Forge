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
#include <fstream>
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
        addEntry(writer, "textures/second.tm2", {'T', 'I', 'M', '2', '2'});
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
    const auto tempDir = std::filesystem::temp_directory_path();
    const auto fixture = tempDir / ("ps2asset-archive-test-" + std::to_string(nonce) + ".zip");
    const auto replacement = tempDir / ("ps2asset-replacement-" + std::to_string(nonce) + ".tm2");
    const auto output = tempDir / ("ps2asset-repacked-" + std::to_string(nonce) + ".zip");
    const auto outputBytes = tempDir / ("ps2asset-repacked-bytes-" + std::to_string(nonce) + ".zip");
    const auto outputBatch = tempDir / ("ps2asset-repacked-batch-" + std::to_string(nonce) + ".zip");
    const auto duplicateOutput = tempDir / ("ps2asset-duplicate-target-" + std::to_string(nonce) + ".zip");
    const auto totalLimitOutput = tempDir / ("ps2asset-total-limit-" + std::to_string(nonce) + ".zip");
    const auto missingBatchOutput = tempDir / ("ps2asset-missing-batch-target-" + std::to_string(nonce) + ".zip");
    const auto missingOutput = tempDir / ("ps2asset-missing-target-" + std::to_string(nonce) + ".zip");
    try {
        createFixture(fixture);
        const auto entries = ps2asset::archive::list_zip_entries(fixture.string());
        assert(entries.size() == 4U);
        bool foundTexture = false;
        bool foundScript = false;
        bool foundSecondTexture = false;
        bool foundDirectory = false;
        for (const auto& entry : entries) {
            if (entry.path == "textures/test.tm2") {
                foundTexture = entry.is_regular_file && !entry.is_directory && entry.uncompressed_size == 4U;
            } else if (entry.path == "textures/second.tm2") {
                foundSecondTexture = entry.is_regular_file && entry.uncompressed_size == 5U;
            } else if (entry.path == "scripts/main.scp") {
                foundScript = entry.is_regular_file && entry.uncompressed_size == 3U;
            } else if (entry.path == "textures/") {
                foundDirectory = entry.is_directory;
            }
        }
        assert(foundTexture && foundSecondTexture && foundScript && foundDirectory);

        // Repack exactly one entry, then read the ZIP back and verify the other
        // files survived. The API must not overwrite an existing output path.
        {
            std::ofstream out(replacement, std::ios::binary);
            const char replacementBytes[] = {'T','I','M','2','-','N','E','W'};
            out.write(replacementBytes, sizeof(replacementBytes));
            assert(out.good());
        }
        ps2asset::archive::replace_zip_entry(fixture.string(), "textures/test.tm2",
                                             replacement.string(), output.string());
        const auto repackedEntries = ps2asset::archive::list_zip_entries(output.string());
        assert(repackedEntries.size() == 4U);
        const auto textures = ps2asset::archive::read_tim2_entries_from_zip(output.string());
        assert(textures.size() == 2U);
        bool primaryReplaced = false;
        bool secondaryPreserved = false;
        for (const auto& texture : textures) {
            if (texture.entry_name == "textures/test.tm2")
                primaryReplaced = texture.bytes == std::vector<std::uint8_t>({'T','I','M','2','-','N','E','W'});
            if (texture.entry_name == "textures/second.tm2")
                secondaryPreserved = texture.bytes == std::vector<std::uint8_t>({'T','I','M','2','2'});
        }
        assert(primaryReplaced && secondaryPreserved);

        // Verify the in-memory API used by the GUI, without staging a payload file.
        const std::vector<std::uint8_t> inMemoryReplacement = {'G','U','I','-','T','M','2'};
        ps2asset::archive::replace_zip_entry_bytes(fixture.string(), "textures/test.tm2",
            inMemoryReplacement, outputBytes.string());
        const auto inMemoryTextures = ps2asset::archive::read_tim2_entries_from_zip(outputBytes.string());
        assert(inMemoryTextures.size() == 2U);
        bool inMemoryPrimaryReplaced = false;
        bool inMemorySecondaryPreserved = false;
        for (const auto& texture : inMemoryTextures) {
            if (texture.entry_name == "textures/test.tm2") inMemoryPrimaryReplaced = texture.bytes == inMemoryReplacement;
            if (texture.entry_name == "textures/second.tm2") inMemorySecondaryPreserved = texture.bytes == std::vector<std::uint8_t>({'T','I','M','2','2'});
        }
        assert(inMemoryPrimaryReplaced && inMemorySecondaryPreserved);

        // Batch API changes multiple target entries in one archive rebuild.
        const std::vector<ps2asset::archive::ZipEntryReplacement> batchReplacements = {
            {"textures/test.tm2", {'B','A','T','C','H','-','A'}},
            {"textures/second.tm2", {'B','A','T','C','H','-','B'}}
        };
        ps2asset::archive::replace_zip_entries_bytes(fixture.string(), batchReplacements, outputBatch.string());
        const auto batchTextures = ps2asset::archive::read_tim2_entries_from_zip(outputBatch.string());
        assert(batchTextures.size() == 2U);
        bool batchA = false;
        bool batchB = false;
        for (const auto& texture : batchTextures) {
            if (texture.entry_name == "textures/test.tm2") batchA = texture.bytes == std::vector<std::uint8_t>({'B','A','T','C','H','-','A'});
            if (texture.entry_name == "textures/second.tm2") batchB = texture.bytes == std::vector<std::uint8_t>({'B','A','T','C','H','-','B'});
        }
        assert(batchA && batchB);

        bool duplicateRejected = false;
        try {
            ps2asset::archive::replace_zip_entries_bytes(fixture.string(), {
                {"textures/test.tm2", {'A'}}, {"textures/test.tm2", {'B'}}
            }, duplicateOutput.string());
        } catch (const std::exception&) { duplicateRejected = true; }
        assert(duplicateRejected);
        assert(!std::filesystem::exists(duplicateOutput));
        assert(!std::filesystem::exists(duplicateOutput.string() + ".tmp-ps2asset"));

        bool totalLimitRejected = false;
        try {
            ps2asset::archive::replace_zip_entries_bytes(fixture.string(), batchReplacements,
                totalLimitOutput.string(), 32U, 10U);
        } catch (const std::exception&) { totalLimitRejected = true; }
        assert(totalLimitRejected);
        assert(!std::filesystem::exists(totalLimitOutput));

        bool missingBatchTargetRejected = false;
        try {
            ps2asset::archive::replace_zip_entries_bytes(fixture.string(), {
                {"textures/test.tm2", {'O','K'}}, {"missing/target.tm2", {'N','O'}}
            }, missingBatchOutput.string());
        } catch (const std::exception&) { missingBatchTargetRejected = true; }
        assert(missingBatchTargetRejected);
        assert(!std::filesystem::exists(missingBatchOutput));
        assert(!std::filesystem::exists(missingBatchOutput.string() + ".tmp-ps2asset"));

        // Missing target and limit failures must not leave an output archive or temporary sibling.
        bool missingTargetRejected = false;
        try {
            ps2asset::archive::replace_zip_entry_bytes(fixture.string(), "missing/target.tm2",
                inMemoryReplacement, missingOutput.string());
        } catch (const std::exception&) {
            missingTargetRejected = true;
        }
        assert(missingTargetRejected);
        assert(!std::filesystem::exists(missingOutput));
        assert(!std::filesystem::exists(missingOutput.string() + ".tmp-ps2asset"));

        bool sizeLimitRejected = false;
        try {
            ps2asset::archive::replace_zip_entry_bytes(fixture.string(), "textures/test.tm2",
                inMemoryReplacement, missingOutput.string(), 2U);
        } catch (const std::exception&) {
            sizeLimitRejected = true;
        }
        assert(sizeLimitRejected);
        assert(!std::filesystem::exists(missingOutput));

        // The source ZIP should still contain its original bytes after both replacement paths.
        const auto sourceTextures = ps2asset::archive::read_tim2_entries_from_zip(fixture.string());
        assert(sourceTextures.size() == 2U);
        bool sourcePrimaryUnchanged = false;
        bool sourceSecondaryUnchanged = false;
        for (const auto& texture : sourceTextures) {
            if (texture.entry_name == "textures/test.tm2") sourcePrimaryUnchanged = texture.bytes == std::vector<std::uint8_t>({'T','I','M','2'});
            if (texture.entry_name == "textures/second.tm2") sourceSecondaryUnchanged = texture.bytes == std::vector<std::uint8_t>({'T','I','M','2','2'});
        }
        assert(sourcePrimaryUnchanged && sourceSecondaryUnchanged);
        bool overwriteRejected = false;
        try { ps2asset::archive::replace_zip_entry(fixture.string(), "textures/test.tm2", replacement.string(), output.string()); }
        catch (const std::exception&) { overwriteRejected = true; }
        assert(overwriteRejected);

        bool limitChecked = false;
        try {
            (void)ps2asset::archive::list_zip_entries(fixture.string(), 2U);
        } catch (const std::exception&) {
            limitChecked = true;
        }
        assert(limitChecked);
        std::filesystem::remove(fixture);
        std::filesystem::remove(replacement);
        std::filesystem::remove(output);
        std::filesystem::remove(outputBytes);
        std::filesystem::remove(outputBatch);
        std::filesystem::remove(duplicateOutput);
        std::filesystem::remove(totalLimitOutput);
        std::filesystem::remove(missingBatchOutput);
        std::filesystem::remove(missingOutput);
    } catch (...) {
        std::error_code ignored;
        std::filesystem::remove(fixture, ignored);
        std::filesystem::remove(replacement, ignored);
        std::filesystem::remove(output, ignored);
        std::filesystem::remove(outputBytes, ignored);
        std::filesystem::remove(outputBatch, ignored);
        std::filesystem::remove(duplicateOutput, ignored);
        std::filesystem::remove(duplicateOutput.string() + ".tmp-ps2asset", ignored);
        std::filesystem::remove(totalLimitOutput, ignored);
        std::filesystem::remove(missingBatchOutput, ignored);
        std::filesystem::remove(missingBatchOutput.string() + ".tmp-ps2asset", ignored);
        std::filesystem::remove(missingOutput, ignored);
        std::filesystem::remove(missingOutput.string() + ".tmp-ps2asset", ignored);
        throw;
    }
    std::cout << "ZIP archive listing tests passed.\n";
    return 0;
}

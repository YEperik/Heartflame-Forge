#pragma once

#include <filesystem>
#include <functional>
#include <iosfwd>
#include <vector>
#include <cstdint>

namespace ps2asset::io {

// Transaction for creating a new file atomically without ever replacing an
// existing path. Content is written to a private sibling directory, then
// committed with a no-clobber hard-link operation. If the filesystem does not
// support hard links, commit fails safely instead of falling back to an
// overwrite-prone rename.
class NewFileTransaction {
public:
    explicit NewFileTransaction(const std::filesystem::path& destination);
    ~NewFileTransaction() noexcept;

    NewFileTransaction(const NewFileTransaction&) = delete;
    NewFileTransaction& operator=(const NewFileTransaction&) = delete;
    NewFileTransaction(NewFileTransaction&&) = delete;
    NewFileTransaction& operator=(NewFileTransaction&&) = delete;

    [[nodiscard]] const std::filesystem::path& temporary_path() const noexcept { return temporary_path_; }
    [[nodiscard]] const std::filesystem::path& destination_path() const noexcept { return destination_path_; }
    void commit();

private:
    std::filesystem::path destination_path_;
    std::filesystem::path temporary_directory_;
    std::filesystem::path temporary_path_;
    bool committed_ = false;
};

// Writes a new file transactionally. Existing output paths are never replaced.
// Throws when the destination already exists, the writer fails, or the target
// filesystem cannot perform the safe no-clobber commit.
void write_new_file_atomically(const std::filesystem::path& destination,
                               const std::function<void(std::ostream&)>& writer);
void write_new_file_atomically(const std::filesystem::path& destination,
                               const std::vector<std::uint8_t>& bytes);

}  // namespace ps2asset::io

#include "ps2asset/io/new_file_transaction.hpp"

#include <atomic>
#include <chrono>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <system_error>

namespace ps2asset::io {
namespace fs = std::filesystem;
namespace {
std::atomic<std::uint64_t> g_temp_counter{0U};

std::string temp_directory_name() {
    const auto ticks = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto serial = g_temp_counter.fetch_add(1U, std::memory_order_relaxed);
    return ".ps2asset-tmp-" + std::to_string(ticks) + "-" + std::to_string(serial);
}

bool is_exists_error(const std::error_code& ec) {
    return ec == std::errc::file_exists || ec == std::errc::directory_not_empty;
}
}  // namespace

NewFileTransaction::NewFileTransaction(const fs::path& destination) {
    if (destination.empty() || destination.filename().empty() ||
        destination.filename() == "." || destination.filename() == "..") {
        throw std::runtime_error("Output path must name a file");
    }

    std::error_code ec;
    destination_path_ = fs::absolute(destination, ec).lexically_normal();
    if (ec) throw std::runtime_error("Cannot resolve output path: " + ec.message());
    const fs::path parent = destination_path_.parent_path();
    if (!fs::is_directory(parent, ec) || ec) {
        throw std::runtime_error("Output parent directory does not exist or is not accessible: " + parent.string());
    }
    if (fs::exists(destination_path_, ec)) {
        throw std::runtime_error("Output already exists; refusing to overwrite: " + destination_path_.string());
    }
    if (ec) throw std::runtime_error("Cannot check output path: " + ec.message());

    // create_directory is the exclusive operation: concurrent processes cannot
    // both claim the same transaction directory. Retry only on a name collision.
    for (unsigned int attempt = 0U; attempt < 128U; ++attempt) {
        const fs::path candidate = parent / temp_directory_name();
        ec.clear();
        if (fs::create_directory(candidate, ec)) {
            temporary_directory_ = candidate;
            temporary_path_ = temporary_directory_ / destination_path_.filename();
            return;
        }
        if (ec && !is_exists_error(ec)) {
            throw std::runtime_error("Cannot create private temporary directory: " + ec.message());
        }
    }
    throw std::runtime_error("Unable to allocate a unique temporary directory for output");
}

NewFileTransaction::~NewFileTransaction() noexcept {
    if (!temporary_directory_.empty()) {
        std::error_code ignored;
        fs::remove_all(temporary_directory_, ignored);
    }
}

void NewFileTransaction::commit() {
    if (committed_) throw std::runtime_error("Output transaction has already been committed");
    std::error_code ec;
    if (!fs::is_regular_file(temporary_path_, ec) || ec) {
        throw std::runtime_error("Temporary output file was not created successfully");
    }

    // create_hard_link is an atomic no-clobber operation on supported local
    // filesystems. It never replaces the destination if another process wins
    // the race after the constructor's initial existence check.
    fs::create_hard_link(temporary_path_, destination_path_, ec);
    if (ec) {
        std::error_code exists_error;
        if (fs::exists(destination_path_, exists_error)) {
            throw std::runtime_error("Output was created by another process; refusing to overwrite: " + destination_path_.string());
        }
        throw std::runtime_error("Cannot commit output safely (filesystem may not support hard links): " + ec.message());
    }
    committed_ = true;
}

void write_new_file_atomically(const fs::path& destination,
                               const std::function<void(std::ostream&)>& writer) {
    if (!writer) throw std::runtime_error("Output writer callback is empty");
    NewFileTransaction transaction(destination);
    std::ofstream output(transaction.temporary_path(), std::ios::binary | std::ios::out);
    if (!output) throw std::runtime_error("Cannot create temporary output file");
    writer(output);
    output.flush();
    if (!output) throw std::runtime_error("Failed while writing temporary output file");
    output.close();
    if (output.fail()) throw std::runtime_error("Failed while closing temporary output file");
    transaction.commit();
}

void write_new_file_atomically(const fs::path& destination,
                               const std::vector<std::uint8_t>& bytes) {
    if (bytes.size() > static_cast<std::size_t>(std::numeric_limits<std::streamsize>::max())) {
        throw std::runtime_error("Output buffer exceeds the stream size limit");
    }
    write_new_file_atomically(destination, [&bytes](std::ostream& output) {
        if (!bytes.empty()) {
            output.write(reinterpret_cast<const char*>(bytes.data()),
                         static_cast<std::streamsize>(bytes.size()));
        }
    });
}

}  // namespace ps2asset::io

#include "ps2asset/io/new_file_transaction.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {
fs::path make_test_dir() {
    const auto tick = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path base = fs::temp_directory_path() / ("ps2asset-io-test-" + std::to_string(tick));
    if (!fs::create_directory(base)) throw std::runtime_error("could not create transaction test directory");
    return base;
}
std::string read_text(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}
}

int main() {
    const fs::path dir = make_test_dir();
    try {
        const fs::path fresh = dir / "fresh.bin";
        ps2asset::io::write_new_file_atomically(fresh, std::vector<std::uint8_t>{1U, 2U, 3U, 4U});
        assert(fs::is_regular_file(fresh));
        {
            std::ifstream in(fresh, std::ios::binary);
            std::vector<unsigned char> got((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
            assert((got == std::vector<unsigned char>{1U, 2U, 3U, 4U}));
        }

        const fs::path existing = dir / "keep.txt";
        { std::ofstream out(existing); out << "KEEP"; }
        bool rejected_existing = false;
        try { ps2asset::io::write_new_file_atomically(existing, std::vector<std::uint8_t>{'N','O'}); }
        catch (const std::exception&) { rejected_existing = true; }
        assert(rejected_existing);
        assert(read_text(existing) == "KEEP");

        const fs::path race = dir / "race.txt";
        bool rejected_at_commit = false;
        {
            ps2asset::io::NewFileTransaction transaction{race};
            { std::ofstream temp(transaction.temporary_path(), std::ios::binary); temp << "OURS"; }
            // Simulate another process creating the final path after our
            // initial check but before the atomic commit operation.
            { std::ofstream external(race, std::ios::binary); external << "THEIRS"; }
            try { transaction.commit(); } catch (const std::exception&) { rejected_at_commit = true; }
        }
        assert(rejected_at_commit);
        assert(read_text(race) == "THEIRS");

        const fs::path failed = dir / "failed.txt";
        bool propagated = false;
        try {
            ps2asset::io::write_new_file_atomically(failed, [](std::ostream& out) {
                out << "partial";
                throw std::runtime_error("fixture writer failure");
            });
        } catch (const std::runtime_error&) { propagated = true; }
        assert(propagated);
        assert(!fs::exists(failed));

        bool missing_parent_rejected = false;
        try { ps2asset::io::write_new_file_atomically(dir / "missing" / "out.bin", std::vector<std::uint8_t>{1U}); }
        catch (const std::exception&) { missing_parent_rejected = true; }
        assert(missing_parent_rejected);
        fs::remove_all(dir);
        std::cout << "All new-file transaction tests passed.\n";
        return 0;
    } catch (...) {
        std::error_code ignored;
        fs::remove_all(dir, ignored);
        throw;
    }
}

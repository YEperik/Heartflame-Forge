#include <assimp/Importer.hpp>
#include <assimp/Exporter.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;
namespace {
bool verify_glb_header(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::array<std::uint8_t, 12> header{};
    in.read(reinterpret_cast<char*>(header.data()), static_cast<std::streamsize>(header.size()));
    if (in.gcount() != static_cast<std::streamsize>(header.size())) return false;
    const std::uint32_t magic = static_cast<std::uint32_t>(header[0]) |
        (static_cast<std::uint32_t>(header[1]) << 8U) |
        (static_cast<std::uint32_t>(header[2]) << 16U) |
        (static_cast<std::uint32_t>(header[3]) << 24U);
    const std::uint32_t version = static_cast<std::uint32_t>(header[4]) |
        (static_cast<std::uint32_t>(header[5]) << 8U) |
        (static_cast<std::uint32_t>(header[6]) << 16U) |
        (static_cast<std::uint32_t>(header[7]) << 24U);
    const std::uint32_t declaredLength = static_cast<std::uint32_t>(header[8]) |
        (static_cast<std::uint32_t>(header[9]) << 8U) |
        (static_cast<std::uint32_t>(header[10]) << 16U) |
        (static_cast<std::uint32_t>(header[11]) << 24U);
    std::error_code ec;
    const auto actualLength = fs::file_size(path, ec);
    return !ec && magic == 0x46546C67U && version == 2U && declaredLength == actualLength;
}
}

int main(int argc, char** argv) {
    if (argc != 3 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
        std::cout << "Usage: ps2asset-model-export-glb <input-supported-by-assimp> <new-output.glb>\n"
                     "Uses Assimp to import a supported scene and export glTF 2.0 binary.\n"
                     "This does not decode proprietary PS2 .mdl data unless Assimp can import it.\n"
                     "Existing output is never overwritten.\n";
        return argc == 3 ? 0 : 2;
    }
    const fs::path inputPath(argv[1]);
    const fs::path outputPath(argv[2]);
    std::error_code ec;
    if (!fs::is_regular_file(inputPath, ec) || ec) {
        std::cerr << "ERROR: input is not a regular file\n";
        return 1;
    }
    const bool outputExists = fs::exists(outputPath, ec);
    if (ec) {
        std::cerr << "ERROR: cannot verify output path: " << ec.message() << '\n';
        return 1;
    }
    if (outputExists) {
        std::cerr << "ERROR: output already exists; refusing to overwrite\n";
        return 1;
    }
    if (inputPath.lexically_normal() == outputPath.lexically_normal()) {
        std::cerr << "ERROR: input and output must be different files\n";
        return 1;
    }
    std::string extension = outputPath.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (extension != ".glb") {
        std::cerr << "ERROR: output extension must be .glb\n";
        return 2;
    }
    if (!outputPath.parent_path().empty() && !fs::is_directory(outputPath.parent_path(), ec)) {
        std::cerr << "ERROR: output parent directory does not exist\n";
        return 1;
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(inputPath.string(), aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType | aiProcess_ValidateDataStructure);
    if (scene == nullptr || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) != 0U || scene->mRootNode == nullptr) {
        std::cerr << "ERROR: Assimp could not import input: " << importer.GetErrorString() << '\n';
        std::cerr << "If this is a custom PS2 .mdl file, it needs a dedicated geometry parser first.\n";
        return 1;
    }

    Assimp::Exporter exporter;
    const aiReturn exported = exporter.Export(scene, "glb2", outputPath.string(), 0U);
    if (exported != aiReturn_SUCCESS) {
        fs::remove(outputPath, ec);
        std::cerr << "ERROR: GLB export failed: " << exporter.GetErrorString() << '\n';
        return 1;
    }
    if (!verify_glb_header(outputPath)) {
        fs::remove(outputPath, ec);
        std::cerr << "ERROR: output does not pass GLB v2 header/length validation; output removed\n";
        return 1;
    }
    std::cout << "OK\ninput=" << inputPath.string() << "\noutput=" << outputPath.string()
              << "\nmeshes=" << scene->mNumMeshes << "\nmaterials=" << scene->mNumMaterials
              << "\noutput_bytes=" << fs::file_size(outputPath) << "\n";
    return 0;
}

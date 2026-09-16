#pragma once

#include "../utils/string.hpp"

struct GlobalAppData;

namespace Renderer {
class Mesh;

enum class MeshLoaderFlags : u64 {
    None = 0ULL,
    UploadTexturesToGPU = 1ULL << 0U,
    StoreTextures = 1ULL << 1U,
    // Don't call setup_mesh
    DontInitializeOpenGL = 1ULL << 2U,
};

constexpr MeshLoaderFlags operator&(MeshLoaderFlags lhs, MeshLoaderFlags rhs)
{
    return static_cast<MeshLoaderFlags>(static_cast<u64>(lhs) & static_cast<u64>(rhs));
}

constexpr MeshLoaderFlags operator|(MeshLoaderFlags lhs, MeshLoaderFlags rhs)
{
    return static_cast<MeshLoaderFlags>(static_cast<u64>(lhs) | static_cast<u64>(rhs));
}

constexpr bool has_flag(MeshLoaderFlags flags, MeshLoaderFlags flag_to_test)
{
    return (flags & flag_to_test) == flag_to_test;
}

enum struct ModelResultEnum {
    Ok,
    InvalidFilePath,
    InvalidFormat,
    UnknownError,
};

struct ModelResult {
    ModelResultEnum type = ModelResultEnum::Ok;
    Utils::String error;
};

ModelResult load_mesh(Mesh& mesh, const char* path, MeshLoaderFlags flags = MeshLoaderFlags::UploadTexturesToGPU | MeshLoaderFlags::StoreTextures);

} // namespace Renderer

#include "mesh.hpp"

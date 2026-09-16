#pragma once

struct GlobalAppData;
namespace Utils {
    class ByteStream;
}

namespace Renderer {

class Mesh;

void compile_mesh(Utils::ByteStream& bytes, const char* file_path);
void compile_mesh(Utils::ByteStream& bytes, Mesh& mesh);
void load_compiled_mesh(Mesh& mesh, const std::span<u8> compiled_mesh);

} // namespace Renderer

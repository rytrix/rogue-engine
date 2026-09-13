#pragma once

#include "camera.hpp"
#include "shader.hpp"
#include "texture.hpp"
#include "vertex.hpp"

namespace Renderer {

// Expects an image file in the format of
//    XX
// XX XX XX XX
//    XX
struct SkyboxInfo {
    const char* file;
};

class Skybox : public NoCopyNoMove {
public:
    Skybox() = default;
    Skybox(SkyboxInfo& info);
    void init(SkyboxInfo& info);

    ~Skybox();

    void draw(const Camera& camera);

    const char* m_name = nullptr;

private:
    bool initialized = false;

    VertexArray m_vao;
    Shader m_shader;
    Texture m_cubemap;

    void setup_shader();
};

} // namespace Renderer

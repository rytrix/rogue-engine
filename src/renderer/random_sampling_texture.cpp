#include "random_sampling_texture.hpp"

#include "shader.hpp"
#include "texture.hpp"

#include "../app_data.hpp"

#include <random>

namespace Renderer {

namespace {

    struct Jitter {
        Jitter();
        float operator()();

    private:
        std::default_random_engine generator;
        std::uniform_real_distribution<float> distrib;
    };

    Jitter::Jitter()
        : distrib(-0.5F, 0.5F)
    {
    }

    float Jitter::operator()()
    {
        return distrib(generator);
    }

} // Anonymous namespace

RandomSamplingTexture RandomSamplingTexture::create(i32 window_size, i32 filter_size, i32 radius)
{
    Handle handle = create_random_sampling_texture(window_size, filter_size);
    return { handle, window_size, filter_size, radius };
}

RandomSamplingTexture::RandomSamplingTexture(Handle handle, i32 window_size, i32 filter_size, i32 radius)
    : m_radius(radius)
    , m_handle(handle)
    , m_window_size(window_size)
    , m_filter_size(filter_size)
{
}

void RandomSamplingTexture::bind_uniforms(Shader& shader, const char* uniform_name)
{
    Utils::String buffer;

    Renderer::Texture* texture = g_global_app_data->m_texture_cache->get(m_handle);
    GLuint texture_unit = Renderer::Texture::get_texture_unit();
    texture->bind(texture_unit);

    shader.set_int(buffer.format("{}.texture", uniform_name).c_str(), static_cast<int>(texture_unit));
    shader.set_int(buffer.format("{}.window_size", uniform_name).c_str(), m_window_size);
    shader.set_int(buffer.format("{}.filter_size", uniform_name).c_str(), m_filter_size);
    shader.set_int(buffer.format("{}.radius", uniform_name).c_str(), m_radius);
}

Handle RandomSamplingTexture::create_random_sampling_texture(i32 window_size, int filter_size)
{
    Jitter jitter;

    i32 buffer_size = window_size * window_size * filter_size * filter_size * 2;

    std::vector<float> data;
    data.resize(buffer_size);

    i32 index = 0;

    for (i32 tex_y = 0; tex_y < window_size; tex_y++) {
        for (i32 tex_x = 0; tex_x < window_size; tex_x++) {
            for (i32 v = filter_size - 1; v >= 0; v--) {
                for (i32 u = 0; u < filter_size; u++) {
                    float x = (u + 0.5F + jitter()) / (float)filter_size;
                    float y = (v + 0.5F + jitter()) / (float)filter_size;

                    util_assert(index + 1 < static_cast<i32>(data.size()), "next index greater than allocated data");
                    data[index] = std::sqrtf(y) * std::cosf(2.0F * std::numbers::pi_v<float> * x);
                    data[index + 1] = std::sqrtf(y) * std::sinf(2.0F * std::numbers::pi_v<float> * x);

                    index += 2;
                }
            }
        }
    }

    i32 num_filter_samples = filter_size * filter_size;

    TextureInfo texture_info;
    texture_info.mipmaps = false;
    texture_info.dimensions = GL_TEXTURE_3D;
    texture_info.internal_format = GL_RGBA32F;
    texture_info.memory_info.size = { .width = num_filter_samples / 2, .height = window_size, .depth = window_size };

    TextureSubimageInfo subimage_info;
    subimage_info.size = texture_info.memory_info.size;
    subimage_info.format = GL_RGBA;
    subimage_info.type = GL_FLOAT;
    subimage_info.pixels = data.data();

    Handle handle = g_global_app_data->m_texture_cache->create(texture_info);
    Texture* texture = g_global_app_data->m_texture_cache->get(handle);
    texture->sub_image(subimage_info);

    return handle;
}

} // namespace Renderer

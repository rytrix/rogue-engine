#include "text.hpp"
#include "../utils/file.hpp"
#include "shader_preprocessor.hpp"

namespace Renderer {

TextRenderer::TextRenderer(const char* font_path, u32 font_height)
{
    init(font_path, font_height);
}

void TextRenderer::init(const char* font_path, u32 font_height)
{
    util_assert(initialized == false, "already initialized");
    if (FT_Init_FreeType(&m_freetype) != 0) {
        util_error("Failed to init freetype");
    }

    if (FT_New_Face(m_freetype, font_path, 0, &m_face) != 0) {
        util_error(std::format("Failed to load font \"{}\"", font_path));
    }

    FT_Set_Pixel_Sizes(m_face, 0, font_height);
    m_font_height = font_height;

    setup_atlas();
    setup_quad();
    setup_shader();
    initialized = true;

    update_view(800.0F, 800.0F);
}

TextRenderer::~TextRenderer()
{
    if (initialized) {
        FT_Done_Face(m_face);
        FT_Done_FreeType(m_freetype);

        initialized = false;
    }
}

void TextRenderer::draw_text(u32 x, u32 y, const char* text, glm::vec3 color, float scale)
{
    util_assert(initialized == true, "not initialized");
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_DEPTH_TEST);

    m_vao.bind();

    m_shader.bind();
    auto texture_unit = Texture::get_texture_unit();
    m_texture_atlas.bind(texture_unit);

    m_shader.set_int("text_atlas", (int)texture_unit);
    m_shader.set_vec3("text_color", color);
    m_shader.set_mat4("projection", m_projection);

    usize text_length = strlen(text);
    m_vertices.resize(text_length);
    for (u32 i = 0; i < text_length; i++) {
        char c = text[i];
        load_glyph(c);

        Character character = m_characters[c];
        m_vertices[i].x_pos = x + ((float)character.bearing.x * scale);
        m_vertices[i].y_pos = y - (((float)character.size.y - (float)character.bearing.y) * scale);

        m_vertices[i].w = (float)character.size.x * scale;
        m_vertices[i].h = (float)character.size.y * scale;

        m_vertices[i].uv_x = character.uv_offset.x;
        m_vertices[i].uv_x_max = m_vertices[i].uv_x + character.uv_size.x;

        m_vertices[i].uv_y = character.uv_offset.y;
        m_vertices[i].uv_y_max = m_vertices[i].uv_y + character.uv_size.y;

        x += (character.advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)
    }

    usize size = m_vertices.size() * sizeof(GlyphInstance);
    if (size > m_ssbo.get_buffer_size()) {
        m_ssbo.deinit();
        m_ssbo.init(3, size);
    }

    void* ptr = m_ssbo.get_ptr();
    memcpy(ptr, m_vertices.data(), size);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ssbo.get_id());
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_vertices.size() * 6);

    Texture::drop_texture_units(1);
    m_ssbo.increment_frame();
}

void TextRenderer::update_view(float width, float height)
{
    util_assert(initialized == true, "not initialized");
    m_projection = glm::ortho(0.0F, width, 0.0F, height);
}

void TextRenderer::setup_atlas()
{
    m_pixel_height = (m_face->size->metrics.ascender - m_face->size->metrics.descender) >> 6;

    Renderer::TextureInfo info;
    info.min_filter = GL_LINEAR;
    info.mag_filter = GL_LINEAR;
    info.wrap_s = GL_CLAMP_TO_EDGE;
    info.wrap_t = GL_CLAMP_TO_EDGE;
    info.wrap_r = GL_CLAMP_TO_EDGE;
    info.internal_format = GL_R8;
    info.memory_info.size.width = ATLAS_WIDTH;
    info.memory_info.size.height = (GLint)m_pixel_height;
    info.memory_info.size.depth = 1;
    m_texture_atlas.init(info);

    m_characters.resize(CHAR_MAX);
    // for (unsigned char c = 0; c < 128; c++) {
    //     load_glyph(c);
    // }
}

int TextRenderer::load_glyph(char c)
{
    if (m_characters[c].valid) {
        return 0;
    }

    if (FT_Load_Char(m_face, c, FT_LOAD_RENDER) != 0) {
        LOG_ERROR(std::format("FREETYPE: Failed to load glyph {}", (char)c));
        return -1;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

    TextureSubimageInfo subimage_info {};
    subimage_info.offsets.width = (GLint)m_current_atlas_width;
    subimage_info.offsets.height = 0;
    subimage_info.size.width = (GLint)m_face->glyph->bitmap.width;
    subimage_info.size.height = (GLint)m_face->glyph->bitmap.rows;
    subimage_info.format = GL_RED;
    subimage_info.type = GL_UNSIGNED_BYTE;
    subimage_info.pixels = m_face->glyph->bitmap.buffer;
    m_texture_atlas.sub_image(subimage_info);

    glPixelStorei(GL_PACK_ALIGNMENT, 1); // enable byte-alignment restriction

    Character& character = m_characters[c];
    character.valid = true;
    character.uv_offset.x = (float)subimage_info.offsets.width / (float)ATLAS_WIDTH;
    character.uv_offset.y = (float)subimage_info.offsets.height / (float)m_pixel_height;
    character.uv_size.x = (float)subimage_info.size.width / (float)ATLAS_WIDTH;
    character.uv_size.y = (float)subimage_info.size.height / (float)m_pixel_height;
    character.size = { m_face->glyph->bitmap.width, m_face->glyph->bitmap.rows };
    character.bearing = { m_face->glyph->bitmap_left, m_face->glyph->bitmap_top };
    character.advance = m_face->glyph->advance.x;

    m_current_atlas_width += m_face->glyph->bitmap.width + 1;

    return 0;
}

void TextRenderer::setup_quad()
{
    m_vao.init();
    m_ssbo.init(3, 20);

    m_vao.bind();
}

void TextRenderer::setup_shader()
{
    ShaderInfoData<2> out;

    const char* file_name = "res/shaders/text_rendering/text_combined.glsl";
    std::vector<char> text_shader_file;
    auto result = Utils::read_file(text_shader_file, file_name);
    if (!result) {
        util_error(std::format("Could not find file \"{}\"", file_name));
    }
    std::string_view text_shader_file_view = { text_shader_file.data(), text_shader_file.size() };

    // Vertex Shader
    out.data.at(0) = std::format("#version 460 core\n");
    out.data.at(0) += get_lines_between_delims(text_shader_file_view, "// Vertex Begin", "// Vertex End");

    // Fragment Shader
    out.data.at(1) += "#version 460 core\n";
    out.data.at(1) += get_lines_between_delims(text_shader_file_view, "// Fragment Begin", "// Fragment End");

    out.populate_info();

    m_shader.init(out.info.data(), out.info.size());
}

} // namespace Renderer

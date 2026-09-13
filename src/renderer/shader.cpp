#include "shader.hpp"

#include "../utils/file.hpp"

#include <fstream>

namespace Renderer {

namespace {

    class OpenGLShader : public NoCopyNoMove {
    public:
        OpenGLShader() = default;
        OpenGLShader(bool is_file, const char* shader, GLenum type);
        ~OpenGLShader();

        void init(bool is_file, const char* shader, GLenum type);

        [[nodiscard]] GLuint get_id() const;
        [[nodiscard]] bool get_error() const;

    private:
        GLuint m_id {};
        bool m_errors = true;

        [[nodiscard]] bool has_errors() const;
    };

    OpenGLShader::OpenGLShader(bool is_file, const char* shader, GLenum type)
    {
        init(is_file, shader, type);
    }

    OpenGLShader::~OpenGLShader()
    {
        if (!m_errors) {
            glDeleteShader(m_id);
        }
    }

    void OpenGLShader::init(bool is_file, const char* shader, GLenum type)
    {
        m_id = glCreateShader(type);

        std::vector<char> shader_text;
        if (is_file) {

            auto result = Utils::read_file(shader_text, shader);
            if (!result) {
                LOG_ERROR(std::format("Could not find file \"{}\"", shader));
                m_errors = true;
                return;
            }
            const char* data_text = shader_text.data();
            glShaderSource(m_id, 1, &data_text, nullptr);
        } else {
            glShaderSource(m_id, 1, &shader, nullptr);
        }

        glCompileShader(m_id);

        int is_compiled = 0;
        glGetShaderiv(m_id, GL_COMPILE_STATUS, &is_compiled);

        if (is_compiled == GL_FALSE) {
            GLint max_length = 0;
            glGetShaderiv(m_id, GL_INFO_LOG_LENGTH, &max_length);

            std::vector<GLchar> error_log(max_length);
            glGetShaderInfoLog(m_id, max_length, &max_length, error_log.data());

            if (is_file) {
                LOG_ERROR(std::format("shader failed to compile: {}\nshader source:\n{}", error_log.data(), shader_text.data()));
            } else {
                LOG_ERROR(std::format("shader failed to compile: {}\nshader source:\n{}", error_log.data(), shader));
            }
            m_errors = true;
        }
        m_errors = false;
    }

    [[nodiscard]] GLuint OpenGLShader::get_id() const
    {
        return m_id;
    }

} // Anonymous namespace

Shader::Shader(ShaderInfo* shader_info, std::size_t shader_count)
{
    init(shader_info, shader_count);
}

Shader::~Shader()
{
    deinit();
}

void Shader::deinit()
{
    if (initialized) {
        if (!m_errors) {
            glDeleteProgram(m_id);
        }
    }
    initialized = false;
}

void Shader::init(ShaderInfo* shader_info, std::size_t shader_count)
{
    util_assert(initialized == false, "already initialized");

    static constexpr size_t MAX_SHADER_COUNT = 5;
    if (shader_count > MAX_SHADER_COUNT) {
        LOG_ERROR(std::format("shader programs do not currently support more than {} shaders\n", MAX_SHADER_COUNT));
        m_errors = true;
        util_assert(m_errors == false, "shader program has errors");
        return;
    }
    std::array<OpenGLShader, MAX_SHADER_COUNT> shaders;

    m_id = glCreateProgram();

    for (std::size_t i = 0; i < shader_count; i++) {
        shaders.at(i).init(shader_info[i].is_file, shader_info[i].shader, shader_info[i].type);

        glAttachShader(m_id, shaders[i].get_id());
    }

    glLinkProgram(m_id);

    m_errors = errors_internal();

    util_assert(m_errors == false, "shader program has errors");

    initialized = true;
}

[[nodiscard]] bool Shader::is_initialized() const
{
    return initialized;
}

bool Shader::has_errors() const
{
    util_assert(initialized == true, "not initialized");
    return m_errors;
}

bool Shader::errors_internal() const
{
    int program_linked = 0;

    glGetProgramiv(m_id, GL_LINK_STATUS, &program_linked);

    if (program_linked == GL_FALSE) {
        GLint max_length = 0;
        glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &max_length);

        std::vector<GLchar> error_log(max_length);
        glGetProgramInfoLog(m_id, max_length, &max_length, error_log.data());

        LOG_ERROR(std::format("shader program failed to link: {}\n", error_log.data()))
        return true;
    }

    return false;
}

void Shader::bind()
{
    util_assert(initialized == true, "not initialized");
    glUseProgram(m_id);
}

void Shader::set_bool(const char* name, bool value)
{
    util_assert(initialized == true, "not initialized");
    glUniform1i(glGetUniformLocation(m_id, name), value);
}

void Shader::set_int(const char* name, int value)
{
    util_assert(initialized == true, "not initialized");
    glUniform1i(glGetUniformLocation(m_id, name), value);
}

void Shader::set_float(const char* name, float value)
{
    util_assert(initialized == true, "not initialized");
    glUniform1f(glGetUniformLocation(m_id, name), value);
}

void Shader::set_vec2(const char* name, glm::vec2 value)
{
    util_assert(initialized == true, "not initialized");
    glUniform2fv(glGetUniformLocation(m_id, name), 1, &value[0]);
}

void Shader::set_vec2s(const char* name, float value1, float value2)
{
    util_assert(initialized == true, "not initialized");
    glUniform2f(glGetUniformLocation(m_id, name), value1, value2);
}

void Shader::set_vec3(const char* name, glm::vec3 value)
{
    util_assert(initialized == true, "not initialized");
    glUniform3fv(glGetUniformLocation(m_id, name), 1, &value[0]);
}

void Shader::set_vec3s(const char* name, float value1, float value2, float value3)
{
    util_assert(initialized == true, "not initialized");
    glUniform3f(glGetUniformLocation(m_id, name), value1, value2, value3);
}

void Shader::set_vec4(const char* name, glm::vec4 value)
{
    util_assert(initialized == true, "not initialized");
    glUniform4fv(glGetUniformLocation(m_id, name), 1, &value[0]);
}

void Shader::set_vec4s(const char* name, float value1, float value2, float value3, float value4)
{
    util_assert(initialized == true, "not initialized");
    glUniform4f(glGetUniformLocation(m_id, name), value1, value2, value3, value4);
}

void Shader::set_mat2(const char* name, glm::mat2 value)
{
    util_assert(initialized == true, "not initialized");
    glUniformMatrix2fv(glGetUniformLocation(m_id, name), 1, GL_FALSE, &value[0][0]);
}

void Shader::set_mat3(const char* name, glm::mat3 value)
{
    util_assert(initialized == true, "not initialized");
    glUniformMatrix3fv(glGetUniformLocation(m_id, name), 1, GL_FALSE, &value[0][0]);
}

void Shader::set_mat4(const char* name, glm::mat4 value)
{
    util_assert(initialized == true, "not initialized");
    glUniformMatrix4fv(glGetUniformLocation(m_id, name), 1, GL_FALSE, &value[0][0]);
}

} // namespace Renderer

#include "window.hpp"
#include "SDL3/SDL_video.h"

namespace Renderer {

namespace {

#ifdef _DEBUG
    void GLAPIENTRY
    MessageCallback([[maybe_unused]] GLenum source,
        GLenum type,
        [[maybe_unused]] GLuint id,
        GLenum severity,
        [[maybe_unused]] GLsizei length,
        const GLchar* message,
        [[maybe_unused]] const void* userParam)
    {
        std::println(stderr, "GL CALLBACK: {} type = 0x{:x}, severity = 0x{:x}, message = {}",
            (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
            type, severity, message);
    }
#endif

} // anonymous namespace

Window::Window(const char* name, int width, int height)
{
    init(name, width, height);
}

void Window::init(const char* name, int width, int height)
{
    util_assert(initialized == false, "attempting to reinit SDL3 window");

    m_width = width;
    m_height = height;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        util_error(std::format("Could not initialize SDL: {}", SDL_GetError()));
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    // SDL_GL_SetAttribute(SDL_GL_FLOATBUFFERS, 1);

    m_window = SDL_CreateWindow(name,
        width, height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

    if (m_window == nullptr) {
        SDL_Quit();
        util_error(std::format("Could not create window: {}", SDL_GetError()));
    }

    m_context = SDL_GL_CreateContext(m_window);
    if (m_context == nullptr) {
        SDL_DestroyWindow(m_window);
        SDL_Quit();
        util_error(std::format("Could not create OpenGL context: {}", SDL_GetError()));
    }

    if (gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress) == 0) {
        SDL_GL_DestroyContext(m_context);
        SDL_DestroyWindow(m_window);
        SDL_Quit();
        util_error("Filed to initialize GLAD");
    }

    SDL_GL_MakeCurrent(m_window, m_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

#ifdef _DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);
#endif

    glViewport(0, 0, width, height);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

    ImGui_ImplSDL3_InitForOpenGL(m_window, m_context);
    ImGui_ImplOpenGL3_Init();

    initialized = true;
}

Window::~Window()
{
    // util_assert(initialized == true, "Attempting to call destructor with uninitialized data");
    if (initialized) {
        if (m_window != nullptr) {
            ImGui_ImplOpenGL3_Shutdown();
            ImGui_ImplSDL3_Shutdown();
            ImGui::DestroyContext();

            SDL_GL_DestroyContext(m_context);
            SDL_DestroyWindow(m_window);
            SDL_Quit();
            m_window = nullptr;
        }
        initialized = false;
    }
}

void Window::process_input_callback(const std::function<void(SDL_Event& event)>& commands)
{
    util_assert(initialized == true, "not initialized");
    m_process_input_fn = commands;
}

void Window::process_input_internal()
{
    util_assert(initialized == true, "not initialized");
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            m_should_close = true;
        }
        if (event.type == SDL_EVENT_WINDOW_RESIZED) {
            SDL_GetWindowSize(m_window, &m_width, &m_height);
        }

        bool consumed_by_imgui = false;

        if (!m_relative_mode) {
            ImGui_ImplSDL3_ProcessEvent(&event);

            ImGuiIO& io = ImGui::GetIO();

            bool mouse_event = event.type == SDL_EVENT_MOUSE_MOTION || event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP || event.type == SDL_EVENT_MOUSE_WHEEL;
            bool keyboard_event = event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP || event.type == SDL_EVENT_TEXT_INPUT;

            if (io.WantCaptureMouse && mouse_event) {
                consumed_by_imgui = true;
            }

            if (io.WantCaptureKeyboard && keyboard_event) {
                consumed_by_imgui = true;
            }
        }

        if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
            consumed_by_imgui = false;
        }

        if (!consumed_by_imgui && m_process_input_fn != nullptr) {
            m_process_input_fn(event);
        }
    }
}

void Window::loop(const std::function<void()>& commands)
{
    util_assert(initialized == true, "not initialized");
    while (!m_should_close) {
        process_input_internal();
        if (m_should_close) {
            break;
        }

        // (After event loop)
        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        commands();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        SDL_GL_SwapWindow(m_window);
    }
}

void Window::set_capture_mouse(bool value)
{
    util_assert(initialized == true, "not initialized");
    SDL_CaptureMouse(value);
}

void Window::set_relative_mode(bool value)
{
    util_assert(initialized == true, "not initialized");
    SDL_SetWindowRelativeMouseMode(m_window, value);
    m_relative_mode = value;
}

void Window::set_swap_interval(int value)
{
    util_assert(initialized == true, "not initialized");
    SDL_GL_SetSwapInterval(value);
}

[[nodiscard]] SDL_Window* Window::get_window_ptr() const
{
    util_assert(initialized == true, "not initialized");
    return m_window;
}

[[nodiscard]] glm::ivec2 Window::get_size() const
{
    util_assert(initialized == true, "not initialized");
    return { m_width, m_height };
}

[[nodiscard]] int Window::get_width() const
{
    util_assert(initialized == true, "not initialized");
    return m_width;
}

[[nodiscard]] int Window::get_height() const
{
    util_assert(initialized == true, "not initialized");
    return m_height;
}

[[nodiscard]] glm::vec2 Window::get_size_f32() const
{
    util_assert(initialized == true, "not initialized");
    return { static_cast<f32>(m_width), static_cast<f32>(m_height) };
}

[[nodiscard]] f32 Window::get_aspect_ratio() const
{
    util_assert(initialized == true, "not initialized");
    auto size = get_size_f32();
    return size.x / size.y;
}

[[nodiscard]] f32 Window::get_mouse_x() const
{
    f32 x;
    SDL_GetMouseState(&x, nullptr);
    return x;
}

[[nodiscard]] f32 Window::get_mouse_y() const
{
    f32 y;
    SDL_GetMouseState(nullptr, &y);
    return y;
}

[[nodiscard]] glm::vec2 Window::get_mouse_pos() const
{
    glm::vec2 pos;
    SDL_GetMouseState(&pos.x, &pos.y);
    return pos;
}

void Window::set_should_close()
{
    util_assert(initialized == true, "not initialized");
    m_should_close = true;
}

void Window::set_window_title(const char* title)
{
    util_assert(initialized == true, "not initialized");
    SDL_SetWindowTitle(m_window, title);
}

const char* Window::get_window_title()
{
    util_assert(initialized == true, "not initialized");
    return SDL_GetWindowTitle(m_window);
}

} // namespace Renderer

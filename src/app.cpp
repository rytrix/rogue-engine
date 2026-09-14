#include "app.hpp"

#include "renderer/text.hpp"

#include "utils/file.hpp"
#include "utils/math/ray.hpp"

App::App()
{
    Physics::Engine::setup_singletons();

    m_app_data.m_window.init("Test Window", 800, 600);
    m_app_data.m_window.set_relative_mode(m_app_data.m_capture_mouse);

    m_app_data.m_camera.init(90.0F, 1.0F, 500.0F, m_app_data.m_window.get_aspect_ratio(), { -2.0F, 1.5F, 4.0F });
    m_app_data.m_camera.set_speed(10.0F);

    m_app_data.m_mesh_cache.init(100);
    m_app_data.m_texture_cache.init(500);

    m_app_data.m_default_textures.init(&m_app_data.m_texture_cache);

    m_app_data.m_text_renderer.init("res/fonts/AdwaitaSans-Regular.ttf", 24);
    m_app_data.m_text_renderer.update_view(m_app_data.m_window.get_width(), m_app_data.m_window.get_height());

    m_app_data.m_line_renderer.init();

    m_scene = new Scene(&m_app_data);
    m_scene->m_name = "default_scene";

    m_app_data.m_gizmo.init(&m_app_data);
    m_app_data.m_entity_selector.init(m_scene, &m_app_data);

    Renderer::SkyboxInfo skybox_info {};
    skybox_info.file = "res/skyboxes/Cubemap_Sky_14-512x512.png";
    m_scene->add_component<Renderer::Skybox>(skybox_info);

    std::vector<char> json_buffer;
    auto json_result = Utils::read_file(json_buffer, "default_scene.json");
    if (json_result) {
        nlohmann::json json_scene = nlohmann::json::parse(json_buffer.data());
        m_scene->from_json(json_scene);
    }

    m_app_data.m_window.process_input_callback([&](SDL_Event& event) {
        if (event.type == SDL_EVENT_WINDOW_RESIZED) {
            m_app_data.m_camera.update_aspect(m_app_data.m_window.get_aspect_ratio());
            m_scene->update();
            m_app_data.m_text_renderer.update_view((f32)m_app_data.m_window.get_width(), (f32)m_app_data.m_window.get_height());
        }
        if (event.type == SDL_EVENT_MOUSE_MOTION) {
            if (m_app_data.m_capture_mouse) {
                m_app_data.m_camera.rotate(event.motion.xrel, -event.motion.yrel);
            }
        }
        if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.key == SDLK_ESCAPE) {
                m_app_data.m_capture_mouse = !m_app_data.m_capture_mouse;
                m_app_data.m_window.set_relative_mode(m_app_data.m_capture_mouse);
            }
            if (event.key.key == SDLK_E) {
                m_scene->m_physics_on = !m_scene->m_physics_on;
            }
            if (event.key.key == SDLK_Q) {
                m_app_data.m_window.set_should_close();
            }
        }

        Event engine_event {};
        engine_event.m_type = Event::Type::SDL;
        engine_event.m_sdl_event = event;
        engine_event.m_consumed = false;

        m_app_data.m_gizmo.on_event(engine_event);
        m_app_data.m_entity_selector.on_event(engine_event);
    });

    m_scene->update();
}

App::~App()
{
    delete m_scene;
    Physics::Engine::cleanup_singletons();
}

void App::fps_counter()
{
    static float time_passed;
    static u32 frames;
    static bool initialized;

    if (!initialized) {
        time_passed = 0.0F;
        frames = 0;
        initialized = true;
    } else {
        time_passed += m_scene->get_clock().delta_time<float>();
        frames += 1;
        if (time_passed >= 1.0F) {
            time_passed = 0;
            m_fps = frames;
            frames = 0;
        }
    }
}

void App::spawn_300_cubes()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution dist_x(-15.0f, 15.0f);
    std::uniform_real_distribution dist_y(1.0f, 20.0f);
    std::uniform_real_distribution dist_z(-15.0f, 15.0f);

    for (u32 i = 0; i < 300; i++) {
        Entity entity = m_scene->create_entity();
        Entity::add_mesh(entity, "res/models/physics_cube/cube.obj");
        Transform transform;
        transform.set_position({ dist_x(gen), dist_y(gen), dist_z(gen) });
        Entity::add_transform(entity, transform);
        Entity::add_convex_hull_body(entity);
    }
}

void App::run()
{
    auto scancodes = [&]() {
        if (m_app_data.m_capture_mouse) {
            const bool* keys = SDL_GetKeyboardState(nullptr);
            float delta_time = m_scene->get_clock().delta_time<float>();
            using Dir = Renderer::Camera::Movement;
            if (keys[SDL_SCANCODE_W]) {
                m_app_data.m_camera.move(Dir::Forward, delta_time);
            }
            if (keys[SDL_SCANCODE_S]) {
                m_app_data.m_camera.move(Dir::Backward, delta_time);
            }
            if (keys[SDL_SCANCODE_A]) {
                m_app_data.m_camera.move(Dir::Left, delta_time);
            }
            if (keys[SDL_SCANCODE_D]) {
                m_app_data.m_camera.move(Dir::Right, delta_time);
            }
            if (keys[SDL_SCANCODE_SPACE]) {
                m_app_data.m_camera.move(Dir::Up, delta_time);
            }
            if (keys[SDL_SCANCODE_LSHIFT]) {
                m_app_data.m_camera.move(Dir::Down, delta_time);
            }
        }
    };

    m_app_data.m_window.loop([&]() {
        fps_counter();

        scancodes();

        m_scene->update();
        m_app_data.m_entity_selector.update();

        m_scene->draw();
        m_app_data.m_entity_selector.draw();

        if (m_draw_bodies) {
            m_scene->m_physics_system->draw_bodies();
        }

        m_app_data.m_line_renderer.draw(m_app_data.m_camera);

        m_app_data.m_text_renderer.draw_text(10,
            m_app_data.m_window.get_height() - m_app_data.m_text_renderer.get_max_pixel_height(),
            Utils::format("Framerate {}", m_fps).c_str(), glm::vec3 { 1.0F });

        const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(ImVec2(main_viewport->WorkPos.x + 20, main_viewport->WorkPos.y + 20), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);

        // Main body of the Demo window starts here.
        if (!ImGui::Begin("Debug Window", nullptr, 0)) {
            // Early out if the window is collapsed, as an optimization.
            ImGui::End();
            return;
        }

        if (ImGui::Checkbox("Toggle vsync", &m_vsync)) {
            LOG_INFO(std::format("Setting swap interval to {}", m_vsync));
            if (m_vsync) {
                m_app_data.m_window.set_swap_interval(1);
            } else {
                m_app_data.m_window.set_swap_interval(0);
            }
        }

        ImGui::Checkbox("Toggle physics", &m_scene->m_physics_on);

        ImGui::Checkbox("Toggle draw physics bodies", &m_draw_bodies);

        if (ImGui::Button("Spawn 300 cubes")) {
            spawn_300_cubes();
        }

        if (ImGui::CollapsingHeader(m_scene->m_name.c_str())) {
            m_scene->draw_debug_imgui();
        }

        ImGui::End();
    });
}

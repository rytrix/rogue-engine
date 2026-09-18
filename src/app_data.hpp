#pragma once

#include "scene/resource_manager.hpp"

#include "renderer/camera.hpp"
#include "renderer/window.hpp"

#include "renderer/default_textures.hpp"

#include "renderer/line_renderer.hpp"
#include "renderer/text.hpp"

#include "scene/systems/event_bus.hpp"

#include "scene/systems/entity_selector.hpp"
#include "scene/systems/gizmo.hpp"

struct GlobalAppData {
    Renderer::Window* m_window;
    Renderer::Camera* m_camera;

    TextureCache* m_texture_cache;
    MeshCache* m_mesh_cache;

    Renderer::DefaultTextures* m_default_textures;

    Renderer::TextRenderer* m_text_renderer;
    Renderer::LineRenderer* m_line_renderer;

    EventBus* m_event_bus;

    EntitySelector* m_entity_selector;
    Gizmo* m_gizmo;

    bool m_capture_mouse = true;
};

extern GlobalAppData* g_app_data;

cmake_minimum_required(VERSION 3.22.0)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_definitions(_DEBUG)
endif()

project(rpg)
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/app.cpp

    src/scene/scene.cpp
    src/scene/entity.cpp
    src/scene/transform.cpp
    src/scene/resource_manager.cpp
    src/scene/components/gizmo.cpp
    src/scene/components/entity_selector.cpp

    src/physics_jolt/engine.cpp
    src/physics_jolt/interface.cpp
    src/physics_jolt/debugrenderer.cpp

    src/physics_box3d/engine.cpp

    src/utils/deltatime.cpp
    src/utils/bytestream.cpp
    src/utils/math/ray.cpp
    src/utils/math/aabb.cpp

    src/game_logic/gear.cpp
    src/game_logic/character.cpp

    src/renderer/animation.cpp
    src/renderer/window.cpp
    src/renderer/extensions.cpp
    src/renderer/buffer.cpp
    src/renderer/vertex.cpp
    src/renderer/shader.cpp
    src/renderer/shader_preprocessor.cpp
    src/renderer/texture.cpp
    src/renderer/camera.cpp
    src/renderer/framebuffer.cpp
    src/renderer/renderbuffer.cpp
    src/renderer/mesh.cpp
    src/renderer/mesh_compiler.cpp
    src/renderer/model.cpp
    src/renderer/gbuffer.cpp
    src/renderer/shadowmap.cpp
    src/renderer/quad.cpp
    src/renderer/frustum_culling.cpp
    src/renderer/default_textures.cpp
    src/renderer/random_sampling_texture.cpp
    src/renderer/text.cpp
    src/renderer/skybox.cpp
    src/renderer/line_renderer.cpp

    src/renderer/light/phong/point.cpp
    src/renderer/light/phong/directional.cpp

    src/renderer/light/pbr/point.cpp
    src/renderer/light/pbr/directional.cpp
    src/renderer/light/pbr/spot.cpp
)

target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address,undefined)

target_precompile_headers(${PROJECT_NAME} PUBLIC src/pch.hpp)
target_compile_options(${PROJECT_NAME} PRIVATE -fno-rtti)

add_subdirectory(vendor/sqlite)
add_subdirectory(vendor/glad)
add_subdirectory(vendor/stb)

set(SDL_SHARED OFF CACHE BOOL "Prefer building static" FORCE)
set(SDL_STATIC ON CACHE BOOL "Prefer building static" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "Not testing SDL3")
add_subdirectory(vendor/sdl3)

include_directories(/usr/include/minizip)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "Enable compiling with extra warnings" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "Disable building assimp tests" FORCE)
add_subdirectory(vendor/assimp)

set(ENABLE_ALL_WARNINGS OFF CACHE BOOL "Stop warnings as errors for JoltPhysics" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libs for JoltPhysics" FORCE)
set(DOUBLE_PRECISION OFF CACHE BOOL "Double precision floats for JoltPhysics" FORCE)
add_subdirectory(vendor/JoltPhysics/Build)

add_subdirectory(vendor/box3d)

add_subdirectory(vendor/freetype)

add_library(imgui
    vendor/imgui/imgui.cpp
    vendor/imgui/imgui_demo.cpp
    vendor/imgui/imgui_draw.cpp
    vendor/imgui/imgui_tables.cpp
    vendor/imgui/imgui_widgets.cpp

    vendor/imgui/backends/imgui_impl_sdl3.cpp
    vendor/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui
    PUBLIC vendor/imgui
    PUBLIC vendor/imgui/backends
)

target_link_libraries(imgui
    GL
    SDL3::SDL3
)

target_link_libraries(${PROJECT_NAME}
    sqlite
    glad
    stb_image
    SDL3::SDL3
    assimp
    Jolt::Jolt
    imgui
    freetype
    box3d::box3d
)

target_include_directories(${PROJECT_NAME}
    PUBLIC src/renderer/include
    PUBLIC vendor
    PUBLIC vendor/glad/include/
    PUBLIC vendor/stb/include
    PUBLIC vendor/sdl3/include
    PUBLIC vendor/glm/
    PUBLIC vendor/assimp/include
    PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/vendor/assimp/include
    PUBLIC vendor/JoltPhysics
    PUBLIC vendor/entt/single_include
    PUBLIC vendor/freetype/include
    PUBLIC vendor/box3d/include
)

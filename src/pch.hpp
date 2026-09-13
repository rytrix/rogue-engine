#pragma once

#include <json/json.hpp>
#include <sqlite/sqlite3.h>
#include <stb_image.h>

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <glad/glad.h>

#include "SDL3/SDL_events.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_scancode.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Geometry/Triangle.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

#include <entt/entt.hpp>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "utils/assert.hpp"
#include "utils/default.hpp"
#include "utils/log.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <print>

#include <array>
#include <cmath>
#include <cstdint>
#include <deque>
#include <numbers>
#include <random>
#include <stdexcept>
#include <stdfloat>
#include <utility>
#include <vector>
#include <bitset>

using usize = std::size_t;
using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using f32 = float;
using f64 = double;

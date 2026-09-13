#include "engine.hpp"

#include "helpers.hpp"
#include "interface.hpp"

#include "../app_data.hpp"
#include "../utils/color.hpp"

namespace Physics {

namespace {
    JPH::JobSystemThreadPool* s_job_system = nullptr;
}

namespace Engine {

    void setup_singletons()
    {
        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();
        s_job_system = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);
    }

    void cleanup_singletons()
    {
        delete s_job_system;
        s_job_system = nullptr;

        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }

} // namespace Engine

System::System(Scene* scene, GlobalAppData* app_data)
    : m_debug_renderer(app_data)
    , m_scene(scene)
{
    m_physics_system.Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, m_broad_phase_layer_interface, m_object_vs_broadphase_layer_filter, m_object_vs_object_layer_filter);
    m_body_interface = &m_physics_system.GetBodyInterface();
}

System::~System()
{
    // Unregisters all types with the factory and cleans up the default material
    JPH::UnregisterTypes();
}

void System::update(float delta_time)
{
    // Cap the input delta time to prevent problems during hitches
    const float max_physics_delta = 0.1f;
    const float clamped_delta = std::min(delta_time, max_physics_delta);

    // If clamped_delta is 0.1s, this yields ceil(0.1 / 0.01666) = 6 steps
    const float target_step_size = 1.0f / 60.0f;
    const int collision_steps = std::max(static_cast<int>(std::ceil(clamped_delta / target_step_size)), 1);

    m_physics_system.Update(clamped_delta, collision_steps, &m_temp_allocator, s_job_system);
}

void System::create_mesh_triangle_list(JPH::TriangleList& triangles, const std::deque<Renderer::Mesh>* meshes)
{
    for (usize i = 0; i < meshes->size(); i++) {
        const Renderer::Mesh* mesh = &meshes->at(i);
        glm::vec3 v1;
        glm::vec3 v2;
        glm::vec3 v3;
        u32 j = 0;
        while (j + 2 < mesh->m_vertex_data_view.m_indices.size()) {
            v1 = mesh->m_vertex_data_view.m_vertices[mesh->m_vertex_data_view.m_indices[j + 0]].m_pos;
            v2 = mesh->m_vertex_data_view.m_vertices[mesh->m_vertex_data_view.m_indices[j + 1]].m_pos;
            v3 = mesh->m_vertex_data_view.m_vertices[mesh->m_vertex_data_view.m_indices[j + 2]].m_pos;
            j += 3;

            JPH::Triangle triangle(vec3_to_float3(v1), vec3_to_float3(v2), vec3_to_float3(v3));
            triangles.push_back(triangle);
        }
    }
}

void System::create_mesh_triangle_list(JPH::TriangleList& triangles, const glm::mat4& model, const std::deque<Renderer::Mesh>* meshes)
{
    for (usize i = 0; i < meshes->size(); i++) {
        const Renderer::Mesh* mesh = &meshes->at(i);
        glm::vec3 v1;
        glm::vec3 v2;
        glm::vec3 v3;
        u32 j = 0;
        while (j + 2 < mesh->m_vertex_data_view.m_indices.size()) {
            v1 = model * glm::vec4(mesh->m_vertex_data_view.m_vertices[mesh->m_vertex_data_view.m_indices[j + 0]].m_pos, 1.0F);
            v2 = model * glm::vec4(mesh->m_vertex_data_view.m_vertices[mesh->m_vertex_data_view.m_indices[j + 1]].m_pos, 1.0F);
            v3 = model * glm::vec4(mesh->m_vertex_data_view.m_vertices[mesh->m_vertex_data_view.m_indices[j + 2]].m_pos, 1.0F);
            j += 3;

            JPH::Triangle triangle(vec3_to_float3(v1), vec3_to_float3(v2), vec3_to_float3(v3));
            triangles.push_back(triangle);
        }
    }
}

void System::create_mesh_triangle_list_base_index(JPH::TriangleList& triangles, const Renderer::Mesh* mesh)
{
    usize offset = 0;
    for (usize i = 0; i < mesh->m_vertex_data_view.m_base_vertices.size(); i++) {
        glm::vec3 v1;
        glm::vec3 v2;
        glm::vec3 v3;

        auto base = mesh->m_vertex_data_view.m_base_vertices[i].m_base;
        auto count = mesh->m_vertex_data_view.m_base_vertices[i].m_count;
        u32 j = offset;
        while (j + 2 < count + offset) {
            v1 = mesh->m_vertex_data_view.m_vertices[mesh->m_vertex_data_view.m_indices[j + 0] + base].m_pos;
            v2 = mesh->m_vertex_data_view.m_vertices[mesh->m_vertex_data_view.m_indices[j + 1] + base].m_pos;
            v3 = mesh->m_vertex_data_view.m_vertices[mesh->m_vertex_data_view.m_indices[j + 2] + base].m_pos;
            j += 3;

            JPH::Triangle triangle(vec3_to_float3(v1), vec3_to_float3(v2), vec3_to_float3(v3));
            triangles.push_back(triangle);
        }

        offset += count;
    }
}

void System::create_mesh_triangle_list_base_index(JPH::TriangleList& triangles, const glm::mat4& transform, const Renderer::Mesh* mesh)
{
    usize offset = 0;
    for (usize i = 0; i < mesh->m_vertex_data_view.m_base_vertices.size(); i++) {
        glm::vec3 v1;
        glm::vec3 v2;
        glm::vec3 v3;

        auto base = mesh->m_vertex_data_view.m_base_vertices[i].m_base;
        auto count = mesh->m_vertex_data_view.m_base_vertices[i].m_count;
        u32 j = offset;
        while (j + 2 < count + offset) {
            v1 = transform * glm::vec4(mesh->m_vertex_data_view.m_vertices[mesh->m_vertex_data_view.m_indices[j + 0] + base].m_pos, 1.0F);
            v2 = transform * glm::vec4(mesh->m_vertex_data_view.m_vertices[mesh->m_vertex_data_view.m_indices[j + 1] + base].m_pos, 1.0F);
            v3 = transform * glm::vec4(mesh->m_vertex_data_view.m_vertices[mesh->m_vertex_data_view.m_indices[j + 2] + base].m_pos, 1.0F);
            j += 3;

            JPH::Triangle triangle(vec3_to_float3(v1), vec3_to_float3(v2), vec3_to_float3(v3));
            triangles.push_back(triangle);
        }

        offset += count;
    }
}

void System::create_mesh_vec3s_base_index(JPH::Array<JPH::Vec3>& triangles, const Renderer::Mesh* mesh)
{
    usize offset = 0;
    for (usize i = 0; i < mesh->m_vertex_data_view.m_base_vertices.size(); i++) {
        glm::vec3 v1;
        glm::vec3 v2;
        glm::vec3 v3;

        auto base = mesh->m_vertex_data_view.m_base_vertices[i].m_base;
        auto count = mesh->m_vertex_data_view.m_base_vertices[i].m_count;
        u32 j = offset;
        while (j + 2 < count + offset) {
            v1 = mesh->m_vertex_data_view.m_vertices[mesh->m_vertex_data_view.m_indices[j + 0] + base].m_pos;
            v2 = mesh->m_vertex_data_view.m_vertices[mesh->m_vertex_data_view.m_indices[j + 1] + base].m_pos;
            v3 = mesh->m_vertex_data_view.m_vertices[mesh->m_vertex_data_view.m_indices[j + 2] + base].m_pos;
            j += 3;

            triangles.push_back(vec3_to_vec3(v1));
            triangles.push_back(vec3_to_vec3(v2));
            triangles.push_back(vec3_to_vec3(v3));
        }

        offset += count;
    }
}

void System::create_mesh_vec3s_base_index(JPH::Array<JPH::Vec3>& triangles, const glm::mat4& transform, const Renderer::Mesh* mesh)
{
    usize offset = 0;
    for (usize i = 0; i < mesh->m_vertex_data_view.m_base_vertices.size(); i++) {
        glm::vec3 v1;
        glm::vec3 v2;
        glm::vec3 v3;

        auto base = mesh->m_vertex_data_view.m_base_vertices[i].m_base;
        auto count = mesh->m_vertex_data_view.m_base_vertices[i].m_count;
        u32 j = offset;
        while (j + 2 < count + offset) {
            v1 = transform * glm::vec4(mesh->m_vertex_data_view.m_vertices[mesh->m_vertex_data_view.m_indices[j + 0] + base].m_pos, 1.0F);
            v2 = transform * glm::vec4(mesh->m_vertex_data_view.m_vertices[mesh->m_vertex_data_view.m_indices[j + 1] + base].m_pos, 1.0F);
            v3 = transform * glm::vec4(mesh->m_vertex_data_view.m_vertices[mesh->m_vertex_data_view.m_indices[j + 2] + base].m_pos, 1.0F);
            j += 3;

            triangles.push_back(vec3_to_vec3(v1));
            triangles.push_back(vec3_to_vec3(v2));
            triangles.push_back(vec3_to_vec3(v3));
        }

        offset += count;
    }
}

std::optional<JPH::BodyID> System::ray_cast(Utils::Ray ray, float max_distance)
{
    const JPH::NarrowPhaseQuery& narrow_phase = m_physics_system.GetNarrowPhaseQuery();

    JPH::RRayCast jph_ray;
    jph_ray.mOrigin.Set(ray.position.x, ray.position.y, ray.position.z);
    jph_ray.mDirection.Set(ray.direction.x * max_distance, ray.direction.y * max_distance, ray.direction.z * max_distance);

    JPH::RayCastSettings settings;
    JPH::ClosestHitCollisionCollector<JPH::CastRayCollector> collector;

    narrow_phase.CastRay(jph_ray, settings, collector);

    if (collector.HadHit()) {
        const JPH::RayCastResult& hit = collector.mHit;

        // float hit_fraction = hit.mFraction;
        // JPH::RVec3 hit_point = jph_ray.GetPointOnRay(hit_fraction);
        JPH::BodyID body_id = hit.mBodyID;

        return body_id;
    }

    return std::nullopt;
}

void System::add_body(JPH::BodyID body)
{
    m_body_interface->AddBody(body, JPH::EActivation::Activate);
}

void System::remove_body(JPH::BodyID body)
{
    m_body_interface->RemoveBody(body);
}

void System::remove_delete_body(JPH::BodyID body)
{
    m_body_interface->RemoveBody(body);
    m_body_interface->DestroyBody(body);
}

class Filter : public JPH::BodyDrawFilter {
public:
    Filter(Scene* scene)
        : m_scene(scene)
    {
    }

    virtual bool ShouldDraw(const JPH::Body& inBody) const override
    {
        auto view = m_scene->m_registry.view<PhysicsInfo>();
        for (auto [entity, info] : view.each()) {
            if (info.m_should_debug_draw && info.m_id == inBody.GetID()) {
                return true;
            }
        }

        return false;
    }

private:
    Scene* m_scene;
};

void System::draw_bodies()
{
    JPH::BodyManager::DrawSettings draw_settings;
    draw_settings.mDrawShapeWireframe = true;
    Filter filter(m_scene);
    m_physics_system.DrawBodies(draw_settings, &m_debug_renderer, &filter);
}

void System::optimize()
{
    m_physics_system.OptimizeBroadPhase();
}

} // namespace Physics

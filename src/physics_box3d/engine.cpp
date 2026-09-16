#include "engine.hpp"

#include "../scene/scene.hpp"

#include "helpers.hpp"

#define ENTITY_IMPL
#include "../scene/entity.hpp"

namespace PhysicsBox3d {

namespace {

    struct b3VerticesIndices {
        std::vector<b3Vec3> m_vertices;
        std::vector<i32> m_indices;

        b3VerticesIndices(Entity entity);
        void init(Entity entity);
    };

    b3VerticesIndices::b3VerticesIndices(Entity entity)
    {
        init(entity);
    }

    void b3VerticesIndices::init(Entity entity)
    {
        auto& mesh = entity.get_component<Renderer::Mesh*>();
        auto& transform = entity.get_component<Utils::Transform>();

        m_vertices.reserve(mesh->m_vertex_data_view.m_vertices.size());
        for (auto& vertices : mesh->m_vertex_data_view.m_vertices) {
            auto model = transform.get_model_matrix();
            glm::vec3 vertex = model * glm::vec4(vertices.m_pos, 1.0F);
            m_vertices.emplace_back(vec3_to_vec3(vertex));
        }

        m_indices.reserve(mesh->m_vertex_data_view.m_indices.size());
        i32 index_offset = 0;
        for (u32 i = 0; i < mesh->m_vertex_data_view.m_base_vertices.size(); i++) {
            auto& base_vertex = mesh->m_vertex_data_view.m_base_vertices[i];
            auto base = base_vertex.m_base;
            auto count = base_vertex.m_count;

            for (i32 j = 0; j < count; j++) {
                i32 local_index = mesh->m_vertex_data_view.m_indices[index_offset + j];

                m_indices.push_back(local_index + base);
            }

            index_offset += count;
        }
    }

} // anonymous namespace

Engine::Engine(Scene* scene)
    : m_scene(scene)
{
    b3WorldDef world_def = b3DefaultWorldDef();

    m_world_id = b3CreateWorld(&world_def);
}

Engine::~Engine()
{
    b3DestroyWorld(m_world_id);
}

void Engine::update(float delta_time)
{
    const float dt_step = 1.0F / 60.0F;
    const int sub_step_count = 4;

    m_dt_accumulator += delta_time;

    while (m_dt_accumulator >= dt_step) {
        b3World_Step(m_world_id, dt_step, sub_step_count);
        m_dt_accumulator -= dt_step;
    }
}

void Engine::set_body_transform(b3BodyId body, const glm::vec3& pos, const glm::quat& rot)
{
    b3Body_SetTransform(body, vec3_to_vec3(pos), quat_to_quat(rot));
}

void Engine::set_body_transform(b3BodyId body, const Utils::Transform& transform)
{
    b3Body_SetTransform(body, vec3_to_vec3(transform.get_position()), quat_to_quat(transform.get_rotation()));
    b3Body_SetAwake(body, true);
}

void Engine::get_body_transform(Utils::Transform& transform, b3BodyId body)
{
    b3WorldTransform b3_transform = b3Body_GetTransform(body);

    glm::vec3 pos = vec3_to_vec3(b3_transform.p);
    glm::quat quat = quat_to_quat(b3_transform.q);

    transform.set_position(pos);
    transform.set_rotation(quat);
}

glm::vec3 Engine::get_body_pos(b3BodyId body)
{
    auto pos = b3Body_GetPosition(body);
    return vec3_to_vec3(pos);
}

glm::quat Engine::get_body_rot(b3BodyId body)
{
    auto pos = b3Body_GetRotation(body);
    return quat_to_quat(pos);
}

b3BodyId Engine::add_body(const b3BodyDef* body)
{
    return b3CreateBody(m_world_id, body);
}

void Engine::remove_body(const b3BodyId body)
{
    b3DestroyBody(body);
}

std::optional<b3BodyId> Engine::ray_cast(Utils::Ray ray, float max_distance)
{
    b3QueryFilter filter = b3DefaultQueryFilter();
    filter.name = "engine raycast";
    auto result = b3World_CastRayClosest(m_world_id, vec3_to_vec3(ray.position), vec3_to_vec3(ray.direction * max_distance), filter);
    if (result.hit) {
        return b3Shape_GetBody(result.shapeId);
    }

    return std::nullopt;
}

EntityInfo Engine::create_mesh_body(Entity entity)
{
    b3VerticesIndices b3_vi(entity);

    b3MeshDef def {};
    def.vertices = b3_vi.m_vertices.data();
    def.vertexCount = b3_vi.m_vertices.size();
    def.indices = b3_vi.m_indices.data();
    def.triangleCount = b3_vi.m_indices.size() / 3;
    def.weldVertices = true;
    def.identifyEdges = true;

    b3MeshData* mesh_data = b3CreateMesh(&def, NULL, 0);
    if (mesh_data == NULL) {
        EntityInfo info {};
        return info;
    }
    b3ShapeDef shape_def = b3DefaultShapeDef();
    b3BodyDef body_def = b3DefaultBodyDef();
    body_def.type = b3BodyType::b3_staticBody;

    auto body_id = add_body(&body_def);

    b3ShapeId shape_id = b3CreateMeshShape(body_id, &shape_def, mesh_data, { 1.0F, 1.0F, 1.0F });
    b3DestroyMesh(mesh_data);

    EntityInfo info {};
    info.m_motion_type = motion_type(body_def.type);
    info.m_type = Type::Mesh;
    info.m_id = body_id;
    info.m_shape = shape_id;
    info.m_should_debug_draw = false;
    info.m_entity = entity;

    return info;
}

EntityInfo Engine::create_hull_body(Entity entity)
{
    auto& mesh = entity.get_component<Renderer::Mesh*>();
    auto& transform = entity.get_component<Utils::Transform>();

    std::vector<b3Vec3> b3_vertices;
    b3_vertices.reserve(mesh->m_vertex_data_view.m_vertices.size());
    for (auto& vertices : mesh->m_vertex_data_view.m_vertices) {
        b3_vertices.emplace_back(vec3_to_vec3(vertices.m_pos));
    }

    b3HullData* hull_data = b3CreateHull(b3_vertices.data(), b3_vertices.size(), std::clamp(b3_vertices.size(), 0UL, 100UL));
    if (hull_data == NULL) {
        EntityInfo info {};
        return info;
    }
    b3ShapeDef shape_def = b3DefaultShapeDef();
    b3BodyDef body_def = b3DefaultBodyDef();
    body_def.type = b3BodyType::b3_dynamicBody;
    body_def.position = vec3_to_vec3(transform.get_position());
    body_def.rotation = quat_to_quat(transform.get_rotation());

    b3BodyId body_id = add_body(&body_def);

    b3ShapeId shape_id = b3CreateHullShape(body_id, &shape_def, hull_data);
    b3DestroyHull(hull_data);

    EntityInfo info {};
    info.m_motion_type = motion_type(body_def.type);
    info.m_type = Type::ConvexHull;
    info.m_id = body_id;
    info.m_shape = shape_id;
    info.m_should_debug_draw = false;
    info.m_entity = entity;

    return info;
}

} // namespace PhysicsBox3d

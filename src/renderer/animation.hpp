#pragma once

#include <assimp/scene.h>
#include <memory_resource>

#include "../utils/bytestream.hpp"
#include "../utils/string.hpp"

namespace Renderer {

template <typename T>
struct KeyFrame {
    float m_time;
    T m_value;
};

struct FrameCache {
    u32 m_prev_position_frame;
    u32 m_prev_rotation_frame;
    u32 m_prev_scaling_frame;
};

struct KeyFrameResult {
    aiVector3D m_scaling;
    aiQuaternion m_rotation;
    aiVector3D m_position;

    [[nodiscard]] glm::mat4 blend_to_mat4(const KeyFrameResult& other, float factor) const;
    [[nodiscard]] glm::mat4 to_mat4() const;
};

struct NodeAnim {
    u32 m_index;
    u32 m_parent_index;
    bool m_has_animation;

    KeyFrame<aiVector3D>* m_scaling_keys;
    usize m_scaling_size;

    KeyFrame<aiQuaternion>* m_rotation_keys;
    usize m_rotation_size;

    KeyFrame<aiVector3D>* m_position_keys;
    usize m_position_size;

    glm::mat4 m_global_transform;
    glm::mat4 m_node_transform;
    glm::mat4 m_offset;

    KeyFrameResult keyframe_to_keyframe_result(float animation_time, FrameCache& cache);
    glm::mat4 keyframe_to_mat4(float animation_time, FrameCache& cache);

private:
    template <typename T>
    u32 find_keyframe(u32& cache, const T* keys, u32 keys_size, float animation_time);

    template <typename T, typename R>
    static R lerp_interpolate(T* p_start, T* p_end, float animation_time);
    static aiQuaternion slerp_interpolate(KeyFrame<aiQuaternion>* p_start, KeyFrame<aiQuaternion>* p_end, float animation_time);
};

struct PerAnimationData;

struct Animation {
    Utils::String m_name;

    void init(const aiScene* scene, const aiAnimation* animation,
        const std::unordered_map<Utils::String, u32>& bone_indices,
        const glm::mat4& global_inverse_transform);

    [[nodiscard]] PerAnimationData* create_per_animation_data();
    static void update_transforms(PerAnimationData* data);
    static void update_transforms_blended(PerAnimationData* first, PerAnimationData* second, float factor);

    [[nodiscard]] float get_ticks_per_second() const { return m_ticks_per_second; };
    void set_ticks_per_second(float ticks_per_second) { m_ticks_per_second = ticks_per_second; };

    [[nodiscard]] float get_total_animation_time() const { return m_total_animation_time; };

    void serialize(Utils::ByteStream& bytestream) const;
    [[nodiscard]] u8* deserialize(u8* start_ptr);

private:
    // Allocate with an arena
    std::pmr::monotonic_buffer_resource m_allocator;

    // Needs to be sorted in a way such that
    // all parents get calculated before each child
    NodeAnim* m_nodes;
    usize m_nodes_size;

    glm::mat4 m_global_inverse_transform;

    float m_total_animation_time = 0.0;
    float m_ticks_per_second = 0.0;

    void evaluate_scene(const aiScene* scene, const aiNode* node, const std::unordered_map<Utils::String, u32>& bone_indices);
    void evaluate_parents(const aiNode* node, const std::unordered_map<Utils::String, u32>& bone_indices, u32 parent_index);
};

struct PerAnimationData {
    FrameCache* m_cache;
    usize m_cache_size;

    glm::mat4* m_final_transforms;
    usize m_final_transforms_size;

    float m_animation_time;

    Animation* m_animation;

    void update_transforms();
    void update_transforms_blended(PerAnimationData* other, float factor);
};

struct AnimationData {
    std::vector<PerAnimationData*> data;
    u32 selected_animation = 0;
    // UINT32_MAX is invalid (so I don't have to have a boolean)
    u32 second_animation = UINT32_MAX;
    float blend_factor = 0.0F;

    bool paused = false;
};

} // namespace Renderer

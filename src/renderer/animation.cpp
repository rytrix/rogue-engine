#include "animation.hpp"

#include "../utils/helpers.hpp"

#include "mesh.hpp"

namespace Renderer {

[[nodiscard]] glm::mat4 KeyFrameResult::blend_to_mat4(const KeyFrameResult& other, float factor) const
{
    factor = std::min(1.0F, std::max(0.0F, factor));

    aiVector3D position = m_position + factor * (other.m_position - m_position);

    aiQuaternion rotation;
    aiQuaternion::Interpolate(rotation, m_rotation, other.m_rotation, factor);
    rotation.Normalize();

    aiVector3D scaling = m_scaling + factor * (other.m_scaling - m_scaling);

    return mat4_to_mat4(aiMatrix4x4(scaling, rotation, position));
}

[[nodiscard]] glm::mat4 KeyFrameResult::to_mat4() const
{
    return mat4_to_mat4(aiMatrix4x4(m_scaling, m_rotation, m_position));
}

KeyFrameResult NodeAnim::keyframe_to_keyframe_result(float animation_time, FrameCache& cache)
{
    aiVector3D pos;
    if (m_position_size == 1) {
        pos = m_position_keys[0].m_value;
    } else {
        auto pos_keyframe = find_keyframe(cache.m_prev_position_frame, m_position_keys, m_position_size, animation_time);
        pos = lerp_interpolate<KeyFrame<aiVector3D>, aiVector3D>(&m_position_keys[pos_keyframe], &m_position_keys[pos_keyframe + 1], animation_time);
        // pos = m_anim->mPositionKeys[pos_keyframes[0]].mValue;
    }

    aiQuaternion rot;
    if (m_rotation_size == 1) {
        rot = m_rotation_keys[0].m_value;
    } else {
        auto rot_keyframe = find_keyframe(cache.m_prev_rotation_frame, m_rotation_keys, m_rotation_size, animation_time);
        rot = slerp_interpolate(&m_rotation_keys[rot_keyframe], &m_rotation_keys[rot_keyframe + 1], animation_time);
        // rot = m_anim->mRotationKeys[rot_keyframes[0]].mValue;
    }

    aiVector3D scale;
    if (m_scaling_size == 1) {
        scale = m_scaling_keys[0].m_value;
    } else {
        auto scale_keyframe = find_keyframe(cache.m_prev_scaling_frame, m_scaling_keys, m_scaling_size, animation_time);
        scale = lerp_interpolate<KeyFrame<aiVector3D>, aiVector3D>(&m_scaling_keys[scale_keyframe], &m_scaling_keys[scale_keyframe + 1], animation_time);
        // scale = m_anim->mScalingKeys[scale_keyframes[0]].mValue;
    }

    return KeyFrameResult { .m_scaling = scale, .m_rotation = rot, .m_position = pos };
}

glm::mat4 NodeAnim::keyframe_to_mat4(float animation_time, FrameCache& cache)
{
    auto result = keyframe_to_keyframe_result(animation_time, cache);

    return result.to_mat4();
}

template <typename T>
u32 NodeAnim::find_keyframe(u32& cache, const T* keys, u32 keys_size, float animation_time)
{
    // util_assert(animation_time <= keys[keys_size - 1].m_time, "keyframe time is greater than the max keyframe time");
    if (animation_time > keys[keys_size - 1].m_time) {
        return keys_size - 1;
    }

    for (u32 i = cache; i < keys_size - 1; i++) {
        if (animation_time >= keys[i].m_time && animation_time < keys[i + 1].m_time) {
            cache = i;
            return cache;
        }
    }
    for (u32 i = 0; i < cache; i++) {
        if (animation_time < keys[i + 1].m_time) {
            cache = i;
            return cache;
        }
    }
    cache = 0;
    return cache;
    util_error("Could not find keyframe");
}

template <typename T, typename R>
R NodeAnim::lerp_interpolate(T* p_start, T* p_end, float animation_time)
{
    float delta_time = p_end->m_time - p_start->m_time;
    float factor = (animation_time - p_start->m_time) / delta_time;
    // util_assert(factor >= 0.0 && factor <= 1.0, std::format("Lerp({}) factor not in range 0 to 1", factor));
    if (factor < 0.0F || factor > 1.0F) {
        LOG_WARN(std::format("Lerp({}) factor not in range 0 to 1", factor));
    }
    factor = std::min(1.0F, std::max(0.0F, factor));

    return p_start->m_value + factor * (p_end->m_value - p_start->m_value);
}

aiQuaternion NodeAnim::slerp_interpolate(KeyFrame<aiQuaternion>* p_start, KeyFrame<aiQuaternion>* p_end, float animation_time)
{
    aiQuaternion result;
    float delta_time = p_end->m_time - p_start->m_time;
    float factor = (animation_time - p_start->m_time) / delta_time;
    // util_assert(factor >= 0.0F && factor <= 1.0F, std::format("Slerp({}) factor not in range 0 to 1", factor));
    if (factor < 0.0F || factor > 1.0F) {
        LOG_WARN(std::format("Slerp({}) factor not in range 0 to 1", factor));
    }
    factor = std::min(1.0F, std::max(0.0F, factor));

    aiQuaternion::Interpolate(result, p_start->m_value, p_end->m_value, factor);
    result.Normalize();

    return result;
}

void Animation::init(const aiScene* scene, const aiAnimation* animation, const std::unordered_map<Utils::String, u32>& bone_indices, const glm::mat4& global_inverse_transform)
{
    m_name = animation->mName.C_Str();

    m_total_animation_time = (float)animation->mDuration;
    m_ticks_per_second = (float)animation->mTicksPerSecond;
    if (m_ticks_per_second == 0.0F) {
        m_ticks_per_second = 25.0F;
    }

    m_nodes_size = bone_indices.size();
    m_nodes = (NodeAnim*)m_allocator.allocate(sizeof(NodeAnim) * m_nodes_size);
    memset(m_nodes, 0, sizeof(NodeAnim) * m_nodes_size);

    util_assert(m_nodes_size <= MAX_BONES,
        std::format("Exceded max bone limit of {} with {} bones",
            MAX_BONES,
            m_nodes_size));

    m_global_inverse_transform = global_inverse_transform;

    evaluate_scene(scene, scene->mRootNode, bone_indices);
    evaluate_parents(scene->mRootNode, bone_indices, UINT32_MAX);

    for (u32 i = 0; i < animation->mNumChannels; i++) {
        aiNodeAnim* ai_node_anim = animation->mChannels[i];

        u32 index = 0;
        if (bone_indices.contains(ai_node_anim->mNodeName.C_Str())) {
            index = bone_indices.at(ai_node_anim->mNodeName.C_Str());
        } else {
            LOG_WARN(std::format("From animation \"{}\", bone \"{}\" not found",
                m_name.c_str(),
                ai_node_anim->mNodeName.C_Str()));
            continue;
        }

        NodeAnim* node_anim = &m_nodes[index];
        node_anim->m_has_animation = true;

        node_anim->m_position_size = ai_node_anim->mNumPositionKeys;
        node_anim->m_position_keys = (KeyFrame<aiVector3D>*)m_allocator.allocate(node_anim->m_position_size * sizeof(KeyFrame<aiVector3D>));
        for (u32 j = 0; j < ai_node_anim->mNumPositionKeys; j++) {
            node_anim->m_position_keys[j].m_time = static_cast<float>(ai_node_anim->mPositionKeys[j].mTime);
            node_anim->m_position_keys[j].m_value = ai_node_anim->mPositionKeys[j].mValue;
        }

        node_anim->m_rotation_size = ai_node_anim->mNumRotationKeys;
        node_anim->m_rotation_keys = (KeyFrame<aiQuaternion>*)m_allocator.allocate(node_anim->m_rotation_size * sizeof(KeyFrame<aiQuaternion>));
        for (u32 j = 0; j < ai_node_anim->mNumRotationKeys; j++) {
            node_anim->m_rotation_keys[j].m_time = static_cast<float>(ai_node_anim->mRotationKeys[j].mTime);
            node_anim->m_rotation_keys[j].m_value = ai_node_anim->mRotationKeys[j].mValue;
        }

        node_anim->m_scaling_size = ai_node_anim->mNumScalingKeys;
        node_anim->m_scaling_keys = (KeyFrame<aiVector3D>*)m_allocator.allocate(node_anim->m_scaling_size * sizeof(KeyFrame<aiVector3D>));
        for (u32 j = 0; j < ai_node_anim->mNumScalingKeys; j++) {
            node_anim->m_scaling_keys[j].m_time = static_cast<float>(ai_node_anim->mScalingKeys[j].mTime);
            node_anim->m_scaling_keys[j].m_value = ai_node_anim->mScalingKeys[j].mValue;
        }
    }
}

void Animation::serialize(Utils::ByteStream& bytestream) const
{
    bytestream.append_bytes(&m_nodes_size, sizeof(m_nodes_size));
    bytestream.align(alignof(NodeAnim));
    bytestream.append_bytes(m_nodes, m_nodes_size * sizeof(NodeAnim));
    bytestream.append_bytes(&m_global_inverse_transform, sizeof(m_global_inverse_transform));
    bytestream.append_bytes(&m_total_animation_time, sizeof(m_total_animation_time));
    bytestream.append_bytes(&m_ticks_per_second, sizeof(m_ticks_per_second));

    for (usize i = 0; i < m_nodes_size; i++) {
        NodeAnim* node = &m_nodes[i];

        bytestream.align(alignof(KeyFrame<aiVector3D>));
        bytestream.append_bytes(&node->m_scaling_size, sizeof(node->m_scaling_size));
        bytestream.append_bytes(node->m_scaling_keys, node->m_scaling_size * sizeof(KeyFrame<aiVector3D>));

        bytestream.align(alignof(KeyFrame<aiQuaternion>));
        bytestream.append_bytes(&node->m_rotation_size, sizeof(node->m_rotation_size));
        bytestream.append_bytes(node->m_rotation_keys, node->m_rotation_size * sizeof(KeyFrame<aiQuaternion>));

        bytestream.align(alignof(KeyFrame<aiVector3D>));
        bytestream.append_bytes(&node->m_position_size, sizeof(node->m_position_size));
        bytestream.append_bytes(node->m_position_keys, node->m_position_size * sizeof(KeyFrame<aiVector3D>));
    }
}

u8* Animation::deserialize(u8* start_ptr)
{
    u8* ptr = start_ptr;
    std::memcpy(&m_nodes_size, ptr, sizeof(m_nodes_size));
    ptr += sizeof(m_nodes_size);

    ptr = Utils::ByteStream::align_ptr(ptr, alignof(NodeAnim));
    m_nodes = (NodeAnim*)ptr;
    ptr += m_nodes_size * sizeof(*m_nodes);

    std::memcpy(&m_global_inverse_transform, ptr, sizeof(glm::mat4));
    ptr += sizeof(m_global_inverse_transform);

    std::memcpy(&m_total_animation_time, ptr, sizeof(m_total_animation_time));
    ptr += sizeof(m_total_animation_time);

    std::memcpy(&m_ticks_per_second, ptr, sizeof(m_ticks_per_second));
    ptr += sizeof(m_ticks_per_second);

    for (usize i = 0; i < m_nodes_size; i++) {
        NodeAnim* node = &m_nodes[i];

        // Scaling
        ptr = Utils::ByteStream::align_ptr(ptr, alignof(KeyFrame<aiVector3D>));
        std::memcpy(&node->m_scaling_size, ptr, sizeof(node->m_scaling_size));
        ptr += sizeof(node->m_scaling_size);

        node->m_scaling_keys = (KeyFrame<aiVector3D>*)ptr;
        ptr += node->m_scaling_size * sizeof(*node->m_scaling_keys);

        // Rotation
        ptr = Utils::ByteStream::align_ptr(ptr, alignof(KeyFrame<aiQuaternion>));
        std::memcpy(&node->m_rotation_size, ptr, sizeof(node->m_rotation_size));
        ptr += sizeof(node->m_rotation_size);

        node->m_rotation_keys = (KeyFrame<aiQuaternion>*)ptr;
        ptr += node->m_rotation_size * sizeof(*node->m_rotation_keys);

        // Position
        ptr = Utils::ByteStream::align_ptr(ptr, alignof(KeyFrame<aiVector3D>));
        std::memcpy(&node->m_position_size, ptr, sizeof(node->m_position_size));
        ptr += sizeof(node->m_position_size);

        node->m_position_keys = (KeyFrame<aiVector3D>*)ptr;
        ptr += node->m_position_size * sizeof(*node->m_position_keys);
    }

    return ptr;
}

void Animation::evaluate_scene(const aiScene* scene, const aiNode* node, const std::unordered_map<Utils::String, u32>& bone_indices)
{
    for (u32 i = 0; i < node->mNumMeshes; i++) {
        auto* mesh = scene->mMeshes[node->mMeshes[i]];
        if (mesh->HasBones()) {
            for (u32 j = 0; j < mesh->mNumBones; j++) {
                aiBone* bone = mesh->mBones[j];
                u32 index = bone_indices.at(bone->mName.C_Str());
                m_nodes[index].m_index = index;
                m_nodes[index].m_has_animation = false;
                m_nodes[index].m_offset = mat4_to_mat4(bone->mOffsetMatrix);
            }
        }
    }

    for (u32 i = 0; i < node->mNumChildren; i++) {
        evaluate_scene(scene, node->mChildren[i], bone_indices);
    }
}

void Animation::evaluate_parents(const aiNode* node, const std::unordered_map<Utils::String, u32>& bone_indices, u32 parent_index)
{
    u32 index = parent_index;
    if (bone_indices.contains(node->mName.C_Str())) {
        index = bone_indices.at(node->mName.C_Str());
        m_nodes[index].m_parent_index = parent_index;
        m_nodes[index].m_node_transform = mat4_to_mat4(node->mTransformation);
    }

    for (u32 i = 0; i < node->mNumChildren; i++) {
        evaluate_parents(node->mChildren[i], bone_indices, index);
    }
}

PerAnimationData* Animation::create_per_animation_data()
{
    PerAnimationData* data = (PerAnimationData*)m_allocator.allocate(sizeof(PerAnimationData));

    data->m_cache = (FrameCache*)m_allocator.allocate(m_nodes_size * sizeof(FrameCache));
    data->m_cache_size = m_nodes_size;
    std::memset(data->m_cache, 0, data->m_cache_size * sizeof(FrameCache));

    data->m_animation_time = 0.0F;

    data->m_final_transforms = (glm::mat4*)m_allocator.allocate(sizeof(glm::mat4) * m_nodes_size);
    data->m_final_transforms_size = m_nodes_size;

    data->m_animation = this;

    return data;
}

void Animation::update_transforms(PerAnimationData* data)
{
    Animation* animation = data->m_animation;
    float animation_time = std::fmod(data->m_animation_time * animation->m_ticks_per_second, animation->m_total_animation_time);

    for (u32 i = 0; i < animation->m_nodes_size; i++) {
        NodeAnim* node = &animation->m_nodes[i];

        glm::mat4 parent_transform;
        if (node->m_parent_index != UINT32_MAX) {
            parent_transform = animation->m_nodes[node->m_parent_index].m_global_transform;
        } else {
            parent_transform = glm::mat4(1.0);
        }

        glm::mat4 node_transform;
        if (node->m_has_animation) {
            node_transform = node->keyframe_to_mat4(animation_time, data->m_cache[i]);
        } else {
            node_transform = node->m_node_transform;
        }

        node->m_global_transform = parent_transform * node_transform;

        data->m_final_transforms[node->m_index] = animation->m_global_inverse_transform * node->m_global_transform * node->m_offset;
    }
}

void Animation::update_transforms_blended(PerAnimationData* first, PerAnimationData* second, float factor)
{
    Animation* animation = first->m_animation;
    Animation* animation_2 = second->m_animation;

    float animation_time = std::fmod(first->m_animation_time * animation->m_ticks_per_second, animation->m_total_animation_time);
    float animation_time_2 = std::fmod(second->m_animation_time * animation_2->m_ticks_per_second, animation_2->m_total_animation_time);

    util_assert(animation->m_nodes_size == animation_2->m_nodes_size, "Animations don't have the same number of nodes");

    for (u32 i = 0; i < animation->m_nodes_size; i++) {
        NodeAnim* node = &animation->m_nodes[i];
        NodeAnim* node_2 = &animation_2->m_nodes[i];

        glm::mat4 parent_transform;
        if (node->m_parent_index != UINT32_MAX) {
            // If the animation is using the same model this should be the same
            parent_transform = animation->m_nodes[node->m_parent_index].m_global_transform;
        } else {
            parent_transform = glm::mat4(1.0);
        }

        glm::mat4 node_transform;
        if (node->m_has_animation && node_2->m_has_animation) {
            auto node_transform_keyframe = node->keyframe_to_keyframe_result(animation_time, first->m_cache[i]);
            auto node_transform_keyframe_2 = node_2->keyframe_to_keyframe_result(animation_time_2, second->m_cache[i]);
            node_transform = node_transform_keyframe.blend_to_mat4(node_transform_keyframe_2, factor);
            // For some reason it looks better if I don't have this so I'm just gonna leave it off for now
            // } else if (node->m_has_animation) {
            //     node_transform = node->keyframe_to_mat4(animation_time, first->m_cache[i]);
            // } else if (node_2->m_has_animation) {
            //     node_transform = node_2->keyframe_to_mat4(animation_time_2, second->m_cache[i]);
        } else {
            node_transform = node->m_node_transform;
        }

        node->m_global_transform = parent_transform * node_transform;

        first->m_final_transforms[node->m_index] = animation->m_global_inverse_transform * node->m_global_transform * node->m_offset;
    }
}

void PerAnimationData::update_transforms()
{
    Animation::update_transforms(this);
}

void PerAnimationData::update_transforms_blended(PerAnimationData* other, float factor)
{
    Animation::update_transforms_blended(this, other, factor);
}

} // namespace Renderer

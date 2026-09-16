#pragma once

#include "../utils/string.hpp"

struct Generation {
    u32 valid : 1;
    u32 id : 31;
};

struct Handle {
    Generation generation;
    u32 index;

    bool operator==(const Handle& other)
    {
        return memcmp(this, &other, sizeof(Handle)) == 0;
    }
};

template <typename DataType>
class ResourceManager : public NoCopyNoMove {
public:
    void init(usize size);
    ~ResourceManager();

    Handle create_handle();
    void destroy_handle(Handle handle);
    DataType* get(Handle handle);
    template <typename... Args>
    DataType* create(Handle handle, Args&&... args);

private:
    static constexpr u32 INVALID_NEXT_FREE = UINT32_MAX;
    struct InternalData {
        Generation generation = { .valid = 0, .id = 0 };
        union {
            DataType data {}; // Valid == 1
            u32 next_free; // Valid == 0
        };
    };

    std::allocator<InternalData> m_allocator;
    InternalData* m_data = nullptr;
    usize m_data_size = 0;
    usize m_data_capacity = 0;
    usize m_next_free_index = INVALID_NEXT_FREE;
};

template <typename DataType>
void ResourceManager<DataType>::init(usize size)
{
    util_assert(size < INVALID_NEXT_FREE, std::format("size {} >= UINT32_MAX", size));
    m_data = m_allocator.allocate(size);
    memset((void*)m_data, 0, sizeof(InternalData) * size);
    m_data_capacity = size;
}

template <typename DataType>
ResourceManager<DataType>::~ResourceManager()
{
    for (u32 i = 0; i < m_data_size; i++) {
        if (m_data[i].generation.valid == 1) {
            std::destroy_at(&m_data[i].data);
        }
    }

    m_allocator.deallocate(m_data, m_data_capacity);
}

template <typename DataType>
Handle ResourceManager<DataType>::create_handle()
{
    util_assert(m_data_size < m_data_capacity,
        std::format("data_size {} >= data_capacity {}", m_data_size, m_data_capacity));

    u32 index;
    if (m_next_free_index != INVALID_NEXT_FREE) {
        u32 next_index = m_data[m_next_free_index].next_free;
        index = m_next_free_index;
        m_next_free_index = next_index;
    } else {
        index = m_data_size++;
    }

    m_data[index].generation.valid = 1;

    Handle handle;
    handle.generation = m_data[index].generation;
    handle.index = index;

    return handle;
}

template <typename DataType>
void ResourceManager<DataType>::destroy_handle(Handle handle)
{
    util_assert(handle.generation.valid == 1, "attempting to remove invalid handle");
    util_assert(handle.index < m_data_size, std::format("handle index {} >= data_size {}", handle.index, m_data_size));

    m_data[handle.index].generation.valid = 0;
    m_data[handle.index].generation.id++;

    m_data[handle.index].data.~DataType();
    m_data[handle.index].next_free = INVALID_NEXT_FREE;

    if (handle.index == m_data_size - 1) {
        m_data_size--;
        return;
    }

    if (m_next_free_index == INVALID_NEXT_FREE) {
        m_next_free_index = handle.index;
        return;
    }

    u32 next_free = m_next_free_index;
    m_next_free_index = handle.index;
    m_data[m_next_free_index].next_free = next_free;
}

template <typename DataType>
DataType* ResourceManager<DataType>::get(Handle handle)
{
    util_assert(handle.index < m_data_capacity, std::format("handle index {} >= data_capacity {}", handle.index, m_data_size));
    util_assert(handle.generation.valid == 1, "attempting to get invalid handle");

    if (handle.index >= m_data_size) {
        LOG_WARN(std::format("data at handle index {} has been invalidated", handle.index));
        return nullptr;
    }

    InternalData& data = m_data[handle.index];
    if (data.generation.valid != 1) {
        LOG_WARN(std::format("data at handle index {} is invalid", handle.index));
        return nullptr;
    }

    if (data.generation.id != handle.generation.id) {
        LOG_WARN(std::format("data at handle index {} has been invalidated", handle.index));
        return nullptr;
    }

    return &data.data;
}

template <typename DataType>
template <typename... Args>
DataType* ResourceManager<DataType>::create(Handle handle, Args&&... args)
{
    DataType* data = get(handle);
    if (data == nullptr) {
        return data;
    } else {
        std::construct_at(data, std::forward<Args>(args)...);
        // new (data) DataType(std::forward<Args>(args)...);
        return data;
    }
}

void run_resource_manager_fuzzer(size_t iterations, size_t pool_size);

template <typename T>
class SceneResourceManager {
public:
    SceneResourceManager(usize size);
    void init(usize size);

    template <typename... Args>
    Handle get_or_create(const Utils::String& name, Args&&... args);
    template <typename... Args>
    Handle create(Args&&... args);

    bool contains(const Utils::String& name);
    Handle get(const Utils::String& name);

    T* get(Handle handle);
    void destroy(Handle handle);

private:
    std::unordered_map<Utils::String, Handle> m_map;
    ResourceManager<T> m_cache;
};


template <typename T>
SceneResourceManager<T>::SceneResourceManager(usize size)
{
    init(size);
}

template <typename T>
void SceneResourceManager<T>::init(usize size)
{
    m_cache.init(size);
}

template <typename T>
template <typename... Args>
Handle SceneResourceManager<T>::get_or_create(const Utils::String& name, Args&&... args)
{
    if (m_map.contains(name)) {
        Handle handle = m_map[name];
        if (m_cache.get(handle) != nullptr) {
            return handle;
        }
    }

    Handle handle = m_cache.create_handle();
    m_map[name] = handle;
    m_cache.create(handle, std::forward<Args>(args)...);

    return handle;
}

template <typename T>
template <typename... Args>
Handle SceneResourceManager<T>::create(Args&&... args)
{
    Handle handle = m_cache.create_handle();
    m_cache.create(handle, std::forward<Args>(args)...);

    return handle;
}

template <typename T>
bool SceneResourceManager<T>::contains(const Utils::String& name)
{
    return m_map.contains(name);
}

template <typename T>
Handle SceneResourceManager<T>::get(const Utils::String& name)
{
    Handle handle = m_map[name];
    if (m_cache.get(handle) == nullptr) {
        util_error("Attempting to get invalid handle");
    }
    return handle;
}

template <typename T>
T* SceneResourceManager<T>::get(Handle handle)
{
    return m_cache.get(handle);
}

template <typename T>
void SceneResourceManager<T>::destroy(Handle handle)
{
    m_cache.destroy_handle(handle);
    for (const auto& pair : m_map) {
        if (pair.second == handle) {
            m_map.erase(pair.first);
            break;
        }
    }
}

namespace Renderer {
class Mesh;
class Texture;
};

#define TextureCache SceneResourceManager<Renderer::Texture>
#define MeshCache SceneResourceManager<Renderer::Mesh>

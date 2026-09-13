#include "bytestream.hpp"

namespace Utils {

ByteStream::ByteStream()
    : ByteStream(DEFAULT_SIZE)
{
}

ByteStream::ByteStream(size_t size_in_bytes)
{
    if (size_in_bytes == 0) {
        return;
    }
    m_data = m_allocator.allocate(size_in_bytes);
    m_capacity = size_in_bytes;
}

ByteStream::~ByteStream()
{
    if (m_data != nullptr) {
        m_allocator.deallocate(m_data, m_capacity);
    }
}

size_t ByteStream::append_bytes(const void* bytes, size_t size_in_bytes)
{
    if (size_in_bytes == 0) {
        return size_in_bytes;
    }

    size_t total_size = m_size + size_in_bytes;

    if (m_capacity < total_size) {
        size_t new_capacity = std::max(m_capacity * 2, total_size);
        resize(new_capacity);
    }

    std::memcpy(m_data + m_size, bytes, size_in_bytes);
    m_size += size_in_bytes;

    return size_in_bytes;
}

void ByteStream::resize(size_t new_capacity)
{
    if (new_capacity <= m_capacity) {
        return;
    }

    u8* old_data = m_data;
    size_t old_capacity = m_capacity;

    m_data = m_allocator.allocate(new_capacity);
    m_capacity = new_capacity;
    if (old_data != nullptr) {
        std::memcpy(m_data, old_data, m_size);
        m_allocator.deallocate(old_data, old_capacity);
    }
}

u8* ByteStream::data()
{
    return m_data;
}

const u8* ByteStream::data() const
{
    return m_data;
}

size_t ByteStream::size() const
{
    return m_size;
}

size_t ByteStream::capacity() const
{
    return m_capacity;
}

} // namespace Utils

#pragma once

namespace Utils {

class ByteStream : NoCopyNoMove {
public:
    ByteStream();
    ByteStream(size_t size_in_bytes);
    ~ByteStream();

    size_t append_bytes(const void* bytes, size_t size_in_bytes);

    void resize(size_t new_capacity);

    u8* data();
    [[nodiscard]] const u8* data() const;
    [[nodiscard]] size_t size() const;
    [[nodiscard]] size_t capacity() const;

private:
    static constexpr size_t DEFAULT_SIZE = 2048;

    std::allocator<u8> m_allocator;
    u8* m_data = nullptr;
    size_t m_capacity = 0;
    size_t m_size = 0;
};

} // namespace Utils

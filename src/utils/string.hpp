#pragma once

namespace Utils {

// A staticly sized string
template <usize Capacity>
class StaticString {
public:
    StaticString() = default;
    StaticString(const char* string);
    StaticString(const std::string& string);
    StaticString(const std::string_view string);

    StaticString& operator=(const char* string);
    StaticString& operator=(const std::string& string);
    StaticString& operator=(const std::string_view string);

    [[nodiscard]] const char* c_str() const;
    [[nodiscard]] std::string_view view() const;
    [[nodiscard]] char* data();
    [[nodiscard]] const char* data() const;
    [[nodiscard]] usize size() const;
    [[nodiscard]] usize capacity() const;

    bool operator==(StaticString other) const;
    bool operator==(std::string_view other) const;
    bool operator==(const char* other) const;

    void set_size(usize size);
    void clear();

    template <typename... Args>
    StaticString& format(std::format_string<Args...> fmt, Args&&... args);

private:
    static constexpr usize SIZE = Capacity - 1;
    char m_data[Capacity] {};
    usize m_size = 0;
};

template <usize Capacity>
StaticString<Capacity>::StaticString(const char* string)
{
    usize len = strlen(string);
    util_assert(len < SIZE, std::format("String size of {} overflowed by const char* string of {}", SIZE, len));
    m_size = len;
    memcpy(m_data, string, len);
}

template <usize Capacity>
StaticString<Capacity>::StaticString(const std::string& string)
{
    usize len = string.size();
    util_assert(len < SIZE, std::format("String size of {} overflowed by std::string of {}", SIZE, len));
    m_size = len;
    memcpy(m_data, string.data(), len);
}

template <usize Capacity>
StaticString<Capacity>::StaticString(const std::string_view string)
{
    usize len = string.size();
    util_assert(len < SIZE, std::format("String size of {} overflowed by std::string_view of {}", SIZE, len));
    m_size = len;
    memcpy(m_data, string.data(), len);
}

template <usize Capacity>
StaticString<Capacity>& StaticString<Capacity>::operator=(const char* string)
{
    usize len = strlen(string);
    util_assert(len < SIZE, std::format("String size of {} overflowed by const char* string of {}", SIZE, len));
    m_size = len;
    memcpy(m_data, string, len);
    return *this;
}

template <usize Capacity>
StaticString<Capacity>& StaticString<Capacity>::operator=(const std::string& string)
{
    usize len = string.size();
    util_assert(len < SIZE, std::format("String size of {} overflowed by std::string of {}", SIZE, len));
    m_size = len;
    memcpy(m_data, string.data(), len);
    return *this;
}

template <usize Capacity>
StaticString<Capacity>& StaticString<Capacity>::operator=(const std::string_view string)
{
    usize len = string.size();
    util_assert(len < SIZE, std::format("String size of {} overflowed by std::string_view of {}", SIZE, len));
    m_size = len;
    memcpy(m_data, string.data(), len);
    return *this;
}

template <usize Capacity>
const char* StaticString<Capacity>::c_str() const
{
    return m_data;
}

template <usize Capacity>
std::string_view StaticString<Capacity>::view() const
{
    return { m_data, m_size };
}

template <usize Capacity>
char* StaticString<Capacity>::data()
{
    return m_data;
}

template <usize Capacity>
const char* StaticString<Capacity>::data() const
{
    return m_data;
}

template <usize Capacity>
usize StaticString<Capacity>::size() const
{
    return m_size;
}

template <usize Capacity>
usize StaticString<Capacity>::capacity() const
{
    return SIZE;
}

template <usize Capacity>
bool StaticString<Capacity>::operator==(StaticString<Capacity> other) const
{
    if (m_size == other.m_size) {
        if (memcmp(m_data, other.m_data, m_size) == 0) {
            return true;
        }
    }
    return false;
}

template <usize Capacity>
bool StaticString<Capacity>::operator==(std::string_view other) const
{
    if (m_size == other.size()) {
        if (memcmp(m_data, other.data(), m_size) == 0) {
            return true;
        }
    }
    return false;
}

template <usize Capacity>
bool StaticString<Capacity>::operator==(const char* other) const
{
    if (m_size == strlen(other)) {
        if (memcmp(m_data, other, m_size) == 0) {
            return true;
        }
    }
    return false;
}

template <usize Capacity>
void StaticString<Capacity>::set_size(usize size)
{
    m_size = size;
}

template <usize Capacity>
void StaticString<Capacity>::clear()
{
    memset(m_data, 0, capacity() + 1);
    m_size = 0;
}

template <usize Capacity>
template <typename... Args>
StaticString<Capacity>& StaticString<Capacity>::format(std::format_string<Args...> fmt, Args&&... args)
{
    clear();
    auto [out, size] = std::format_to_n(m_data, capacity(), fmt, std::forward<Args>(args)...);
    m_size = std::min((usize)size, SIZE);
    util_assert(m_size != 0, "String::format returned size = 0");
    return *this;
}

using String = StaticString<256>;

template <typename... Args>
String format(std::format_string<Args...> fmt, Args&&... args)
{
    Utils::String format_result;
    format_result.format(fmt, std::forward<Args>(args)...);
    return format_result;
}

constexpr String format(const char* fmt)
{
    Utils::String format_result(fmt);
    return format_result;
}

} // namespace Utils

template <>
struct std::hash<Utils::String> {
    std::size_t operator()(Utils::String const& string) const noexcept
    {
        return std::hash<std::string_view> {}(string.view());
    }
};

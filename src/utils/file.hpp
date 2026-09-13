#pragma once

#include <fstream>

namespace Utils {

[[nodiscard]] inline bool read_file(std::vector<char>& buffer, const char* path)
{
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file) {
        LOG_ERROR(std::format("could not open file \"{}\"", path));
        return false;
    }

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    buffer.reserve(size + 1);
    buffer.resize(size);
    file.read(buffer.data(), size);

    buffer.push_back(0);

    return true;
}

[[nodiscard]] inline bool write_file(const char* path, std::span<char> data)
{
    std::ofstream file(path, std::ios::out | std::ios::binary);
    if (!file) {
        LOG_ERROR(std::format("could not open file \"{}\" for writing", path));
        return false;
    }

    file.write(data.data(), data.size());
    if (!file) {
        LOG_ERROR(std::format("failed to write data to file \"{}\"", path));
        return false;
    }

    return true;
}

inline bool is_file_newer(const char* file1, const char* file2) {
    std::error_code ec1;
    auto time1 = std::filesystem::last_write_time(file1, ec1);

    if (ec1) {
        return false;
    }

    std::error_code ec2;
    auto time2 = std::filesystem::last_write_time(file2, ec2);

    if (ec2) {
        return false;
    }

    return time1 > time2;
}

} // namespace Utils

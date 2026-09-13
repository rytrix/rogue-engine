#pragma once

namespace Utils {

enum class LogLevel {
    Trace,
    Info,
    Debug,
    Warn,
    Error,
};

static constexpr LogLevel LOG_LEVEL = LogLevel::Info;
static constexpr bool SHOW_TIMESTAMP = true;
static constexpr bool SHOW_FILE = true;
static constexpr bool SHOW_LINE = true;

template <typename StrType>
constexpr void log(StrType msg, LogLevel level, const char* file, int line)
{
    if (static_cast<int>(level) >= static_cast<int>(LOG_LEVEL)) {
        std::string local_msg;
        if (level == LogLevel::Trace) {
            local_msg += "Trace: ";
        } else if (level == LogLevel::Info) {
            local_msg += "Info: ";
        } else if (level == LogLevel::Debug) {
            local_msg += "Debug: ";
        } else if (level == LogLevel::Warn) {
            local_msg += "Warn: ";
        } else if (level == LogLevel::Error) {
            local_msg += "Error: ";
        }
        if constexpr (SHOW_TIMESTAMP) {
            const auto now = std::chrono::system_clock::now();
            const auto local_time = std::chrono::current_zone()->to_local(now);
            local_msg += std::format("{:%H:%M:%S} ", local_time);
            // local_msg += std::format("{:%Y-%m-%d %H:%M:%S} ", local_time);
        }
        if constexpr (SHOW_FILE) {
            local_msg += file;
            if constexpr (SHOW_LINE) {
                local_msg += std::format(":{}", line);
            }
        } else if constexpr (SHOW_LINE) {
            local_msg += std::format("{}", line);
        }
        std::println(stderr, "{}: {}", local_msg, msg);
    }
}

#define LOG_TRACE(msg)                                                                              \
    if constexpr (static_cast<int>(Utils::LogLevel::Trace) >= static_cast<int>(Utils::LOG_LEVEL)) { \
        Utils::log(msg, Utils::LogLevel::Trace, __FILE__, __LINE__);                                \
    }

#define LOG_INFO(msg)                                                                              \
    if constexpr (static_cast<int>(Utils::LogLevel::Info) >= static_cast<int>(Utils::LOG_LEVEL)) { \
        Utils::log(msg, Utils::LogLevel::Info, __FILE__, __LINE__);                                \
    }

#define LOG_DEBUG(msg)                                                                              \
    if constexpr (static_cast<int>(Utils::LogLevel::Debug) >= static_cast<int>(Utils::LOG_LEVEL)) { \
        Utils::log(msg, Utils::LogLevel::Debug, __FILE__, __LINE__);                                \
    }

#define LOG_WARN(msg)                                                                              \
    if constexpr (static_cast<int>(Utils::LogLevel::Warn) >= static_cast<int>(Utils::LOG_LEVEL)) { \
        Utils::log(msg, Utils::LogLevel::Warn, __FILE__, __LINE__);                                \
    }

#define LOG_ERROR(msg)                                                                              \
    if constexpr (static_cast<int>(Utils::LogLevel::Error) >= static_cast<int>(Utils::LOG_LEVEL)) { \
        Utils::log(msg, Utils::LogLevel::Error, __FILE__, __LINE__);                                \
    }

} // namespace Utils

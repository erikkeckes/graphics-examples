#pragma once

#include <cassert>

#define USE_LOG_COLOR

#define FMT_HEADER_ONLY
#include <fmt/include/fmt/core.h>

namespace SE
{
    class Log
    {
    public:
        template <typename FormatString, typename... Args>
        static void Info(const FormatString& fmt, const Args&... args);

        template <typename FormatString, typename... Args>
        static void Warn(const FormatString& fmt, const Args&... args);

        template <typename FormatString, typename... Args>
        static void Error(const FormatString& fmt, const Args&... args);

        template <typename FormatString, typename... Args>
        static void Critical(const FormatString& fmt, const Args&... args);

    private:
        static constexpr auto magenta = "\033[35m";
        static constexpr auto green = "\033[32m";
        static constexpr auto red = "\033[31m";
        static constexpr auto reset = "\033[0m";
    };

    template <typename FormatString, typename... Args>
    inline void Log::Info(const FormatString& fmt, const Args&... args)
    {
        printf("[%sinfo%s] ", green, reset);
        fmt::print(fmt, args...);
        printf("\n");
    }

    template <typename FormatString, typename... Args>
    inline void Log::Warn(const FormatString& fmt, const Args&... args)
    {
        printf("[%swarn%s] ", magenta, reset);
        fmt::print(fmt, args...);
        printf("\n");
    }

    template <typename FormatString, typename... Args>
    inline void Log::Error(const FormatString& fmt, const Args&... args)
    {
        printf("[%serror%s] ", red, reset);
        fmt::print(fmt, args...);
        printf("\n");
    }

    template <typename FormatString, typename... Args>
    inline void Log::Critical(const FormatString& fmt, const Args&... args)
    {
        printf("[%serror%s] ", red, reset);
        fmt::print(fmt, args...);
        printf("\n");
        assert(false);
    }

}  // namespace SE
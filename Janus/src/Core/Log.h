#pragma once

#ifndef SPDLOG_ACTIVE_LEVEL
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#endif
#include <spdlog/spdlog.h>
#include <intrin.h>

namespace JN
{
    void LogInit();
}

#define JN_LOG_DEBUG(...) SPDLOG_TRACE(__VA_ARGS__)
#define JN_LOG_INFO(...)  SPDLOG_INFO(__VA_ARGS__)
#define JN_LOG_WARN(...)  SPDLOG_WARN(__VA_ARGS__)
#define JN_LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define JN_LOG_FATAL(...) SPDLOG_CRITICAL(__VA_ARGS__)

// Always on. Logs, then breaks into the debugger.
#define JN_CHECK(cond, ...)                          \
    do                                               \
    {                                                \
        if (!(cond))                                 \
        {                                            \
            JN_LOG_FATAL("CHECK failed: {}", #cond); \
            JN_LOG_FATAL(__VA_ARGS__);               \
            __debugbreak();                          \
        }                                            \
    } while (0)

// Debug builds only. Compiled out entirely when NDEBUG is set.
#ifdef NDEBUG
#define JN_ASSERT(cond, ...) ((void)0)
#else
#define JN_ASSERT(cond, ...) JN_CHECK(cond, __VA_ARGS__)
#endif

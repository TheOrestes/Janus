#pragma once

#ifndef SPDLOG_ACTIVE_LEVEL
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#endif
#include <spdlog/spdlog.h>
#include <intrin.h>

namespace JN
{
    extern spdlog::logger* G_pEngineLogger;
    extern spdlog::logger* G_pAppLogger;
    extern spdlog::logger* G_pD3D12Logger;

    void LogInit();
}

#define JN_LOG_DEBUG(...) SPDLOG_LOGGER_TRACE(JN::G_pEngineLogger, __VA_ARGS__)
#define JN_LOG_INFO(...)  SPDLOG_LOGGER_INFO(JN::G_pEngineLogger, __VA_ARGS__)
#define JN_LOG_WARN(...)  SPDLOG_LOGGER_WARN(JN::G_pEngineLogger, __VA_ARGS__)
#define JN_LOG_ERROR(...) SPDLOG_LOGGER_ERROR(JN::G_pEngineLogger, __VA_ARGS__)
#define JN_LOG_FATAL(...) SPDLOG_LOGGER_CRITICAL(JN::G_pEngineLogger, __VA_ARGS__)

#define APP_LOG_DEBUG(...) SPDLOG_LOGGER_TRACE(JN::G_pAppLogger, __VA_ARGS__)
#define APP_LOG_INFO(...)  SPDLOG_LOGGER_INFO(JN::G_pAppLogger, __VA_ARGS__)
#define APP_LOG_WARN(...)  SPDLOG_LOGGER_WARN(JN::G_pAppLogger, __VA_ARGS__)
#define APP_LOG_ERROR(...) SPDLOG_LOGGER_ERROR(JN::G_pAppLogger, __VA_ARGS__)
#define APP_LOG_FATAL(...) SPDLOG_LOGGER_CRITICAL(JN::G_pAppLogger, __VA_ARGS__)

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

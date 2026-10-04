#include "Core/Log.h"

namespace JN
{
    void LogInit()
    {
        spdlog::set_pattern("%^[%T] [%l] %s:%#  %v%$");
        spdlog::set_level(spdlog::level::trace);
    }
}

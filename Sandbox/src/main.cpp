#include "Core/Log.h"

int main()
{
    JN::LogInit();

    JN_LOG_DEBUG("debug {}", 1);
    JN_LOG_INFO("info");
    JN_LOG_WARN("warn");
    JN_LOG_ERROR("error");
    JN_ASSERT(1 + 1 == 2, "math works");

    return 0;
}

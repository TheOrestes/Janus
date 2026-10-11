#include "Core/FileSystem.h"
#include "Core/Log.h"

#include <fstream>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace JN
{
    std::filesystem::path GetExecutableDirectory()
    {
        // 260-char limit is fine for now; a longer install path trips the JN_CHECK below.
        wchar_t buffer[MAX_PATH];
        DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
        JN_CHECK(length > 0 && length < MAX_PATH, "GetModuleFileNameW failed or path too long");

        return std::filesystem::path(buffer).parent_path();
    }

    //-----------------------------------------------------------------------------------------------------------------
    std::filesystem::path GetAssetPath(const std::filesystem::path& relative)
    {
        return GetExecutableDirectory() / "Assets" / relative;
    }

    //-----------------------------------------------------------------------------------------------------------------
    bool ReadAssetFile(const std::filesystem::path& relative, std::vector<char>& outData)
    {
        const std::filesystem::path path = GetAssetPath(relative);
        // Open at the end (ate) to learn the size, then rewind and read it all.
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file)
        {
            JN_LOG_ERROR("Cannot open asset: {}", path.string());
            return false;
        }

        outData.resize(static_cast<size_t>(file.tellg()));
        file.seekg(0);
        file.read(outData.data(), static_cast<std::streamsize>(outData.size()));
        return true;
    }
}

#pragma once

#include <filesystem>
#include <vector>

namespace JN
{
    std::filesystem::path GetExecutableDirectory();

    std::filesystem::path GetAssetPath(const std::filesystem::path& relative);

    bool ReadAssetFile(const std::filesystem::path& relative, std::vector<char>& outData);
}

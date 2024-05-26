#include "asset_manager.hpp"
#include "logger.hpp"
#include <cstring>
#include <filesystem>

namespace Humongous::Systems
{

namespace fs = std::filesystem;

void AssetManager::Init()
{
    const fs::path assetDir = HGASSETDIRPATH;

    HGINFO("Asset directory is: %s", HGASSETDIRPATH);

    if(!fs::exists(assetDir) || !std::filesystem::is_directory(assetDir))
    {
        HGFATAL("Unable to access asset directory!");
        return;
    }

    for(const auto& entry: fs::recursive_directory_iterator(assetDir))
    {
        if(!entry.is_regular_file()) { continue; }

        if(strcmp(entry.path().extension().string().substr(1).c_str(), "spv") == 0)
        {
            std::string fileNameWithExtension = entry.path().filename().string();
            size_t      dotPos = fileNameWithExtension.find('.', fileNameWithExtension.find('.') + 1);
            std::string fileName = entry.path().stem().string().substr(0, dotPos);

            m_shaderMap.emplace(fileName, entry.path().string());
        }
        else if(strcmp(entry.path().extension().string().substr(1).c_str(), "glb") == 0)
        {
            m_modelMap.emplace(entry.path().stem().string(), entry.path().string());
        }
        else if(strcmp(entry.path().extension().string().substr(1).c_str(), "ktx") == 0)
        {
            m_textureMap.emplace(entry.path().stem().string(), entry.path().string());
        }
    }
}

std::string AssetManager::GetAsset(const AssetType type, const std::string_view asset)
{
    switch(type)
    {
        case AssetType::SHADER:
            if(m_shaderMap.find(static_cast<std::string>(asset)) != m_shaderMap.end()) { return m_shaderMap.at(static_cast<std::string>(asset)); }
            else { return ""; }
            break;

        case AssetType::MODEL:
            if(m_modelMap.find(static_cast<std::string>(asset)) != m_modelMap.end()) { return m_modelMap.at(static_cast<std::string>(asset)); }
            else { return ""; }
            break;
        case AssetType::TEXTURE:
            if(m_textureMap.find(static_cast<std::string>(asset)) != m_textureMap.end())
            {
                return m_textureMap.at(static_cast<std::string>(asset));
            }
            else { return ""; }
            break;
    }
}

} // namespace Humongous::Systems

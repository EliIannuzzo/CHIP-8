#include "QuirkStorage.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <imgui.h>

const char* CONFIG_PATH = "config.json";

using json = nlohmann::json;
namespace fs = std::filesystem;
void QuirkStorage::LoadConfig(const std::string& romPath)
{
    fs::path fullPath(romPath);
    std::string romName = fullPath.filename().string();
    
    // If config.json doesn't exist, create an empty one
    if (!fs::exists(CONFIG_PATH)) {
        std::ofstream newFile(CONFIG_PATH);
        newFile << json::object();  // Writes {}
        newFile.close();
    }
    
    // Read the file
    std::ifstream file(CONFIG_PATH);
    json configJson;
    file >> configJson;

    // If ROM not found in config, use defaults
    if (!configJson.contains(romName)) {
        ResetToDefault();
        SaveConfig(romName);
        return;
    }

    // Load quirks from JSON
    auto& quirks = configJson[romName];
    mModernShift            = quirks.value("ModernShiftQuirk", false);
    mModernLoadStore        = quirks.value("ModernLoadStoreQuirk", false);
    mSuperChipJump          = quirks.value("JumpQuirk", false);

    SaveConfig(romName);
}

void QuirkStorage::SaveConfig(const std::string& romPath)
{
    fs::path fullPath(romPath);
    std::string romName = fullPath.filename().string();
    
    // Load current config or start fresh
    json configJson;
    if (std::filesystem::exists(CONFIG_PATH)) {
        std::ifstream inFile(CONFIG_PATH);
        inFile >> configJson;
    }

    // Save current quirk settings for this ROM
    configJson[romName] = {
        { "ModernShiftQuirk", mModernShift },
        { "ModernLoadStoreQuirk", mModernLoadStore },
        { "JumpQuirk", mSuperChipJump }
    };

    std::ofstream outFile(CONFIG_PATH);
    outFile << configJson.dump(4);  // Pretty-print with 4-space indent
}

void QuirkStorage::ResetToDefault()
{
    mModernShift        = false;
    mModernLoadStore    = true;
    mSuperChipJump      = false;
}

void QuirkStorage::DrawImGuiMenu()
{
    if (ImGui::CollapsingHeader("Quirks"))
    {
        ImGui::Checkbox("Modern Shift modifies VX",         &mModernShift);
        ImGui::Checkbox("Modern Load/Store (FX55/FX65)",    &mModernLoadStore);
        ImGui::Checkbox("Jump with offset uses V0",         &mSuperChipJump);
    }
}
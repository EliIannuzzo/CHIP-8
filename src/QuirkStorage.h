#pragma once
#include <string>

class QuirkStorage
{
public:
    void LoadConfig(const std::string& romPath);
    void SaveConfig(const std::string& romPath);

    void ResetToDefault();
    void DrawImGuiMenu();
    
    // Quirks ==========================================================================================================
    bool mModernShift               = false;
    bool mModernLoadStore           = true;
    bool mSuperChipJump             = false;
};

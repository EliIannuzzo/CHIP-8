#include "Application.h"
#include <cassert>
#include "nfd.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_sdlrenderer3.h>
#include <filesystem>

// #todo
// - Use scancodes so different keyboard layouts are supported.
// - Support multiple common numpad configurations.
const std::array<uint8_t, 16> gSDLKeys = {
    SDLK_X,
    SDLK_1,
    SDLK_2,
    SDLK_3,
    SDLK_Q,
    SDLK_W,
    SDLK_E,
    SDLK_A,
    SDLK_S,
    SDLK_D,
    SDLK_Z,
    SDLK_C,
    SDLK_4,
    SDLK_R,
    SDLK_F,
    SDLK_V,
};

const float TIMER_INTERVAL = 1000.0f / 60.0f;

namespace fs = std::filesystem;
Application::Application(const int width, const int height)
{
    assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS));
    
    mWindow     = SDL_CreateWindow("CHIP-8 Emulator", width, height, SDL_WINDOW_RESIZABLE);
    mRenderer   = SDL_CreateRenderer(mWindow, nullptr);
    mTexture    = SDL_CreateTexture(mRenderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,  OUTPUT_WIDTH, OUTPUT_HEIGHT);

    SDL_SetTextureScaleMode(mTexture, SDL_SCALEMODE_NEAREST);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplSDL3_InitForSDLRenderer(mWindow, mRenderer);
    ImGui_ImplSDLRenderer3_Init(mRenderer);

    mRomPath = "bin\\roms\\1-ibm-logo.ch8";
    mEmulator.LoadROM(mRomPath);
}

Application::~Application()
{
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    
    SDL_DestroyTexture(mTexture);
    SDL_DestroyRenderer(mRenderer);
    SDL_DestroyWindow(mWindow);

    SDL_Quit();
}

bool Application::PollEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
        {
            return false;
        }

        // Update keypad
        const bool isKeyDown = event.type == SDL_EVENT_KEY_DOWN;
        auto it = std::find(gSDLKeys.begin(), gSDLKeys.end(), event.key.key);
        if (it != gSDLKeys.end())
        {
            mEmulator.mKeypad[std::distance(gSDLKeys.begin(), it)] = isKeyDown;
        }
        
        // Forward event to ImGui
        ImGui_ImplSDL3_ProcessEvent(&event);
    }
    
    return true;
}

void Application::Update(float deltaTime)
{
    // Accumulate time
    mInstructionAccumulator += deltaTime;
    mTimerAccumulator += deltaTime;

    // Run emulator instructions at desired speed
    while (mInstructionAccumulator >= GetTimePerInstruction())
    {
        mEmulator.Process();
        mInstructionAccumulator -= GetTimePerInstruction();
    }

    // Tick timers at 60Hz
    while (mTimerAccumulator >= TIMER_INTERVAL)
    {
        mEmulator.DecrementTimers();
        mTimerAccumulator -= TIMER_INTERVAL;
    }
}

void Application::Render()
{
    SDL_RenderClear(mRenderer);
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    
    ImGui::NewFrame();
    RenderMenuBar();
    RenderOutputPanel();
    RenderDebugPanel();
    ImGui::Render();
    
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), mRenderer);
    SDL_RenderPresent(mRenderer);
}

void Application::RenderMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Load ROM")) {
                nfdchar_t* outPath = nullptr;
                nfdresult_t result = NFD_OpenDialog("ch8,rom,bin", nullptr, &outPath);

                if (result == NFD_OKAY)
                {
                    fs::path absolutePath(outPath);
                    fs::path relativePath = fs::relative(absolutePath, fs::current_path());
                    
                    mRomPath = relativePath.string();
                    mEmulator = Chip();
                    mEmulator.LoadROM(mRomPath);
                    free(outPath);
                }
                else if (result == NFD_CANCEL)
                {
                    // User canceled the dialog, no action needed
                }
                else
                {
                    std::cerr << "NFD Error: " << NFD_GetError() << std::endl;
                }
            }
            if (ImGui::MenuItem("Restart")) {
                mEmulator = Chip();
                mEmulator.LoadROM(mRomPath);
            }
            if (ImGui::MenuItem("Save Quirks")) {
                mEmulator.mQuirks.SaveConfig(mRomPath);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void Application::RenderOutputPanel()
{
    int pitch = sizeof(uint32_t) * OUTPUT_WIDTH; 
    SDL_UpdateTexture(mTexture, nullptr, mEmulator.mDisplayOutput.data(), pitch);
    
    ImVec2 windowSize = ImGui::GetIO().DisplaySize;
    float panelWidth = windowSize.x * 0.7f;
    float panelHeight = windowSize.y - ImGui::GetFrameHeight();

    ImGui::SetNextWindowPos(ImVec2(0, ImGui::GetFrameHeight()), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight), ImGuiCond_Always);
    ImGui::Begin("CHIP-8 Display", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoMove);

    ImGui::SliderFloat("Instructions Per Second", &mInstructionsPerSecond, 1.f, 1000.f);

    ImVec2 avail = ImGui::GetContentRegionAvail();

    // Calculate aspect ratio of the CHIP-8 output
    float aspect = static_cast<float>(OUTPUT_WIDTH) / OUTPUT_HEIGHT;
    float targetWidth = avail.x;
    float targetHeight = avail.x / aspect;

    // If that height is too much, scale down to fit vertically
    if (targetHeight > avail.y)
    {
        targetHeight = avail.y;
        targetWidth = avail.y * aspect;
    }

    ImGui::Image((ImTextureID)mTexture, ImVec2(targetWidth, targetHeight));
    ImGui::End();
}

void Application::RenderDebugPanel()
{
    ImVec2 windowSize = ImGui::GetIO().DisplaySize;
    float panelWidth = windowSize.x * 0.3f;
    float panelHeight = windowSize.y - ImGui::GetFrameHeight();
    
    ImGui::SetNextWindowPos(ImVec2(windowSize.x * 0.7f, ImGui::GetFrameHeight()), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight), ImGuiCond_Always);
    ImGui::Begin("Debug Panel", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                 ImGuiWindowFlags_NoMove);

    ImGui::Text("Program Counter: 0x%X", mEmulator.mProgramCounter);
    ImGui::Text("Opcode: 0x%X", mEmulator.mInstruction);
    ImGui::Separator();
    
    mEmulator.mQuirks.DrawImGuiMenu();
    
    if (ImGui::CollapsingHeader("Registers"))
    {
        for (int i = 0; i < 16; ++i)
        {
            ImGui::Text("V[%X] = 0x%02X", i, mEmulator.mVariableRegisters[i]);
        }
    }
    if (ImGui::CollapsingHeader("Keypad State"))
    {
        for (int i = 0; i < gSDLKeys.size(); ++i)
        {
            std::string label = "0x" + std::to_string(i) + " (";
            label += SDL_GetKeyName(gSDLKeys[i]);
            label += ")";

            ImGui::Text("%s: %s", label.c_str(), mEmulator.mKeypad[i] ? "Pressed" : "Released");
        }
    }
    ImGui::End();
}

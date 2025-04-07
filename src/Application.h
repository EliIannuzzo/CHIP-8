#pragma once
#include <SDL3/SDL.h>
#include "Chip8.h"

class Application
{
public:
    Application(const int windowWidth, const int windowHeight);
    ~Application();

    bool PollEvents();
    void Update(float deltaTime);
    void Render();

    void RenderMenuBar();
    void RenderOutputPanel();
    void RenderDebugPanel();
    
    float GetTimePerInstruction() { return 1000.f / mInstructionsPerSecond; }
private:
    Chip mEmulator;
    std::string mRomPath;
    
    SDL_Window* mWindow;
    SDL_Renderer* mRenderer;
    SDL_Texture* mTexture;

    float mInstructionsPerSecond = 700.f;
    float mInstructionAccumulator = 0.0f;
    float mTimerAccumulator = 0.0f;
};
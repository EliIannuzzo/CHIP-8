#include "Application.h"
#include "Chip8.h"
#include <SDL3/SDL.h>
#include "imgui.h"
#include <iostream>

int main(int argc, char* argv[])
{
    Application app = Application(1280, 720);

    uint64_t lastTime = SDL_GetPerformanceCounter();
    const uint64_t frequency = SDL_GetPerformanceFrequency();

    bool running = true;
    while (running)
    {
        uint64_t currentTime = SDL_GetPerformanceCounter();
        float deltaTime = (currentTime - lastTime) * 1000.0f / frequency;
        lastTime = currentTime;

        running = app.PollEvents();
        app.Update(deltaTime);
        app.Render();
    }
    return 0;
}
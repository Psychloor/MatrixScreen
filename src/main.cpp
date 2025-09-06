#include <cstdlib>
#include <iostream>
#include <ostream>
#include <vector>

#include "matrix_renderer.hpp"
#include "SDL3/SDL_main.h"

#include "SDL3/SDL.h"

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
    SDL_SetAppMetadata("Matrix Screensaver", "1.0.0", "com.shizzle.matrixscreensaver");
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n' << std::flush;
        SDL_Quit();
    }

    int displayCount = 0;
    const auto displayIds = SDL_GetDisplays(&displayCount);
    if (displayIds == nullptr)
    {
        std::cerr << "SDL_GetDisplays failed: " << SDL_GetError() << '\n' << std::flush;
        SDL_Quit();
    }
    std::cout << "Found " << displayCount << " displays\n";
    std::cout << std::flush;

    std::vector<MatrixRenderer> renderers {};
    for (int i = 0; i < displayCount; ++i)
    {
        SDL_Rect rect;
        // ReSharper disable once CppDFANullDereference
        if (!SDL_GetDisplayBounds(displayIds[i], &rect)) // NOLINT(*-pro-bounds-pointer-arithmetic)
        {
            continue;
        }

        std::cout << "Display " << i << ": " << rect.w << "x" << rect.h << '\n';
        std::cout << std::flush;

        renderers.emplace_back(rect, displayIds[i]);
        if (!renderers.back().isValid())
        {
            std::cerr << "Failed to create renderer for display " << i << '\n' << std::flush;
            renderers.pop_back();
        }
    }
    SDL_free(displayIds);

    const double FREQUENCY = static_cast<double>(SDL_GetPerformanceFrequency());
    constexpr double FPS = 60.0f;
    constexpr double TIMESTEP = 1.0f / FPS;

    bool running = true;

    double accumulator = 0.0f;
    double lastTime = static_cast<double>(SDL_GetPerformanceCounter());
    
    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_EVENT_QUIT)
            {
                running = false;
            }
        }

        const double currentTime = static_cast<double>(SDL_GetPerformanceCounter());
        const double deltaTime = (currentTime - lastTime) / FREQUENCY;
        lastTime = currentTime;

        accumulator += std::min(deltaTime, 0.25);
        while (accumulator >= TIMESTEP)
        {
            accumulator -= TIMESTEP;
            for (auto& renderer : renderers)
            {
                renderer.update(TIMESTEP);
            }
        }

        for (auto& renderer : renderers)
        {
            renderer.render();
        }
    }
    

    SDL_Quit();
    return EXIT_SUCCESS;
}

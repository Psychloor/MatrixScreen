#include <cstdlib>
#include <iostream>
#include <ostream>
#include <random>
#include <vector>

#include "SDL3/SDL_main.h"

#include "SDL3/SDL.h"

#include "matrix_renderer.hpp"

static constinit float MOUSE_THRESHOLD = 10.0F;
// 16 should be random enough compared to 'std::mt19937::state_size' 624
static constinit size_t RANDOM_SEED_SIZE = 16;

static constexpr double FPS = 60.0f;
static constexpr double TIMESTEP = 1.0f / FPS;

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv)
{
    SDL_SetAppMetadata("Matrix Screensaver", "1.0.0", "com.matrix-screensaver");
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

    std::vector<MatrixRenderer> renderers{};
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

    const double frequency = static_cast<double>(SDL_GetPerformanceFrequency());

    bool running = true;

    double accumulator = 0.0f;
    double lastTime = static_cast<double>(SDL_GetPerformanceCounter());

    std::random_device rd;
    std::vector<std::seed_seq::result_type> seeds(RANDOM_SEED_SIZE);
    std::ranges::generate(seeds, std::ref(rd));
    std::seed_seq seq(std::begin(seeds), std::end(seeds));

    std::mt19937 gen(seq);

    while (running)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_EVENT_QUIT: running = false;
                    break;

                case SDL_EVENT_KEY_DOWN: if (event.key.key == SDLK_ESCAPE)
                    {
                        running = false;
                    }
                    break;

                case SDL_EVENT_MOUSE_MOTION: if (std::abs(event.motion.xrel) >= MOUSE_THRESHOLD || std::abs(
                        event.motion.yrel) >= MOUSE_THRESHOLD)
                    {
                        running = false;
                    }
                    break;

                default: break;
            }
        }

        const double currentTime = static_cast<double>(SDL_GetPerformanceCounter());
        const double deltaTime = (currentTime - lastTime) / frequency;
        lastTime = currentTime;

        accumulator += std::min(deltaTime, 0.25);
        while (accumulator >= TIMESTEP)
        {
            accumulator -= TIMESTEP;
            for (auto& renderer : renderers)
            {
                renderer.update(TIMESTEP, gen);
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

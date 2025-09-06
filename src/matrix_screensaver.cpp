//
// Created by blomq on 2025-09-06.
//

#include "matrix_screensaver.hpp"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <ostream>
#include <random>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "matrix_renderer.hpp"

namespace
{
    constexpr float MOUSE_THRESHOLD = 10.0F; // NOLINT(*-avoid-magic-numbers)
    // 16 should be random enough compared to `std::mt19937::state_size` 624
    constexpr size_t RANDOM_SEED_SIZE = 16; // NOLINT(*-avoid-magic-numbers)

    // 1000 Herts should be enough for now
    constexpr double FPS = 1'000.0F;
    constexpr double TIMESTEP = 1.0F / FPS;
}


MatrixScreensaver::MatrixScreensaver(const ScreensaverMode mode, const HWND previewWindow) :
    mode_(mode), previewWindow_(previewWindow)
{}

MatrixScreensaver::~MatrixScreensaver()
{
    SDL_Quit();
}

int MatrixScreensaver::runScreensaver()
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n' << std::flush;
        return -1;
    }

    if (!TTF_Init())
    {
        std::cerr << "TTF_Init failed: " << SDL_GetError() << '\n' << std::flush;
        SDL_Quit();
        return -1;
    }

    // Disable screensaver while running
    SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "0");

    SDL_HideCursor();

    int displayCount = 0;
    const auto displayIds = SDL_GetDisplays(&displayCount);

    for (int i = 0; i < displayCount; ++i)
    {
        SDL_Rect rect;
        if (SDL_GetDisplayBounds(displayIds[i], &rect)) // NOLINT(*-pro-bounds-pointer-arithmetic)
        {
            renderers_.emplace_back(rect, displayIds[i]); // NOLINT(*-pro-bounds-pointer-arithmetic)
        }
    }
    SDL_free(displayIds);

    return mainLoop();
}

int MatrixScreensaver::runConfiguration()
{
    return 0;
}

int MatrixScreensaver::runPreview()
{
    if (previewWindow_ == nullptr)
    {
        std::cerr << "Preview window handle is null" << '\n' << std::flush;
        return -1;
    }

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n' << std::flush;
        return -1;
    }

    if (!TTF_Init())
    {
        std::cerr << "TTF_Init failed: " << SDL_GetError() << '\n' << std::flush;
        SDL_Quit();
        return -1;
    }

    RECT previewRect;
    GetClientRect(previewWindow_, &previewRect);

    SDL_Rect rect;
    rect.x = previewRect.left;
    rect.y = previewRect.top;
    rect.w = previewRect.right - previewRect.left;
    rect.h = previewRect.bottom - previewRect.top;

    renderers_.emplace_back(rect, previewWindow_);
    if (!renderers_.back().isValid())
    {
        return -1;
    }

    return mainLoop();
}

int MatrixScreensaver::mainLoop()
{
    const auto frequency = static_cast<double>(SDL_GetPerformanceFrequency());
    double accumulator = 0.0;
    auto lastTime = static_cast<double>(SDL_GetPerformanceCounter());

    std::random_device rd;
    std::vector<std::seed_seq::result_type> seeds(RANDOM_SEED_SIZE);
    std::ranges::generate(seeds, std::ref(rd));
    std::seed_seq seq(std::begin(seeds), std::end(seeds));

    std::mt19937 gen(seq);

    for (auto&& renderer : renderers_)
    {
        renderer.setupStreams(gen);
    }

    while (running_)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_EVENT_QUIT: [[fallthrough]];
                case SDL_EVENT_KEY_DOWN: [[fallthrough]];
                case SDL_EVENT_MOUSE_BUTTON_DOWN: running_ = false;
                    break;

                case SDL_EVENT_MOUSE_MOTION: if (std::abs(event.motion.xrel) >= MOUSE_THRESHOLD || std::abs(
                        event.motion.yrel) >= MOUSE_THRESHOLD)
                    {
                        running_ = false;
                    }
                    break;

                default: break;
            }
        }

        const auto currentTime = static_cast<double>(SDL_GetPerformanceCounter());
        const double deltaTime = (currentTime - lastTime) / frequency;
        lastTime = currentTime;

       accumulator += std::min(deltaTime, 0.25); // NOLINT(*-avoid-magic-numbers)
        while (accumulator >= TIMESTEP)
        {
            accumulator -= TIMESTEP;
            for (auto& renderer : renderers_)
            {
                renderer.update(TIMESTEP, gen);
            }
        }

        for (auto& renderer : renderers_)
        {
            renderer.render();
        }

        SDL_DelayNS(1);
    }

    return EXIT_SUCCESS;
}

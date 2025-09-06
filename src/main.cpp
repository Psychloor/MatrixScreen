#include <cstdlib>
#include <iostream>
#include <ostream>

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
    }

    SDL_free(displayIds);
    SDL_Quit();
    return EXIT_SUCCESS;
}

#include "SDL3/SDL_main.h"

#include "SDL3/SDL.h"

#include "matrix_screensaver.hpp"

int main([[maybe_unused]] const int argc, [[maybe_unused]] char** argv)
{
    SDL_SetAppMetadata("Matrix Screensaver", "1.0.0", "com.matrix-screensaver");
     auto [mode, previewWindow]  = ScreensaverArgs::parse(argc, argv);

    // Dev Time ignore that shit
    //MatrixScreensaver screensaver(mode, previewWindow);
    MatrixScreensaver screensaver(ScreensaverMode::Screensaver, nullptr);
    return screensaver.run();
}

#ifndef MATRIXSCREENSAVER_MATRIX_RENDERER_HPP
#define MATRIXSCREENSAVER_MATRIX_RENDERER_HPP

#include <memory>
#include <random>

#include <SDL3/SDL.h>

#include "matrix_screensaver.hpp"

using SdlWindowPtr = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;
using SdlRendererPtr = std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;

class MatrixRenderer
{
public:
    explicit MatrixRenderer(const SDL_Rect& bounds, SDL_DisplayID displayId);
    explicit MatrixRenderer(const SDL_Rect& bounds, HWND previewWindow);
    ~MatrixRenderer();

    void update(double deltaTime, std::mt19937& gen);
    void render();

    [[nodiscard]] bool isValid() const;

    MatrixRenderer(const MatrixRenderer& other) = delete;
    MatrixRenderer(MatrixRenderer&& other) noexcept;
    MatrixRenderer& operator=(const MatrixRenderer& other) = delete;
    MatrixRenderer& operator=(MatrixRenderer&& other) noexcept;

private:
    bool valid_ = false;
    SDL_Rect bounds_;

    SdlWindowPtr window_;
    SdlRendererPtr renderer_;
};


#endif //MATRIXSCREENSAVER_MATRIX_RENDERER_HPP

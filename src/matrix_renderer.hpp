#ifndef MATRIXSCREENSAVER_MATRIX_RENDERER_HPP
#define MATRIXSCREENSAVER_MATRIX_RENDERER_HPP

#include <memory>

#include <SDL3/SDL.h>

using SdlWindowPtr = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;
using SdlRendererPtr = std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;

class MatrixRenderer
{
    public:
    explicit MatrixRenderer(const SDL_Rect& bounds, SDL_DisplayID displayId);
    ~MatrixRenderer();

    void update(double deltaTime);
    void render();

    [[nodiscard]] bool isValid() const;

    MatrixRenderer(const MatrixRenderer& other) = delete;
    MatrixRenderer(MatrixRenderer&& other) noexcept;
    MatrixRenderer& operator=(const MatrixRenderer& other) = delete;
    MatrixRenderer& operator=(MatrixRenderer&& other) noexcept;

private:
    bool valid_ = false;
    SdlWindowPtr window_;
    SdlRendererPtr renderer_;
};


#endif //MATRIXSCREENSAVER_MATRIX_RENDERER_HPP
//
// Created by blomq on 2025-09-06.
//

#include "matrix_renderer.hpp"

MatrixRenderer::MatrixRenderer(const SDL_Rect& bounds, const SDL_DisplayID displayId) :
    window_{nullptr, SDL_DestroyWindow},
    renderer_{nullptr, SDL_DestroyRenderer}
{
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, bounds.w);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, bounds.h);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, bounds.x);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, bounds.y);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER,
                         SDL_WINDOW_BORDERLESS | SDL_WINDOW_FULLSCREEN);

    window_ = SdlWindowPtr(SDL_CreateWindowWithProperties(props), SDL_DestroyWindow);
    SDL_DestroyProperties(props);
    if (!window_)
    {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n' << std::flush;
        return;
    }

    if (!SDL_SetWindowPosition(window_.get(), bounds.x, bounds.y))
    {
        std::cerr << "SDL_SetWindowPosition failed: " << SDL_GetError() << '\n' << std::flush;
        return;
    }
    if (!SDL_SetWindowSize(window_.get(), bounds.w, bounds.h))
    {
        std::cerr << "SDL_SetWindowSize failed: " << SDL_GetError() << '\n' << std::flush;
        return;
    }
   /* if (!SDL_ShowWindow(window_.get()))
    {
        std::cerr << "SDL_ShowWindow failed: " << SDL_GetError() << '\n' << std::flush;
        return;
    }*/
    //SDL_SetWindowAlwaysOnTop(window_.get(), true);

    renderer_ = SdlRendererPtr(SDL_CreateRenderer(window_.get(), nullptr), SDL_DestroyRenderer);

    valid_ = true;
}

MatrixRenderer::~MatrixRenderer()
{
    renderer_.reset();
    window_.reset();
}

void MatrixRenderer::update(double deltaTime)
{

}

void MatrixRenderer::render()
{
    SDL_RenderClear(renderer_.get());


    SDL_RenderPresent(renderer_.get());
}

bool MatrixRenderer::isValid() const
{
    return valid_;
}

MatrixRenderer::MatrixRenderer(MatrixRenderer&& other) noexcept :
    valid_(other.valid_),
    window_(std::move(other.window_)),
    renderer_(std::move(other.renderer_))
{}

MatrixRenderer& MatrixRenderer::operator=(MatrixRenderer&& other) noexcept
{
    if (this == &other)
    {
        return *this;
    }
    valid_ = other.valid_;
    window_ = std::move(other.window_);
    renderer_ = std::move(other.renderer_);
    return *this;
}

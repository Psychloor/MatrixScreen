//
// Created by blomq on 2025-09-06.
//

#include "matrix_renderer.hpp"

#include <array>
#include <iostream>

namespace
{
    // @formatter:off
    constinit auto MATRIX_CHARS = std::to_array({L"ア", L"イ", L"ウ", L"エ", L"オ",
        L"カ", L"キ", L"ク", L"ケ", L"コ", L"サ", L"シ", L"ス", L"セ", L"ソ",L"タ", L"チ", L"ツ", L"テ",
        L"ト", L"ナ", L"ニ", L"ヌ", L"ネ", L"ノ", L"ハ", L"ヒ", L"フ", L"ヘ", L"ホ",L"マ", L"ミ", L"ム",
        L"メ", L"モ", L"ヤ", L"ユ", L"ヨ", L"ラ", L"リ", L"ル", L"レ", L"ロ", L"ワ", L"ヲ", L"ン",L"0",
        L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9",L"A",L"B",L"C"});
    // @formatter:on
    constexpr size_t MATRIX_CHARS_SIZE = MATRIX_CHARS.size();
}

MatrixRenderer::MatrixRenderer(const SDL_Rect& bounds, const SDL_DisplayID  /*displayId*/) :
    bounds_(bounds),
    window_{nullptr, SDL_DestroyWindow},
    renderer_{nullptr, SDL_DestroyRenderer}
{
    const SDL_PropertiesID props = SDL_CreateProperties();
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

    SDL_SetWindowAlwaysOnTop(window_.get(), true);

    renderer_ = SdlRendererPtr(SDL_CreateRenderer(window_.get(), nullptr), SDL_DestroyRenderer);
    if (!renderer_)
    {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << '\n' << std::flush;
        return;
    }

    characterDistribution_ = std::uniform_int_distribution<size_t>(0, MATRIX_CHARS_SIZE - 1);
    valid_ = true;
}

MatrixRenderer::MatrixRenderer(const SDL_Rect& bounds, HWND previewWindow) :
    bounds_(bounds),
    window_{nullptr, SDL_DestroyWindow},
    renderer_{nullptr, SDL_DestroyRenderer}
{
    window_ = SdlWindowPtr(SDL_CreateWindow(nullptr, bounds.w, bounds.h, SDL_WINDOW_BORDERLESS), SDL_DestroyWindow);
    if (!window_)
    {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n' << std::flush;
        return;
    }

    SDL_Window* parentWindow = SDL_GetWindowFromID(reinterpret_cast<SDL_WindowID>(previewWindow));
    SDL_SetWindowParent(window_.get(), parentWindow);
    SDL_ShowWindow(window_.get());

    renderer_ = SdlRendererPtr(SDL_CreateRenderer(window_.get(), nullptr), SDL_DestroyRenderer);
    if (!renderer_)
    {
        std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << '\n' << std::flush;
        return;
    }

    SDL_SetWindowAlwaysOnTop(window_.get(), true);
    characterDistribution_ = std::uniform_int_distribution<size_t>(0, MATRIX_CHARS_SIZE - 1);
    valid_ = true;
}

MatrixRenderer::~MatrixRenderer()
{
    renderer_.reset();
    window_.reset();
}

void MatrixRenderer::update(double deltaTime, std::mt19937& gen)
{}

void MatrixRenderer::render()
{
    SDL_SetRenderDrawColor(renderer_.get(), 0, 0, 0, 255);
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
{
}

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

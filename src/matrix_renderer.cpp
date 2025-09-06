//
// Created by blomq on 2025-09-06.
//

#include "matrix_renderer.hpp"

#include <array>
#include <iostream>
#include <random>
#include <string_view>

namespace
{
    // Character set kept for potential future use (e.g., switching glyphs over time).
    // We don't render text here (SDL_ttf-free); we draw rectangles for speed/compat.
    constinit auto MATRIX_CHARS = std::to_array({L"ア", L"イ", L"ウ", L"エ", L"オ",
        L"カ", L"キ", L"ク", L"ケ", L"コ", L"サ", L"シ", L"ス", L"セ", L"ソ",L"タ", L"チ", L"ツ", L"テ",
        L"ト", L"ナ", L"ニ", L"ヌ", L"ネ", L"ノ", L"ハ", L"ヒ", L"フ", L"ヘ", L"ホ",L"マ", L"ミ", L"ム",
        L"メ", L"モ", L"ヤ", L"ユ", L"ヨ", L"ラ", L"リ", L"ル", L"レ", L"ロ", L"ワ", L"ヲ", L"ン",L"0",
        L"1", L"2", L"3", L"4", L"5", L"6", L"7", L"8", L"9",L"A",L"B",L"C"});
    constexpr size_t MATRIX_CHARS_SIZE = MATRIX_CHARS.size();

    inline SDL_Color head_color()
    {
        return SDL_Color{ 200, 255, 200, 255 };
    }
    inline SDL_Color tail_color(float t) // t in [0,1]
    {
        const Uint8 g = static_cast<Uint8>(std::clamp(40.0f + 215.0f * t, 40.0f, 255.0f));
        const Uint8 a = static_cast<Uint8>(std::clamp(32.0f + 223.0f * t, 32.0f, 255.0f));
        return SDL_Color{ 0, g, 0, a };
    }
}

// Constructors

MatrixRenderer::MatrixRenderer(const SDL_Rect& bounds, const SDL_DisplayID /*displayId*/) :
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

    // Enable alpha blending for fading tails
    SDL_SetRenderDrawBlendMode(renderer_.get(), SDL_BLENDMODE_BLEND);

    characterDistribution_ = std::uniform_int_distribution<size_t>(0, MATRIX_CHARS_SIZE - 1);

    initFontIfPossible(); // sets cellW_/cellH_ even without fonts
    std::mt19937 gen{ std::random_device{}() };
    setupStreams(gen);

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

    // Enable alpha blending for fading tails
    SDL_SetRenderDrawBlendMode(renderer_.get(), SDL_BLENDMODE_BLEND);

    SDL_SetWindowAlwaysOnTop(window_.get(), true);
    characterDistribution_ = std::uniform_int_distribution<size_t>(0, MATRIX_CHARS_SIZE - 1);

    initFontIfPossible(); // sets cellW_/cellH_ even without fonts
    std::mt19937 gen{ std::random_device{}() };
    setupStreams(gen);

    valid_ = true;
}

MatrixRenderer::~MatrixRenderer()
{
    // No SDL_ttf resources used; just clear any cached textures if added later
    glyphCache_.clear();
    renderer_.reset();
    window_.reset();
}

// Simulation setup and helpers

void MatrixRenderer::initFontIfPossible()
{
    // We’re not using fonts here; set sensible cell size for a Matrix look
    cellW_ = 16;
    cellH_ = 24;
}

void MatrixRenderer::setupStreams(std::mt19937& gen)
{
    cols_ = std::max(1, bounds_.w / std::max(1, cellW_));
    streams_.clear();
    streams_.reserve(static_cast<size_t>(cols_));

    for (int c = 0; c < cols_; ++c)
    {
        Stream s;
        s.speed = speedDist_(gen);
        s.length = lengthDist_(gen);
        s.y = -static_cast<float>(std::uniform_int_distribution<int>(0, bounds_.h)(gen)); // start above screen
        s.chars.resize(static_cast<size_t>(s.length));

        for (int i = 0; i < s.length; ++i)
        {
            const size_t idx = characterDistribution_(gen);
            s.chars[static_cast<size_t>(i)] = MatrixCharacter{ MATRIX_CHARS[idx][0], static_cast<float>(i) };
        }

        streams_.emplace_back(std::move(s));
    }
}

// Stubbed glyph rendering API to satisfy header; unused in this SDL_ttf-free version
SdlTexturePtr MatrixRenderer::renderGlyphTexture(wchar_t /*ch*/, SDL_Color /*color*/)
{
    return SdlTexturePtr(nullptr, SDL_DestroyTexture);
}
SDL_Texture* MatrixRenderer::getGlyphTexture(wchar_t /*ch*/, SDL_Color /*color*/)
{
    return nullptr;
}

// Update and render

void MatrixRenderer::update(double deltaTime, std::mt19937& gen)
{
    for (auto& s : streams_)
    {
        s.y += s.speed * static_cast<float>(deltaTime);

        const float tailHeightPx = static_cast<float>(s.length * cellH_);
        if (s.y - tailHeightPx > static_cast<float>(bounds_.h))
        {
            // Respawn above top with new length & speed
            s.speed = speedDist_(gen);
            s.length = lengthDist_(gen);
            s.y = -static_cast<float>(std::uniform_int_distribution<int>(0, bounds_.h)(gen));

            s.chars.resize(static_cast<size_t>(s.length));
            for (int i = 0; i < s.length; ++i)
            {
                const size_t idx = characterDistribution_(gen);
                s.chars[static_cast<size_t>(i)].character = MATRIX_CHARS[idx][0];
                s.chars[static_cast<size_t>(i)].age = static_cast<float>(i);
            }
        }
        else
        {
            // Occasionally flicker a character to add variety
            if (std::uniform_int_distribution<int>(0, 7)(gen) == 0 && !s.chars.empty())
            {
                const int pos = std::uniform_int_distribution<int>(0, std::max(0, s.length - 1))(gen);
                const size_t idx = characterDistribution_(gen);
                s.chars[static_cast<size_t>(pos)].character = MATRIX_CHARS[idx][0];
            }
        }
    }
}

void MatrixRenderer::render()
{
    SDL_SetRenderDrawColor(renderer_.get(), 0, 0, 0, 255);
    SDL_RenderClear(renderer_.get());

    // Draw each column’s stream as rectangles (bright head + fading tail)
    for (int c = 0; c < cols_; ++c)
    {
        const int x = c * cellW_;
        const Stream& s = streams_[static_cast<size_t>(c)];

        for (int i = 0; i < s.length; ++i)
        {
            const float yPos = s.y - static_cast<float>(i * cellH_);
            if (yPos < -cellH_ || yPos > bounds_.h) continue;

            const bool isHead = (i == 0);
            const float t = 1.0f - static_cast<float>(i) / static_cast<float>(std::max(1, s.length - 1));
            const SDL_Color color = isHead ? head_color() : tail_color(t);

            SDL_SetRenderDrawColor(renderer_.get(), color.r, color.g, color.b, color.a);
            SDL_FRect rect{ static_cast<float>(x),
                            yPos,
                            static_cast<float>(cellW_ - 1),
                            static_cast<float>(cellH_ - 1) };
            SDL_RenderFillRect(renderer_.get(), &rect);
        }
    }

    SDL_RenderPresent(renderer_.get());
}

// Identity/validity moves

bool MatrixRenderer::isValid() const
{
    return valid_;
}

MatrixRenderer::MatrixRenderer(MatrixRenderer&& other) noexcept :
    valid_(other.valid_),
    bounds_(other.bounds_),
    characterDistribution_(other.characterDistribution_),
    window_(std::move(other.window_)),
    renderer_(std::move(other.renderer_)),
    cellW_(other.cellW_),
    cellH_(other.cellH_),
    cols_(other.cols_),
    streams_(std::move(other.streams_)),
    speedDist_(other.speedDist_),
    lengthDist_(other.lengthDist_),
    font_(nullptr)
{}

MatrixRenderer& MatrixRenderer::operator=(MatrixRenderer&& other) noexcept
{
    if (this == &other) return *this;

    valid_ = other.valid_;
    bounds_ = other.bounds_;
    characterDistribution_ = other.characterDistribution_;
    window_ = std::move(other.window_);
    renderer_ = std::move(other.renderer_);
    cellW_ = other.cellW_;
    cellH_ = other.cellH_;
    cols_ = other.cols_;
    streams_ = std::move(other.streams_);
    speedDist_ = other.speedDist_;
    lengthDist_ = other.lengthDist_;
    font_ = nullptr;
    glyphCache_.clear();
    return *this;
}

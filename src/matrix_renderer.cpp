#include "matrix_renderer.hpp"

#include <algorithm>
#include <array>
#include <filesystem>
#include <iostream>
#include <random>
#include <string>
#include <string_view>

// SDL3_ttf
#include <SDL3_ttf/SDL_ttf.h>

// For UTF-16 -> UTF-8 conversion on Windows
#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
    // Character set kept for potential future use (e.g., switching glyphs over time).
    // We don't render text here (SDL_ttf-free); we draw rectangles for speed/compat.
    constexpr auto MATRIX_CHARS = std::to_array({L"ア",
                                                 L"イ",
                                                 L"ウ",
                                                 L"エ",
                                                 L"オ",
                                                 L"カ",
                                                 L"キ",
                                                 L"ク",
                                                 L"ケ",
                                                 L"コ",
                                                 L"サ",
                                                 L"シ",
                                                 L"ス",
                                                 L"セ",
                                                 L"ソ",
                                                 L"タ",
                                                 L"チ",
                                                 L"ツ",
                                                 L"テ",
                                                 L"ト",
                                                 L"ナ",
                                                 L"ニ",
                                                 L"ヌ",
                                                 L"ネ",
                                                 L"ノ",
                                                 L"ハ",
                                                 L"ヒ",
                                                 L"フ",
                                                 L"ヘ",
                                                 L"ホ",
                                                 L"マ",
                                                 L"ミ",
                                                 L"ム",
                                                 L"メ",
                                                 L"モ",
                                                 L"ヤ",
                                                 L"ユ",
                                                 L"ヨ",
                                                 L"ラ",
                                                 L"リ",
                                                 L"ル",
                                                 L"レ",
                                                 L"ロ",
                                                 L"ワ",
                                                 L"ヲ",
                                                 L"ン",
                                                 L"0",
                                                 L"1",
                                                 L"2",
                                                 L"3",
                                                 L"4",
                                                 L"5",
                                                 L"6",
                                                 L"7",
                                                 L"8",
                                                 L"9",
                                                 L"A",
                                                 L"B",
                                                 L"C"});
    constexpr size_t MATRIX_CHARS_SIZE = MATRIX_CHARS.size();
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

    // Log renderer name to spot per-monitor driver differences
    const char* rname = SDL_GetRendererName(renderer_.get());
    std::cerr << "Renderer created (primary path). Driver=" << (rname ? rname : "(null)")
        << " bounds=(" << bounds_.x << "," << bounds_.y << "," << bounds_.w << "x" << bounds_.h << ")\n";

    characterDistribution_ = std::uniform_int_distribution<size_t>(0, MATRIX_CHARS_SIZE - 1);

    initFontIfPossible(); // sets cellW_/cellH_ and font_ if available
    std::cerr << "Font status (primary path): " << (font_ ? "loaded" : "not loaded, using rectangles") << '\n';

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

    SDL_SetRenderDrawBlendMode(renderer_.get(), SDL_BLENDMODE_BLEND);

    const char* rname = SDL_GetRendererName(renderer_.get());
    std::cerr << "Renderer created (preview path). Driver=" << (rname ? rname : "(null)")
        << " bounds=(" << bounds_.x << "," << bounds_.y << "," << bounds_.w << "x" << bounds_.h << ")\n";

    SDL_SetWindowAlwaysOnTop(window_.get(), true);
    characterDistribution_ = std::uniform_int_distribution<size_t>(0, MATRIX_CHARS_SIZE - 1);

    initFontIfPossible();
    std::cerr << "Font status (preview path): " << (font_ ? "loaded" : "not loaded, using rectangles") << '\n';

    valid_ = true;
}

MatrixRenderer::~MatrixRenderer()
{
    // Free glyph textures
    glyphCache_.clear();

    // Close font if open
    if (font_ != nullptr)
    {
        TTF_CloseFont(static_cast<TTF_Font*>(font_));
        font_ = nullptr;
    }

    renderer_.reset();
    window_.reset();
}

// Simulation setup and helpers

// Helper: choose a per-monitor font size based on window height.
// Aim for about 50 rows; clamp to a sane range.
static int ChooseFontPtForBounds(const int height_px)
{
    static constexpr int ROWS = 50; // tune to taste (40..60)
    const int targetCellH = std::max(12, height_px / ROWS);
    // Use target cell height directly as a point-size heuristic (works well in practice).
    return std::clamp(targetCellH, 12, 64);
}

void MatrixRenderer::initFontIfPossible()
{
    const int pt = ChooseFontPtForBounds(bounds_.h);

    // Get renderer information for debugging
    const char* rendererName = SDL_GetRendererName(renderer_.get());
    const std::string driverName = rendererName ? rendererName : "unknown";
    std::cerr << "Initializing font for renderer: " << driverName << " pt=" << pt << '\n';

    // Test texture creation capability before loading fonts
    if (SDL_Surface* testSurf = SDL_CreateSurface(16, 16, SDL_PIXELFORMAT_RGBA32))
    {
        SDL_Texture* testTex = SDL_CreateTextureFromSurface(renderer_.get(), testSurf);
        if (!testTex)
        {
            std::cerr << "WARNING: Renderer " << driverName << " cannot create basic textures: "
                << SDL_GetError() << ". Font rendering will be disabled.\n";
            SDL_DestroySurface(testSurf);
            font_ = nullptr;
            cellW_ = 16;
            cellH_ = 24;
            return;
        }
        SDL_DestroyTexture(testTex);
        SDL_DestroySurface(testSurf);
    }

    // 1) Try bundled font first
    if (const char* base = SDL_GetBasePath())
    {
        try
        {
            std::filesystem::path p(base);
            p /= "fonts";
            #ifdef _WIN32
            p /= "NotoSansCJK-Regular.ttc";
            #else
            p /= "NotoSansMonoCJK-Regular.ttc";
            #endif
            if (std::filesystem::exists(p))
            {
                TTF_Font* f = TTF_OpenFont(p.string().c_str(), pt);
                if (f)
                {
                    // Test font rendering capability
                    if (SDL_Surface* testGlyph = TTF_RenderText_Blended(f, "A", 0, SDL_Color{255, 255, 255, 255}))
                    {
                        if (SDL_Texture* testGlyphTex = SDL_CreateTextureFromSurface(renderer_.get(), testGlyph))
                        {
                            font_ = f;
                            std::cerr << "Successfully loaded bundled font: " << p.string() << " on renderer " <<
                                driverName << '\n';

                            const int lineSkip = TTF_GetFontLineSkip(f);
                            if (lineSkip > 0)
                            {
                                cellH_ = lineSkip;
                                cellW_ = std::max(12, lineSkip * 2 / 3);
                            }
                            else
                            {
                                cellW_ = 16;
                                cellH_ = 24;
                            }

                            SDL_DestroyTexture(testGlyphTex);
                            SDL_DestroySurface(testGlyph);
                            return;
                        }
                        else
                        {
                            std::cerr << "Font glyph texture creation failed on renderer " << driverName
                                << ": " << SDL_GetError() << '\n';
                        }
                        SDL_DestroySurface(testGlyph);
                    }
                    else
                    {
                        std::cerr << "Font glyph rendering failed: " << SDL_GetError() << '\n';
                    }
                    TTF_CloseFont(f);
                }
                else
                {
                    std::cerr << "TTF_OpenFont failed (bundled): " << p.string() << " error=" << SDL_GetError() << '\n';
                }
            }
        }
        catch (...)
        {
            std::cerr << "Exception while loading bundled font, trying system fonts\n";
        }
    }

    // 2) Try system fonts with the same testing approach
    #ifdef _WIN32
    const char* primaryCandidates[] = {
        "C:/Windows/Fonts/consola.ttf",
        "C:/Windows/Fonts/segoeui.ttf"
    };
    const char* cjkFallbacks[] = {
        "C:/Windows/Fonts/meiryo.ttc",
        "C:/Windows/Fonts/meiryob.ttc",
        "C:/Windows/Fonts/YuGothM.ttc",
        "C:/Windows/Fonts/YuGothR.ttc",
        "C:/Windows/Fonts/msgothic.ttc",
        "C:/Windows/Fonts/msmincho.ttc"
    };
    #else
    const char* primaryCandidates[] = {
        "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf"
    };
    const char* cjkFallbacks[] = {
        "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
        "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"
    };
    #endif

    TTF_Font* primary = nullptr;
    for (const char* path : primaryCandidates)
    {
        TTF_Font* f = TTF_OpenFont(path, pt);
        if (f)
        {
            // Test rendering capability
            if (SDL_Surface* testGlyph = TTF_RenderText_Blended(f, "A", 0, SDL_Color{255, 255, 255, 255}))
            {
                if (SDL_Texture* testGlyphTex = SDL_CreateTextureFromSurface(renderer_.get(), testGlyph))
                {
                    primary = f;
                    std::cerr << "Loaded primary system font: " << path << " on renderer " << driverName << '\n';
                    SDL_DestroyTexture(testGlyphTex);
                    SDL_DestroySurface(testGlyph);
                    break;
                }
                else
                {
                    std::cerr << "Primary font texture creation failed on renderer " << driverName
                        << " for " << path << ": " << SDL_GetError() << '\n';
                }
                SDL_DestroySurface(testGlyph);
            }
            TTF_CloseFont(f);
        }
    }

    if (!primary)
    {
        std::cerr << "No compatible primary font found for renderer " << driverName << ". Using rectangles.\n";
        font_ = nullptr;
        cellW_ = 16;
        cellH_ = 24;
        return;
    }

    // Add CJK fallbacks
    for (const char* path : cjkFallbacks)
    {
        if (TTF_Font* fb = TTF_OpenFont(path, pt))
        {
            if (TTF_AddFallbackFont(primary, fb) == 0)
            {
                std::cerr << "Added CJK fallback: " << path << " to renderer " << driverName << '\n';
            }
            else
            {
                std::cerr << "TTF_AddFallbackFont failed for: " << path << " error=" << SDL_GetError() << '\n';
                TTF_CloseFont(fb);
            }
        }
    }

    font_ = primary;

    const int lineSkip = TTF_GetFontLineSkip(primary);
    if (lineSkip > 0)
    {
        cellH_ = lineSkip;
        cellW_ = std::max(12, lineSkip * 2 / 3);
    }
    else
    {
        cellW_ = 16;
        cellH_ = 24;
        std::cerr << "TTF_GetFontLineSkip failed; using defaults. Error: " << SDL_GetError() << '\n';
    }

    std::cerr << "Font initialization complete for renderer " << driverName
        << " - cellW=" << cellW_ << " cellH=" << cellH_ << '\n';
}

// Render a single codepoint as a texture, with explicit format normalization.
SdlTexturePtr MatrixRenderer::renderGlyphTexture(const wchar_t ch, SDL_Color /*color*/) const
{
    if (font_ == nullptr)
    {
        return SdlTexturePtr(nullptr, SDL_DestroyTexture);
    }

    // Convert the single codepoint to UTF-8 text
    #ifdef _WIN32
    const wchar_t wbuf[2] = {ch, 0};
    const int len = WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0)
    {
        std::cerr << "WideCharToMultiByte failed for wchar: " << static_cast<unsigned>(ch) << '\n';
        return SdlTexturePtr(nullptr, SDL_DestroyTexture);
    }
    std::string u8(static_cast<size_t>(len), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, u8.data(), len, nullptr, nullptr);
    if (!u8.empty() && u8.back() == '\0')
        u8.pop_back();
    #else
    std::string u8 = (ch < 128) ? std::string(1, static_cast<char>(ch)) : std::string("?");
    #endif

    // 1) Render text to an SDL_Surface via SDL3_ttf
    SDL_Surface* surf = TTF_RenderText_Blended(font_, u8.c_str(), 0,
                                               SDL_Color{255, 255, 255, 255});
    if (!surf)
    {
        std::cerr << "TTF_RenderText_Blended failed for '" << u8 << "': " << SDL_GetError() << '\n';
        return SdlTexturePtr(nullptr, SDL_DestroyTexture);
    }

    // 2) Get renderer info to determine best texture format
    const char* rendererName = SDL_GetRendererName(renderer_.get());
    const std::string driverName = rendererName ? rendererName : "unknown";

    // Choose format based on renderer type for maximum compatibility
    SDL_PixelFormat targetFormat;
    if (driverName.find("direct3d") != std::string::npos || driverName.find("d3d") != std::string::npos)
    {
        // D3D11/D3D12 prefers BGRA format
        targetFormat = SDL_PIXELFORMAT_BGRA32;
    }
    else
    {
        // OpenGL and others work best with RGBA
        targetFormat = SDL_PIXELFORMAT_RGBA32;
    }

    // 3) Convert surface to the renderer-appropriate format
    SDL_Surface* conv = SDL_ConvertSurface(surf, targetFormat);
    if (!conv)
    {
        std::cerr << "SDL_ConvertSurface failed for format " << SDL_GetPixelFormatName(targetFormat)
            << " on renderer " << driverName << ": " << SDL_GetError() << '\n';

        // Fallback: try the other format
        const SDL_PixelFormat fallbackFormat = (targetFormat == SDL_PIXELFORMAT_BGRA32)
            ? SDL_PIXELFORMAT_RGBA32
            : SDL_PIXELFORMAT_BGRA32;
        conv = SDL_ConvertSurface(surf, fallbackFormat);

        if (!conv)
        {
            std::cerr << "SDL_ConvertSurface fallback also failed: " << SDL_GetError() << '\n';
            SDL_DestroySurface(surf);
            return SdlTexturePtr(nullptr, SDL_DestroyTexture);
        }
        else
        {
            std::cerr << "Successfully used fallback format " << SDL_GetPixelFormatName(fallbackFormat) << '\n';
        }
    }
    SDL_DestroySurface(surf);

    // 4) Create texture from the converted surface
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer_.get(), conv);
    if (!tex)
    {
        std::cerr << "SDL_CreateTextureFromSurface failed on renderer " << driverName
            << ": " << SDL_GetError() << '\n';
        SDL_DestroySurface(conv);
        return SdlTexturePtr(nullptr, SDL_DestroyTexture);
    }
    SDL_DestroySurface(conv);

    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    return SdlTexturePtr(tex, SDL_DestroyTexture);
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
            s.chars[static_cast<size_t>(i)] = MatrixCharacter{MATRIX_CHARS[idx][0], static_cast<float>(i)};
        }

        streams_.emplace_back(std::move(s));
    }
}

SDL_Texture* MatrixRenderer::getGlyphTexture(wchar_t ch, SDL_Color color)
{
    // Cache by character only; color is applied with SDL_SetTextureColorMod/SDL_SetTextureAlphaMod when rendering.
    if (const auto it = glyphCache_.find(ch);
        it != glyphCache_.end() && it->second)
    {
        return it->second.get();
    }

    SdlTexturePtr texture = renderGlyphTexture(ch, SDL_Color{255, 255, 255, 255});
    SDL_Texture* texPtr = texture.get();
    if (texPtr)
    {
        glyphCache_.emplace(ch, std::move(texture));
    }

    return texPtr;
}

// Update and render

void MatrixRenderer::update(const double deltaTime, std::mt19937& gen)
{
    for (auto& s : streams_)
    {
        s.y += s.speed * static_cast<float>(deltaTime);

        const float tailHeightPx = static_cast<float>(s.length * cellH_);
        if (s.y - tailHeightPx > static_cast<float>(bounds_.h))
        {
            // Respawn above top with new length and speed
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
            if (std::uniform_int_distribution(0, 400)(gen) == 0 && !s.chars.empty())
            {
                const int pos = std::uniform_int_distribution(0, std::max(0, s.length - 1))(gen);
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

    auto headColor = []() -> SDL_Color
    {
        return SDL_Color{200, 255, 200, 255};
    };
    auto tailColor = [](const float t) -> SDL_Color
    {
        // t in [0,1]
        const Uint8 g = static_cast<Uint8>(std::clamp(40.0f + 215.0f * t, 40.0f, 255.0f));
        const Uint8 a = static_cast<Uint8>(std::clamp(32.0f + 223.0f * t, 32.0f, 255.0f));
        return SDL_Color{0, g, 0, a};
    };

    for (int c = 0; c < cols_; ++c)
    {
        const int x = c * cellW_;
        const Stream& s = streams_[static_cast<size_t>(c)];

        for (int i = 0; i < s.length; ++i)
        {
            const float yPos = s.y - static_cast<float>(i * cellH_);
            if (yPos < -cellH_ || yPos > bounds_.h)
                continue;

            const bool isHead = (i == 0);
            const float t = 1.0f - static_cast<float>(i) / static_cast<float>(std::max(1, s.length - 1));
            const SDL_Color color = isHead ? headColor() : tailColor(t);

            if (font_ != nullptr)
            {
                if (SDL_Texture* tex = getGlyphTexture(s.chars[static_cast<size_t>(i)].character, color))
                {
                    // SDL3: SDL_GetTextureSize returns floats
                    float tw = 0.0f, th = 0.0f;
                    SDL_GetTextureSize(tex, &tw, &th);

                    SDL_FRect dst{
                        static_cast<float>(x) + (cellW_ - tw) * 0.5f,
                        yPos + (cellH_ - th) * 0.5f,
                        tw,
                        th
                    };

                    SDL_SetTextureColorMod(tex, color.r, color.g, color.b);
                    SDL_SetTextureAlphaMod(tex, color.a);
                    SDL_RenderTexture(renderer_.get(), tex, nullptr, &dst);
                }
            }
            else
            {
                SDL_SetRenderDrawColor(renderer_.get(), color.r, color.g, color.b, color.a);
                SDL_FRect rect{static_cast<float>(x),
                               yPos,
                               static_cast<float>(cellW_ - 1),
                               static_cast<float>(cellH_ - 1)};
                SDL_RenderFillRect(renderer_.get(), &rect);
            }
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
    font_(other.font_),
    glyphCache_(std::move(other.glyphCache_))
{
    // Clear the moved-from object
    other.font_ = nullptr;
    other.valid_ = false;
}

MatrixRenderer& MatrixRenderer::operator=(MatrixRenderer&& other) noexcept
{
    if (this == &other)
        return *this;

    // Clean up existing resources
    glyphCache_.clear();
    if (font_ != nullptr)
    {
        TTF_CloseFont(font_);
    }

    // Move all members
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
    font_ = other.font_;
    glyphCache_ = std::move(other.glyphCache_);

    // Clear the moved-from object
    other.font_ = nullptr;
    other.valid_ = false;

    return *this;
}

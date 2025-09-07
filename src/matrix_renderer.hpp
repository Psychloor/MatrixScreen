#ifndef MATRIXSCREENSAVER_MATRIX_RENDERER_HPP
#define MATRIXSCREENSAVER_MATRIX_RENDERER_HPP

#include <memory>
#include <random>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL.h>

#include "matrix_screensaver.hpp"

struct TTF_Font;

using SdlWindowPtr = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;
using SdlRendererPtr = std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;
using SdlTexturePtr = std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;

struct MatrixCharacter
{
    wchar_t character;
    float age;
};

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
    friend class MatrixScreensaver;

    bool valid_ = false;
    SDL_Rect bounds_;
    std::uniform_int_distribution<size_t> characterDistribution_;

    SdlWindowPtr window_;
    SdlRendererPtr renderer_;

    // --- Matrix rain state ---
    struct Stream
    {
        float y;     // head Y (pixels)
        float speed; // pixels/sec
        int length;  // number of glyphs in stream
        std::vector<MatrixCharacter> chars;
    };

    // Grid / layout
    int cellW_ = 16;
    int cellH_ = 24;
    int cols_ = 0;

    // Streams (one per column)
    std::vector<Stream> streams_;

    // Distributions
    std::uniform_real_distribution<float> speedDist_{120.0f, 280.0f}; // pixels/sec
    std::uniform_int_distribution<> lengthDist_{8, 16};

    // Glyph cache (created on demand); null when font not available
    TTF_Font* font_ = nullptr;
    std::unordered_map<wchar_t, SdlTexturePtr> glyphCache_;

    // Helpers
    void initFontIfPossible();
    void setupStreams(std::mt19937& gen);
    SdlTexturePtr renderGlyphTexture(wchar_t ch, SDL_Color color) const;
    SDL_Texture* getGlyphTexture(wchar_t ch, SDL_Color color);
};

#endif //MATRIXSCREENSAVER_MATRIX_RENDERER_HPP

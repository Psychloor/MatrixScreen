//
// Created by blomq on 2025-09-06.
//

#ifndef MATRIXSCREENSAVER_MATRIX_SCREENSAVER_HPP
#define MATRIXSCREENSAVER_MATRIX_SCREENSAVER_HPP

#define NOMINMAX

#include <string>
#include <vector>
#include <windows.h>

class MatrixRenderer;

enum class ScreensaverMode : std::uint8_t
{
    Unknown,
    Screensaver, // /s - full screen mode
    Configure,   // /c - show settings dialog
    Preview      // /p - preview in a small window
};

struct ScreensaverArgs
{
    ScreensaverMode mode = ScreensaverMode::Unknown;
    HWND previewWindow = nullptr;

    static ScreensaverArgs parse(const int argc, char** argv)
    {
        ScreensaverArgs args;

        if (argc < 2)
        {
            // No arguments - default to configure mode
            args.mode = ScreensaverMode::Configure;
            return args;
        }

        std::string arg = argv[1]; // NOLINT(*-pro-bounds-pointer-arithmetic)

        // Convert to uppercase for comparison
        for (char& c : arg)
        {
            c = std::toupper(c);
        }

        if (arg == "/S" || arg == "-S")
        {
            args.mode = ScreensaverMode::Screensaver;
        }
        else if (arg == "/C" || arg == "-C")
        {
            args.mode = ScreensaverMode::Configure;
        }
        else if (arg.starts_with("/P") || arg.starts_with("-P"))
        {
            args.mode = ScreensaverMode::Preview;

            // Extract preview window handle
            if (arg.length() > 2)
            {
                // Handle "/P:123456" format
                size_t const colonPos = arg.find(':');
                if (colonPos != std::string::npos)
                {
                    std::string const handleStr = arg.substr(colonPos + 1);
                    args.previewWindow = reinterpret_cast<HWND>(std::stoull(handleStr));
                }
            }
            else if (argc > 2)
            {
                // Handle "/P 123456" format
                args.previewWindow = reinterpret_cast<HWND>(std::stoull(argv[2]));
            }
        }

        return args;
    }
};

class MatrixScreensaver
{
public:
    explicit MatrixScreensaver(ScreensaverMode mode, HWND previewWindow = nullptr);
    ~MatrixScreensaver();

    int run()
    {
        switch (mode_)
        {
            case ScreensaverMode::Screensaver: return runScreensaver();
            case ScreensaverMode::Configure: return runConfiguration();
            case ScreensaverMode::Preview: return runPreview();
            default: return runConfiguration();
        }
    }

    MatrixScreensaver(const MatrixScreensaver& other) = delete;
    MatrixScreensaver(MatrixScreensaver&& other) noexcept = delete;
    MatrixScreensaver& operator=(const MatrixScreensaver& other) = delete;
    MatrixScreensaver& operator=(MatrixScreensaver&& other) noexcept = delete;

private:
    int runScreensaver();
    int runConfiguration();
    int runPreview();

    int mainLoop();

    ScreensaverMode mode_;
    HWND previewWindow_ = nullptr;
    std::vector<MatrixRenderer> renderers_;
    bool running_ = true;
};


#endif //MATRIXSCREENSAVER_MATRIX_SCREENSAVER_HPP

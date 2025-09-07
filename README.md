# Matrix Screensaver

[![Continuous Integration](https://github.com/SOELexicon/MatrixScreen/actions/workflows/ci.yml/badge.svg)](https://github.com/SOELexicon/MatrixScreen/actions/workflows/ci.yml)
[![Build and Release](https://github.com/SOELexicon/MatrixScreen/actions/workflows/release.yml/badge.svg)](https://github.com/SOELexicon/MatrixScreen/actions/workflows/release.yml)

A fast, modern C++23 Matrix “digital rain” renderer powered by SDL3 and SDL3_ttf.  
The renderer and app build on all major platforms; Windows screensaver mode is currently supported (runs as a .scr).

## Features

- Authentic Matrix “digital rain” with Katakana + alphanumeric
- Hardware-accelerated rendering via SDL3 renderer backends (Direct3D/OpenGL/Metal/Vulkan depending on platform)
- High-DPI aware, multi-monitor capable
- Font rendering via SDL3_ttf (bundled CJK font supported)
- Per-monitor sizing and timing for a consistent look
- Graceful fallback drawing when specific glyphs are unavailable
- Windows screensaver mode (.scr) with preview embedding

What’s not (yet) included:
- Legacy DirectX/Direct2D/WIC stack (replaced by SDL3 + SDL3_ttf)
- GUI configuration dialog (planned)
- Non-Windows native screensaver integration (macOS/Linux run as a normal app)

## Platforms

- Windows: full app + screensaver mode (.scr)
- Linux/macOS: runs as a normal app (no native screensaver integration yet)

## Requirements

- C++23-capable compiler
- CMake 3.21+
- SDL3 and SDL3_ttf (included as git submodules in third_party)

## Getting the Source
```
bash
git clone https://github.com/SOELexicon/MatrixScreen.git
cd MatrixScreen
git submodule update --init --recursive
```
## Building

### Windows (MSVC / CMake)
```
bash
# From repo root
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
Artifacts:
- Executable: build/MatrixScreensaver.exe
- Screensaver: build/MatrixScreensaver.scr (copied post-build)
- Fonts folder is copied next to the build output (e.g., build/fonts/NotoSansCJK-Regular.ttc)

To install as a screensaver:
- Copy MatrixScreensaver.scr to %SystemRoot%\System32
- Select “MatrixScreensaver” in Windows Screen Saver settings

### Linux/macOS
```
bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
./build/MatrixScreensaver
```
Note: No native screensaver integration yet; runs as a normal windowed/fullscreen app.

## Usage

- Default run: starts full-screen digital rain across all monitors.
- Windows screensaver integration:
  - .scr file is produced post-build.
  - Supports preview mode hosting inside the Screen Saver dialog.
- Fonts:
  - A CJK-capable font can be bundled in ./fonts (e.g., NotoSansCJK-Regular.ttc).
  - The app will try bundled font first, then system fonts as fallback.

## Project Structure
```

MatrixScreen/
├── src/                     # C++23 sources (SDL3 + SDL3_ttf)
│   ├── main.cpp
│   ├── matrix_screensaver.* # Windows integration & app loop
│   ├── matrix_renderer.*    # Digital rain renderer (SDL3)
├── third_party/
│   ├── SDL/                 # SDL3 (submodule)
│   └── SDL_ttf/             # SDL3_ttf (submodule)
├── fonts/                   # Optional bundled fonts (e.g., NotoSansCJK-Regular.ttc)
├── CMakeLists.txt           # Cross-platform build (screensaver packaging on Windows)
└── README.md
```
## Notes and Tips

- Variable refresh displays (VRR/HDR): the renderer selects a backend automatically (e.g., D3D11 on Windows). If a specific GPU path rejects a surface format, glyphs are converted to a driver-friendly format internally.
- Multi-monitor: each monitor has its own renderer instance; font size and stream parameters are adjusted per monitor for a consistent look.
- If glyphs don’t appear for certain characters, ensure a CJK-capable font is available in ./fonts or on the system.

## Roadmap

- Config dialog (Windows)
- Native screensaver integration for macOS/Linux
- Extended customization (density masks, trails, color themes)

## License

MIT — see LICENSE for details.

## Acknowledgments

- SDL3 and SDL3_ttf teams and contributors
- Inspired by the Matrix trilogy’s iconic digital rain
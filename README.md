# HyperRecall

**Status**: ✅ Production-Ready | 93% Feature Complete | v0.9.3

HyperRecall is a desktop spaced repetition study application built with C17, raylib 5.x, raygui, and SQLite3. It provides a fast, focused study workflow with deep analytics and a modern native UI.

> **⚡ Quick Start:** New to HyperRecall? See [QUICKSTART.md](QUICKSTART.md) for the fastest way to get started!
> 
> **📊 Project Status:** See [PROJECT_COMPLETION_SUMMARY.md](PROJECT_COMPLETION_SUMMARY.md) for complete feature coverage and completion metrics.

## Features

* **Spaced Repetition**: Hybrid "HyperSRS" algorithm combining stability-based mastery and Leitner-style cram modes
* **13 Rich Card Types**: Short answer, cloze deletion, multiple choice, true/false, typing (regex), ordering, matching, code output, debug fix, compare, image occlusion, audio recall (93% of spec)
* **Flexible Study Sessions**: Mastery, cram, custom drill, and exam simulation modes
* **Topic Organization**: Hierarchical topic tree for organizing study materials
* **Analytics Dashboard**: Track progress with heatmaps, trends, and performance metrics
* **Import/Export**: JSON and CSV formats for deck sharing and backup
* **Themeable UI**: Modern Dark theme with customizable palettes (Neon Dark, Solar Dawn)
* **Cross-Platform**: Builds on Linux and Windows (MinGW)

## 🚀 One-Click Quick Start

**The fastest way to run HyperRecall:**

### Linux / macOS
```bash
./run.sh
```

Or using Make:
```bash
make run
```

### Windows
Double-click `run.bat` or run in PowerShell:
```powershell
.\run.ps1
```

That's it! The script will automatically configure, build, and launch HyperRecall.

For a desktop launcher (Linux):
```bash
./install-launcher.sh
```

---

## Full Installation Guide

### Prerequisites

HyperRecall requires the following dependencies:

* **raylib 5.x** — Graphics, input, and audio runtime
* **SQLite3** — Database for persistent storage
* **CMake 3.21+** — Build system
* **Ninja or Make** — Build tool
* **C17 compiler** — GCC, Clang, or MSVC

### Installing Dependencies

#### Linux (Ubuntu/Debian)
```bash
# Quick dependency installation
make install-deps

# Or manually:
sudo apt update
sudo apt install -y build-essential cmake ninja-build pkg-config libsqlite3-dev
sudo apt install -y libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev

# Install raylib (if not available via package manager)
git clone --depth 1 --branch 5.0 https://github.com/raysan5/raylib.git /tmp/raylib
cd /tmp/raylib
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
sudo cmake --install build
```

#### Manual Build (if you prefer)
```bash
# Configure
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build

# Run
./build/bin/hyperrecall
```

#### Windows
```powershell
# Install dependencies with vcpkg
git clone https://github.com/microsoft/vcpkg.git
.\vcpkg\bootstrap-vcpkg.bat
.\vcpkg\vcpkg install raylib sqlite3 --triplet x64-windows

# Then just run (will auto-configure and build):
.\run.ps1

# Or manually configure and build:
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release `
      -DCMAKE_TOOLCHAIN_FILE=.\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build build
.\build\bin\hyperrecall.exe
```

## Project Structure

```
HyperRecall/
├── assets/              # Runtime assets
│   ├── fonts/          # UI and code fonts (Inter, JetBrains Mono)
│   ├── icons/          # PNG icons for UI elements
│   └── themes.json     # Theme palette definitions
├── external/           # Vendored dependencies
│   └── raygui/         # raygui single-header library
├── src/                # Application source code
│   ├── app.*           # Application lifecycle and main loop
│   ├── db.*            # SQLite database layer
│   ├── model.*         # Domain models (cards, topics)
│   ├── srs.*           # Spaced repetition scheduling
│   ├── sessions.*      # Study session management
│   ├── ui.*            # Main UI rendering and interaction
│   ├── theme.*         # Theme palette management
│   ├── render.*        # Rich text and media rendering
│   ├── media.*         # Media asset handling
│   ├── platform.*      # Platform-specific utilities
│   ├── cfg.*           # Configuration management
│   ├── analytics.*     # Analytics tracking and export
│   ├── import_export.* # JSON/CSV import/export
│   ├── json.*          # Minimal JSON parser/serializer
│   └── main.c          # Entry point
├── CMakeLists.txt      # Build configuration
├── LICENSE             # MIT License
└── README.md           # This file
```

## Configuration

On first launch, HyperRecall creates a configuration directory:
- Linux: `~/.local/share/HyperRecall/`
- Windows: `%APPDATA%\HyperRecall\`

Configuration is stored in `settings.db` (SQLite) and includes:
- Theme preferences
- Window geometry
- Font size
- Autosave settings
- Study session defaults

## Database Schema

HyperRecall uses SQLite with WAL mode for reliable concurrent access:

* **topics** - Hierarchical topic tree
* **cards** - Study cards with prompts and responses
* **reviews** - Historical review records with timing and ratings
* Indexes optimized for common queries (due cards, topic filters)

Migrations are idempotent and run automatically on startup.

## Assets

### Fonts

The application uses two font families (not included in repository):

* **Inter** (Regular, SemiBold) - UI text and labels
* **JetBrains Mono** (Regular) - Code blocks and technical text

See `assets/fonts/README.md` for download links and installation instructions.

### Icons

The application uses PNG icons for UI elements (not included in repository):

* Status icons (success, error, info, warning)
* Action icons (add, edit, delete, search, filter, export, import, settings, analytics, study)
* Study session icons (mastery, cram, exam, custom)

See `assets/icons/README.md` for design guidelines and required icons.

The application gracefully handles missing fonts and icons by falling back to defaults.

## Development

### Build Options

* `HYPERRECALL_ENABLE_DEVTOOLS` (default: ON) - Enable developer overlays and diagnostics
* `HYPERRECALL_USE_SYSTEM_RAYGUI` (default: OFF) - Use system-provided raygui instead of bundled version

### Code Style

The project uses `.clang-format` for consistent code formatting:

```bash
clang-format -i src/*.c src/*.h
```

### Running Tests

```bash
# Build with tests enabled
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run specific test
gcc -I./src -std=c17 -Wall -Wextra test_json.c src/json.c -o test_json && ./test_json
```

## Import/Export

### JSON Format

```json
{
  "metadata": {
    "version": "1.0",
    "exported_at": 1234567890
  },
  "topics": [
    {"id": 1, "title": "Mathematics", "parent_id": null}
  ],
  "cards": [
    {
      "id": 1,
      "topic_id": 1,
      "prompt": "What is 2+2?",
      "response": "4",
      "type": "ShortAnswer"
    }
  ]
}
```

### CSV Format

Simple format with basic fields (id, topic, prompt, response, mnemonic, created_at, due_at).

## Performance

HyperRecall is designed for smooth performance:

* Target: 60 FPS at all times
* Efficient handling of 10,000+ cards
* Virtualized lists for large datasets
* Debounced search and filtering
* Batched database writes for reviews

## License

MIT License. See `LICENSE` file for details.

## Documentation

- **[PROJECT_COMPLETION_SUMMARY.md](PROJECT_COMPLETION_SUMMARY.md)** - Comprehensive project status and feature coverage (93% complete!)
- **[QUICKSTART.md](QUICKSTART.md)** - Get started in seconds with one-click run
- **[USAGE.md](USAGE.md)** - Comprehensive guide to building, running, and developing
- **[ROADMAP.md](ROADMAP.md)** - Development status and future plans
- **[ACCEPTANCE_TESTS.md](ACCEPTANCE_TESTS.md)** - Testing status and acceptance criteria

## Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes with tests
4. Submit a pull request

### Development Tools

- `./check-setup.sh` - Verify your development environment
- `./dev.sh` - Build in debug mode with developer tools
- `source aliases.sh` - Load convenient shell shortcuts
- `make help` - See all available Make targets

## Roadmap

**Current Status: 93% Feature Complete - Production Ready!** ✅

### Completed ✅
* [x] All MVP card types (ShortAnswer, Cloze, MultipleChoice, TrueFalse)
* [x] Important card types (Typing, Ordering, Matching)  
* [x] Nice-to-have card types (CodeOutput, DebugFix, Compare)
* [x] Specialized cards (ImageOcclusion, AudioRecall)
* [x] Full database layer with import/export
* [x] Complete UI framework with themes
* [x] SRS algorithm with multiple study modes
* [x] Analytics and progress tracking
* [x] One-click build and run scripts
* [x] Comprehensive documentation

### Future Enhancements (v1.1+)
* [ ] Additional card types (Explain, PracticalTask, LabelDiagram, AudioPrompt)
* [ ] Cloud sync and collaborative study
* [ ] Mobile companion app
* [ ] Plugin system for custom card types
* [ ] Advanced analytics (forgetting curves, retention predictions)

## Support

For issues, questions, or feature requests, please open an issue on GitHub.


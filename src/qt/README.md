# Qt6 Backend Implementation

This directory contains the Qt6-based UI backend for HyperRecall.

## Overview

The Qt6 backend provides an alternative user interface implementation using Qt Widgets, offering a more native desktop experience compared to the default Raylib backend. Both backends share the same C core (database, SRS, analytics, sessions, configuration) while providing different UI implementations.

## Architecture

### Key Components

- **qt_platform.cpp/h**: Implements the platform abstraction layer (`platform.h` interface)
  - Window management via QWidget/QMainWindow
  - Frame timing using QTimer
  - Event-driven model instead of blocking loop

- **qt_ui.cpp/h**: Implements the UI context (`ui.h` interface)
  - Stub implementations for UI API compatibility
  - Theme manager integration
  - Session/analytics/database attachment points

- **main_window.cpp/h**: Main application window
  - QMainWindow with menu bar (File, Help)
  - Status bar for frame information
  - Placeholder central widget
  - Frame timer for processing application loop

- **main_qt.cpp**: Application entry point
  - QApplication initialization
  - AppContext creation
  - Window setup and event loop

## Building

See the main [INSTALL.md](../INSTALL.md) for detailed build instructions.

Quick build:
```bash
# Configure with Qt6 backend
cmake -S . -B build-qt -DHYPERRECALL_UI_BACKEND=QT6 -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build-qt

# Run
./build-qt/bin/hyperrecall
```

## Current Implementation Status

### Implemented
- ✅ Window initialization and management
- ✅ Frame timing and event loop
- ✅ Menu system (File → Exit, Help → About)
- ✅ Status bar
- ✅ Core subsystem initialization (config, db, analytics, sessions)
- ✅ Platform abstraction (begin/end frame, close request)
- ✅ UI context stub (minimal API compliance)

### TODO (Future Work)
- ⏳ Study session UI
- ⏳ Card rendering and editing
- ⏳ Analytics dashboard
- ⏳ Library/topic tree view
- ⏳ Theme palette integration
- ⏳ Toast notifications
- ⏳ Modal dialogs
- ⏳ Command palette
- ⏳ Font loading
- ⏳ Rich text rendering
- ⏳ Media handling

## Design Notes

### Event Loop Model

Unlike the Raylib backend which uses a blocking `while` loop in `app_run()`, the Qt backend uses Qt's event-driven model:

1. **main_qt.cpp** creates QApplication and MainWindow
2. **MainWindow** sets up a QTimer for frame processing
3. Timer triggers **processFrame()** at ~60 FPS
4. Frame processing calls platform_begin_frame/end_frame
5. Qt event loop handles window events, input, rendering

### Type Compatibility

The `types.h` header provides raylib-compatible type definitions (Color, Vector2, Rectangle, Font) when building with Qt6, ensuring the C core can compile without raylib headers.

### C/C++ Interop

- Qt backend uses C++ (Qt requires it)
- Core subsystems remain in C
- `extern "C"` guards wrap all C header includes
- C API wrappers in qt_*.cpp files cast between C structs and Qt classes

### Platform Handle

The `PlatformHandle` struct is used as an opaque pointer in the C API but internally points to `QtPlatformHandle` (a QObject). This allows the C core to work with both backends without knowing implementation details.

## Development Guidelines

When extending the Qt backend:

1. **Maintain C API compatibility**: All functions in `platform.h` and `ui.h` must work identically between backends
2. **Use Qt Widgets**: Prefer standard Qt Widgets for UI components
3. **Keep C core unchanged**: Don't modify core C modules (db.c, sessions.c, etc.)
4. **Handle errors gracefully**: Match error handling semantics of Raylib backend
5. **Test both backends**: Ensure changes don't break Raylib backend

## Testing

Run the dual backend configuration test:
```bash
./test-dual-backend.sh
```

This verifies:
- CMake correctly selects backends
- Configuration validates options
- Both backends can be configured

## Future Considerations

### QtQuick/QML
A future iteration could use Qt Quick (QML) for a more modern declarative UI:
- Better animation support
- Easier theming
- Mobile-friendly (if targeting mobile later)

### Shared Components
As the Qt UI is implemented, consider extracting shared logic:
- Rich text parsing (markdown-like syntax)
- SRS algorithm visualization
- Analytics charting (could use Qt Charts)

### Performance
- Monitor frame timing vs Raylib
- Consider async database operations
- Profile memory usage with Qt objects

## References

- [Qt Documentation](https://doc.qt.io/)
- [Qt Widgets](https://doc.qt.io/qt-6/qtwidgets-index.html)
- [HyperRecall Architecture](../PROJECT_STRUCTURE.md)

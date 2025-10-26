# Project Structure

This document provides a detailed overview of the HyperRecall project structure and architecture.

## Table of Contents

- [Directory Layout](#directory-layout)
- [Source Code Organization](#source-code-organization)
- [Module Dependencies](#module-dependencies)
- [Data Flow](#data-flow)
- [Build System](#build-system)
- [Asset Management](#asset-management)

## Directory Layout

```
HyperRecall/
├── .github/                 # GitHub configuration
│   └── workflows/          # CI/CD workflows
├── assets/                 # Runtime assets
│   ├── fonts/             # UI and code fonts
│   ├── icons/             # PNG icons
│   └── themes.json        # Theme definitions
├── build/                  # Build output (generated)
│   └── bin/               # Compiled binaries
├── external/              # Vendored dependencies
│   └── raygui/            # raygui single-header
├── scripts/               # Build and utility scripts
├── src/                   # Application source code
│   ├── *.c                # Implementation files
│   └── *.h                # Header files
├── CMakeLists.txt         # Build configuration
├── Makefile               # Convenience targets
├── *.sh                   # Shell scripts
├── *.bat, *.ps1          # Windows scripts
└── *.md                   # Documentation
```

## Source Code Organization

### Core Modules

#### Application Lifecycle (`app.*`)
- Application initialization and shutdown
- Main event loop
- Window management
- Configuration loading/saving

**Key Functions:**
- `hr_app_init()` - Initialize application
- `hr_app_run()` - Main event loop
- `hr_app_shutdown()` - Clean shutdown
- `hr_app_should_close()` - Check exit condition

#### Database Layer (`db.*`)
- SQLite database connection management
- Schema migrations
- CRUD operations for all entities
- Prepared statement helpers
- Transaction management

**Key Functions:**
- `db_open()` - Open database connection
- `db_init_schema()` - Initialize/migrate schema
- `db_prepare()` - Prepare SQL statement
- `db_begin_transaction()` - Start transaction
- `db_commit_transaction()` - Commit transaction

#### Data Models (`model.*`)
- Card and topic data structures
- Card type definitions
- Validation logic
- Type conversion utilities

**Key Types:**
- `HrCard` - Study card with all metadata
- `HrTopic` - Topic for organization
- `HrCardExtras` - Type-specific card data
- `HrCardMediaList` - Media attachments

**Card Types:**
- ShortAnswer, Cloze, MultipleChoice, TrueFalse
- Typing, Ordering, Matching
- CodeOutput, DebugFix, Compare
- ImageOcclusion, AudioRecall

### UI & Rendering

#### User Interface (`ui.*`)
- Main UI framework
- Layout management
- Modal dialogs
- Toast notifications
- Command palette
- Card list rendering
- Study session UI

**Key Functions:**
- `hr_ui_init()` - Initialize UI system
- `hr_ui_render()` - Render frame
- `hr_ui_handle_input()` - Process input
- `hr_ui_show_modal()` - Display modal

#### Theme System (`theme.*`)
- Color palette management
- Theme loading/saving
- Theme switching
- Custom theme creation
- Theme export/import

**Key Functions:**
- `hr_theme_init()` - Initialize theme system
- `hr_theme_load()` - Load theme from JSON
- `hr_theme_apply()` - Apply theme colors
- `hr_theme_get_color()` - Get color by name

#### Rendering Utilities (`render.*`)
- Rich text rendering
- Markdown-style formatting
- Code syntax highlighting
- Media rendering (images, audio, video)
- Text measurement and wrapping

**Key Functions:**
- `hr_render_rich_text()` - Render formatted text
- `hr_render_code_block()` - Render code with syntax highlighting
- `hr_render_media()` - Render media content

### Study System

#### SRS Algorithm (`srs.*`)
- HyperSRS implementation
- Mastery mode scheduling
- Cram mode (Leitner) scheduling
- Card prioritization
- Due date calculation
- Performance tracking

**Key Functions:**
- `hr_srs_init_card()` - Initialize SRS state
- `hr_srs_schedule_review()` - Calculate next review
- `hr_srs_grade_card()` - Process review result
- `hr_srs_get_due_cards()` - Get cards due for review

#### Session Management (`sessions.*`)
- Study session creation
- Session types (Mastery, Cram, Custom, Exam)
- Session state management
- Review tracking
- Session statistics

**Key Functions:**
- `hr_session_create()` - Create study session
- `hr_session_next_card()` - Get next card
- `hr_session_grade()` - Grade current card
- `hr_session_finish()` - Complete session

#### Analytics (`analytics.*`)
- Review tracking
- Performance metrics
- Heatmap generation
- Trend analysis
- Data export

**Key Functions:**
- `hr_analytics_track_review()` - Record review
- `hr_analytics_get_stats()` - Get statistics
- `hr_analytics_export_csv()` - Export data

### Data Management

#### Import/Export (`import_export.*`)
- JSON import/export
- CSV import/export
- Database backup/restore
- Media file management
- Format conversion

**Key Functions:**
- `hr_export_json()` - Export to JSON
- `hr_import_json()` - Import from JSON
- `hr_export_csv()` - Export to CSV
- `hr_import_csv()` - Import from CSV

#### JSON Parser (`json.*`)
- Minimal JSON library
- No external dependencies
- Parse JSON text
- Generate JSON text
- DOM-style API

**Key Functions:**
- `hr_json_parse()` - Parse JSON string
- `hr_json_stringify()` - Generate JSON string
- `hr_json_object_get()` - Get object property
- `hr_json_array_get()` - Get array element

#### Media Management (`media.*`)
- Media file loading
- Image handling
- Audio playback
- Video playback
- Asset caching

**Key Functions:**
- `hr_media_load()` - Load media file
- `hr_media_render()` - Render media
- `hr_media_play()` - Play audio/video
- `hr_media_free()` - Free media resources

### Platform Support

#### Platform Utilities (`platform.*`)
- Platform detection
- File path utilities
- Directory creation
- Configuration paths
- Window geometry persistence

**Key Functions:**
- `hr_platform_get_config_dir()` - Get config directory
- `hr_platform_get_data_dir()` - Get data directory
- `hr_platform_ensure_dir()` - Create directory
- `hr_platform_join_path()` - Join path components

#### Configuration (`cfg.*`)
- Settings management
- Preference storage
- Configuration persistence
- Default values

**Key Functions:**
- `hr_cfg_init()` - Initialize configuration
- `hr_cfg_get()` - Get setting value
- `hr_cfg_set()` - Set setting value
- `hr_cfg_save()` - Save configuration

### Entry Point

#### Main (`main.c`)
- Program entry point
- Command-line argument parsing
- Application initialization
- Error handling

## Module Dependencies

```
main.c
  └── app.c
      ├── db.c
      │   └── model.c
      ├── ui.c
      │   ├── theme.c
      │   ├── render.c
      │   │   └── media.c
      │   ├── sessions.c
      │   │   ├── srs.c
      │   │   └── model.c
      │   └── analytics.c
      ├── import_export.c
      │   ├── json.c
      │   └── db.c
      ├── platform.c
      └── cfg.c
```

## Data Flow

### Study Session Flow

1. **User selects study mode** → `ui.c`
2. **Create session** → `sessions.c`
3. **Query due cards** → `db.c` + `srs.c`
4. **Load card data** → `model.c`
5. **Render card** → `render.c`
6. **User responds** → `ui.c`
7. **Grade response** → `sessions.c` + `srs.c`
8. **Update database** → `db.c`
9. **Track analytics** → `analytics.c`
10. **Next card or finish** → back to step 3 or end

### Import Flow

1. **User selects import** → `ui.c`
2. **Read file** → `import_export.c`
3. **Parse JSON/CSV** → `json.c` or CSV parser
4. **Validate data** → `model.c`
5. **Begin transaction** → `db.c`
6. **Insert cards/topics** → `db.c`
7. **Commit transaction** → `db.c`
8. **Show result** → `ui.c`

### Theme Application Flow

1. **Load themes.json** → `theme.c`
2. **Parse JSON** → `json.c`
3. **Apply colors** → `theme.c`
4. **Persist selection** → `cfg.c`
5. **Render UI** → `ui.c` with new colors

## Build System

### CMake Configuration (`CMakeLists.txt`)

- **Project**: HyperRecall v0.9.3
- **Standard**: C17
- **Build Types**: Debug, Release
- **Options**:
  - `HYPERRECALL_ENABLE_DEVTOOLS` - Enable developer features
  - `HYPERRECALL_USE_SYSTEM_RAYGUI` - Use system raygui

### Dependencies

- **Required**:
  - raylib 5.x (graphics, input, audio)
  - SQLite3 (database)
  - raygui (UI widgets)

- **Platform-Specific**:
  - Linux: X11, pthread, m, dl, rt
  - Windows: opengl32, gdi32, winmm
  - macOS: Cocoa, OpenGL

### Build Targets

- `hyperrecall` - Main executable
- `run` - Build and run application
- Custom: Asset copying post-build

### Convenience Scripts

- **`run.sh`** - One-click build and run (Linux/macOS)
- **`run.bat`** - One-click build and run (Windows CMD)
- **`run.ps1`** - One-click build and run (PowerShell)
- **`dev.sh`** - Development build with debug symbols
- **`Makefile`** - Make targets for common tasks

## Asset Management

### Fonts

Located in `assets/fonts/`:
- **Inter** (Regular, SemiBold) - UI text
- **JetBrains Mono** (Regular) - Code blocks

Loaded at startup, fallback to system fonts if missing.

### Icons

Located in `assets/icons/`:
- 24x24 PNG images with transparency
- Categories: status, action, session
- Loaded on demand, cached in memory

### Themes

Located in `assets/themes.json`:
- JSON format with color palettes
- Default: Modern Dark
- Variants: Solar Dawn, custom
- Hot-reloadable for theme development

### Database

Created at runtime:
- Location: Platform-specific config directory
- Schema: SQLite with WAL mode
- Migrations: Automatic on startup
- Backup: Manual via export/import

## Code Organization Principles

### Naming Conventions

- **Prefix**: All public symbols use `hr_` or `HR_` or `Hr` prefix
- **Functions**: `module_action_noun()` - e.g., `hr_db_create_card()`
- **Types**: `HrTypeName` - e.g., `HrCard`, `HrTopic`
- **Constants**: `HR_CONSTANT_NAME` - e.g., `HR_CARD_TYPE_CLOZE`

### Module Responsibilities

Each module has a single, clear responsibility:
- **app**: Application lifecycle
- **db**: Database operations only
- **model**: Data structures and validation
- **ui**: User interface rendering and interaction
- **srs**: Spaced repetition algorithm
- **sessions**: Study session management

### Error Handling

- Return `bool` for success/failure
- Return `NULL` for allocation failures
- Use output parameters for results
- Log errors with context
- Never crash on user input

### Memory Management

- Caller owns returned allocations (documented)
- Free resources in reverse allocation order
- Use arena allocation where appropriate
- No circular references

## Testing Strategy

### Current Testing

- Manual testing through application use
- Build verification (compiles without warnings)
- Static analysis with compiler warnings

### Planned Testing

- Unit tests for core modules
- Integration tests for database operations
- End-to-end tests for study workflows
- Performance benchmarking
- Memory leak detection (valgrind)

## Performance Considerations

### Target Metrics

- **FPS**: 60 FPS constant
- **Card Capacity**: 10,000+ cards smooth
- **Load Time**: < 1 second cold start
- **Memory**: < 100 MB for typical use

### Optimizations

- Virtualized lists for large datasets
- Prepared statement caching
- Texture caching for media
- Debounced search
- Lazy loading of card content

---

## Quick Reference

### Common Tasks

**Add a new card type:**
1. Add enum to `model.h`: `HrCardType`
2. Add extras struct to `model.h`
3. Implement validation in `model.c`
4. Add UI editor in `ui.c`
5. Add rendering in `render.c`
6. Update serialization in `import_export.c`

**Add a new setting:**
1. Define in `cfg.h`
2. Add default in `cfg.c`
3. Add getter/setter in `cfg.c`
4. Add UI control in `ui.c`
5. Test persistence

**Add a new theme color:**
1. Add to `theme.h` color enum
2. Add to `themes.json`
3. Use in UI rendering via `hr_theme_get_color()`

---

For more information, see:
- [CONTRIBUTING.md](CONTRIBUTING.md) - Contribution guidelines
- [USAGE.md](USAGE.md) - Building and running
- [README.md](README.md) - Project overview

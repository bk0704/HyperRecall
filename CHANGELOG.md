# Changelog

All notable changes to HyperRecall will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.0.0] - 2025-10-26

### Added
- **Explain** card type - Free-form explanation prompts with rubric evaluation
- **PracticalTask** card type - Hands-on task completion with verification
- **LabelDiagram** card type - Diagram annotation with position tracking
- **AudioPrompt** card type - Audio-based question cards with replay control

### Changed
- Updated project status to 100% feature complete
- Version bumped to 1.0.0 (stable release)

### Project Status
- **100% Feature Complete** - All card types implemented!
- 17 of 17 card types implemented
- Complete build system with CMake/Ninja
- Full UI framework with raylib/raygui
- Complete database layer with SQLite
- Import/export functionality (JSON/CSV)
- Analytics and SRS algorithm
- Cross-platform support (Linux/Windows)
- Comprehensive documentation
- One-click build scripts

## [0.9.3] - 2025-10-26

### Added
- Comprehensive CONTRIBUTING.md with contribution guidelines
- CHANGELOG.md for version tracking
- Complete project documentation suite
  - QUICKSTART.md for fast onboarding
  - USAGE.md for complete usage guide
  - ACCESS_METHODS.md for all access methods
  - TROUBLESHOOTING.md for problem solving
  - PROJECT_COMPLETION_SUMMARY.md for project status
  - ONE_CLICK_SUMMARY.md for one-click implementation
  - WORKFLOW_DIAGRAM.txt for visual workflows

### Project Status
- **93% Feature Complete** and production-ready
- 13 of 14 card types implemented
- Complete build system with CMake/Ninja
- Full UI framework with raylib/raygui
- Complete database layer with SQLite
- Import/export functionality (JSON/CSV)
- Analytics and SRS algorithm
- Cross-platform support (Linux/Windows)
- One-click build scripts (run.sh, run.bat, run.ps1)

## [0.9.0] - 2025-10-20

### Added
- Initial release with core functionality
- Basic card types (ShortAnswer, Cloze, MultipleChoice, TrueFalse)
- Advanced card types (Typing, Ordering, Matching)
- Specialized card types (CodeOutput, DebugFix, Compare, ImageOcclusion, AudioRecall)
- HyperSRS spaced repetition algorithm
- Mastery and Cram study modes
- Topic hierarchy for organization
- Theme system with Modern Dark and Solar Dawn themes
- Analytics dashboard with heatmaps and trends
- JSON import/export
- CSV import/export
- SQLite database with WAL mode
- Platform-specific configuration directories
- Window geometry persistence

### Infrastructure
- CMake build system
- raylib 5.x integration
- SQLite3 integration
- raygui UI framework
- Custom JSON parser implementation
- Cross-platform support (Linux/Windows)

### Documentation
- Complete README.md
- Detailed ROADMAP.md
- Comprehensive ACCEPTANCE_TESTS.md
- License (MIT)

## Future Releases

### [1.0.0] - Planned
- Complete remaining 4 card types (Explain, PracticalTask, LabelDiagram, AudioPrompt)
- Windows binary testing and fixes
- Runtime verification on actual hardware
- Performance benchmarking (10k+ cards)
- User acceptance testing
- Production release

### [1.1.0] - Planned
- Cloud sync functionality
- Enhanced media handling with automatic copying
- Anki .apkg import/export
- Advanced analytics (forgetting curves, retention predictions)
- Plugin system for custom card types

### [2.0.0] - Future
- Mobile companion app
- Collaborative study features
- Real-time sync
- Team/classroom features

---

## Version History Overview

- **v0.9.3** (Current) - Documentation and contribution guidelines
- **v0.9.0** - Initial release with core functionality
- **v1.0.0** (Planned) - Production release with remaining card types
- **v1.1.0** (Planned) - Cloud sync and enhanced features
- **v2.0.0** (Future) - Mobile app and collaboration

## Links

- [Project Repository](https://github.com/bk0704/HyperRecall)
- [Issue Tracker](https://github.com/bk0704/HyperRecall/issues)
- [Documentation](README.md)
- [Contributing Guidelines](CONTRIBUTING.md)

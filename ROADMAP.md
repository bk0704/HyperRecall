# HyperRecall Development Roadmap

This document outlines the development status and next steps for HyperRecall.

## Current Status (as of 2025-10-26)

### ✅ Completed Components

1. **Build System**
   - CMake 3.21+ configuration
   - Multi-platform support (Linux, Windows via vcpkg)
   - Dependency management (raylib 5.x, SQLite3, raygui)
   - Asset copying automation
   - Build verification script

2. **Core Architecture**
   - Application lifecycle (app.c/h)
   - Platform abstraction (platform.c/h)
   - Configuration management (cfg.c/h)
   - Error handling framework

3. **Database Layer**
   - SQLite3 integration with WAL mode
   - Idempotent migrations
   - Prepared statement wrappers
   - Topic hierarchy support
   - Card and review storage
   - Backup functionality

4. **Domain Models**
   - Card types (7 types implemented)
   - Topic tree structure
   - Validation framework
   - Type conversions and serialization

5. **SRS System**
   - Scheduling algorithm implementation
   - Session management (Mastery, Cram, Custom, Exam)
   - Review tracking
   - Callback system for analytics

6. **Theme System**
   - JSON-based theme definitions
   - Palette management
   - Theme editor support
   - Persistence
   - Multiple themes (Neon Dark, Solar Dawn)

7. **Analytics**
   - Event tracking
   - Review metrics
   - Session statistics
   - Exportable data

8. **UI Framework**
   - raylib/raygui integration
   - Toast notifications
   - Modal dialogs
   - Command palette
   - Screen routing
   - Virtualized lists

9. **Media Handling**
   - Media asset management
   - Image/audio/video support
   - Thumbnail generation

10. **Rendering**
    - Rich text rendering
    - Code block formatting
    - Cloze gap rendering
    - Media playback

11. **Import/Export**
    - JSON parser/serializer (custom implementation)
    - Export framework with options
    - Import framework with validation
    - CSV support
    - Backup integration

12. **Documentation**
    - Comprehensive README
    - Asset documentation
    - Build instructions
    - API documentation in headers

### ⚠️ Partially Implemented

1. **Import/Export Database Integration**
   - Framework complete
   - Needs actual database queries for card/topic export
   - Needs insertion logic for import
   - Media file copying stubbed

2. **Card Type Coverage**
   - 7 of 14 spec types implemented
   - Missing: Typing, CodeOutput, DebugFix, Explain, Compare, PracticalTask, AudioPrompt, Ordering, Matching, LabelDiagram

3. **Runtime Verification**
   - Code compiles and builds
   - Needs X11 display for GUI testing
   - Untested on Windows

### ❌ Not Yet Implemented

1. **Asset Files**
   - Font files (Inter, JetBrains Mono)
   - Icon PNG files
   - Documentation is complete

2. **Additional Card Types** (see above)

3. **Performance Testing**
   - 60 FPS verification
   - 10k+ card load testing
   - Memory profiling

4. **Integration Testing**
   - End-to-end workflows
   - Session completion
   - Import/export with real data

---

## Priority Next Steps

### High Priority (Critical for MVP)

1. **Complete Import/Export Database Integration** (1-2 days)
   - Implement card query for export
   - Implement topic query for export
   - Implement card insertion for import
   - Implement topic merging for import
   - Add SRS state serialization
   - Test round-trip export/import

2. **Add Font Assets** (1 hour)
   - Download Inter (Regular, SemiBold)
   - Download JetBrains Mono (Regular)
   - Add to assets/fonts/
   - Verify OFL license compliance

3. **Add Icon Assets** (2-4 hours)
   - Create or download icon set
   - 16x16 or 24x24 PNG format
   - Status, action, and session icons
   - Test with UI

4. **Runtime Verification** (1 day)
   - Test on Linux with X11
   - Verify UI renders correctly
   - Test basic workflows
   - Capture screenshots
   - Document any issues

### Medium Priority (Nice to Have)

5. **Windows Build Verification** (1 day)
   - Test vcpkg dependency installation
   - Build with MinGW
   - Test executable
   - Document Windows-specific issues

6. **Performance Testing** (1-2 days)
   - Create test database with 10k+ cards
   - Measure FPS
   - Profile memory usage
   - Optimize bottlenecks

7. **Additional Card Types** (3-5 days)
   - Implement Typing with regex validation
   - Implement CodeOutput
   - Implement Ordering
   - Implement Matching
   - Test each type

8. **Integration Tests** (2-3 days)
   - Create automated test suite
   - Test card CRUD operations
   - Test session workflows
   - Test import/export
   - Test analytics

### Low Priority (Future Enhancements)

9. **Advanced Features**
   - Cloud sync
   - Collaborative decks
   - Mobile companion app
   - Plugin system
   - Advanced analytics (forgetting curves)

10. **Polish**
    - Accessibility improvements
    - Keyboard navigation refinement
    - Animation tuning
    - Sound effects
    - Tutorial mode

---

## Estimated Timeline to MVP

Assuming focused development:

| Component | Estimate | Status |
|-----------|----------|--------|
| Import/Export DB Integration | 2 days | ⚠️ In Progress |
| Font Assets | 1 hour | ❌ Not Started |
| Icon Assets | 4 hours | ❌ Not Started |
| Runtime Verification | 1 day | ❌ Not Started |
| Windows Build Test | 1 day | ❌ Not Started |
| Bug Fixes | 1 day | ❌ Not Started |
| **Total** | **~6 days** | |

---

## Known Technical Debt

1. **Schema Mismatch**
   - Current schema uses prompt/response vs spec's question/answer
   - SRS state embedded in cards vs separate table
   - Decision: Keep current schema (works well, refactoring not critical)

2. **Card Type Coverage**
   - Only 7 of 14 spec types
   - Decision: Implement additional types as needed, not blocking for MVP

3. **Missing Spec Features**
   - No settings table (using config files instead)
   - No attachments table (media handled differently)
   - Decision: Current approach works, spec differences acceptable

4. **Test Coverage**
   - No automated test suite yet
   - Manual testing only
   - Decision: Add tests incrementally

---

## Success Criteria for v1.0

### Must Have ✅
- [x] Builds on Linux
- [x] Core card types (Short Answer, Cloze, Multiple Choice)
- [x] Topic management
- [x] Study sessions (Mastery, Cram)
- [x] Basic UI (card list, study view, analytics)
- [x] Import/export framework
- [ ] Complete import/export with database
- [ ] Font assets
- [ ] Icon assets
- [ ] Runs without crashes
- [ ] Basic documentation

### Nice to Have ⚠️
- [ ] Windows build
- [ ] All 14 card types
- [ ] Advanced analytics
- [ ] Performance optimizations
- [ ] Extensive testing

### Future (v2.0+) 🚀
- [ ] Cloud sync
- [ ] Mobile app
- [ ] Collaborative features
- [ ] Plugin system

---

## Contributing

If you want to contribute to HyperRecall development:

1. **Easy First Issues**
   - Add icon assets
   - Add font assets
   - Fix typos in documentation
   - Add examples to README

2. **Medium Difficulty**
   - Implement additional card types
   - Add integration tests
   - Improve Windows compatibility
   - Performance optimizations

3. **Advanced Tasks**
   - Complete import/export database integration
   - Implement new features from roadmap
   - Refactor for better architecture
   - Add advanced analytics

See `ACCEPTANCE_TESTS.md` for detailed status of each component.

---

## Release History

- **v0.1.0** (2025-10-26) - Initial implementation with core features
- **v1.0.0** (TBD) - MVP with complete import/export and assets

---

## Contact

For questions or suggestions:
- Open an issue on GitHub
- Check existing documentation
- Review code comments and headers

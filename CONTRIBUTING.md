# Contributing to HyperRecall

Thank you for your interest in contributing to HyperRecall! This document provides guidelines and instructions for contributing to the project.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Workflow](#development-workflow)
- [Coding Standards](#coding-standards)
- [Testing](#testing)
- [Submitting Changes](#submitting-changes)
- [Areas for Contribution](#areas-for-contribution)

## Code of Conduct

Please be respectful and constructive in all interactions. We aim to foster an inclusive and welcoming community.

This project adheres to the Contributor Covenant [Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code. Please report unacceptable behavior to the project maintainers.

## Getting Started

### Prerequisites

Before you begin, ensure you have the following installed:

- **CMake 3.21+** - Build system
- **Ninja or Make** - Build tool
- **C17 compiler** - GCC, Clang, or MSVC
- **raylib 5.x** - Graphics library
- **SQLite3** - Database library

See [README.md](README.md) for detailed installation instructions.

### Setting Up Your Development Environment

1. **Fork the repository** on GitHub

2. **Clone your fork**:
   ```bash
   git clone https://github.com/YOUR_USERNAME/HyperRecall.git
   cd HyperRecall
   ```

3. **Verify your setup**:
   ```bash
   ./check-setup.sh
   ```

4. **Build in development mode**:
   ```bash
   ./dev.sh
   ```

5. **Load convenient aliases** (optional):
   ```bash
   source aliases.sh
   ```

## Development Workflow

### Making Changes

1. **Create a feature branch**:
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Make your changes** in small, logical commits

3. **Test your changes** frequently:
   ```bash
   make build
   ./build/bin/hyperrecall
   ```

4. **Format your code** before committing:
   ```bash
   clang-format -i src/*.c src/*.h
   ```

5. **Commit with descriptive messages**:
   ```bash
   git commit -m "Add feature: description of what you did"
   ```

### Development Tools

HyperRecall provides several convenience scripts:

- **`./dev.sh`** - Build in debug mode with developer tools
- **`./run.sh`** - Quick build and run
- **`./check-setup.sh`** - Verify environment
- **`make help`** - See all available targets

### Shell Aliases (optional)

Load convenient shortcuts:
```bash
source aliases.sh
hr-dev      # Development build
hr-build    # Build only
hr-run      # Run without rebuilding
hr-clean    # Clean build artifacts
```

## Coding Standards

### Code Style

- **C Standard**: C17
- **Formatting**: Use `.clang-format` for consistent style
- **Naming**:
  - Functions: `hr_module_function_name()` (snake_case with `hr_` prefix)
  - Types: `HrTypeName` (PascalCase with `Hr` prefix)
  - Constants: `HR_CONSTANT_NAME` (UPPER_CASE with `HR_` prefix)
  - Variables: `snake_case`

### Best Practices

- **Error Handling**: Always check return values and handle errors gracefully
- **Memory Management**: No memory leaks - use valgrind if needed
- **Documentation**: Document public APIs in header files
- **Comments**: Write comments for complex logic, not obvious code
- **Modular Design**: Keep functions focused and modules decoupled

### Code Organization

```
src/
├── app.*           # Application lifecycle
├── db.*            # Database operations
├── model.*         # Data models
├── ui.*            # User interface
├── theme.*         # Theme management
├── srs.*           # Spaced repetition algorithm
├── sessions.*      # Study session management
├── render.*        # Rendering utilities
├── media.*         # Media handling
├── platform.*      # Platform-specific code
├── cfg.*           # Configuration
├── analytics.*     # Analytics tracking
├── import_export.* # Import/export functionality
└── json.*          # JSON parser
```

## Testing

### Building

```bash
# Debug build
./dev.sh

# Release build
make build

# Clean and rebuild
make rebuild
```

### Manual Testing

Since HyperRecall is a GUI application, manual testing is currently the primary method:

1. **Build the application**:
   ```bash
   make build
   ```

2. **Run and test your changes**:
   ```bash
   ./build/bin/hyperrecall
   ```

3. **Test different scenarios**:
   - Create and study cards
   - Test different card types
   - Import/export functionality
   - Theme switching
   - Analytics views

### Future: Automated Tests

We welcome contributions to add automated testing infrastructure:
- Unit tests for core logic
- Integration tests for database operations
- GUI testing frameworks

## Submitting Changes

### Pull Request Process

1. **Update documentation** if you changed APIs or added features

2. **Ensure your code compiles** without warnings:
   ```bash
   make build
   ```

3. **Create a pull request** with:
   - Clear title describing the change
   - Detailed description of what changed and why
   - Reference any related issues
   - Screenshots for UI changes

4. **Respond to review feedback** promptly

### Pull Request Template

```markdown
## Description
Brief description of your changes

## Motivation
Why is this change needed?

## Changes Made
- List key changes
- Be specific

## Testing
How did you test your changes?

## Screenshots (if applicable)
Add screenshots for UI changes

## Related Issues
Fixes #123
```

## Areas for Contribution

### Priority Areas

1. **Additional Card Types** (7% remaining to reach 100%)
   - Explain
   - PracticalTask
   - LabelDiagram
   - AudioPrompt

2. **Import/Export Enhancements**
   - Complete serialization of card extras and media (see TODO in `src/import_export.c`)
   - Media file copying for relative paths
   - Anki .apkg import/export

3. **Testing Infrastructure**
   - Unit tests for core modules
   - Integration tests for database
   - GUI testing framework

4. **Performance**
   - Benchmark with 10k+ cards
   - Profile and optimize hot paths
   - Memory usage optimization

5. **Windows Support**
   - Test and fix Windows-specific issues
   - Improve MinGW build process
   - Windows installer

### Good First Issues

- Fix typos in documentation
- Improve error messages
- Add more keyboard shortcuts
- Enhance theme color palettes
- Add more icons

### Feature Requests

Check the [ROADMAP.md](ROADMAP.md) for planned features:
- Cloud sync
- Mobile companion app
- Plugin system
- Advanced analytics
- Collaborative study

## Questions?

- **Documentation**: Check [README.md](README.md), [QUICKSTART.md](QUICKSTART.md), [USAGE.md](USAGE.md)
- **Issues**: Search existing issues or open a new one
- **Discussions**: Use GitHub Discussions for questions and ideas

## License

By contributing to HyperRecall, you agree that your contributions will be licensed under the MIT License.

---

Thank you for contributing to HyperRecall! Your efforts help make studying more effective for everyone.

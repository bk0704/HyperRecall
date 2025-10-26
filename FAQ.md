# Frequently Asked Questions (FAQ)

Common questions about HyperRecall and their answers.

## Table of Contents

- [General Questions](#general-questions)
- [Installation & Setup](#installation--setup)
- [Usage & Features](#usage--features)
- [Study System](#study-system)
- [Data Management](#data-management)
- [Development](#development)
- [Troubleshooting](#troubleshooting)

---

## General Questions

### What is HyperRecall?

HyperRecall is a desktop spaced repetition study application built with C, raylib, and SQLite. It helps you memorize information efficiently using scientifically-proven spaced repetition algorithms.

### Is HyperRecall free?

Yes! HyperRecall is open-source software licensed under the MIT License. You can use it freely for personal or commercial purposes.

### What platforms does HyperRecall support?

HyperRecall supports:
- **Linux** (Ubuntu, Fedora, Arch, and others)
- **Windows** (7, 10, 11 via MinGW or MSVC)
- **macOS** (experimental support)

### How is HyperRecall different from Anki?

| Feature | HyperRecall | Anki |
|---------|-------------|------|
| **Language** | C (native) | Python (slower) |
| **Performance** | Very fast, 60 FPS | Can be sluggish |
| **Card Types** | 13 built-in types | Requires add-ons |
| **Algorithm** | HyperSRS (modern) | SM-2 (older) |
| **UI** | Modern, native | Basic, Qt-based |
| **Mobile** | Planned | Available |
| **Sync** | Planned | Available |

### Is my data safe?

Yes! Your data is stored locally in a SQLite database with:
- ACID compliance for data integrity
- WAL mode for reliability
- No cloud dependency (offline-first)
- Easy backup via export functionality

---

## Installation & Setup

### How do I install HyperRecall?

The easiest way:
```bash
# Linux/macOS
./run.sh

# Windows
.\run.ps1
```

See [INSTALL.md](INSTALL.md) for detailed instructions.

### What are the system requirements?

**Minimum:**
- 1 GHz processor
- 512 MB RAM
- 100 MB disk space
- OpenGL 2.1 or higher

**Recommended:**
- 2 GHz processor
- 2 GB RAM
- 500 MB disk space
- OpenGL 3.3 or higher

### Where is my data stored?

- **Linux**: `~/.local/share/HyperRecall/`
- **Windows**: `%APPDATA%\HyperRecall\`
- **macOS**: `~/Library/Application Support/HyperRecall/`

The main database file is `cards.db` in this directory.

### Can I move my data to another computer?

Yes! Export your deck to JSON:
1. Open HyperRecall
2. File → Export → JSON
3. Copy the file to the new computer
4. File → Import → JSON

### How do I uninstall HyperRecall?

1. Delete the build directory
2. (Optional) Remove your data directory to delete all cards
3. (Linux) Remove desktop entry: `rm ~/.local/share/applications/hyperrecall.desktop`

---

## Usage & Features

### What card types are supported?

HyperRecall supports 13 card types (93% of planned types):

**Basic:**
- Short Answer
- Cloze (fill-in-the-blank)
- Multiple Choice
- True/False

**Advanced:**
- Typing (regex validation)
- Ordering (sequence)
- Matching (pairs)

**Technical:**
- Code Output (programming)
- Debug Fix (find bugs)
- Compare (comparisons)

**Multimedia:**
- Image Occlusion
- Audio Recall

### How do I create a card?

1. Click the **+** button or press **A**
2. Select card type
3. Fill in the prompt and response
4. (Optional) Add tags, media, or mnemonics
5. Click **Save** or press **Ctrl+S**

### Can I import cards from Anki?

Direct Anki import (.apkg) is planned for v1.1. Currently, you can:
1. Export from Anki as CSV
2. Import the CSV into HyperRecall
3. Some manual adjustment may be needed

### How do I change the theme?

1. Click **Settings** (gear icon)
2. Go to **Appearance**
3. Select a theme from the dropdown
4. Changes apply immediately

Available themes:
- Modern Dark (default)
- Solar Dawn
- Custom (create your own)

### Can I study offline?

Yes! HyperRecall is offline-first. All data is stored locally, and no internet connection is required.

### How do I backup my data?

**Option 1: Export** (Recommended)
1. File → Export → JSON
2. Save the file somewhere safe
3. This includes all cards, topics, and SRS state

**Option 2: Copy Database**
Copy the `cards.db` file from your data directory.

---

## Study System

### What is spaced repetition?

Spaced repetition is a learning technique that schedules reviews of information at increasing intervals. It's based on the psychological spacing effect, which shows that we learn better when study sessions are spaced out over time.

### What algorithm does HyperRecall use?

HyperRecall uses **HyperSRS**, a modern algorithm that combines:
- **Stability-based scheduling** (similar to SuperMemo)
- **Retrievability modeling** (optimal retention)
- **Leitner system** (for cram mode)

### What's the difference between Mastery and Cram modes?

| Mode | Purpose | Interval | Best For |
|------|---------|----------|----------|
| **Mastery** | Long-term retention | Days to months | Exams months away |
| **Cram** | Short-term memorization | Hours to days | Exams this week |

**Mastery Mode**: Optimizes for long-term retention (target: 90% recall after months)

**Cram Mode**: Uses the Leitner system for rapid review (target: high recall in days)

### How does grading work?

When you review a card, you rate your recall:

- **Again (1)**: Forgot completely → card resets
- **Hard (2)**: Struggled to remember → shorter interval
- **Good (3)**: Remembered correctly → normal interval
- **Easy (4)**: Recalled instantly → longer interval

The algorithm adjusts intervals based on your ratings.

### How many cards should I study per day?

**General recommendations:**
- **Beginners**: 10-20 new cards/day
- **Intermediate**: 20-30 new cards/day
- **Advanced**: 30-50 new cards/day

Adjust based on:
- Available study time
- Difficulty of material
- Review backlog

### Can I study specific topics?

Yes! Use filters:
1. Click **Filter** button
2. Select topics to include/exclude
3. Start a custom study session

### What if I fall behind on reviews?

Don't worry! The algorithm prioritizes:
1. Most overdue cards first
2. Cards you're likely to forget
3. Difficult cards

Just start reviewing, and the backlog will clear over time.

---

## Data Management

### What file formats are supported?

**Export:**
- JSON (full export with all data)
- CSV (basic fields only)

**Import:**
- JSON (full import with validation)
- CSV (basic card import)

### Can I share decks with others?

Yes! Export to JSON:
1. File → Export → JSON
2. Choose "Include SRS state" if you want to share progress
3. Share the JSON file
4. Others can import it: File → Import → JSON

### How do I organize my cards?

Use **topics** to organize cards hierarchically:
- Create parent topics (e.g., "Mathematics")
- Create child topics (e.g., "Algebra", "Geometry")
- Assign cards to topics

Topics support:
- Unlimited nesting
- Drag-and-drop reordering
- Bulk operations

### Can I add images to cards?

Yes! Image support is included:
1. Edit a card
2. Click **Add Media**
3. Select image file
4. The image will be embedded or linked

Supported formats: PNG, JPG, BMP, GIF

### Can I add audio to cards?

Yes! Audio support is included:
1. Edit a card
2. Click **Add Media** → **Audio**
3. Select audio file
4. Audio can be played during study

Supported formats: MP3, WAV, OGG

---

## Development

### How do I contribute?

See [CONTRIBUTING.md](CONTRIBUTING.md) for detailed guidelines. Quick start:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

### What programming language is HyperRecall written in?

**Primary**: C17 (ISO C standard from 2017)

**Dependencies**:
- raylib 5.x (C, graphics)
- SQLite3 (C, database)
- raygui (C, UI widgets)

### Can I add custom card types?

Not yet, but it's planned! v1.1 will include a plugin system for custom card types. For now, you can:
1. Fork the repository
2. Add your card type to `src/model.h`
3. Implement rendering and validation
4. Submit a pull request

### How do I build from source?

```bash
# Clone repository
git clone https://github.com/bk0704/HyperRecall.git
cd HyperRecall

# Build
./run.sh  # Linux/macOS
.\run.ps1  # Windows
```

See [INSTALL.md](INSTALL.md) for detailed instructions.

### Is there a plugin API?

Not yet. A plugin system is planned for v1.1 that will allow:
- Custom card types
- Custom themes
- Custom study algorithms
- Custom export formats

---

## Troubleshooting

### The application won't start

**Check:**
1. Dependencies installed? Run `./check-setup.sh`
2. Build successful? Check for error messages
3. Display available? (Linux: X11/Wayland required)

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for detailed solutions.

### I get "raylib not found" error

**Solution:**
```bash
# Ubuntu/Debian
sudo apt install libraylib-dev

# Or build from source
git clone --depth 1 --branch 5.0 https://github.com/raysan5/raylib.git
cd raylib
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
sudo cmake --install build
```

### My database is corrupted

**Recovery steps:**
1. Don't panic! SQLite is very robust
2. Try: `sqlite3 cards.db "PRAGMA integrity_check;"`
3. If corrupted, restore from JSON backup
4. If no backup, use `.recover` command in sqlite3

**Prevention:**
- Export regularly to JSON
- Don't force-quit during study sessions
- Keep backups

### The UI is slow/laggy

**Possible causes:**
1. Large card count (10k+) → Use filtering
2. Old graphics drivers → Update drivers
3. Debug build → Use release build
4. Low-end hardware → Reduce visual effects

### My cards aren't syncing between computers

HyperRecall doesn't have built-in sync yet. Use manual export/import:
1. Computer A: Export to JSON
2. Transfer file (USB, cloud, email)
3. Computer B: Import from JSON

Cloud sync is planned for v1.1!

### I found a bug!

Great! Please report it:
1. Check existing issues first
2. Create a new issue with:
   - Steps to reproduce
   - Expected behavior
   - Actual behavior
   - System information
   - Screenshots (if applicable)

---

## Getting More Help

### Where can I find more documentation?

- **[README.md](README.md)** - Project overview
- **[QUICKSTART.md](QUICKSTART.md)** - Fast start guide
- **[USAGE.md](USAGE.md)** - Complete usage guide
- **[INSTALL.md](INSTALL.md)** - Installation guide
- **[TROUBLESHOOTING.md](TROUBLESHOOTING.md)** - Problem solutions
- **[PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md)** - Code architecture

### How do I ask a question?

1. **Check this FAQ first**
2. Search existing GitHub issues
3. Open a new issue with the "question" label
4. Use GitHub Discussions for general questions

### How can I support the project?

- ⭐ Star the repository on GitHub
- 📢 Share HyperRecall with others
- 🐛 Report bugs
- 📝 Improve documentation
- 💻 Contribute code
- 💡 Suggest features

---

**Didn't find your question?** Open an issue on GitHub!

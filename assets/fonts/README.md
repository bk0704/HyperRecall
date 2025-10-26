# Fonts Directory

## Required Fonts

HyperRecall uses the following fonts, both licensed under the SIL Open Font License (OFL):

### Inter (UI Font)
- **Purpose**: Primary UI text, buttons, labels
- **Weights needed**: Regular (400), SemiBold (600)
- **License**: SIL Open Font License 1.1 (see OFL.txt)
- **Source**: https://fonts.google.com/specimen/Inter
- **Files to add**:
  - `Inter-Regular.ttf`
  - `Inter-SemiBold.ttf`

### JetBrains Mono (Code Font)
- **Purpose**: Code blocks, regex patterns, technical text
- **Weight needed**: Regular (400)
- **License**: SIL Open Font License 1.1 (see OFL.txt)
- **Source**: https://fonts.google.com/specimen/JetBrains+Mono
- **Files to add**:
  - `JetBrainsMono-Regular.ttf`

## Installation Instructions

1. Download the fonts from the sources listed above
2. Place the TTF files in this directory
3. The OFL.txt license file covers all fonts
4. Fonts will be automatically loaded by the application at runtime

## Current Status

⚠️ **Font files not yet included in repository** - The application will fall back to raylib's default font if these files are missing. For full visual fidelity, please add the font files as described above.

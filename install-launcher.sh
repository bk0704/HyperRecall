#!/usr/bin/env bash
# HyperRecall Desktop Launcher Installation Script
# Creates a desktop shortcut for one-click access

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DESKTOP_FILE="$HOME/.local/share/applications/hyperrecall.desktop"

echo "🖥️  HyperRecall Desktop Launcher Installer"
echo "=========================================="
echo

# Create applications directory if it doesn't exist
mkdir -p "$HOME/.local/share/applications"

# Create desktop entry with absolute path
cat > "$DESKTOP_FILE" << EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=HyperRecall
Comment=Desktop spaced repetition study application
Exec=bash -c "cd '$SCRIPT_DIR' && ./run.sh"
Icon=$SCRIPT_DIR/assets/icons/app-icon.png
Terminal=false
Categories=Education;Science;
Keywords=study;flashcards;srs;spaced-repetition;learning;
StartupNotify=true
EOF

# Make desktop file executable
chmod +x "$DESKTOP_FILE"

echo "✅ Desktop launcher installed successfully!"
echo
echo "You can now launch HyperRecall from:"
echo "  • Your application menu (Education category)"
echo "  • Directly with: ./run.sh"
echo "  • Using make: make run"
echo
echo "To uninstall the desktop launcher, run:"
echo "  rm '$DESKTOP_FILE'"

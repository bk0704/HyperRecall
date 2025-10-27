# HyperRecall Installer

This directory contains the installer scripts for creating distributable packages of HyperRecall.

## Windows Installer (NSIS)

The Windows installer uses NSIS (Nullsoft Scriptable Install System) to create a `.exe` installer that:

- Installs HyperRecall to `Program Files`
- Creates Start Menu shortcuts
- Creates a Desktop shortcut
- Includes all necessary runtime dependencies (DLLs)
- Includes all assets (fonts, icons, themes)
- Provides an uninstaller
- Registers the application with Windows

### Building the Installer Manually

Prerequisites:
- NSIS installed on Windows (available via Chocolatey: `choco install nsis`)
- Built HyperRecall executable in `build/bin/`
- vcpkg dependencies installed in `vcpkg/installed/x64-windows/`

To build:
```powershell
makensis /DVERSION="1.0.0" installer/windows/installer.nsi
```

This will create `HyperRecall-Setup-1.0.0.exe` in the repository root.

### Automated Build via GitHub Actions

The installer is automatically built when you:
1. Push a tag starting with `v` (e.g., `v1.0.0`)
2. Manually trigger the release workflow with a version number

The GitHub Actions workflow (`.github/workflows/release.yml`) will:
1. Build HyperRecall for Windows
2. Build the NSIS installer
3. Create a GitHub release
4. Upload the installer as a release asset

## Linux Distribution

Linux builds are distributed as:
- Tarball (`.tar.gz`) containing the binary, assets, and installation scripts
- Desktop integration via the included `install-launcher.sh` script

The tarball is created automatically by the release workflow.

## Creating a New Release

### Method 1: Git Tag (Recommended)

```bash
# Create and push a new version tag
git tag v1.0.0
git push origin v1.0.0
```

This automatically triggers the release workflow, builds for all platforms, and creates a GitHub release.

### Method 2: Manual Workflow Trigger

1. Go to Actions → Release workflow
2. Click "Run workflow"
3. Enter the version (e.g., `v1.0.0`)
4. Click "Run workflow"

## Release Artifacts

After the workflow completes, the GitHub release will include:
- `HyperRecall-Setup-{VERSION}.exe` - Windows installer
- `HyperRecall-Linux-x64.tar.gz` - Linux portable binary

## Troubleshooting

### Windows Installer Issues

If the installer fails to build:
1. Ensure NSIS is installed: `choco install nsis`
2. Verify the build directory exists: `build/bin/hyperrecall.exe`
3. Check that vcpkg dependencies are built: `vcpkg/installed/x64-windows/bin/*.dll`
4. Review the NSIS compiler output for specific errors

### Missing Dependencies

The installer includes all runtime dependencies from vcpkg. If a DLL is missing:
1. Rebuild with vcpkg: `vcpkg install raylib sqlite3 --triplet x64-windows`
2. Check the vcpkg bin directory: `vcpkg/installed/x64-windows/bin/`

## Customization

### Changing the Icon

To use a custom icon, create `assets/icons/app.ico` and uncomment the icon definitions in `installer.nsi`.

### Modifying Install Location

Edit the `InstallDir` directive in `installer.nsi`:
```nsis
InstallDir "$PROGRAMFILES64\HyperRecall"
```

### Adding More Files

Add files to the MainProgram section in `installer.nsi`:
```nsis
Section -MainProgram
    ; Add your files here
    File "path\to\file"
SectionEnd
```

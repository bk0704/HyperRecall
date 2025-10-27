# Release Guide for HyperRecall

This guide explains how to create and publish releases of HyperRecall with automated Windows installers and Linux packages.

## Quick Start

To create a new release, run one command:

**Linux/macOS:**
```bash
./create-release.sh 1.0.1
```

**Windows:**
```powershell
.\create-release.ps1 1.0.1
```

Then follow the instructions to push the tag. That's it! The rest is automatic.

## What Happens Automatically

When you push a version tag (like `v1.0.1`), GitHub Actions automatically:

1. **Builds for Windows**
   - Installs all dependencies via vcpkg
   - Compiles HyperRecall for Windows (x64)
   - Creates a Windows installer using NSIS
   - The installer includes all DLLs and assets

2. **Builds for Linux**
   - Installs all dependencies
   - Compiles HyperRecall for Linux (x64)
   - Creates a portable tarball with everything needed

3. **Creates a GitHub Release**
   - Generates release notes
   - Uploads both installers
   - Makes them available for download

## Release Artifacts

Each release includes:

### Windows Installer (`HyperRecall-Setup-X.X.X.exe`)
- Full installer with all dependencies
- Creates Start Menu and Desktop shortcuts
- Registers with Windows "Programs and Features"
- Includes uninstaller
- Users just download and double-click to install

### Linux Tarball (`HyperRecall-Linux-x64.tar.gz`)
- Portable binary package
- Includes all assets
- Includes installation helper scripts
- Users extract and run `./hyperrecall`

## Manual Release Process

If you prefer to do it manually without the helper scripts:

### Step 1: Update Version
```bash
# Edit the VERSION file
echo "1.0.1" > VERSION
git add VERSION
git commit -m "Bump version to 1.0.1"
```

### Step 2: Create Tag
```bash
git tag v1.0.1
```

### Step 3: Push
```bash
git push origin main        # or your branch
git push origin v1.0.1
```

### Step 4: Watch the Build
Go to: https://github.com/bk0704/HyperRecall/actions

The release workflow will automatically run and create the release.

## Manual Workflow Trigger

You can also trigger a release without creating a tag:

1. Go to: https://github.com/bk0704/HyperRecall/actions
2. Click on "Release" workflow
3. Click "Run workflow"
4. Enter the version (e.g., `v1.0.1`)
5. Click "Run workflow"

This is useful for re-running a failed build or creating test releases.

## Troubleshooting

### Build Fails on Windows

**Issue**: NSIS can't find files
**Solution**: Check that the file paths in `installer/windows/installer.nsi` are correct

**Issue**: Missing DLLs
**Solution**: Ensure vcpkg installed all dependencies correctly

### Build Fails on Linux

**Issue**: raylib not found
**Solution**: The workflow builds raylib from source, so this should rarely fail. Check GitHub Actions logs.

### Tag Already Exists

If you need to recreate a tag:
```bash
git tag -d v1.0.1                    # Delete locally
git push origin :refs/tags/v1.0.1   # Delete remotely
```

Then create it again.

### Wrong Version Number in Release

The version comes from the git tag. Make sure:
- Tag format is `vX.Y.Z` (e.g., `v1.0.1`)
- VERSION file matches (e.g., `1.0.1`)

## Testing Locally

### Test Windows Installer Build

Prerequisites:
- Windows with NSIS installed: `choco install nsis`
- Built HyperRecall: `build/bin/hyperrecall.exe` exists
- vcpkg dependencies installed

```powershell
makensis /DVERSION="1.0.1" installer/windows/installer.nsi
```

The installer will be created at `installer/windows/HyperRecall-Setup-1.0.1.exe`

### Test Linux Tarball Build

```bash
# After building
mkdir -p HyperRecall-Linux-x64
cp build/bin/hyperrecall HyperRecall-Linux-x64/
cp -r build/bin/assets HyperRecall-Linux-x64/
cp README.md LICENSE HyperRecall-Linux-x64/
tar -czf HyperRecall-Linux-x64.tar.gz HyperRecall-Linux-x64
```

## Customizing the Installer

### Windows Installer

Edit `installer/windows/installer.nsi` to:
- Change install location
- Add/remove files
- Customize shortcuts
- Add license page (define LICENSE_TXT)
- Add custom icon (create `assets/icons/app.ico`)

### Release Workflow

Edit `.github/workflows/release.yml` to:
- Add more platforms (macOS, ARM, etc.)
- Change build configuration
- Add signing for installers
- Customize release notes

## Best Practices

1. **Always update VERSION file** to match the tag
2. **Test locally first** if making installer changes
3. **Use semantic versioning** (MAJOR.MINOR.PATCH)
4. **Write good commit messages** - they appear in release notes
5. **Check Actions logs** if something fails

## Version Numbers

Follow semantic versioning:
- `1.0.0` - Major release (breaking changes)
- `1.1.0` - Minor release (new features)
- `1.0.1` - Patch release (bug fixes)

## Getting Help

If you encounter issues:
1. Check GitHub Actions logs for build errors
2. Review `installer/README.md` for detailed info
3. Test the installer build locally to debug
4. Open an issue with the error logs

## Files You Might Need to Edit

- `VERSION` - Current version number
- `.github/workflows/release.yml` - Release automation
- `installer/windows/installer.nsi` - Windows installer config
- `README.md` - Update release links if needed

## Summary

Creating releases is now fully automated! Just run the release script, push the tag, and let GitHub Actions do the rest. Your users get professional installers without any manual work.

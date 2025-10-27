# How to Create Release Binaries NOW

## Quick Instructions (For Repository Owner)

You are **one merge + one click** away from having downloadable .exe and .tar.gz installers.

### Step 1: Merge This PR
Merge this pull request to fix the broken workflows.

### Step 2: Trigger the Release (Choose ONE method)

#### Method A: Via GitHub Website (EASIEST - 2 clicks)
1. Go to: https://github.com/bk0704/HyperRecall/actions/workflows/release.yml
2. Click the **"Run workflow"** button (top right)
3. Enter version: `v1.0.0` (or any version you want)
4. Click the green **"Run workflow"** button
5. Wait 10-15 minutes
6. Done! Your .exe will be at: https://github.com/bk0704/HyperRecall/releases

#### Method B: Via Git Tag (Traditional)
```bash
git checkout main
git pull
git tag v1.0.0
git push origin v1.0.0
```

#### Method C: Use the Helper Script
```bash
./create-release.sh 1.0.0
git push origin main
git push origin v1.0.0
```

## What You'll Get

After triggering the release, GitHub Actions will automatically build and publish:

- **Windows Installer**: `HyperRecall-Setup-1.0.0.exe` (full installer with all dependencies)
- **Linux Tarball**: `HyperRecall-Linux-x64.tar.gz` (portable binary package)

Both will be available at: https://github.com/bk0704/HyperRecall/releases

## Why This Works Now

The previous workflows were trying to install `raylib` but the project uses `Qt6`. This PR fixed both:
- `.github/workflows/build.yml` 
- `.github/workflows/release.yml`

They now correctly install Qt6 dependencies, so the builds will succeed.

## Monitoring Progress

Watch the build here: https://github.com/bk0704/HyperRecall/actions

The build takes about 10-15 minutes. You'll see:
1. "Build Windows Release" - creates the .exe installer
2. "Build Linux Release" - creates the .tar.gz 
3. "Create GitHub Release" - publishes both files

## If It Fails

If the workflow fails, check the logs in GitHub Actions. Common issues:
- vcpkg timeout (re-run the workflow)
- Missing dependencies (check the logs for specifics)

## Summary

**NO MORE BULLSHIT.** Once this PR is merged:
1. Click "Run workflow" on GitHub
2. Wait 10-15 minutes
3. Download your .exe

That's it.

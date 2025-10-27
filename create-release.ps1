# Quick release script for HyperRecall
# Usage: .\create-release.ps1 <version>
# Example: .\create-release.ps1 1.0.1

param(
    [Parameter(Mandatory=$true)]
    [string]$Version
)

$ErrorActionPreference = "Stop"

# Validate version format (x.y.z)
if ($Version -notmatch '^\d+\.\d+\.\d+$') {
    Write-Error "Error: Version must be in format x.y.z (e.g., 1.0.1)"
    exit 1
}

$Tag = "v$Version"

Write-Host "Creating release for HyperRecall $Version" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check if tag already exists
$tagExists = git rev-parse $Tag 2>$null
if ($LASTEXITCODE -eq 0) {
    Write-Error "Error: Tag $Tag already exists!"
    Write-Host "Delete it first with: git tag -d $Tag; git push origin :refs/tags/$Tag" -ForegroundColor Yellow
    exit 1
}

# Update VERSION file
Write-Host "Updating VERSION file..." -ForegroundColor Green
Set-Content -Path "VERSION" -Value $Version -NoNewline
git add VERSION
git commit -m "Bump version to $Version" 2>$null
if ($LASTEXITCODE -ne 0) {
    Write-Host "No changes to commit" -ForegroundColor Yellow
}

# Create tag
Write-Host "Creating tag $Tag..." -ForegroundColor Green
git tag -a $Tag -m "Release version $Version"

Write-Host ""
Write-Host "Ready to push! Run these commands:" -ForegroundColor Cyan
Write-Host ""
Write-Host "  git push origin main  # or your branch name" -ForegroundColor White
Write-Host "  git push origin $Tag" -ForegroundColor White
Write-Host ""
Write-Host "This will trigger the automated release workflow." -ForegroundColor Yellow
Write-Host "Check progress at: https://github.com/bk0704/HyperRecall/actions" -ForegroundColor Yellow

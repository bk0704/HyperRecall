#!/bin/bash
# Quick release script for HyperRecall
# Usage: ./create-release.sh <version>
# Example: ./create-release.sh 1.0.1

set -e

if [ -z "$1" ]; then
    echo "Usage: $0 <version>"
    echo "Example: $0 1.0.1"
    exit 1
fi

VERSION=$1
TAG="v${VERSION}"

# Validate version format (x.y.z)
if ! [[ $VERSION =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "Error: Version must be in format x.y.z (e.g., 1.0.1)"
    exit 1
fi

echo "Creating release for HyperRecall ${VERSION}"
echo "========================================"

# Check if tag already exists
if git rev-parse "$TAG" >/dev/null 2>&1; then
    echo "Error: Tag $TAG already exists!"
    echo "Delete it first with: git tag -d $TAG && git push origin :refs/tags/$TAG"
    exit 1
fi

# Update VERSION file
echo "Updating VERSION file..."
echo "$VERSION" > VERSION
git add VERSION
git commit -m "Bump version to ${VERSION}" || echo "No changes to commit"

# Create and push tag
echo "Creating tag $TAG..."
git tag -a "$TAG" -m "Release version ${VERSION}"

echo ""
echo "Ready to push! Run these commands:"
echo ""
echo "  git push origin main  # or your branch name"
echo "  git push origin $TAG"
echo ""
echo "This will trigger the automated release workflow."
echo "Check progress at: https://github.com/bk0704/HyperRecall/actions"

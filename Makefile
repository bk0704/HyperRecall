# HyperRecall Makefile Wrapper
# Provides convenient shortcuts for common development tasks

.PHONY: all run build clean configure help install

# Default target
all: build

# One-click run target - builds and runs the application
run:
	@echo "🚀 Building and running HyperRecall..."
	@if [ ! -d "build" ]; then \
		echo "📦 Configuring build for the first time..."; \
		$(MAKE) configure; \
	fi
	@cmake --build build
	@echo "✅ Build complete! Launching..."
	@cd build/bin && ./hyperrecall

# Build the project
build:
	@if [ ! -d "build" ]; then \
		echo "📦 Build directory not found. Configuring..."; \
		$(MAKE) configure; \
	fi
	@echo "🔨 Building HyperRecall..."
	@cmake --build build

# Configure the build system
configure:
	@echo "⚙️  Configuring CMake..."
	@if command -v ninja >/dev/null 2>&1; then \
		cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release; \
	else \
		cmake -S . -B build -DCMAKE_BUILD_TYPE=Release; \
	fi

# Clean build artifacts
clean:
	@echo "🧹 Cleaning build directory..."
	@rm -rf build

# Install system dependencies (Ubuntu/Debian)
install-deps:
	@echo "📦 Installing dependencies (Ubuntu/Debian)..."
	@if command -v apt >/dev/null 2>&1; then \
		sudo apt update && sudo apt install -y build-essential cmake ninja-build pkg-config libsqlite3-dev qt6-base-dev; \
	elif command -v dnf >/dev/null 2>&1; then \
		sudo dnf install -y @development-tools cmake ninja-build pkgconf-pkg-config sqlite-devel qt6-qtbase-devel; \
	elif command -v pacman >/dev/null 2>&1; then \
		sudo pacman -Sy --needed base-devel cmake ninja sqlite qt6-base; \
	else \
		echo "❌ Unsupported package manager. Please install dependencies manually."; \
		exit 1; \
	fi

# Help target
help:
	@echo "HyperRecall Makefile Commands:"
	@echo "  make run          - Build and run HyperRecall (one command!)"
	@echo "  make build        - Build the application"
	@echo "  make configure    - Configure the build system"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make install-deps - Install system dependencies (Linux only)"
	@echo "  make help         - Show this help message"

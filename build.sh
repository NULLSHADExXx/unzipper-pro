#!/bin/bash

# Unzipper Pro 2.0 - Build Script

set -e

echo "=========================================="
echo "Unzipper Pro 2.0 - Build Script"
echo "=========================================="

# Check for required tools
if ! command -v cmake &> /dev/null; then
    echo "❌ CMake not found. Install it first."
    exit 1
fi

if ! command -v pkg-config &> /dev/null; then
    echo "❌ pkg-config not found. Install it first."
    exit 1
fi

# Detect OS
OS=$(uname -s)
echo "📦 Detected OS: $OS"

# Install dependencies based on OS
if [ "$OS" = "Linux" ]; then
    echo "📥 Installing Linux dependencies..."
    if command -v apt-get &> /dev/null; then
        sudo apt-get update
        sudo apt-get install -y build-essential cmake qt6-base-dev libarchive-dev
    elif command -v dnf &> /dev/null; then
        sudo dnf install -y gcc-c++ cmake qt6-base-devel libarchive-devel
    else
        echo "⚠️  Unsupported package manager. Please install: build-essential cmake qt6-base-dev libarchive-dev"
    fi
elif [ "$OS" = "Darwin" ]; then
    echo "📥 Installing macOS dependencies..."
    if ! command -v brew &> /dev/null; then
        echo "❌ Homebrew not found. Install from https://brew.sh"
        exit 1
    fi
    brew install cmake qt@6 libarchive
    export PKG_CONFIG_PATH="$(brew --prefix libarchive)/lib/pkgconfig:${PKG_CONFIG_PATH:-}"
fi

# Create build directory
echo "📁 Creating build directory..."
mkdir -p build
cd build

# Configure
echo "⚙️  Configuring CMake..."
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
echo "🔨 Building..."
cmake --build . --config Release -j$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)

# Success
echo ""
echo "=========================================="
echo "✅ Build successful!"
echo "=========================================="
echo ""
echo "Run the application:"
if [ "$OS" = "Darwin" ]; then
    echo "  open UnzipperPro.app"
else
    echo "  ./UnzipperPro"
fi
echo ""

# Unzipper Pro 2.0 - Installation Guide

## Quick Start

### Windows
1. Download `UnzipperPro-Setup-2.0.exe`
2. Run the installer
3. Click "Next" through the wizard
4. Click "Finish"
5. Launch from Start Menu or Desktop shortcut

### macOS
1. Download `UnzipperPro-2.0.dmg`
2. Open the DMG file
3. Drag "Unzipper Pro" to Applications folder
4. Launch from Applications or Launchpad

### Linux
```bash
# Ubuntu/Debian
sudo apt install ./unzipper-pro_2.0_amd64.deb

# Fedora/RHEL
sudo dnf install ./unzipper-pro-2.0-1.x86_64.rpm

# Or run directly
./UnzipperPro
```

---

## Building from Source

### Step 1: Install Dependencies

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    git \
    qt6-base-dev \
    libarchive-dev \
    pkg-config
```

**Fedora/RHEL:**
```bash
sudo dnf install -y \
    gcc-c++ \
    cmake \
    git \
    qt6-base-devel \
    libarchive-devel \
    pkgconfig
```

**macOS (with Homebrew):**
```bash
brew install cmake qt@6 libarchive pkg-config
```

**Windows (MSVC):**
- Download Visual Studio 2019+ from https://visualstudio.microsoft.com
- Install "Desktop development with C++"
- Download CMake from https://cmake.org
- Download Qt6 from https://www.qt.io/download-open-source

### Step 2: Clone Repository

```bash
git clone https://github.com/YOUR_USERNAME/unzipper-pro.git
cd unzipper-pro
```

### Step 3: Build

**Linux/macOS:**
```bash
chmod +x build.sh
./build.sh
```

**Windows:**
```cmd
build.bat
```

**Manual build:**
```bash
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release -j$(nproc)
```

### Step 4: Run

**Linux/macOS:**
```bash
./build/UnzipperPro
```

**Windows:**
```cmd
build\Release\UnzipperPro.exe
```

---

## Packaging for Distribution

### Create Windows Installer

```bash
# Install NSIS
choco install nsis

# Create installer
makensis unzipper.nsi
```

### Create macOS DMG

```bash
# Create DMG
hdiutil create -volname "Unzipper Pro" \
    -srcfolder build/UnzipperPro.app \
    -ov -format UDZO \
    UnzipperPro-2.0.dmg
```

### Create Linux DEB Package

```bash
# Install dependencies
sudo apt install debhelper

# Build DEB
dpkg-buildpackage -us -uc
```

### Create Linux RPM Package

```bash
# Install dependencies
sudo dnf install rpm-build

# Build RPM
rpmbuild -ba unzipper-pro.spec
```

---

## System Requirements

### Minimum
- **CPU:** Dual-core 1.5 GHz
- **RAM:** 2 GB
- **Disk:** 100 MB free space
- **OS:** Windows 7+, macOS 10.13+, Ubuntu 18.04+

### Recommended
- **CPU:** Quad-core 2.0 GHz
- **RAM:** 8 GB
- **Disk:** 1 GB SSD
- **OS:** Windows 10+, macOS 11+, Ubuntu 20.04+

### For 100GB+ Files
- **RAM:** 4 GB minimum (streaming handles rest)
- **Disk:** Enough space for extracted files
- **Network:** (if extracting from network drive)

---

## Troubleshooting

### Build Errors

**"CMake not found"**
```bash
# Ubuntu/Debian
sudo apt install cmake

# macOS
brew install cmake

# Windows
Download from https://cmake.org
```

**"Qt6 not found"**
```bash
# Ubuntu/Debian
sudo apt install qt6-base-dev

# macOS
brew install qt@6

# Windows
Download from https://www.qt.io
```

**"libarchive not found"**
```bash
# Ubuntu/Debian
sudo apt install libarchive-dev

# macOS
brew install libarchive

# Windows
Download from https://github.com/libarchive/libarchive
```

### Runtime Errors

**"Cannot open archive"**
- Verify file is not corrupted
- Check file permissions: `ls -la archive.zip`
- Try redownloading the file

**"Insufficient disk space"**
- Check available space: `df -h`
- Free up space or choose different output folder

**"Permission denied"**
- Check output folder permissions
- Run with appropriate permissions
- Linux: `chmod 755 output_folder`

**Application crashes**
- Update to latest version
- Check system logs
- Report issue with error details

---

## Uninstallation

### Windows
1. Open "Add or Remove Programs"
2. Find "Unzipper Pro"
3. Click "Uninstall"
4. Follow the wizard

### macOS
1. Open Finder
2. Go to Applications
3. Drag "Unzipper Pro" to Trash
4. Empty Trash

### Linux
```bash
# Ubuntu/Debian
sudo apt remove unzipper-pro

# Fedora/RHEL
sudo dnf remove unzipper-pro

# Or if built from source
rm -rf ~/unzipper-pro/build/UnzipperPro
```

---

## Advanced Configuration

### Environment Variables

Parallel extraction is configured in Settings (Edit → Settings → Max parallel extractions). No environment variables are used.

### Configuration Files

Settings are stored via Qt QSettings:
- **Windows:** `%APPDATA%/UnzipperPro/UnzipperPro.ini`
- **macOS:** `~/Library/Preferences/UnzipperPro.UnzipperPro.plist`
- **Linux:** `~/.config/UnzipperPro/UnzipperPro.conf`

---

## Support

Open an issue on GitHub for bug reports or feature requests.

---

**Unzipper Pro 2.0** - Professional Archive Extraction Tool

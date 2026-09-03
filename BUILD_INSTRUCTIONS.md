# Build Instructions for AprilTag Detector

This project provides comprehensive build scripts for compiling the AprilTag Detector on multiple platforms and architectures.

## Supported Platforms and Architectures

| Platform | Architectures | Notes |
|----------|--------------|-------|
| Linux | x64, arm, arm64 | Uses native or cross-compilation toolchains |
| Windows | x64, arm64 | Uses MinGW or Visual Studio for cross-compilation |
| macOS | x64, arm64 | Supports Apple Silicon (M1/M2) and Intel |

## Prerequisites

### Common Requirements
- **CMake** 3.20 or later
- **Git** for source control
- **C++20** compatible compiler

### Platform-Specific Requirements

#### Linux
- **GCC** 10+ or **Clang** 10+ (for native builds)
- **Cross-compilation toolchains** (for ARM builds):
  - ARM 32-bit: `gcc-arm-linux-gnueabihf` or `arm-linux-gnueabihf-g++`
  - ARM 64-bit: `gcc-aarch64-linux-gnu` or `aarch64-linux-gnu-g++`

**Install on Ubuntu/Debian:**
```bash
# Native build tools
sudo apt update
sudo apt install build-essential cmake git

# ARM 32-bit cross-compilation
sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf

# ARM 64-bit cross-compilation
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu
```

#### Windows
- **Visual Studio 2022** (for native Windows builds)
- **MinGW-w64** (for cross-compilation from Linux/macOS)
  - Windows x64: `x86_64-w64-mingw32-g++`
  - Windows ARM64: `aarch64-w64-mingw32-g++`

**Install MinGW-w64 on Ubuntu/Debian:**
```bash
sudo apt install mingw-w64
```

#### macOS
- **Xcode Command Line Tools**
- **Clang** (included with Xcode)

**Install Xcode tools:**
```bash
xcode-select --install
```

## Build Scripts

This project provides three build scripts for different environments:

### 1. Bash Script (`build.sh`)

**Usage:**
```bash
./build.sh [target] [arch] [config]
```

**Parameters:**
- `target`: Platform to build for (`linux`, `windows`, `macos`, `all`)
- `arch`: Architecture (`x64`, `arm`, `arm64`, `all`)
- `config`: Build configuration (`Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel`)

**Examples:**
```bash
# Build everything in Release mode
./build.sh all all Release

# Build Linux ARM64 in Debug mode
./build.sh linux arm64 Debug

# Build macOS M1/M2 (arm64) in Release mode
./build.sh macos arm64 Release

# Build Windows x64 in Release mode
./build.sh windows x64 Release

# Build Linux for both x64 and arm64
./build.sh linux all Release
```

### 2. PowerShell Script (`build.ps1`)

For Windows users or cross-compilation from Windows.

**Usage:**
```powershell
.\build.ps1 -Target <target> -Arch <arch> -Config <config>
```

**Parameters:**
- `-Target`: Platform to build for (`Linux`, `Windows`, `MacOS`, `All`)
- `-Arch`: Architecture (`x64`, `arm`, `arm64`, `All`)
- `-Config`: Build configuration (`Debug`, `Release`, `RelWithDebInfo`, `MinSizeRel`)

**Examples:**
```powershell
# Build everything in Release mode
.\build.ps1 -Target All -Arch All -Config Release

# Build Windows x64 in Release mode (native)
.\build.ps1 -Target Windows -Arch x64 -Config Release

# Build Linux ARM64 in Debug mode (cross-compilation)
.\build.ps1 -Target Linux -Arch arm64 -Config Debug
```

### 3. Python Script (`build.py`)

Cross-platform build script that works on any system with Python 3.6+.

**Usage:**
```bash
python build.py [options]
```

**Options:**
```
-t, --target    Target platform (linux, windows, macos, all)
-a, --arch      Architecture (x64, arm, arm64, all)
-c, --config    Build configuration (Debug, Release, RelWithDebInfo, MinSizeRel)
-j, --jobs      Number of parallel jobs (default: CPU count)
--clean         Clean build directory before building
-h, --help      Show help message
```

**Examples:**
```bash
# Build everything in Release mode
python build.py -t all -a all -c Release

# Build Linux ARM64 with 8 parallel jobs
python build.py -t linux -a arm64 -c Release -j 8

# Clean and build macOS for both architectures
python build.py -t macos -a all -c Release --clean

# Build Windows x64 in Debug mode
python build.py -t windows -a x64 -c Debug
```

## Build Output

All builds are created in the `build/` directory with the following structure:

```
build/
├── linux_x64_Release/
│   ├── install/
│   └── ...
├── linux_arm64_Release/
│   ├── install/
│   └── ...
├── windows_x64_Release/
│   ├── install/
│   └── ...
├── windows_arm64_Release/
│   ├── install/
│   └── ...
├── macos_x64_Release/
│   ├── install/
│   └── ...
└── macos_arm64_Release/
    ├── install/
    └── ...
```

Each build directory contains:
- `install/` - Installed binaries and libraries
- CMake build files and objects

## Common Build Scenarios

### Scenario 1: Native Build on Linux x64

```bash
# Using bash script
./build.sh linux x64 Release

# Using Python script
python build.py -t linux -a x64 -c Release
```

### Scenario 2: Cross-Compile for Raspberry Pi (ARM 32-bit)

```bash
# Install ARM toolchain first
sudo apt install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf

# Build for ARM 32-bit
./build.sh linux arm Release
```

### Scenario 3: Cross-Compile for Raspberry Pi 4/5 (ARM 64-bit)

```bash
# Install ARM64 toolchain first
sudo apt install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# Build for ARM 64-bit
./build.sh linux arm64 Release
```

### Scenario 4: Cross-Compile for Windows from Linux

```bash
# Install MinGW-w64 first
sudo apt install mingw-w64

# Build Windows x64
./build.sh windows x64 Release

# Build Windows ARM64
./build.sh windows arm64 Release
```

### Scenario 5: Native Build on macOS (Apple Silicon)

```bash
# Build for Apple Silicon (M1/M2)
./build.sh macos arm64 Release

# Build for Intel Mac
./build.sh macos x64 Release

# Build both architectures (universal binary)
./build.sh macos all Release
```

### Scenario 6: Cross-Compile for macOS from Linux

**Note:** Cross-compiling for macOS from Linux requires the osxcross toolchain.

```bash
# Install osxcross (follow instructions at https://github.com/tpoechtrager/osxcross)

# Build for macOS ARM64
export CC=o64-clang
export CXX=o64-clang++
./build.sh macos arm64 Release
```

## Troubleshooting

### Common Issues

1. **Missing Toolchain**
   - Error: `arm-linux-gnueabihf-g++ not found`
   - Solution: Install the appropriate toolchain (see Prerequisites section)

2. **CMake Not Found**
   - Error: `cmake: command not found`
   - Solution: Install CMake or add it to your PATH

3. **Compiler Not Found**
   - Error: `CMake configuration failed: No C++ compiler found`
   - Solution: Install a C++ compiler and ensure it's in your PATH

4. **Cross-Compilation Issues**
   - Error: `Incompatible target architecture`
   - Solution: Verify your toolchain supports the target architecture

5. **Permission Issues**
   - Error: `Permission denied` when running scripts
   - Solution: Make scripts executable: `chmod +x build.sh build.py`

### Verbose Output

For more detailed error messages, run CMake with verbose output:

```bash
# In the build directory
cmake --build . --verbose
```

## Cleaning Builds

To clean all build artifacts:

```bash
# Remove entire build directory
rm -rf build/

# Or use Python script with --clean flag
python build.py --clean
```

## Customizing the Build

### CMake Options

You can pass additional CMake options by modifying the build scripts or setting environment variables:

```bash
# Build with custom options
export CMAKE_OPTIONS="-DCustomOption=ON"
./build.sh linux x64 Release
```

### Custom Toolchains

To use a custom toolchain, set the appropriate environment variables before running the build:

```bash
# Use custom compiler
export CC=/path/to/custom-gcc
export CXX=/path/to/custom-g++
./build.sh linux x64 Release
```

## Performance Tips

1. **Parallel Builds**: Use all available CPU cores for faster builds
   ```bash
   # Bash script uses CPU count automatically
   # Python script: -j flag
   python build.py -t all -a all -c Release -j $(nproc)
   ```

2. **Incremental Builds**: Only rebuild changed files
   ```bash
   # CMake automatically handles incremental builds
   ./build.sh linux x64 Release
   ```

3. **CCache**: Use ccache for faster rebuilds
   ```bash
   # Install ccache
   sudo apt install ccache
   
   # Build with ccache
   export CC="ccache gcc"
   export CXX="ccache g++"
   ./build.sh linux x64 Release
   ```

## Continuous Integration

For CI/CD pipelines, use the build scripts with specific targets:

```yaml
# GitHub Actions example
- name: Build Linux x64
  run: ./build.sh linux x64 Release

- name: Build Linux ARM64
  run: ./build.sh linux arm64 Release

- name: Build Windows x64
  run: ./build.sh windows x64 Release

- name: Build macOS
  run: ./build.sh macos all Release
```

## Notes

- The build scripts automatically detect the host system and configure builds accordingly
- Cross-compilation requires appropriate toolchains to be installed
- Some platform/architecture combinations may not be supported (e.g., Windows ARM 32-bit)
- The scripts create separate build directories for each target/architecture/configuration combination
- Build artifacts are installed in the `install/` subdirectory of each build directory

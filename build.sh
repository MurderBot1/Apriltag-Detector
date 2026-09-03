#!/bin/bash

# AprilTag Detector - Multi-Platform Build Script
# Builds for: Linux (arm/x64), Windows (arm/x64), macOS (M1/M2 + Intel)
# Usage: ./build.sh [target] [arch] [config]
#   target: linux, windows, macos, all (default: all)
#   arch:   x64, arm, arm64, all (default: all)
#   config: Debug, Release (default: Release)
#
# Examples:
#   ./build.sh all all Release      # Build everything in Release
#   ./build.sh linux arm64 Debug     # Build Linux ARM64 in Debug
#   ./build.sh macos arm64 Release   # Build macOS M1/M2 in Release
#   ./build.sh windows x64 Release   # Build Windows x64 in Release

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$SCRIPT_DIR"
BUILD_DIR="$PROJECT_DIR/build"

# Default values
TARGETS=${1:-all}
ARCHS=${2:-all}
CONFIG=${3:-Release}

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if a command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to get the number of CPU cores
get_cpu_cores() {
    if [[ "$OSTYPE" == "darwin"* ]]; then
        sysctl -n hw.ncpu
    else
        nproc 2>/dev/null || echo 4
    fi
}

CORES=$(get_cpu_cores)

# Detect host system
HOST_OS=""
HOST_ARCH=""

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    HOST_OS="linux"
    HOST_ARCH=$(uname -m)
elif [[ "$OSTYPE" == "darwin"* ]]; then
    HOST_OS="macos"
    HOST_ARCH=$(uname -m)
elif [[ "$OSTYPE" == "cygwin" || "$OSTYPE" == "msys" || "$OSTYPE" == "win32" ]]; then
    HOST_OS="windows"
    HOST_ARCH=$(uname -m)
else
    log_error "Unable to detect host OS: $OSTYPE"
    exit 1
fi

log_info "Host System: $HOST_OS ($HOST_ARCH)"
log_info "Build Configuration: $CONFIG"
log_info "Parallel Jobs: $CORES"

# Function to build for a specific target and architecture
build_target() {
    local target="$1"
    local arch="$2"
    local config="$3"
    local build_name="${target}_${arch}_${config}"
    local build_path="$BUILD_DIR/$build_name"
    
    log_info "Building $build_name..."
    
    # Create build directory
    mkdir -p "$build_path"
    
    # Common CMake arguments
    local cmake_args=(
        -DCMAKE_BUILD_TYPE="$config"
        -DCMAKE_INSTALL_PREFIX="$build_path/install"
        -DBUILD_TESTS=OFF
        -DBUILD_WEB_UI=ON
    )
    
    # Target-specific setup
    if [[ "$target" == "linux" ]]; then
        cmake_args+=(
            -DPLATFORM=linux
        )
        
        if [[ "$arch" == "arm" ]]; then
            cmake_args+=(
                -DCMAKE_SYSTEM_PROCESSOR=arm
                -DCMAKE_CXX_FLAGS="-march=armv7-a -mfpu=neon -mfloat-abi=hard"
                -DCMAKE_C_FLAGS="-march=armv7-a -mfpu=neon -mfloat-abi=hard"
            )
            # Try to find arm-linux-gnueabihf toolchain
            if command_exists arm-linux-gnueabihf-g++; then
                cmake_args+=(
                    -DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc
                    -DCMAKE_CXX_COMPILER=arm-linux-gnueabihf-g++
                )
                log_info "Using arm-linux-gnueabihf toolchain for ARM"
            else
                log_warning "arm-linux-gnueabihf toolchain not found, using native compiler with ARM flags"
            fi
            
        elif [[ "$arch" == "arm64" ]]; then
            cmake_args+=(
                -DCMAKE_SYSTEM_PROCESSOR=aarch64
                -DCMAKE_CXX_FLAGS="-march=armv8-a"
                -DCMAKE_C_FLAGS="-march=armv8-a"
            )
            # Try to find aarch64 toolchain
            if command_exists aarch64-linux-gnu-g++; then
                cmake_args+=(
                    -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc
                    -DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++
                )
                log_info "Using aarch64-linux-gnu toolchain for ARM64"
            else
                log_warning "aarch64-linux-gnu toolchain not found, using native compiler with ARM64 flags"
            fi
            
        elif [[ "$arch" == "x64" ]]; then
            cmake_args+=(
                -DCMAKE_SYSTEM_PROCESSOR=x86_64
                -DCMAKE_CXX_FLAGS="-march=x86-64"
                -DCMAKE_C_FLAGS="-march=x86-64"
            )
        fi
        
    elif [[ "$target" == "windows" ]]; then
        cmake_args+=(
            -DPLATFORM=windows
        )
        
        # For Windows cross-compilation, we need MinGW or similar
        if [[ "$HOST_OS" != "windows" ]]; then
            if [[ "$arch" == "x64" ]]; then
                if command_exists x86_64-w64-mingw32-g++; then
                    cmake_args+=(
                        -DCMAKE_SYSTEM_NAME=Windows
                        -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc
                        -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++
                        -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres
                    )
                    log_info "Using x86_64-w64-mingw32 toolchain for Windows x64"
                else
                    log_error "x86_64-w64-mingw32 toolchain not found for Windows x64 cross-compilation"
                    return 1
                fi
                
            elif [[ "$arch" == "arm" || "$arch" == "arm64" ]]; then
                if command_exists aarch64-w64-mingw32-g++; then
                    cmake_args+=(
                        -DCMAKE_SYSTEM_NAME=Windows
                        -DCMAKE_C_COMPILER=aarch64-w64-mingw32-gcc
                        -DCMAKE_CXX_COMPILER=aarch64-w64-mingw32-g++
                        -DCMAKE_RC_COMPILER=aarch64-w64-mingw32-windres
                    )
                    log_info "Using aarch64-w64-mingw32 toolchain for Windows ARM64"
                else
                    log_error "aarch64-w64-mingw32 toolchain not found for Windows ARM64 cross-compilation"
                    return 1
                fi
            fi
        else
            # Native Windows build
            if [[ "$arch" == "arm64" ]]; then
                cmake_args+=(
                    -DCMAKE_SYSTEM_PROCESSOR=ARM64
                    -DCMAKE_GENERATOR_PLATFORM=ARM64
                )
            elif [[ "$arch" == "x64" ]]; then
                cmake_args+=(
                    -DCMAKE_SYSTEM_PROCESSOR=x64
                    -DCMAKE_GENERATOR_PLATFORM=x64
                )
            fi
            
            # Use Visual Studio generator if available
            if command_exists cmake && cmake --help | grep -q "Visual Studio"; then
                cmake_args+=(-G "Visual Studio 17 2022")
                log_info "Using Visual Studio 17 2022 generator"
            fi
        fi
        
    elif [[ "$target" == "macos" ]]; then
        cmake_args+=(
            -DPLATFORM=macos
        )
        
        if [[ "$arch" == "arm64" ]]; then
            # For M1/M2 Macs
            cmake_args+=(
                -DCMAKE_SYSTEM_PROCESSOR=arm64
                -DCMAKE_OSX_ARCHITECTURES=arm64
                -DCMAKE_APPLE_SILICON_PROCESSOR=arm64
            )
            
            # If building on Intel Mac for ARM64, use cross-compilation
            if [[ "$HOST_ARCH" != "arm64" && "$HOST_ARCH" != "aarch64" ]]; then
                if command_exists clang++ && clang++ -v 2>&1 | grep -q "Apple"; then
                    cmake_args+=(
                        -DCMAKE_C_COMPILER=clang
                        -DCMAKE_CXX_COMPILER=clang++
                        -DCMAKE_CXX_FLAGS="-arch arm64"
                        -DCMAKE_C_FLAGS="-arch arm64"
                        -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
                    )
                    log_info "Cross-compiling for macOS ARM64 (Apple Silicon)"
                else
                    log_error "clang compiler not found for macOS ARM64 cross-compilation"
                    return 1
                fi
            fi
            
        elif [[ "$arch" == "x64" ]]; then
            cmake_args+=(
                -DCMAKE_SYSTEM_PROCESSOR=x86_64
                -DCMAKE_OSX_ARCHITECTURES=x86_64
            )
            
            # If building on ARM Mac for x64, use cross-compilation
            if [[ "$HOST_ARCH" == "arm64" || "$HOST_ARCH" == "aarch64" ]]; then
                if command_exists clang++ && clang++ -v 2>&1 | grep -q "Apple"; then
                    cmake_args+=(
                        -DCMAKE_C_COMPILER=clang
                        -DCMAKE_CXX_COMPILER=clang++
                        -DCMAKE_CXX_FLAGS="-arch x86_64"
                        -DCMAKE_C_FLAGS="-arch x86_64"
                        -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15
                    )
                    log_info "Cross-compiling for macOS x64 (Intel)"
                else
                    log_error "clang compiler not found for macOS x64 cross-compilation"
                    return 1
                fi
            fi
        fi
    fi
    
    # Run CMake configuration
    log_info "Configuring CMake for $build_name..."
    cd "$build_path"
    
    if ! cmake "$PROJECT_DIR" "${cmake_args[@]}"; then
        log_error "CMake configuration failed for $build_name"
        cd "$PROJECT_DIR"
        return 1
    fi
    
    # Build the project
    log_info "Building $build_name with $CORES parallel jobs..."
    if ! cmake --build . --config "$config" --parallel "$CORES"; then
        log_error "Build failed for $build_name"
        cd "$PROJECT_DIR"
        return 1
    fi
    
    # Install (optional)
    log_info "Installing $build_name..."
    if ! cmake --install . --config "$config"; then
        log_warning "Install step failed for $build_name (continuing...)"
    fi
    
    cd "$PROJECT_DIR"
    log_success "Successfully built $build_name"
    return 0
}

# Function to validate target
validate_target() {
    local target="$1"
    case "$target" in
        linux|windows|macos) echo "$target" ;;
        all) echo "$target" ;;
        *) 
            log_error "Invalid target: $target. Must be one of: linux, windows, macos, all"
            exit 1
            ;;
    esac
}

# Function to validate architecture
validate_arch() {
    local arch="$1"
    case "$arch" in
        x64|arm|arm64) echo "$arch" ;;
        all) echo "$arch" ;;
        *) 
            log_error "Invalid architecture: $arch. Must be one of: x64, arm, arm64, all"
            exit 1
            ;;
    esac
}

# Function to validate configuration
validate_config() {
    local config="$1"
    case "$config" in
        Debug|Release|RelWithDebInfo|MinSizeRel) echo "$config" ;;
        *) 
            log_error "Invalid configuration: $config. Must be one of: Debug, Release, RelWithDebInfo, MinSizeRel"
            exit 1
            ;;
    esac
}

# Validate inputs
TARGETS=$(validate_target "$TARGETS")
ARCHS=$(validate_arch "$ARCHS")
CONFIG=$(validate_config "$CONFIG")

# Determine which targets to build
if [[ "$TARGETS" == "all" ]]; then
    TARGET_LIST=("linux" "windows" "macos")
else
    TARGET_LIST=("$TARGETS")
fi

# Determine which architectures to build
if [[ "$ARCHS" == "all" ]]; then
    ARCH_LIST=("x64" "arm64" "arm")
else
    ARCH_LIST=("$ARCHS")
fi

# Create output directory
mkdir -p "$BUILD_DIR"

log_info "Starting build process..."
log_info "Targets: ${TARGET_LIST[*]}"
log_info "Architectures: ${ARCH_LIST[*]}"
log_info "Configuration: $CONFIG"

# Track build status
BUILD_COUNT=0
SUCCESS_COUNT=0
FAILURE_COUNT=0

# Build all combinations
for target in "${TARGET_LIST[@]}"; do
    for arch in "${ARCH_LIST[@]}"; do
        # Skip unsupported combinations
        if [[ "$target" == "windows" && "$arch" == "arm" ]]; then
            log_warning "Skipping Windows ARM (32-bit) - not commonly supported"
            continue
        fi
        
        if [[ "$target" == "macos" && "$arch" == "arm" ]]; then
            log_warning "Skipping macOS ARM (32-bit) - macOS only supports x64 and arm64"
            continue
        fi
        
        BUILD_COUNT=$((BUILD_COUNT + 1))
        
        if build_target "$target" "$arch" "$CONFIG"; then
            SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
        else
            FAILURE_COUNT=$((FAILURE_COUNT + 1))
        fi
    done
done

# Print summary
echo ""
log_info "Build Summary:"
log_info "  Total builds attempted: $BUILD_COUNT"
log_success "  Successful: $SUCCESS_COUNT"
if [[ $FAILURE_COUNT -gt 0 ]]; then
    log_error "  Failed: $FAILURE_COUNT"
fi

if [[ $FAILURE_COUNT -gt 0 ]]; then
    exit 1
fi

exit 0

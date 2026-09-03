# AprilTag Detector - Multi-Platform Build Script for Windows
# Builds for: Linux (arm/x64), Windows (arm/x64), macOS (M1/M2 + Intel)
# Usage: .\build.ps1 [-Target <target>] [-Arch <arch>] [-Config <config>]
#   Target: Linux, Windows, MacOS, All (default: All)
#   Arch:   x64, arm, arm64, All (default: All)
#   Config: Debug, Release (default: Release)
#
# Examples:
#   .\build.ps1 -Target All -Arch All -Config Release      # Build everything in Release
#   .\build.ps1 -Target Linux -Arch arm64 -Config Debug     # Build Linux ARM64 in Debug
#   .\build.ps1 -Target MacOS -Arch arm64 -Config Release   # Build macOS M1/M2 in Release
#   .\build.ps1 -Target Windows -Arch x64 -Config Release   # Build Windows x64 in Release

param(
    [string]$Target = "All",
    [string]$Arch = "All",
    [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

# Get script directory
$ScriptDir = $PSScriptRoot
$ProjectDir = $ScriptDir
$BuildDir = Join-Path -Path $ProjectDir -ChildPath "build"

# ANSI color codes for PowerShell
$RED = "`e[0;31m"
$GREEN = "`e[0;32m"
$YELLOW = "`e[1;33m"
$BLUE = "`e[0;34m"
$NC = "`e[0m"

# Function to write colored output
function Write-Info {
    param([string]$Message)
    Write-Host "$BLUE[INFO]$NC $Message"
}

function Write-Success {
    param([string]$Message)
    Write-Host "$GREEN[SUCCESS]$NC $Message"
}

function Write-Warning {
    param([string]$Message)
    Write-Host "$YELLOW[WARNING]$NC $Message"
}

function Write-Error {
    param([string]$Message)
    Write-Host "$RED[ERROR]$NC $Message"
}

# Function to check if a command exists
function Test-Command {
    param([string]$Command)
    return (Get-Command -Name $Command -ErrorAction SilentlyContinue) -ne $null
}

# Function to get the number of CPU cores
function Get-CpuCores {
    return [System.Environment]::ProcessorCount
}

$Cores = Get-CpuCores

# Detect host system
$HostOS = ""
$HostArch = ""

if ($IsWindows) {
    $HostOS = "windows"
    $HostArch = $env:PROCESSOR_ARCHITECTURE
} elseif ($IsLinux) {
    $HostOS = "linux"
    $HostArch = (uname -m)
} elseif ($IsMacOS) {
    $HostOS = "macos"
    $HostArch = (uname -m)
} else {
    Write-Error "Unable to detect host OS"
    exit 1
}

Write-Info "Host System: $HostOS ($HostArch)"
Write-Info "Build Configuration: $Config"
Write-Info "Parallel Jobs: $Cores"

# Function to build for a specific target and architecture
function Build-Target {
    param(
        [string]$Target,
        [string]$Arch,
        [string]$Config
    )
    
    $BuildName = "${Target}_${Arch}_${Config}"
    $BuildPath = Join-Path -Path $BuildDir -ChildPath $BuildName
    
    Write-Info "Building $BuildName..."
    
    # Create build directory
    if (-not (Test-Path -Path $BuildPath)) {
        New-Item -ItemType Directory -Path $BuildPath -Force | Out-Null
    }
    
    # Common CMake arguments
    $CMakeArgs = @(
        "-DCMAKE_BUILD_TYPE=$Config",
        "-DCMAKE_INSTALL_PREFIX=$BuildPath\install",
        "-DBUILD_TESTS=OFF",
        "-DBUILD_WEB_UI=ON"
    )
    
    # Target-specific setup
    if ($Target -eq "linux") {
        $CMakeArgs += "-DPLATFORM=linux"
        
        if ($Arch -eq "arm") {
            $CMakeArgs += @(
                "-DCMAKE_SYSTEM_PROCESSOR=arm",
                "-DCMAKE_CXX_FLAGS=-march=armv7-a -mfpu=neon -mfloat-abi=hard",
                "-DCMAKE_C_FLAGS=-march=armv7-a -mfpu=neon -mfloat-abi=hard"
            )
            # Try to find arm-linux-gnueabihf toolchain
            if (Test-Command arm-linux-gnueabihf-g++) {
                $CMakeArgs += @(
                    "-DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc",
                    "-DCMAKE_CXX_COMPILER=arm-linux-gnueabihf-g++"
                )
                Write-Info "Using arm-linux-gnueabihf toolchain for ARM"
            } else {
                Write-Warning "arm-linux-gnueabihf toolchain not found, using native compiler with ARM flags"
            }
            
        } elseif ($Arch -eq "arm64") {
            $CMakeArgs += @(
                "-DCMAKE_SYSTEM_PROCESSOR=aarch64",
                "-DCMAKE_CXX_FLAGS=-march=armv8-a",
                "-DCMAKE_C_FLAGS=-march=armv8-a"
            )
            # Try to find aarch64 toolchain
            if (Test-Command aarch64-linux-gnu-g++) {
                $CMakeArgs += @(
                    "-DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc",
                    "-DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++"
                )
                Write-Info "Using aarch64-linux-gnu toolchain for ARM64"
            } else {
                Write-Warning "aarch64-linux-gnu toolchain not found, using native compiler with ARM64 flags"
            }
            
        } elseif ($Arch -eq "x64") {
            $CMakeArgs += @(
                "-DCMAKE_SYSTEM_PROCESSOR=x86_64",
                "-DCMAKE_CXX_FLAGS=-march=x86-64",
                "-DCMAKE_C_FLAGS=-march=x86-64"
            )
        }
        
    } elseif ($Target -eq "windows") {
        $CMakeArgs += "-DPLATFORM=windows"
        
        # For Windows native build
        if ($HostOS -eq "windows") {
            if ($Arch -eq "arm64") {
                $CMakeArgs += @(
                    "-DCMAKE_SYSTEM_PROCESSOR=ARM64",
                    "-DCMAKE_GENERATOR_PLATFORM=ARM64"
                )
            } elseif ($Arch -eq "x64") {
                $CMakeArgs += @(
                    "-DCMAKE_SYSTEM_PROCESSOR=x64",
                    "-DCMAKE_GENERATOR_PLATFORM=x64"
                )
            }
            
            # Use Visual Studio generator
            $Generators = & cmake --help 2>$null | Select-String -Pattern "Visual Studio"
            if ($Generators) {
                $LatestVS = $Generators | Select-Object -First 1 | ForEach-Object { $_.Line }
                $CMakeArgs += "-G `$LatestVS"
                Write-Info "Using generator: $LatestVS"
            }
        } else {
            # Cross-compilation from non-Windows
            if ($Arch -eq "x64") {
                if (Test-Command x86_64-w64-mingw32-g++) {
                    $CMakeArgs += @(
                        "-DCMAKE_SYSTEM_NAME=Windows",
                        "-DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc",
                        "-DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++",
                        "-DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres"
                    )
                    Write-Info "Using x86_64-w64-mingw32 toolchain for Windows x64"
                } else {
                    Write-Error "x86_64-w64-mingw32 toolchain not found for Windows x64 cross-compilation"
                    return $false
                }
                
            } elseif ($Arch -eq "arm64") {
                if (Test-Command aarch64-w64-mingw32-g++) {
                    $CMakeArgs += @(
                        "-DCMAKE_SYSTEM_NAME=Windows",
                        "-DCMAKE_C_COMPILER=aarch64-w64-mingw32-gcc",
                        "-DCMAKE_CXX_COMPILER=aarch64-w64-mingw32-g++",
                        "-DCMAKE_RC_COMPILER=aarch64-w64-mingw32-windres"
                    )
                    Write-Info "Using aarch64-w64-mingw32 toolchain for Windows ARM64"
                } else {
                    Write-Error "aarch64-w64-mingw32 toolchain not found for Windows ARM64 cross-compilation"
                    return $false
                }
            }
        }
        
    } elseif ($Target -eq "macos") {
        $CMakeArgs += "-DPLATFORM=macos"
        
        if ($Arch -eq "arm64") {
            $CMakeArgs += @(
                "-DCMAKE_SYSTEM_PROCESSOR=arm64",
                "-DCMAKE_OSX_ARCHITECTURES=arm64",
                "-DCMAKE_APPLE_SILICON_PROCESSOR=arm64"
            )
            
            # If building on Intel Mac for ARM64
            if ($HostOS -eq "macos" -and $HostArch -ne "arm64" -and $HostArch -ne "aarch64") {
                if (Test-Command clang++) {
                    $CMakeArgs += @(
                        "-DCMAKE_C_COMPILER=clang",
                        "-DCMAKE_CXX_COMPILER=clang++",
                        "-DCMAKE_CXX_FLAGS=-arch arm64",
                        "-DCMAKE_C_FLAGS=-arch arm64",
                        "-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0"
                    )
                    Write-Info "Cross-compiling for macOS ARM64 (Apple Silicon)"
                } else {
                    Write-Error "clang compiler not found for macOS ARM64 cross-compilation"
                    return $false
                }
            }
            
        } elseif ($Arch -eq "x64") {
            $CMakeArgs += @(
                "-DCMAKE_SYSTEM_PROCESSOR=x86_64",
                "-DCMAKE_OSX_ARCHITECTURES=x86_64"
            )
            
            # If building on ARM Mac for x64
            if ($HostOS -eq "macos" -and ($HostArch -eq "arm64" -or $HostArch -eq "aarch64")) {
                if (Test-Command clang++) {
                    $CMakeArgs += @(
                        "-DCMAKE_C_COMPILER=clang",
                        "-DCMAKE_CXX_COMPILER=clang++",
                        "-DCMAKE_CXX_FLAGS=-arch x86_64",
                        "-DCMAKE_C_FLAGS=-arch x86_64",
                        "-DCMAKE_OSX_DEPLOYMENT_TARGET=10.15"
                    )
                    Write-Info "Cross-compiling for macOS x64 (Intel)"
                } else {
                    Write-Error "clang compiler not found for macOS x64 cross-compilation"
                    return $false
                }
            }
        }
    }
    
    # Run CMake configuration
    Write-Info "Configuring CMake for $BuildName..."
    Push-Location -Path $BuildPath
    
    try {
        & cmake $ProjectDir $CMakeArgs
        if ($LASTEXITCODE -ne 0) {
            throw "CMake configuration failed"
        }
    } catch {
        Write-Error "CMake configuration failed for $BuildName: $_"
        Pop-Location
        return $false
    }
    
    # Build the project
    Write-Info "Building $BuildName with $Cores parallel jobs..."
    try {
        & cmake --build . --config $Config --parallel $Cores
        if ($LASTEXITCODE -ne 0) {
            throw "Build failed"
        }
    } catch {
        Write-Error "Build failed for $BuildName: $_"
        Pop-Location
        return $false
    }
    
    # Install (optional)
    Write-Info "Installing $BuildName..."
    try {
        & cmake --install . --config $Config
        if ($LASTEXITCODE -ne 0) {
            Write-Warning "Install step failed for $BuildName (continuing...)"
        }
    } catch {
        Write-Warning "Install step failed for $BuildName: $_"
    }
    
    Pop-Location
    Write-Success "Successfully built $BuildName"
    return $true
}

# Function to validate target
function Validate-Target {
    param([string]$Target)
    $ValidTargets = @("linux", "windows", "macos", "all")
    if ($ValidTargets -contains $Target.ToLower()) {
        return $Target.ToLower()
    } else {
        Write-Error "Invalid target: $Target. Must be one of: $($ValidTargets -join ', ')"
        exit 1
    }
}

# Function to validate architecture
function Validate-Arch {
    param([string]$Arch)
    $ValidArchs = @("x64", "arm", "arm64", "all")
    if ($ValidArchs -contains $Arch.ToLower()) {
        return $Arch.ToLower()
    } else {
        Write-Error "Invalid architecture: $Arch. Must be one of: $($ValidArchs -join ', ')"
        exit 1
    }
}

# Function to validate configuration
function Validate-Config {
    param([string]$Config)
    $ValidConfigs = @("Debug", "Release", "RelWithDebInfo", "MinSizeRel")
    if ($ValidConfigs -contains $Config) {
        return $Config
    } else {
        Write-Error "Invalid configuration: $Config. Must be one of: $($ValidConfigs -join ', ')"
        exit 1
    }
}

# Validate inputs
$Target = Validate-Target -Target $Target
$Arch = Validate-Arch -Arch $Arch
$Config = Validate-Config -Config $Config

# Determine which targets to build
if ($Target -eq "all") {
    $TargetList = @("linux", "windows", "macos")
} else {
    $TargetList = @($Target)
}

# Determine which architectures to build
if ($Arch -eq "all") {
    $ArchList = @("x64", "arm64", "arm")
} else {
    $ArchList = @($Arch)
}

# Create output directory
if (-not (Test-Path -Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir -Force | Out-Null
}

Write-Info "Starting build process..."
Write-Info "Targets: $($TargetList -join ', ')"
Write-Info "Architectures: $($ArchList -join ', ')"
Write-Info "Configuration: $Config"

# Track build status
$BuildCount = 0
$SuccessCount = 0
$FailureCount = 0

# Build all combinations
foreach ($TargetItem in $TargetList) {
    foreach ($ArchItem in $ArchList) {
        # Skip unsupported combinations
        if ($TargetItem -eq "windows" -and $ArchItem -eq "arm") {
            Write-Warning "Skipping Windows ARM (32-bit) - not commonly supported"
            continue
        }
        
        if ($TargetItem -eq "macos" -and $ArchItem -eq "arm") {
            Write-Warning "Skipping macOS ARM (32-bit) - macOS only supports x64 and arm64"
            continue
        }
        
        $BuildCount++
        
        if (Build-Target -Target $TargetItem -Arch $ArchItem -Config $Config) {
            $SuccessCount++
        } else {
            $FailureCount++
        }
    }
}

# Print summary
Write-Host "`n"
Write-Info "Build Summary:"
Write-Info "  Total builds attempted: $BuildCount"
Write-Success "  Successful: $SuccessCount"
if ($FailureCount -gt 0) {
    Write-Error "  Failed: $FailureCount"
}

if ($FailureCount -gt 0) {
    exit 1
}

exit 0

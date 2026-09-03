#!/usr/bin/env python3
"""
AprilTag Detector - Multi-Platform Build Script
Builds for: Linux (arm/x64), Windows (arm/x64), macOS (M1/M2 + Intel)

Usage:
    python build.py [options]

Options:
    --target, -t    Target platform: linux, windows, macos, all (default: all)
    --arch, -a      Architecture: x64, arm, arm64, all (default: all)
    --config, -c    Build configuration: Debug, Release, RelWithDebInfo, MinSizeRel (default: Release)
    --jobs, -j      Number of parallel jobs (default: CPU count)
    --clean         Clean build directory before building
    --help, -h      Show this help message

Examples:
    python build.py -t all -a all -c Release      # Build everything in Release
    python build.py -t linux -a arm64 -c Debug     # Build Linux ARM64 in Debug
    python build.py -t macos -a arm64 -c Release   # Build macOS M1/M2 in Release
    python build.py -t windows -a x64 -c Release   # Build Windows x64 in Release
"""

import argparse
import os
import platform
import subprocess
import sys
from pathlib import Path
from typing import List, Optional, Tuple
import shutil


# ANSI color codes
class Colors:
    RED = "\033[0;31m"
    GREEN = "\033[0;32m"
    YELLOW = "\033[1;33m"
    BLUE = "\033[0;34m"
    NC = "\033[0m"


class BuildLogger:
    @staticmethod
    def info(message: str) -> None:
        print(f"{Colors.BLUE}[INFO]{Colors.NC} {message}")
    
    @staticmethod
    def success(message: str) -> None:
        print(f"{Colors.GREEN}[SUCCESS]{Colors.NC} {message}")
    
    @staticmethod
    def warning(message: str) -> None:
        print(f"{Colors.YELLOW}[WARNING]{Colors.NC} {message}")
    
    @staticmethod
    def error(message: str) -> None:
        print(f"{Colors.RED}[ERROR]{Colors.NC} {message}")


class BuildConfig:
    def __init__(self, target: str, arch: str, config: str):
        self.target = target.lower()
        self.arch = arch.lower()
        self.config = config
        self.build_name = f"{self.target}_{self.arch}_{self.config}"
        self.build_path = None  # Will be set later


class BuildSystem:
    def __init__(self, project_dir: Path, build_dir: Path, jobs: int):
        self.project_dir = project_dir
        self.build_dir = build_dir
        self.jobs = jobs
        self.host_os = self._detect_host_os()
        self.host_arch = self._detect_host_arch()
    
    def _detect_host_os(self) -> str:
        system = platform.system().lower()
        if system == "linux":
            return "linux"
        elif system == "darwin":
            return "macos"
        elif system == "windows":
            return "windows"
        else:
            BuildLogger.error(f"Unable to detect host OS: {system}")
            sys.exit(1)
    
    def _detect_host_arch(self) -> str:
        machine = platform.machine().lower()
        if machine in ["x86_64", "amd64"]:
            return "x64"
        elif machine in ["aarch64", "arm64"]:
            return "arm64"
        elif machine.startswith("arm"):
            return "arm"
        else:
            return machine
    
    def _command_exists(self, command: str) -> bool:
        return shutil.which(command) is not None
    
    def _run_command(self, cmd: List[str], cwd: Optional[Path] = None) -> bool:
        BuildLogger.info(f"Running: {' '.join(cmd)}")
        try:
            result = subprocess.run(
                cmd,
                cwd=cwd,
                check=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True
            )
            if result.returncode != 0:
                BuildLogger.error(f"Command failed: {' '.join(cmd)}")
                return False
            return True
        except subprocess.CalledProcessError as e:
            BuildLogger.error(f"Command failed with error: {e.stderr}")
            return False
        except Exception as e:
            BuildLogger.error(f"Unexpected error: {str(e)}")
            return False
    
    def _get_cmake_args(self, config: BuildConfig) -> List[str]:
        args = [
            f"-DCMAKE_BUILD_TYPE={config.config}",
            f"-DCMAKE_INSTALL_PREFIX={config.build_path}/install",
            "-DBUILD_TESTS=OFF",
            "-DBUILD_WEB_UI=ON"
        ]
        
        # Target-specific setup
        if config.target == "linux":
            args.append("-DPLATFORM=linux")
            
            if config.arch == "arm":
                args.extend([
                    "-DCMAKE_SYSTEM_PROCESSOR=arm",
                    "-DCMAKE_CXX_FLAGS=-march=armv7-a -mfpu=neon -mfloat-abi=hard",
                    "-DCMAKE_C_FLAGS=-march=armv7-a -mfpu=neon -mfloat-abi=hard"
                ])
                if self._command_exists("arm-linux-gnueabihf-g++"):
                    args.extend([
                        "-DCMAKE_C_COMPILER=arm-linux-gnueabihf-gcc",
                        "-DCMAKE_CXX_COMPILER=arm-linux-gnueabihf-g++"
                    ])
                    BuildLogger.info("Using arm-linux-gnueabihf toolchain for ARM")
                else:
                    BuildLogger.warning("arm-linux-gnueabihf toolchain not found, using native compiler with ARM flags")
                    
            elif config.arch == "arm64":
                args.extend([
                    "-DCMAKE_SYSTEM_PROCESSOR=aarch64",
                    "-DCMAKE_CXX_FLAGS=-march=armv8-a",
                    "-DCMAKE_C_FLAGS=-march=armv8-a"
                ])
                if self._command_exists("aarch64-linux-gnu-g++"):
                    args.extend([
                        "-DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc",
                        "-DCMAKE_CXX_COMPILER=aarch64-linux-gnu-g++"
                    ])
                    BuildLogger.info("Using aarch64-linux-gnu toolchain for ARM64")
                else:
                    BuildLogger.warning("aarch64-linux-gnu toolchain not found, using native compiler with ARM64 flags")
                    
            elif config.arch == "x64":
                args.extend([
                    "-DCMAKE_SYSTEM_PROCESSOR=x86_64",
                    "-DCMAKE_CXX_FLAGS=-march=x86-64",
                    "-DCMAKE_C_FLAGS=-march=x86-64"
                ])
                
        elif config.target == "windows":
            args.append("-DPLATFORM=windows")
            
            if self.host_os != "windows":
                # Cross-compilation from non-Windows
                if config.arch == "x64":
                    if self._command_exists("x86_64-w64-mingw32-g++"):
                        args.extend([
                            "-DCMAKE_SYSTEM_NAME=Windows",
                            "-DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc",
                            "-DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++",
                            "-DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres"
                        ])
                        BuildLogger.info("Using x86_64-w64-mingw32 toolchain for Windows x64")
                    else:
                        BuildLogger.error("x86_64-w64-mingw32 toolchain not found for Windows x64 cross-compilation")
                        return []
                        
                elif config.arch in ["arm", "arm64"]:
                    if self._command_exists("aarch64-w64-mingw32-g++"):
                        args.extend([
                            "-DCMAKE_SYSTEM_NAME=Windows",
                            "-DCMAKE_C_COMPILER=aarch64-w64-mingw32-gcc",
                            "-DCMAKE_CXX_COMPILER=aarch64-w64-mingw32-g++",
                            "-DCMAKE_RC_COMPILER=aarch64-w64-mingw32-windres"
                        ])
                        BuildLogger.info("Using aarch64-w64-mingw32 toolchain for Windows ARM64")
                    else:
                        BuildLogger.error("aarch64-w64-mingw32 toolchain not found for Windows ARM64 cross-compilation")
                        return []
            else:
                # Native Windows build
                if config.arch == "arm64":
                    args.extend([
                        "-DCMAKE_SYSTEM_PROCESSOR=ARM64",
                        "-DCMAKE_GENERATOR_PLATFORM=ARM64"
                    ])
                elif config.arch == "x64":
                    args.extend([
                        "-DCMAKE_SYSTEM_PROCESSOR=x64",
                        "-DCMAKE_GENERATOR_PLATFORM=x64"
                    ])
                
                # Use Visual Studio generator if available
                try:
                    result = subprocess.run(
                        ["cmake", "--help"],
                        capture_output=True,
                        text=True
                    )
                    if "Visual Studio" in result.stdout:
                        # Find the latest Visual Studio generator
                        generators = [line for line in result.stdout.split('\n') if 'Visual Studio' in line]
                        if generators:
                            latest_vs = generators[-1].strip()
                            args.append(f"-G {latest_vs}")
                            BuildLogger.info(f"Using generator: {latest_vs}")
                except:
                    pass
                    
        elif config.target == "macos":
            args.append("-DPLATFORM=macos")
            
            if config.arch == "arm64":
                args.extend([
                    "-DCMAKE_SYSTEM_PROCESSOR=arm64",
                    "-DCMAKE_OSX_ARCHITECTURES=arm64",
                    "-DCMAKE_APPLE_SILICON_PROCESSOR=arm64"
                ])
                
                # If building on Intel Mac for ARM64
                if self.host_os == "macos" and self.host_arch != "arm64":
                    if self._command_exists("clang++"):
                        args.extend([
                            "-DCMAKE_C_COMPILER=clang",
                            "-DCMAKE_CXX_COMPILER=clang++",
                            "-DCMAKE_CXX_FLAGS=-arch arm64",
                            "-DCMAKE_C_FLAGS=-arch arm64",
                            "-DCMAKE_OSX_DEPLOYMENT_TARGET=11.0"
                        ])
                        BuildLogger.info("Cross-compiling for macOS ARM64 (Apple Silicon)")
                    else:
                        BuildLogger.error("clang compiler not found for macOS ARM64 cross-compilation")
                        return []
                        
            elif config.arch == "x64":
                args.extend([
                    "-DCMAKE_SYSTEM_PROCESSOR=x86_64",
                    "-DCMAKE_OSX_ARCHITECTURES=x86_64"
                ])
                
                # If building on ARM Mac for x64
                if self.host_os == "macos" and self.host_arch == "arm64":
                    if self._command_exists("clang++"):
                        args.extend([
                            "-DCMAKE_C_COMPILER=clang",
                            "-DCMAKE_CXX_COMPILER=clang++",
                            "-DCMAKE_CXX_FLAGS=-arch x86_64",
                            "-DCMAKE_C_FLAGS=-arch x86_64",
                            "-DCMAKE_OSX_DEPLOYMENT_TARGET=10.15"
                        ])
                        BuildLogger.info("Cross-compiling for macOS x64 (Intel)")
                    else:
                        BuildLogger.error("clang compiler not found for macOS x64 cross-compilation")
                        return []
        
        return args
    
    def build(self, config: BuildConfig) -> bool:
        BuildLogger.info(f"Building {config.build_name}...")
        
        # Create build directory
        config.build_path = self.build_dir / config.build_name
        config.build_path.mkdir(parents=True, exist_ok=True)
        
        # Get CMake arguments
        cmake_args = self._get_cmake_args(config)
        if not cmake_args:
            return False
        
        # Run CMake configuration
        BuildLogger.info(f"Configuring CMake for {config.build_name}...")
        if not self._run_command(["cmake", str(self.project_dir)] + cmake_args, cwd=config.build_path):
            return False
        
        # Build the project
        BuildLogger.info(f"Building {config.build_name} with {self.jobs} parallel jobs...")
        if not self._run_command(
            ["cmake", "--build", ".", "--config", config.config, "--parallel", str(self.jobs)],
            cwd=config.build_path
        ):
            return False
        
        # Install (optional)
        BuildLogger.info(f"Installing {config.build_name}...")
        if not self._run_command(
            ["cmake", "--install", ".", "--config", config.config],
            cwd=config.build_path
        ):
            BuildLogger.warning(f"Install step failed for {config.build_name} (continuing...)")
        
        BuildLogger.success(f"Successfully built {config.build_name}")
        return True
    
    def validate_target(self, target: str) -> str:
        valid_targets = ["linux", "windows", "macos", "all"]
        target_lower = target.lower()
        if target_lower not in valid_targets:
            BuildLogger.error(f"Invalid target: {target}. Must be one of: {', '.join(valid_targets)}")
            sys.exit(1)
        return target_lower
    
    def validate_arch(self, arch: str) -> str:
        valid_archs = ["x64", "arm", "arm64", "all"]
        arch_lower = arch.lower()
        if arch_lower not in valid_archs:
            BuildLogger.error(f"Invalid architecture: {arch}. Must be one of: {', '.join(valid_archs)}")
            sys.exit(1)
        return arch_lower
    
    def validate_config(self, config: str) -> str:
        valid_configs = ["Debug", "Release", "RelWithDebInfo", "MinSizeRel"]
        if config not in valid_configs:
            BuildLogger.error(f"Invalid configuration: {config}. Must be one of: {', '.join(valid_configs)}")
            sys.exit(1)
        return config


def main():
    parser = argparse.ArgumentParser(
        description="AprilTag Detector - Multi-Platform Build Script",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument(
        "-t", "--target",
        type=str,
        default="all",
        help="Target platform: linux, windows, macos, all (default: all)"
    )
    parser.add_argument(
        "-a", "--arch",
        type=str,
        default="all",
        help="Architecture: x64, arm, arm64, all (default: all)"
    )
    parser.add_argument(
        "-c", "--config",
        type=str,
        default="Release",
        help="Build configuration: Debug, Release, RelWithDebInfo, MinSizeRel (default: Release)"
    )
    parser.add_argument(
        "-j", "--jobs",
        type=int,
        default=os.cpu_count() or 4,
        help="Number of parallel jobs (default: CPU count)"
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        help="Clean build directory before building"
    )
    
    args = parser.parse_args()
    
    # Get project directory
    project_dir = Path(__file__).parent.absolute()
    build_dir = project_dir / "build"
    
    # Initialize build system
    build_system = BuildSystem(project_dir, build_dir, args.jobs)
    
    # Validate inputs
    target = build_system.validate_target(args.target)
    arch = build_system.validate_arch(args.arch)
    config = build_system.validate_config(args.config)
    
    # Clean if requested
    if args.clean and build_dir.exists():
        BuildLogger.info(f"Cleaning build directory: {build_dir}")
        shutil.rmtree(build_dir)
    
    # Create build directory
    build_dir.mkdir(parents=True, exist_ok=True)
    
    # Determine which targets to build
    if target == "all":
        target_list = ["linux", "windows", "macos"]
    else:
        target_list = [target]
    
    # Determine which architectures to build
    if arch == "all":
        arch_list = ["x64", "arm64", "arm"]
    else:
        arch_list = [arch]
    
    BuildLogger.info("Starting build process...")
    BuildLogger.info(f"Targets: {', '.join(target_list)}")
    BuildLogger.info(f"Architectures: {', '.join(arch_list)}")
    BuildLogger.info(f"Configuration: {config}")
    BuildLogger.info(f"Parallel Jobs: {args.jobs}")
    
    # Track build status
    build_count = 0
    success_count = 0
    failure_count = 0
    
    # Build all combinations
    for target_item in target_list:
        for arch_item in arch_list:
            # Skip unsupported combinations
            if target_item == "windows" and arch_item == "arm":
                BuildLogger.warning("Skipping Windows ARM (32-bit) - not commonly supported")
                continue
            
            if target_item == "macos" and arch_item == "arm":
                BuildLogger.warning("Skipping macOS ARM (32-bit) - macOS only supports x64 and arm64")
                continue
            
            build_count += 1
            build_config = BuildConfig(target_item, arch_item, config)
            
            if build_system.build(build_config):
                success_count += 1
            else:
                failure_count += 1
    
    # Print summary
    print()
    BuildLogger.info("Build Summary:")
    BuildLogger.info(f"  Total builds attempted: {build_count}")
    BuildLogger.success(f"  Successful: {success_count}")
    if failure_count > 0:
        BuildLogger.error(f"  Failed: {failure_count}")
    
    if failure_count > 0:
        sys.exit(1)
    
    sys.exit(0)


if __name__ == "__main__":
    main()

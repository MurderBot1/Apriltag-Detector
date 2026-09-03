#pragma once

// Platform detection macros

#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS 1
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#elif defined(__APPLE__)
    #define PLATFORM_MACOS 1
    #include <TargetConditionals.h>
#elif defined(__linux__)
    #define PLATFORM_LINUX 1
    #include <unistd.h>
#endif

// Architecture detection
#if defined(__x86_64__) || defined(_M_X64)
    #define ARCH_X64 1
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define ARCH_ARM64 1
#elif defined(__arm__) || defined(_M_ARM)
    #define ARCH_ARM32 1
#endif

// String identifiers
namespace Platform {
    
    constexpr const char* getPlatformString() {
        #if PLATFORM_WINDOWS
            return "windows";
        #elif PLATFORM_MACOS
            return "macos";
        #elif PLATFORM_LINUX
            return "linux";
        #else
            return "unknown";
        #endif
    }
    
    constexpr const char* getArchString() {
        #if ARCH_X64
            return "x64";
        #elif ARCH_ARM64
            return "arm64";
        #elif ARCH_ARM32
            return "arm";
        #else
            return "unknown";
        #endif
    }
    
    // Path separator
    constexpr char getPathSeparator() {
        #if PLATFORM_WINDOWS
            return '\\';
        #else
            return '/';
        #endif
    }
    
    // Process ID
    long long getCurrentProcessId() {
        #if PLATFORM_WINDOWS
            return static_cast<long long>(GetCurrentProcessId());
        #else
            return static_cast<long long>(::getpid());
        #endif
    }
    
    // Dynamic library extension
    constexpr const char* getLibraryExtension() {
        #if PLATFORM_WINDOWS
            return ".dll";
        #elif PLATFORM_MACOS
            return ".dylib";
        #else
            return ".so";
        #endif
    }
    
    // Executable extension
    constexpr const char* getExecutableExtension() {
        #if PLATFORM_WINDOWS
            return ".exe";
        #else
            return "";
        #endif
    }
    
} // namespace Platform
